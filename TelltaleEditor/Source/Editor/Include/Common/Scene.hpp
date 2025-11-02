#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>
#include <Core/BitSet.hpp>
#include <Meta/Meta.hpp>

#include <Resource/ResourceRegistry.hpp>
#include <Resource/PropertySet.hpp>

#include <Common/InputMapper.hpp>
#include <Common/Mesh.hpp>
#include <Common/Skeleton.hpp>

#include <Symbols.hpp>

#include <vector>
#include <type_traits>

// ========================================= PREDEFS =========================================

class Animation;
class AnimationManager;
class PlaybackController;
class Scene;
class SceneRuntime;
class RenderContext;
class EditorUI;
struct Node;
struct SceneAgent;
struct SceneMessage;
struct RenderFrame;
enum class SceneMessageType : U32;

// ========================================= OBJECT MANAGER =========================================

// Object manager. Inherit from this to own objects. Just managed instances of c++ classes.
class ObjOwner
{
public:
    
    struct ObjDataBase
    {
        
        const std::type_info& Type;
        Ptr<ObjDataBase> Next;
        String Name;
        
        inline ObjDataBase(String&& nm, const std::type_info& ti) : Type(ti), Next(), Name(std::move(nm)) {}

        virtual ~ObjDataBase() = default;
        
    };
    
    template<typename T> struct ObjData : ObjDataBase
    {
        
        T Obj;
        
        inline ObjData(String nm, T&& value) :
            ObjDataBase(std::move(nm), typeid(T)), Obj(std::move(value)) {}

        virtual ~ObjData() = default;
        
    };
    
    template<typename T> struct ObjDataRef : ObjDataBase
    {
        
        T& Ref;
        
        inline ObjDataRef(String nm, T& value) : ObjDataBase(std::move(nm), typeid(T)), Ref(value) {}

        virtual ~ObjDataRef() = default;
        
    };
    
    virtual ~ObjOwner();
    
    virtual void FreeOwnedObjects() final; // frees all
    
    // add an object by construct
    template<typename T, typename... Args> T* CreateObjData(String name, Args&&... args)
    {
        T obj(std::forward<Args>(args)...);
        Ptr<ObjData<T>> pObjData = TTE_NEW_PTR(ObjData<T>, MEMORY_TAG_OBJECT_DATA, std::move(name), std::move(obj));
        _AddObjData(pObjData);
        return &pObjData->Obj;
    }
    
    // add an object by rvalue
    template<typename T> T* AddObjData(String name, T&& obj)
    {
        Ptr<ObjData<T>> pObjData = TTE_NEW_PTR(ObjData<T>, MEMORY_TAG_OBJECT_DATA, std::move(name), std::move(obj));
        _AddObjData(pObjData);
        return &pObjData->Obj;
    }
    
    // add a weak ref to an object. used for mapping to faster module member variables
    template<typename T> void AddObjDataRef(String name, T& objRef)
    {
        Ptr<ObjDataRef<T>> pObjData = TTE_NEW_PTR(ObjDataRef<T>, MEMORY_TAG_OBJECT_DATA, std::move(name), objRef);
        _AddObjData(pObjData);
    }
    
    // remove an object by name
    template<typename T> void RemoveObjData(const String& name)
    {
        _RemoveObjData(name, typeid(T));
    }
    
    // get object by name, type checked. if not exist, create one by setting create to true
    template<typename T> T* GetObjData(const String& name, Bool bCreate)
    {
        Ptr<ObjDataBase> pObj{};
        _GetObjData(pObj, name, typeid(T));
        if(pObj)
            return &((ObjData<T>*)pObj.get())->Obj;
        if(bCreate)
        {
            // must have default constructor
            Ptr<ObjData<T>> pObjData = TTE_NEW_PTR(ObjData<T>, MEMORY_TAG_OBJECT_DATA, std::move(name), T{});
            _AddObjData(pObjData);
            return &((ObjData<T>*)pObjData.get())->Obj;
        }
        return nullptr;
    }
    
    // Similar to above. Gets latest created obj data matching T.
    template<typename T> T* GetObjDataByType()
    {
        Ptr<ObjDataBase> pObj{};
        String n{};
        _GetObjData(pObj, n, typeid(T));
        return pObj ? &((ObjData<T>*)pObj.get())->Obj : nullptr;
    }
    
private:
    
    void _AddObjData(Ptr<ObjDataBase> pObj);
    void _RemoveObjData(const String& obj, const std::type_info& ti);
    void _GetObjData(Ptr<ObjDataBase>& pOut, const String& name, const std::type_info& ti);
    
    Ptr<ObjDataBase> Objects;
    
};

// ========================================= NODE HEIRARCHY =========================================

enum class NodeFlags
{
    NODE_GLOBAL_TRANSFORM_VALID = 1, // node transform is valid
    NODE_ENVIRONMENT_TILE = 2, // is a static environment tile which cannot be transformed (moved)
};

enum class NodeListenerFlags
{
    NODE_LISTENER_STATIC = 1, // is set to static, disallowing object to move
};

// Callbacks for node updates
class NodeListener : public std::enable_shared_from_this<NodeListener>
{
    
    friend class Scene;
    Ptr<NodeListener> Next;
    Ptr<Node> Listening;
    Flags ListenerFlags;
    
public:
    
    NodeListener() = default;
    
    virtual void OnTransformChanged(Node* pTileNode) = 0;
    
    virtual void OnAttachmentChanged() = 0; // called when attachments changed relative to this.
    
    virtual void SetStatic(Bool bStatic) final; // set static listener to stop object movement
    
    virtual void AdandonNode() final; // remove this listener from the node, if attached.
    
    virtual Bool IsStatic() final; // is static.
    
    virtual ~NodeListener();
    
};

// Nodes are fundamental for abstract attachment of other agents and anything to each other in the scene to move together
// All agents have nodes but not all nodes have agents. Nodes can be skeleton entries (eg fingers), etc.
struct Node : ObjOwner
{
    
    String Name;
    Flags Fl;
    U32 StaticListeners = 0; // number of static listeners. Static means the object is set unmoveable.
    String AgentName;
    
    WeakPtr<Node> Parent, NextSibling, PrevSibling, FirstChild; // no linked list explicitly
    
    Transform LocalTransform, GlobalTransform; // local is from parent, global is world transform.
    
    Ptr<NodeListener> Listeners; // listeners
    
    Scene* AttachedScene = nullptr; // scene belongs to

    Node() = default;
    ~Node();

};

// Node visitor callback function. Return true to keep iterating.
using NodeVisitor = Bool(Ptr<Node> pNode, void* user);

// tree traversal type for node visiting
enum class NodeVisitorTraversal
{
    PRE_ORDER,
    IN_ORDER, // not really used. impl visits first child, itself, then other children
    POST_ORDER
};

// ========================================= SCENE MODULES =========================================

#include <SceneModules.inl>

// ========================================= SCENE / SCENE AGENT =========================================

/// An agent in the scene, or game object unity terms.
struct SceneAgent
{
    
    const Symbol NameSymbol; // as a symbol
    const String Name; // agent name
    
    mutable Scene* OwningScene = nullptr;
    mutable Meta::ClassInstance Props; // agent properties
    mutable Ptr<Node> AgentNode; // NOTNULL. Is always set only at runtime in rendering!
    mutable I32 ModuleIndices[(I32)SceneModuleType::NUM]; // points into arrays inside scene.

    Transform InitialTransform;
    
    inline SceneAgent() : Name(""), NameSymbol()
    {
        for(I32 i = 0; i < (I32)SceneModuleType::NUM; i++)
            ModuleIndices[i] = -1;
    }
    
    inline Bool operator==(const SceneAgent& rhs) const
    {
        return rhs.NameSymbol == NameSymbol;
    }
    
    inline SceneAgent(String _name) : Name(_name), NameSymbol(_name)
    {
        for(I32 i = 0; i < (I32)SceneModuleType::NUM; i++)
            ModuleIndices[i] = -1;
    }
    
    void SetVisible(Bool bVisible);
    
private:
    
    friend class Scene;
    
public:
    
};

// comparator for scene agents
struct SceneAgentComparator {
    
    using is_transparent = void;
    
    inline Bool operator()(const Symbol& a, const Symbol& b) const
    {
        return a < b;
    }
    
    inline Bool operator()(const Symbol& a, U64 b) const
    {
        return a.GetCRC64() < b;
    }
    
    inline Bool operator()(U64 a, const Symbol& b) const {
        return a < b.GetCRC64();
    }
};


enum class SceneFlags
{
    RUNNING = 1,
    HIDDEN = 2,
    // exclude from save games
};

namespace SceneModuleUtil 
{

    struct _SetupAgentModule;

    template<SceneModuleType M>
    inline SceneModule<M>& GetSceneModule(Scene& scene, const SceneAgent& agent);

}

/// A collection of agents. This is the common scene class for all games by telltale. This does not have any of the code for serialistion, that is done when lua injects scene information
/// from its scene meta class into this. This is a common class to all games and represents a common telltale scene.
class Scene : public HandleableRegistered<Scene>
{

    friend class HandleableRegistered<Scene>;

    Scene(Scene&& rhs) noexcept;
    Scene(const Scene& rhs);

public:
    
    using AgentMap = std::map<Symbol, Ptr<SceneAgent>, SceneAgentComparator>;
    
    static constexpr CString ClassHandle = "Handle<Scene>";
    static constexpr CString Class = "Scene";
    static constexpr CString Extension = "scene";
    
    inline Scene(Ptr<ResourceRegistry> reg) : HandleableRegistered<Scene>(std::move(reg)) {}

    ~Scene();
    
    inline void SetName(String name)
    {
        Name = name;
    }
    
    inline String GetName()
    {
        return Name;
    }

    // by selectable: use selectable. only selectable module objects (like in game). else does by render meshes.
    String GetAgentAtScreenPosition(Camera& cam, U32 screenX, U32 screenY, Bool bBySelectable);
    
    // Add a new agent. agent properties can be a nullptr, to start with default props. YOU MUST DISCARD AGENT PROPERTIES AFTER PASSING IT IN. copy it then pass if not!
    void AddAgent(const String& Name, SceneModuleTypes modules, Meta::ClassInstance AgentProperties, Transform initialTransform = {}, Bool SetupAgent = false);
    
    // Adds an agent module to the given agent. This does NOT setup the agent (eg for run / edit). See how the add module popup in module ui.cpp does it, recurisvely selecting the module template and setting it up after this.
    void AddAgentModule(const String& Name, SceneModuleType module);
    
    // Removes an agent module from a given agent.
    void RemoveAgentModule(const Symbol& Name, SceneModuleType module);
    
    Meta::ClassInstance GetAgentProps(const Symbol& Name);
    
    // Returns if the given agent exists
    Bool ExistsAgent(const Symbol& Name);
    
    // Returns if the given agent has the given module
    Bool HasAgentModule(const Symbol& Name, SceneModuleType module);

    // Remvoes an agent from the scene
    void RemoveAgent(const Symbol& Name);

    SceneModuleTypes GetAgentModules(const Symbol& Name);
    
    // Gets the given module for the given agent. This agent must have this module!
    template<SceneModuleType Type>
    inline SceneModule<Type>& GetAgentModule(const Symbol& Agent);

    // traverse a node and its children in the specified traversal ordering type with an optional user data argument.
    static void PerformNodeTraversal(Ptr<Node> baseNode, NodeVisitorTraversal traverseType, NodeVisitor* visitor, void* user);
    
    // Registers scene normalisers and specialisers
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    virtual void FinaliseNormalisationAsync() override;
    
    static void UpdateNodeWorldTransform(Ptr<Node> node, Transform world, Bool bStaticUpdateAllow); // update node *world* transform properly.
    
    static void UpdateNodeLocalTransform(Ptr<Node> node, Transform local, Bool bStaticUpdateAllow); // update node *local* transform properly.
    
    // node name can be empty, meaning convert node local space into world space.
    static Transform NodeLocalToNode(const Ptr<Node>& node, Transform nodeLocalSpaceTransform, const String& nodeName);
    
    static Transform GetNodeWorldTransform(Ptr<Node> node); // get correct updated world transform

    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::SCENE;
    }

    inline const AgentMap& GetAgents() const
    {
        return _Agents;
    }

    inline std::vector<WeakPtr<Camera>>& GetViewStack() 
    {
        return _ViewStack;
    }

    template<SceneModuleType M>
    inline const std::vector<SceneModule<M>>& GetModuleView() const
    {
        return _Modules.GetModuleArray<M>();
    }

    // Does not PLAY it! You must call Play on the returned controller.
    Ptr<PlaybackController> PlayAnimation(const Symbol& agent, Ptr<Animation> pAnim);
    
private:
    
    // ==== RENDER CONTEXT USE ONLY. THESE FUNCTIONS ARE DEFINED IN RENDERSCENE.CPP, SEPARATE TO REST DEFINED IN COMMON/SCENE.CPP
    
    // Internal call. Called by render context in async to process scene messages. Returns true if processed, ie valid message.
    Bool AsyncProcessRenderMessage(SceneRuntime& context, SceneMessage message, const SceneAgent* pAgent);
    
    // Async so only access this scene. Setup render information. Thread safe with perform render, detach and async process messages.
    void OnAsyncRenderAttach(SceneRuntime& context);
    
    // When the scene needs to stop. Free anything needed.
    void OnAsyncRenderDetach(SceneRuntime& context);
    
    // Called each frame async. To ensure speed this is async and should be performed isolated. Issue render instructions here.
    void PerformAsyncRender(SceneRuntime& context, RenderFrame& frame, Float deltaTime);
    
    void AsyncProcessEvents(SceneRuntime& context, const std::vector<RuntimeInputEvent>& events);
    
    void _SetupAgentsModules(); // at created to setup agents
    void _SetupAgent(std::map<Symbol, Ptr<SceneAgent>, SceneAgentComparator>::iterator agent);
    
    // ===== NODES
    
    static Bool _ValidateNodeAttachment(Ptr<Node> node, Ptr<Node> potentialChild);
    
    static Bool _ValidateTransformUpdate(Ptr<Node>, Node* pTileNode); // update transform with its parent tile if known
    
    static void _UpdateListenerAttachments(Ptr<Node> node); // update listener that attachment has changed
    
    static void _CalculateGlobalNodeTransform(Ptr<Node>); // Updates its transform
    
    static void _InvalidateNode(Ptr<Node>, Node* pTile, Bool bAllowStaticUpdate); // mark as invalid so needing new transform calculation
    
    // pub Node API. Private in scene class as only allowed in update async, eg script updates
    
    static void AddNodeListener(Ptr<Node> node, Ptr<NodeListener> pListener); // add a listener. only pointer nodes!
    
    static void RemoveNodeListener(Ptr<Node> node, NodeListener* pListener); // remove a listener
    
    static Bool TestChildNode(Ptr<Node> node, Ptr<Node> potentialChild); // test if potentialChild is a child of node
    
    static Bool TestParentNode(Ptr<Node> node, Ptr<Node> potentialParent); // test if potentialParent is a parent of node
    
    static Ptr<Node> FindChildNode(Ptr<Node> node, Symbol name); // find child node
    
    // attach new node to parent. preserve world position means its local transform is adjusted to ignore parent
    static void AttachNode(Ptr<Node> node, Ptr<Node> parent, Bool bPreserveWorldPosition, Bool bInitialAttach);
    
    // unattach the node from parent
    static void UnAttachNode(Ptr<Node>, Bool bPreserveWorldPosition, Bool bForceUnattach);
    
    // optionally re-attach all children of node into node's parent node if it exists
    static void UnAttachAllChildren(Ptr<Node> node, Bool bAttachChildrenToParent);
    
    // ===== GENERAL FUNCTIONALITY PRIVATE AND USE BY SCRIPTING ETC.
    
    // =====
    
    String Name; // scene name
    Flags _Flags;
    AgentMap _Agents; // scene agent list
    std::vector<WeakPtr<Camera>> _ViewStack; // view stack
    std::vector<AnimationManager*> _AnimationMgrs; // anim managers
    std::set<PlaybackController*> _Controllers;
    
    // ==== DATA ORIENTATED AGENT MODULE ATTACHMENTS

    SceneModuleContainer _Modules;
    
    friend class SceneRuntime;
    friend struct SceneAgent;
    friend class NodeListener;
    friend class AnimationManager;
    friend class SkeletonInstance;
    friend class PlaybackController;
    friend class SceneAPI;
    friend class EditorUI;
    friend class InspectorView;
    friend class SceneRenderer;
    friend class Camera;

    template<SceneModuleType M>
    friend struct SceneModule;

    friend struct SceneModuleUtil::_SetupAgentModule;

    template<SceneModuleType M>
    friend SceneModule<M>& SceneModuleUtil::GetSceneModule(Scene& scene, const SceneAgent& agent);
    
};

namespace SceneModuleUtil
{

    // recursively perform a function on each scene module type in the range. No need to write if-else chains
    template<SceneModuleType Module, SceneModuleType Last>
    struct _RecursiveModuleIterator
    {

        template<typename _IteratorFn>
        static void _Perform(_IteratorFn&& fn)
        {
            if (fn.template Apply<Module>()) // apply must return a bool and no params. return true to keep recursion going
                _RecursiveModuleIterator<(SceneModuleType)((U32)Module + 1), Last>::_Perform(std::forward<_IteratorFn>(fn));
        }

    };

    // end case, do nothing and stop recursion
    template<SceneModuleType Last>
    struct _RecursiveModuleIterator<Last, Last>
    {

        template<typename _IteratorFn>
        static void _Perform(_IteratorFn&& fn)
        {
            fn.template Apply<Last>();
        }

    };

    enum class ModuleRange
    {
        ALL,
        PRE_RENDERABLE,
        POST_RENDERABLE,
    };

    // Perform using one of the structs below as the template param
    template<typename _Fi>
    inline void PerformRecursiveModuleOperation(ModuleRange R, _Fi&& fn)
    {
        if (R == ModuleRange::ALL)
        {
            _RecursiveModuleIterator<SceneModuleType::FIRST_PRERENDERABLE, SceneModuleType::LAST_POSTRENDERABLE>
                ::_Perform(std::forward<_Fi>(fn));
        }
        else if (R == ModuleRange::PRE_RENDERABLE)
        {
            _RecursiveModuleIterator<SceneModuleType::FIRST_PRERENDERABLE, SceneModuleType::LAST_PRERENDERABLE>
                ::_Perform(std::forward<_Fi>(fn));
        }
        else if (R == ModuleRange::POST_RENDERABLE)
        {
            if (SceneModuleType::LAST_PRERENDERABLE != SceneModuleType::FIRST_POSTRENDERABLE)
            {
                _RecursiveModuleIterator<SceneModuleType::FIRST_POSTRENDERABLE, SceneModuleType::LAST_POSTRENDERABLE>
                    ::_Perform(std::forward<_Fi>(fn));
            }
        }
        else
        {
            TTE_ASSERT(false, "Invalid module range");
        }
    }

    struct _CollectAgentModules
    {

        SceneModuleTypes& Modules;
        SceneAgent& Agent;
        Ptr<ResourceRegistry>& Registry;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (PropertySet::IsMyParent(Agent.Props, SceneModule<Module>::GetModulePropertySet(), true, Registry))
            {
                Modules.Set(Module, true);
            }
            return true;
        }

    };

    struct _RenderUIModules
    {

        Scene& S;
        SceneAgent& Agent;
        EditorUI& Editor;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (S.HasAgentModule(Agent.NameSymbol, Module))
            {
                auto& module = S.GetAgentModule<Module>(Agent.NameSymbol);
                module.RenderUI(Editor, Agent);
            }
            return true;
        }

    };

    // setup agents. used as recursive above
    struct _SetupAgentSubset
    {

        Scene& S;
        SceneAgent& Agent;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (S.HasAgentModule(Agent.NameSymbol, Module))
            {
                auto& module = S.GetAgentModule<Module>(Agent.NameSymbol);
                module.AgentNode = Agent.AgentNode;
                module.OnSetupAgent(&Agent);
            }
            return true;
        }

    };

    struct _SetupAgentModule
    {

        Scene& S;
        Ptr<Node> AgentNode;
        SceneModuleType ModuleType;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (ModuleType == Module)
            {
                if (S.HasAgentModule(AgentNode->AgentName, Module))
                {
                    auto& module = S.GetAgentModule<Module>(AgentNode->AgentName);
                    module.AgentNode = AgentNode;
                    module.OnSetupAgent(S._Agents.find(AgentNode->AgentName)->second.get());
                }
                return false;
            }
            return true;
        }

    };

    struct _AddModuleRecurser
    {

        Scene& S;
        SceneModuleType Desired;
        I32& OutIndex;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (Desired == Module)
            {
                auto& vec = *const_cast<std::vector<SceneModule<Module>>*>(&S.GetModuleView<Module>());
                OutIndex = (I32)vec.size();
                vec.emplace_back();
                return false; // done
            }
            return true;
        }

    };

    struct _ModuleVectorMoveRecurser
    {

        Scene& From;
        Scene& To;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            auto& toVec = *const_cast<std::vector<SceneModule<Module>>*>(&To.GetModuleView<Module>());
            auto& fromVec = *const_cast<std::vector<SceneModule<Module>>*>(&From.GetModuleView<Module>());
            toVec = std::move(fromVec);
            return true;
        }

    };

    struct _ModuleVectorCopyRecurser
    {

        const Scene& From;
        Scene& To;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            auto& toVec = *const_cast<std::vector<SceneModule<Module>>*>(&To.GetModuleView<Module>());
            const auto& fromVec = From.GetModuleView<Module>();
            toVec = fromVec;
            return true;
        }

    };

    struct _RemoveModuleRecurser
    {

        Scene& S;
        SceneModuleType Mod;
        I32 RemovalIndex;
        Bool& result;
        SceneAgent& Agent;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (Mod == Module)
            {
                auto& vec = *const_cast<std::vector<SceneModule<Module>>*>(&S.GetModuleView<Module>());
                auto it = vec.begin();
                std::advance(it, RemovalIndex);
                it->OnModuleRemove(&Agent);
                vec.erase(it);
                result = true;
                return false;
            }
            return true;
        }

    };

    struct _ModuleIDCollector
    {

        std::vector<std::pair<SceneModuleType, CString>>& OutIDs;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            OutIDs.push_back(std::make_pair(Module, SceneModule<Module>::ModuleID));
            return true;
        }

    };

    struct _GetModuleInfo
    {

        SceneModuleType Type;
        String* OutID = nullptr, * OutPropName = nullptr, * OutName = nullptr;

        template<SceneModuleType Module>
        inline Bool Apply()
        {
            if (Module == Type)
            {
                if (OutID)
                    *OutID = SceneModule<Module>::ModuleID;
                if (OutPropName)
                    *OutPropName = SceneModule<Module>::GetModulePropertySet();
                if (OutName)
                    *OutName = SceneModule<Module>::ModuleName;
                return false;
            }
            return true;
        }

    };

    template<SceneModuleType M>
    inline SceneModule<M>& GetSceneModule(Scene& scene, const SceneAgent& agent)
    {
        TTE_ASSERT(agent.ModuleIndices[(U32)M] != -1, "Agent %s does not have this module", agent.Name.c_str());
        return scene._Modules.GetModuleArray<M>()[agent.ModuleIndices[(U32)M]];
    }

}

template<SceneModuleType Type>
inline SceneModule<Type>& Scene::GetAgentModule(const Symbol& Agent)
{
    auto it = _Agents.find(Agent);
    TTE_ASSERT(it != _Agents.end(), "The agent does not exist [GetAgentModule<>]");
    return SceneModuleUtil::GetSceneModule<Type>(*this, *it->second.get());
}
