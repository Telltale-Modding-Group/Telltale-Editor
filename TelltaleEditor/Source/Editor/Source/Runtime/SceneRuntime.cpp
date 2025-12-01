#include <Runtime/SceneRuntime.hpp>
#include <AnimationManager.hpp>

// SCENE RUNTIME CLASS IMPL

#define INTERNAL_START_PRIORITY 0x0F0C70FF

static U32 luaResourceGetLowQualityPreloadEnabled(LuaManager& man)
{
    ScriptManager::GetGlobal(man, "_SceneRuntime", true);
    Bool bHasRuntime = man.Type(-1) != LuaType::NIL;
    SceneRuntime* runtime = bHasRuntime ? (SceneRuntime*)man.ToPointer(-1) : nullptr;
    man.Pop(1);
    man.PushBool(bHasRuntime ? runtime->GetUseLowQualityPreloadPacks() : false);
    return 1;
}

static U32 luaResourceEnableLowQualityPreload(LuaManager& man)
{
    Bool bUse = man.ToBool(1);
    ScriptManager::GetGlobal(man, "_SceneRuntime", true);
    Bool bHasRuntime = man.Type(-1) != LuaType::NIL;
    SceneRuntime* runtime = bHasRuntime ? (SceneRuntime*)man.ToPointer(-1) : nullptr;
    man.Pop(1);
    if(bHasRuntime && runtime->GetUseLowQualityPreloadPacks() != bUse)
    {
        runtime->SetUseLowQualityPreloadPacks(bUse);
        TTE_LOG("%s Low End Preload Packs", bUse ? "Enabling" : "Disabling");
    }
    return 0;
}

SceneRuntime::SceneRuntime(RenderContext& context, const Ptr<ResourceRegistry>& pResourceSystem)
        : RenderLayer("Scene Runtime", context), SnapshotDependentObject("Scene Runtime Object")
{
    _AttachedRegistry = pResourceSystem;
    _ScriptManager.Initialise(Meta::GetInternalState().GetActiveGame().LVersion);
    
    // add lua collections to script VM
    InjectFullLuaAPI(_ScriptManager, true); // treat as worker
    _ScriptManager.PushOpaque(this);
    ScriptManager::SetGlobal(_ScriptManager, "_SceneRuntime", true);
    _AttachedRegistry->BindLuaManager(_ScriptManager);
    
    LuaFunctionCollection col{};
    PUSH_FUNC(col, "ResourceGetLowQualityPreloadEnabled", &luaResourceGetLowQualityPreloadEnabled, "boolGetLowQualityPreloadEnabled()",
              "Returns false. Here for compatibility reasons.");
    PUSH_FUNC(col, "ResourceEnableLowQualityPreload", &luaResourceEnableLowQualityPreload, "nil ResourceEnableLowQualityPreload(bOnOff)",
              "Toggles whether to use low quality preload packages. Must have an attached scene runtime, ie must be called from a game script running.");
    ScriptManager::RegisterCollection(_ScriptManager, col);
    
    // On Attach
    
}

SceneRuntime::~SceneRuntime()
{
    // On Detach
    
    for(auto& scene: _AsyncScenes)
    {
        scene->OnAsyncRenderDetach(*this);
    }
    
    _ScriptManager.GC();
    _AsyncScenes.clear();
    _AttachedRegistry.reset();
}

void SceneRuntime::AsyncProcessEvents(const std::vector<RuntimeInputEvent> &events)
{
    for(auto it = _AsyncScenes.rbegin(); it != _AsyncScenes.rend(); it++)
    {
        (*it)->AsyncProcessEvents(*this, events);
    }
}

void SceneRuntime::SetUseLowQualityPreloadPacks(Bool bOnOff)
{
    _Flags.Set(SceneRuntimeFlag::USE_LOW_QUALITY_PRELOAD_PACKS, bOnOff);
}

Bool SceneRuntime::GetUseLowQualityPreloadPacks()
{
    return _Flags.Test(SceneRuntimeFlag::USE_LOW_QUALITY_PRELOAD_PACKS);
}

RenderNDCScissorRect SceneRuntime::AsyncUpdate(RenderFrame &frame, RenderNDCScissorRect scissor, Float deltaTime)
{
    // UPDATE RESOURCES
    _AttachedRegistry->Update(MAX(1.0f / 1000.f, MIN(5.f / 1000.f, deltaTime))); // 5 ms cap
    
    // PERFORM SCENE MESSAGES
    _MessagesLock.lock();
    std::priority_queue<SceneMessage> messages = std::move(_AsyncMessages);
    _MessagesLock.unlock();
    
    while(!messages.empty())
    {
        SceneMessage msg = messages.top(); messages.pop();
        if(msg.Type == SceneMessageType::START_INTERNAL)
        {
            // Start a scene (NON TELLTALE SCRIPTING WAY (using scene open / add)
            Ptr<Scene> pScene = TTE_NEW_PTR(Scene, MEMORY_TAG_SCENE_DATA, std::move(*((Scene*)msg.Arguments)));
            TTE_ATTACH_DBG_STR(pScene.get(), "Runtime Scene " + pScene->Name);
            _AsyncScenes.push_back(std::move(pScene));
            _AsyncScenes.back()->OnAsyncRenderAttach(*this); // on attach. ready to prepare this frame.
            TTE_DEL((Scene*)msg.Arguments); // delete the temp alloc
            continue;
        }
        else if(msg.Scene.length() == 0)
        {
            AsyncProcessGlobalMessage(msg);
            if(msg.Arguments)
            {
                TTE_FREE(msg.Arguments); // free any arguments
            }
            continue;;
        }
        for(auto sceneit = _AsyncScenes.begin(); sceneit != _AsyncScenes.end(); sceneit++)
        {
            Ptr<Scene> pScene = *sceneit;
            if(CompareCaseInsensitive(pScene->GetName(), msg.Scene))
            {
                Ptr<SceneAgent> pAgent{};
                if(msg.Agent)
                {
                    auto it = pScene->_Agents.find(msg.Agent);
                    if(it != pScene->_Agents.end())
                    {
                        pAgent = it->second;
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
                        pScene->OnAsyncRenderDetach(*this);
                        _AsyncScenes.erase(sceneit); // remove the scene. done.
                    }
                    else
                    {
                        TTE_ASSERT(pScene->AsyncProcessRenderMessage(*this, msg, pAgent.get()), "Scene message could not be processed!");
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
        scene->PerformAsyncRender(*this, frame, deltaTime);
    
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

// ACTUAL SCENE RUNTIME

void SceneRuntime::AsyncProcessGlobalMessage(SceneMessage message)
{
    if(message.Type == SceneMessageType::OPEN_SCENE) // TODO. PRELOAD PACKS WHEN WE GET TO THEM.
    {
        String* filename = (String*)message.Arguments;
        String* entryPoint = filename + 1;
        
        Handle<Scene> hScene{};
        Ptr<Scene> pScene{};
        hScene.SetObject(_AttachedRegistry, Symbol(*filename), false, true);
        _AsyncScenes.push_back(pScene = hScene.GetObject(_AttachedRegistry, true));
        ScriptManager::RunText(_ScriptManager, *entryPoint, filename->c_str(), false);
        
        filename->~String();
        entryPoint->~String();
    }
    else if(message.Type == SceneMessageType::ADD_SCENE)
    {
        AddSceneInfo* info = (AddSceneInfo*)message.Arguments;
        
        Handle<Scene> hScene{};
        Ptr<Scene> pScene{};
        hScene.SetObject(_AttachedRegistry, Symbol(info->FileName), false, true);
        _AsyncScenes.push_back(pScene = hScene.GetObject(_AttachedRegistry, true));
        ScriptManager::RunText(_ScriptManager, info->EntryPoint, info->FileName.c_str(), false);
        
        info->~AddSceneInfo();
    }
    else
    {
        TTE_ASSERT(false, "Scene message unknown");
    }
}

// Called whenever a message for the scene needs to be processed.
Bool Scene::AsyncProcessRenderMessage(SceneRuntime& context, SceneMessage message, const SceneAgent* pAgent)
{
    return false;
}

void Scene::AsyncProcessEvents(SceneRuntime& context, const std::vector<RuntimeInputEvent>& events)
{
    
}

// Called when the scene is being started. here just push one view camera
void Scene::OnAsyncRenderAttach(SceneRuntime& context)
{
    auto reg = GetRegistry();
    if(reg)
    {
        TTE_ASSERT(reg == context._AttachedRegistry, "Cannot mix resource registries"); // Use one global one.
    }
    else
        _Registry = context._AttachedRegistry;
    
    _SetupAgentsModules();
}

// Called when the scene is being stopped
void Scene::OnAsyncRenderDetach(SceneRuntime& context)
{

}

// The role of this function is populate the renderer with draw commands for the scene, ie go through renderables and draw them
void Scene::PerformAsyncRender(SceneRuntime& rtContext, RenderFrame& frame, Float deltaTime)
{

}

Ptr<PlaybackController> Scene::PlayAnimation(const Symbol &agentName, Ptr<Animation> pAnim)
{
    for(auto& agent: _Agents)
    {
        if(agent.second->NameSymbol == agentName)
        {
            AnimationManager* pManager = agent.second->AgentNode->GetObjData<AnimationManager>("", true);
            pManager->_SetNode(agent.second->AgentNode);
            return pManager->ApplyAnimation(this, std::move(pAnim));
        }
    }
    return nullptr;
}
