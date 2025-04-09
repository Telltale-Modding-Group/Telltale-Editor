#include <Runtime/SceneRuntime.hpp>

#define INTERNAL_START_PRIORITY 0x0F0C70FF

SceneRuntime::SceneRuntime(RenderContext& context, const Ptr<ResourceRegistry>& pResourceSystem)
        : RenderLayer("Scene Runtime", context), GameDependentObject("Scene Runtime Object")
{
    _AttachedRegistry = pResourceSystem;
    _ScriptManager.Initialise(Meta::GetInternalState().GetActiveGame().LVersion);
    
    // add lua collections to script VM
    InjectFullLuaAPI(_ScriptManager, true); // treat as worker
    
    // On Attach
    
}

SceneRuntime::~SceneRuntime()
{
    // On Detach
    
    for(auto& scene: _AsyncScenes)
    {
        scene.OnAsyncRenderDetach(*this);
    }
    
    _ScriptManager.GC();
    _AsyncScenes.clear();
    _AttachedRegistry.reset();
}

void SceneRuntime::AsyncProcessEvents(const std::vector<RuntimeInputEvent> &events)
{
    for(auto& event: events)
    {
        if(event.Type == InputMapper::EventType::BEGIN)
            _KeysDown.Set(event.Code, true);
        else if(event.Type == InputMapper::EventType::END)
            _KeysDown.Set(event.Code, false);
    }
    for(auto it = _AsyncScenes.rbegin(); it != _AsyncScenes.rend(); it++)
        it->AsyncProcessEvents(*this, events);
}

RenderNDCScissorRect SceneRuntime::AsyncUpdate(RenderFrame &frame, RenderNDCScissorRect scissor, Float deltaTime)
{
    // PERFORM SCENE MESSAGES
    
    _MessagesLock.lock();
    std::priority_queue<SceneMessage> messages = std::move(_AsyncMessages);
    _MessagesLock.unlock();
    
    while(!messages.empty())
    {
        SceneMessage msg = messages.top(); messages.pop();
        if(msg.Type == SceneMessageType::START_INTERNAL)
        {
            // Start a scene
            _AsyncScenes.emplace_back(std::move(*((Scene*)msg.Arguments)));
            _AsyncScenes.back().OnAsyncRenderAttach(*this); // on attach. ready to prepare this frame.
            TTE_DEL((Scene*)msg.Arguments); // delete the temp alloc
            continue;
        }
        for(auto sceneit = _AsyncScenes.begin(); sceneit != _AsyncScenes.end(); sceneit++)
        {
            Scene& scene = *sceneit;
            if(CompareCaseInsensitive(scene.GetName(), msg.Scene))
            {
                const SceneAgent* pAgent = nullptr;
                if(msg.Agent)
                {
                    auto it = scene._Agents.find(msg.Agent);
                    if(it != scene._Agents.end())
                    {
                        pAgent = &(*it);
                    }
                    else
                    {
                        String sym = SymbolTable::Find(msg.Agent);
                        TTE_LOG("WARN: cannot execute render scene message as agent %s was not found", sym.c_str());
                    }
                }
                if(msg.Agent.GetCRC64() == 0 || pAgent != nullptr)
                {
                    if(msg.Type == SceneMessageType::STOP)
                    {
                        scene.OnAsyncRenderDetach(*this);
                        _AsyncScenes.erase(sceneit); // remove the scene. done.
                    }
                    else
                    {
                        scene.AsyncProcessRenderMessage(*this, msg, pAgent);
                        if(msg.Arguments)
                        {
                            TTE_FREE(msg.Arguments); // free any arguments
                        }
                    }
                }
                break;
            }
        }
    }
    
    // HIGH LEVEL RENDER (ASYNC)
    for(auto& scene : _AsyncScenes)
        scene.PerformAsyncRender(*this, frame, deltaTime);
    
    return scissor; // same scissor
}

void* SceneRuntime::AllocateMessageArguments(U32 sz)
{
    return TTE_ALLOC(sz, MEMORY_TAG_TEMPORARY);
}

void SceneRuntime::SendMessage(SceneMessage message)
{
    if(message.Type == SceneMessageType::START_INTERNAL)
        TTE_ASSERT(message.Priority == INTERNAL_START_PRIORITY, "Cannot post a start scene message"); // priority set internally
    std::lock_guard<std::mutex> _L{_MessagesLock};
    _AsyncMessages.push(std::move(message));
}

void SceneRuntime::PushScene(Scene&& scene)
{
    Scene* pTemporary = TTE_NEW(Scene, MEMORY_TAG_TEMPORARY, std::move(scene)); // should be TTE_ALLOC, but handled separately.
    SceneMessage msg{};
    msg.Arguments = pTemporary;
    msg.Priority = INTERNAL_START_PRIORITY;
    msg.Type = SceneMessageType::START_INTERNAL;
    SendMessage(msg);
}


Bool SceneRuntime::IsKeyDown(InputCode key)
{
    TTE_ASSERT(InputMapper::IsCommonInputCode(key), "Invalid key code"); // ONLY ACCEPT COMMON CODES. Platform ones are mapped.
    return _KeysDown[key];
}
