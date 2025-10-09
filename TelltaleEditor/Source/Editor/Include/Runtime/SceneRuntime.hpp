#pragma once

#include <Core/Config.hpp>
#include <Renderer/RenderContext.hpp>
#include <Common/Common.hpp>

#include <queue>

// SCENE RUNTIME MANAGER.

enum class SceneMessageType : U32
{
    START_INTERNAL, // INTERNAL! do not post this externally. internally starts the scene in the next frame.
    OPEN_SCENE, // Open a file. Takes in TWO std  STRINGs filename and entry point (lua code to run). Send to runtime (scene and agent in scenemessage leave empty).
    ADD_SCENE, // Add scene file. Args are instance of AddSceneInfo
    STOP, // stops the scene rendering and removes it from the stack.
};

// A message that can be sent to a scene. This can be to update one of the agents positions, etc.
struct SceneMessage
{
    
    String Scene; // scene name, if needed. if not used, message is sent to the scene runtime (eg scene open, add, not modifying existing scene)
    Symbol Agent; // scene agent sym, if needed.
    void* Arguments = nullptr; // arguments to this message if needed. Must allocate context.AllocateMessageAruments Freed after use internally.
    SceneMessageType Type; // message type
    U32 Priority = 0; // priority of the message. higher ones are done first.
    
    inline Bool operator<(const SceneMessage& rhs) const
    {
        return Priority < rhs.Priority;
    }
    
};

// SCENE MESSAGE ARGUMENT STRUCTS

struct AddSceneInfo
{
    String FileName, EntryPoint;
    I32 AgentPriority = 1000;
    Bool CallCallbacks = true;
};

enum class SceneRuntimeFlag
{
    USE_LOW_QUALITY_PRELOAD_PACKS = 1,
};

// This class manages running a group of scenes. This includes rendering, audio and scripts.
// This attaches to a render context but expects it be alive the whole time this is object is, so make sure it is.
class SceneRuntime : public SnapshotDependentObject, public RenderLayer
{
public:
    
    // Pushes a scene to the currently rendering scene stack. No accesses to this can be made directly after this (internally held).
    // Stop the scene using a stop message
    void PushScene(Scene&&);
    
    void* AllocateMessageArguments(U32 size); // freed automatically when executing the message (temp aloc)
    
    // Default constructs T.
    template<typename T>
    T* AllocateMessageArguments(U32 arraySize);
    
    void SendMessage(SceneMessage message); // send a message to a scene to do something
    
    void SetUseLowQualityPreloadPacks(Bool bOnOff);
    Bool GetUseLowQualityPreloadPacks();
    
    inline Ptr<ResourceRegistry>& GetResourceRegistry() { return _AttachedRegistry; } // use only while updating
    
    SceneRuntime& operator=(const SceneRuntime&) = delete; // no copy or move
    SceneRuntime(const SceneRuntime&) = delete;
    SceneRuntime(SceneRuntime&&) = delete;
    SceneRuntime& operator=(SceneRuntime&&) = delete;
    
    SceneRuntime(RenderContext& context, const Ptr<ResourceRegistry>& pResourceSystem);
    
    ~SceneRuntime();
    
protected:
    
    inline LuaManager& GetScriptManager() { return _ScriptManager; } // use only while updating
    
    virtual RenderNDCScissorRect AsyncUpdate(RenderFrame& frame, RenderNDCScissorRect scissor, Float deltaTime) override; // render
    
    virtual void AsyncProcessEvents(const std::vector<RuntimeInputEvent>& events) override;
    
    void AsyncProcessGlobalMessage(SceneMessage message);
    
private:
    
    friend class Scene;

    LuaManager _ScriptManager;
    
    Flags _Flags;
    
    std::vector<Ptr<Scene>> _AsyncScenes; // populator job access ONLY (ensure one thread access at a time). list of active rendering scenes. ACTIVE SCENES.
    std::mutex _MessagesLock; // for below
    std::priority_queue<SceneMessage> _AsyncMessages; // messages stack
    
    Ptr<ResourceRegistry> _AttachedRegistry; // attached resource registry

};

template<typename T>
T* SceneRuntime::AllocateMessageArguments(U32 arraySize)
{
    T* pMemory = (T*)AllocateMessageArguments(arraySize * sizeof(T));
    for(U32 i = 0; i < arraySize; i++)
        new (pMemory + i) T();
    return pMemory;
}
