#pragma once

#include <Core/Config.hpp>
#include <Renderer/RenderContext.hpp>
#include <Common/Scene.hpp>

#include <queue>

// SCENE RUNTIME MANAGER.

enum class SceneMessageType : U32
{
    START_INTERNAL, // INTERNAL! do not post this externally. internally starts the scene in the next frame.
    STOP, // stops the scene rendering and removes it from the stack.
};

// A message that can be sent to a scene. This can be to update one of the agents positions, etc.
struct SceneMessage
{
    
    String Scene; // scene name
    Symbol Agent; // scene agent sym, if needed (is a lot)
    void* Arguments = nullptr; // arguments to this message if needed. Must allocate context.AllocateMessageAruments Freed after use internally.
    SceneMessageType Type; // message type
    U32 Priority = 0; // priority of the message. higher ones are done first.
    
    inline Bool operator<(const SceneMessage& rhs) const
    {
        return Priority < rhs.Priority;
    }
    
};

// This class manages running a group of scenes. This includes rendering, audio and scripts.
// This attaches to a render context but expects it be alive the whole time this is object is, so make sure it is.
class SceneRuntime : public GameDependentObject, public RenderLayer
{
public:
    
    // Pushes a scene to the currently rendering scene stack. No accesses to this can be made directly after this (internally held).
    // Stop the scene using a stop message
    void PushScene(Scene&&);
    
    void* AllocateMessageArguments(U32 size); // freed automatically when executing the message (temp aloc)
    
    void SendMessage(SceneMessage message); // send a message to a scene to do something
    
    SceneRuntime& operator=(const SceneRuntime&) = delete; // no copy or move
    SceneRuntime(const SceneRuntime&) = delete;
    SceneRuntime(SceneRuntime&&) = delete;
    SceneRuntime& operator=(SceneRuntime&&) = delete;
    
    SceneRuntime(RenderContext& context, const Ptr<ResourceRegistry>& pResourceSystem);
    
    ~SceneRuntime();
    
protected:
    
    inline const Ptr<ResourceRegistry>& GetResourceRegistry() { return _AttachedRegistry; } // use only while updating
    
    inline LuaManager& GetScriptManager() { return _ScriptManager; } // use only while updating
    
    virtual RenderNDCScissorRect AsyncUpdate(RenderFrame& frame, RenderNDCScissorRect scissor, Float deltaTime) override; // render
    
private:
    
    friend class Scene;

    LuaManager _ScriptManager;
    
    std::vector<Scene> _AsyncScenes; // populator job access ONLY (ensure one thread access at a time). list of active rendering scenes.
    
    std::mutex _MessagesLock; // for below
    std::priority_queue<SceneMessage> _AsyncMessages; // messages stack
    
    Ptr<ResourceRegistry> _AttachedRegistry; // attached resource registry

};
