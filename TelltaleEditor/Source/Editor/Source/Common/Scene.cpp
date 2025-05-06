#include <Common/Scene.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/Callbacks.hpp>
#include <Symbols.hpp>

class SceneAPI
{
public:
    
    static Scene* Task(LuaManager& man)
    {
        return (Scene*)man.ToPointer(1);
    }
    
    static U32 luaSceneSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        pScene->SetName(man.ToString(2));
        return 0;
    }
    
    static U32 luaSceneSetHidden(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        if(man.ToBool(2))
            pScene->_Flags.Add(SceneFlags::HIDDEN);
        else
            pScene->_Flags.Remove(SceneFlags::HIDDEN);
        return 0;
    }
    
    // state,agentname, props (will be copied)
    static U32 luaScenePushAgent(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        String agent = man.ToString(2);
        if(pScene->ExistsAgent(agent))
        {
            TTE_LOG("WARNING: When normalising scene %s the agent %s already exists! Ignoring...", pScene->Name.c_str(), agent.c_str());
            return 0;
        }
        Meta::ClassInstance props = Meta::CopyInstance(Meta::AcquireScriptInstance(man, 3));
        pScene->AddAgent(agent, {}, std::move(props));
        return 0;
    }
    
};

void Scene::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    PUSH_FUNC(Col, "CommonSceneSetName", &SceneAPI::luaSceneSetName, "nil CommonSceneSetName(state, name)", "Sets the name of the common scene");
    PUSH_FUNC(Col, "CommonSceneSetHidden", &SceneAPI::luaSceneSetHidden,"nil CommonSceneSetHidden(state, bHidden)", "Sets if the common scene is initially hidden");
    PUSH_FUNC(Col, "CommonScenePushAgent", &SceneAPI::luaScenePushAgent, "nil CommonScenePushAgent(state, name, props)", "Pushes an agent to the common scene");
}

void Scene::FinaliseNormalisationAsync()
{
    
}

void Scene::_SetupAgent(std::map<Symbol, Ptr<SceneAgent>, SceneAgentComparator>::iterator agent)
{
    Ptr<SceneAgent> pAgent = agent->second;
    
    // create node
    Ptr<Node> pNode;
    pAgent->AgentNode = pNode = TTE_NEW_PTR(Node, MEMORY_TAG_OBJECT_DATA);
    pNode->AgentName = pAgent->Name;
    pNode->Name = pAgent->Name;
    pNode->AttachedScene = this;
    pAgent->OwningScene = this;
    
    // SETUP RUNTIME PROPERTIES
    
    if(!PropertySet::ExistsKey(pAgent->Props, kRuntimeVisibilityKey, true))
    {
        PropertySet::Set<Bool>(pAgent->Props, kRuntimeVisibilityKey, true, "bool");
    }
    
    // ADD CALLBACKS
    
    PropertySet::AddCallback(pAgent->Props, kRuntimeVisibilityKey, ALLOCATE_METHOD_CALLBACK(pAgent, &SceneAgent::SetVisible, SceneAgent, Bool));
    
    // 1 (Sync1) prepare non dependent mesh agents
    
    SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::PRE_RENDERABLE,
                                                     SceneModuleUtil::_SetupAgentSubset{*this, *pAgent});
    
    // 2 (Sync2) prepare depent modules on mesh
    
    SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::POST_RENDERABLE,
                                                     SceneModuleUtil::_SetupAgentSubset{*this, *pAgent});
}

void SceneAgent::SetVisible(Bool bVisible)
{
    if(OwningScene && CompareCaseInsensitive(OwningScene->GetName(), Name))
    {
        if(!bVisible)
            OwningScene->_Flags.Add(SceneFlags::HIDDEN);
        else
            OwningScene->_Flags.Remove(SceneFlags::HIDDEN);
    }
}

void Scene::_SetupAgentsModules()
{
    if(_Flags.Test(SceneFlags::RUNNING))
        return;
    for(auto agent = _Agents.begin(); agent != _Agents.end(); agent++)
    {
        _SetupAgent(agent);
    }
}

Meta::ClassInstance Scene::GetAgentProps(const Symbol & sym)
{
    for(auto& agent: _Agents)
    {
        if(agent.first == sym)
        {
            return agent.second->Props;
        }
    }
    return {};
}

void Scene::AddAgentModule(const String& Name, SceneModuleType module)
{
    Symbol sym{Name};
    for(auto& agent: _Agents)
    {
        if(agent.second->NameSymbol == sym)
        {
            if(agent.second->ModuleIndices[(U32)module] == -1)
            {
                I32 index = -1;
                SceneModuleUtil::PerformRecursiveModuleOperation<SceneModuleUtil::_AddModuleRecurser>(
                    SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_AddModuleRecurser{*this, module, index});
                if(index == -1)
                {
                    TTE_ASSERT(false, "Invalid module enum!");
                    return;
                }
                agent.second->ModuleIndices[(U32)module] = index;
            }
            break;
        }
    }
}

void Scene::RemoveAgentModule(const Symbol& sym, SceneModuleType module)
{
    I32 toRemove = -1;
    
    // First pass: Find the agent and remove the module
    for (auto it = _Agents.begin(); it != _Agents.end(); it++)
    {
        const Ptr<SceneAgent>& agent = it->second;
        if (agent->NameSymbol == sym)
        {
            if (agent->ModuleIndices[(U32)module] != -1)
            {
                toRemove = agent->ModuleIndices[(U32)module];
                Bool bResult = false;
                
                SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_RemoveModuleRecurser{*this, module, toRemove, bResult});
                
                if(!bResult)
                {
                    TTE_ASSERT(false, "Invalid module type");
                    return;
                }
                
                // Clear the module index for the current agent (or mark it as invalid)
                agent->ModuleIndices[(U32)module] = -1;
            }
            break;
        }
    }
    
    if (toRemove != -1)
    {
        for (auto& agent : _Agents)
        {
            if (agent.second->ModuleIndices[(U32)module] > toRemove)
            {
                agent.second->ModuleIndices[(U32)module] = agent.second->ModuleIndices[(U32)module] - 1;
            }
        }
    }
}


Bool Scene::ExistsAgent(const Symbol& sym)
{
    for(auto& agent: _Agents)
        if(agent.second->NameSymbol == sym)
            return true;
    return false;
}

Bool Scene::HasAgentModule(const Symbol& sym, SceneModuleType module)
{
    for(auto& agent: _Agents)
        if(agent.second->NameSymbol == sym)
            return agent.second->ModuleIndices[(U32)module] != -1;
    return false;
}

void Scene::AddAgent(const String& Name, SceneModuleTypes modules, Meta::ClassInstance props)
{
    TTE_ASSERT(!ExistsAgent(Name), "Agent %s already exists in %s", Name.c_str(), Name.c_str());
    Ptr<SceneAgent> pAgent = TTE_NEW_PTR(SceneAgent, MEMORY_TAG_SCENE_DATA, Name);
    auto agentIt = _Agents.insert({Symbol(Name), pAgent});
    auto& agentPtr = agentIt.first->second;
    agentPtr->OwningScene = this;
    agentPtr->Props = props;
    if(!agentPtr->Props)
    {
        U32 clazz = Meta::FindClass(Meta::GetInternalState().GetActiveGame().PropsClassName, 0);
        TTE_ASSERT(clazz!=0, "Property set class not found!");
        agentPtr->Props = Meta::CreateInstance(clazz);
    }
    else
    {
        TTE_ASSERT(Meta::GetClass(props.GetClassID()).Name == Meta::GetInternalState().GetActiveGame().PropsClassName, "At Scene::AddAgent -> properties class mismatch!");
    }
    for(auto it = modules.begin(); it != modules.end(); it++)
    {
        AddAgentModule(Name, *it);
    }
    if(_Flags.Test(SceneFlags::RUNNING))
    {
        _SetupAgent(agentIt.first);
    }
}

// NODES

Bool Scene::_ValidateNodeAttachment(Ptr<Node> node, Ptr<Node> potentialChild)
{
    return !TestChildNode(node, potentialChild);
}

void Scene::_UpdateListenerAttachments(Ptr<Node>node)
{
    Ptr<NodeListener> listener = node->Listeners;
    while(listener)
    {
        listener->OnAttachmentChanged();
        listener = listener->Next;
    }
    Ptr<Node> child = node->FirstChild;
    while(child)
    {
        _UpdateListenerAttachments(child);
        child = child->NextSibling;
    }
}

void Scene::_InvalidateNode(Ptr<Node> node, Node* pParent, Bool bAllowStaticUpdate)
{
    node->Fl.Remove(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID);
    Ptr<NodeListener> listener = node->Listeners;
    while(listener)
    {
        listener->OnTransformChanged(nullptr);
        listener = listener->Next;
    }
    Node* pParentTile = node->Fl.Test(NodeFlags::NODE_ENVIRONMENT_TILE) ? node.get() : nullptr;
    // invalidate *all* children in tree as need recalc.
    for(Ptr<Node> pChild = node->FirstChild; pChild; pChild = pChild->NextSibling)
    {
        if(bAllowStaticUpdate || _ValidateTransformUpdate(pChild, pParentTile))
            _InvalidateNode(pChild, pParentTile, bAllowStaticUpdate);
    }
}

Transform Scene::GetNodeWorldTransform(Ptr<Node>node)
{
    if(!node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
        _CalculateGlobalNodeTransform(node);
    return node->GlobalTransform;
}

void Scene::_CalculateGlobalNodeTransform(Ptr<Node> node)
{
    if(node->Parent)
    {
        if(!node->Parent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(node->Parent); // parent needs update first as not a root
        node->GlobalTransform = node->Parent->GlobalTransform * node->LocalTransform;
        node->GlobalTransform.Normalise();
    }
    else
    {
        node->GlobalTransform = node->LocalTransform;
    }
    node->Fl.Add(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID);
}

Transform Scene::NodeLocalToNode(const Ptr<Node>& node, Transform nodeLocalSpaceTransform, const String& max)
{
    Transform result = nodeLocalSpaceTransform;
    
    Ptr<Node> current = node->Parent;
    
    while (current)
    {
        result = current->LocalTransform * result;
        result.Normalise();
        if (max.length() != 0 && CompareCaseInsensitive(current->Name, max))
            break;
        current = current->Parent;
    }
    
    return result;
}

void Scene::UpdateNodeLocalTransform(Ptr<Node> node, Transform trans, Bool bAllowSt)
{
    Bool bAllow = bAllowSt || _ValidateTransformUpdate(node, nullptr);
    if (!bAllow)
        return;
    
    node->LocalTransform = trans; // use as-is, assumes local space
    node->LocalTransform.Normalise();
    
    _InvalidateNode(node, nullptr, bAllowSt);
}

void Scene::UpdateNodeWorldTransform(Ptr<Node> node, Transform trans, Bool bAllowSt)
{
    Bool bAllow = bAllowSt || _ValidateTransformUpdate(node, nullptr);
    if(!bAllow)
        return;
    if(node->Parent)
    {
        // CALCULATE GLOBAL
        if(!node->Parent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(node->Parent);
        // CALCULATE LOCAL
        node->LocalTransform = trans / node->Parent->GlobalTransform;
    }
    else
    {
        node->LocalTransform = trans; // update local, no parent
    }
    node->LocalTransform.Normalise();
    _InvalidateNode(node, nullptr, bAllowSt);
}

Bool Scene::_ValidateTransformUpdate(Ptr<Node> node, Node *pTileNode)
{
    return pTileNode || !node->StaticListeners; // if theres a parent tile always allow. if no static listeners, allow.
}

void NodeListener::SetStatic(Bool bStatic)
{
    if(Listening)
    {
        Bool bAlreadyStatic = ListenerFlags.Test(NodeListenerFlags::NODE_LISTENER_STATIC);
        if(bStatic != bAlreadyStatic)
        {
            if(bStatic)
            {
                Listening->StaticListeners++;
                ListenerFlags.Add(NodeListenerFlags::NODE_LISTENER_STATIC);
            }
            else
            {
                Listening->StaticListeners--;
                ListenerFlags.Remove(NodeListenerFlags::NODE_LISTENER_STATIC);
            }
        }
    }
}

NodeListener::~NodeListener()
{
    AdandonNode();
}

Bool NodeListener::IsStatic()
{
    return ListenerFlags.Test(NodeListenerFlags::NODE_LISTENER_STATIC);
}

void NodeListener::AdandonNode()
{
    if(Listening)
        Scene::RemoveNodeListener(Listening, this);
    ListenerFlags = 0;
}

void Scene::AddNodeListener(Ptr<Node> node, Ptr<NodeListener> pListener)
{
    pListener->AdandonNode();
    pListener->Listening = node;
    pListener->Next = node->Listeners;
    node->Listeners = pListener;
    if(pListener->IsStatic())
        node->StaticListeners++;
}

void Scene::RemoveNodeListener(Ptr<Node> node, NodeListener *pListener)
{
    NodeListener* pCurrent = node->Listeners.get();
    NodeListener* pPrev = nullptr;
    while(pCurrent)
    {
        if(pCurrent == pListener)
        {
            if(pPrev)
                pPrev->Next = std::move(pCurrent->Next);
            else
                node->Listeners = std::move(pCurrent->Next);
            if(pCurrent->IsStatic())
                node->StaticListeners--;
            pListener->ListenerFlags = 0;
            pListener->Listening.reset();
            return;
        }
        pPrev = pCurrent;
        pCurrent = pCurrent->Next.get();
    }
}

Ptr<Node> Scene::FindChildNode(Ptr<Node> node, Symbol name)
{
    if(Symbol(node->Name) == name)
        return node;
    Ptr<Node> pChild = node->FirstChild;
    Ptr<Node> result{};
    if(!pChild || !(result = FindChildNode(pChild, name)))
    {
        pChild = pChild->NextSibling;
        if(pChild)
            return FindChildNode(pChild, name);
    }
    return result;
}

Bool Scene::TestChildNode(Ptr<Node> node, Ptr<Node> potentialChild)
{
    if(node == potentialChild)
        return true;
    Ptr<Node> pChild = node->FirstChild;
    while(pChild)
    {
        if(pChild == potentialChild)
            return true;
        if(TestChildNode(pChild, potentialChild))
            return true;
        pChild = pChild->NextSibling;
    }
    return false;
}

Bool Scene::TestParentNode(Ptr<Node> pNode, Ptr<Node> potentialParent)
{
    while(pNode)
    {
        if(pNode == potentialParent)
            return true;
        pNode = pNode->Parent;
    }
    return false;
}

void Scene::AttachNode(Ptr<Node> node, Ptr<Node> parent, Bool bPreserveWorldPosition, Bool bInitialAttach)
{
    if(node != parent && (bInitialAttach || _ValidateTransformUpdate(node, nullptr)))
    {
        if(node->Parent)
            UnAttachNode(node, bPreserveWorldPosition, true);
        if(!parent->Parent || _ValidateNodeAttachment(parent->Parent, node))
        {
            node->Parent = parent;
            node->NextSibling = parent->FirstChild;
            if(parent->FirstChild)
            {
                parent->FirstChild->PrevSibling = node;
            }
            parent->FirstChild = node;
            _InvalidateNode(node, nullptr, bInitialAttach || bPreserveWorldPosition);
            if(bPreserveWorldPosition)
            {
                if(!node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
                    _CalculateGlobalNodeTransform(node); // ensure we know its transform
                UpdateNodeLocalTransform(node, node->LocalTransform, true);
            }
            _UpdateListenerAttachments(node);
        }
    }
}

void Scene::UnAttachNode(Ptr<Node> node, Bool bPreserveWorldPosition, Bool bForceUnattach)
{
    if(node && node->Parent && (bForceUnattach || _ValidateTransformUpdate(node, nullptr)))
    {
        if(bPreserveWorldPosition && !node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(node); // ensure we know its transform
        
        Ptr<Node> prev = node->PrevSibling;
        Ptr<Node> next = node->NextSibling;
        if(prev)
        {
            prev->NextSibling = std::move(next);
            if(node->NextSibling)
            {
                node->NextSibling->PrevSibling = std::move(prev);
            }
        }
        else
        { // first in d-ll
            node->Parent->FirstChild = std::move(next);
            if(node->NextSibling)
            {
                node->NextSibling->PrevSibling = nullptr;
            }
        }
        node->NextSibling = node->PrevSibling = nullptr;
        
        // preserve world pos if needed
        if(bPreserveWorldPosition)
        {
            if(node->Parent)
            {
                if(!node->Parent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
                    _CalculateGlobalNodeTransform(node->Parent);
                node->LocalTransform = node->GlobalTransform / node->Parent->GlobalTransform;
            }
            _InvalidateNode(node, nullptr, true);
        }
        else
            _InvalidateNode(node, nullptr, bForceUnattach);
        
        node->Parent = nullptr;
        
        // finally signal attachment change
        _UpdateListenerAttachments(node);
    }
}

void Scene::UnAttachAllChildren(Ptr<Node> node, Bool bAttachChildrenToParent)
{
    if(!node->Parent)
        bAttachChildrenToParent = false;
    Ptr<Node> child = node->FirstChild;
    while(child)
    {
        UnAttachNode(child, true, true);
        if(bAttachChildrenToParent)
            AttachNode(child, node->Parent, true, true);
        child = child->NextSibling;
    }
}

// OBJECT CACHE MANAGER

void ObjOwner::_AddObjData(Ptr<ObjDataBase> pObj)
{
    pObj->Next = Objects;
    Objects = std::move(pObj);
}

void ObjOwner::_RemoveObjData(const String& obj, const std::type_info& ti)
{
    Ptr<ObjDataBase> prev{};
    Ptr<ObjDataBase> cur = Objects;
    while(cur)
    {
        if(cur->Type == ti && CompareCaseInsensitive(obj, cur->Name))
        {
            if(prev)
                prev->Next = cur->Next;
            else
                Objects = cur->Next;
            return;
        }
        prev = cur;
        cur = cur->Next;
    }
}

void ObjOwner::_GetObjData(Ptr<ObjDataBase>& pOut, const String& obj, const std::type_info& ti)
{
    Ptr<ObjDataBase> cur = Objects;
    while(cur)
    {
        if(cur->Type == ti && CompareCaseInsensitive(obj, cur->Name))
        {
            pOut = cur;
            return;
        }
        cur = cur->Next;
    }
}

void ObjOwner::FreeOwnedObjects()
{
    Ptr<ObjDataBase> cur = Objects;
    while(cur)
    {
        Ptr<ObjDataBase> cache = cur->Next;
        cur->Next.reset();
        cur = cache;
    }
    Objects.reset();
}

ObjOwner::~ObjOwner()
{
    FreeOwnedObjects();
}
