#include <Runtime/SceneRuntime.hpp>
#include <AnimationManager.hpp>

// SCENE RUNTIME CLASS IMPL

#define INTERNAL_START_PRIORITY 0x0F0C70FF

SceneRuntime::SceneRuntime(RenderContext& context, const Ptr<ResourceRegistry>& pResourceSystem)
        : RenderLayer("Scene Runtime", context), GameDependentObject("Scene Runtime Object")
{
    _AttachedRegistry = pResourceSystem;
    _ScriptManager.Initialise(Meta::GetInternalState().GetActiveGame().LVersion);
    
    // add lua collections to script VM
    InjectFullLuaAPI(_ScriptManager, true); // treat as worker
    _ScriptManager.PushOpaque(this);
    ScriptManager::SetGlobal(_ScriptManager, "_SceneRuntime", true);
    
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
                Ptr<SceneAgent> pAgent{};
                if(msg.Agent)
                {
                    auto it = scene._Agents.find(msg.Agent);
                    if(it != scene._Agents.end())
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
                        scene.OnAsyncRenderDetach(*this);
                        _AsyncScenes.erase(sceneit); // remove the scene. done.
                    }
                    else
                    {
                        scene.AsyncProcessRenderMessage(*this, msg, pAgent.get());
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

// ACTUAL SCENE RUNTIME


// Called whenever a message for the scene needs to be processed.
void Scene::AsyncProcessRenderMessage(SceneRuntime& context, SceneMessage message, const SceneAgent* pAgent)
{
    
}

// TESTING !!
static Vector3 cameraPosition = Vector3(0.0f, 0.0f, -10.0f); // Camera position in world space
static Vector3 cameraRot{};
static Float dt = 0.0f;
static Vector2 lastMouse{};
static Vector2 mouseAmount{};
static Ptr<PlaybackController> testAnimController{};

void Scene::AsyncProcessEvents(SceneRuntime& context, const std::vector<RuntimeInputEvent>& events)
{
    for (auto& event : events)
    {
        if (event.Code == InputCode::MOUSE_MOVE)
        {
            if (context.IsKeyDown(InputCode::MOUSE_LEFT_BUTTON))
            {
                Float xCursorDiff = event.X - lastMouse.x;
                Float yCursorDiff = event.Y - lastMouse.y;
                
                // Apply sensitivity
                mouseAmount.x -= (PI_F * xCursorDiff * 0.6f); // Yaw
                mouseAmount.y -= (PI_F * yCursorDiff * 0.6f); // Pitch
                
                // Clamp pitch
                mouseAmount.y = fmaxf(-PI_F * 0.5f, fminf(mouseAmount.y, PI_F * 0.5f));
                
                cameraRot = Vector3(mouseAmount.y, mouseAmount.x, 0.0f); // pitch, yaw
            }
            lastMouse = Vector2(event.X, event.Y);
        }
        
        // Calculate direction vectors
        Vector3 forward = Vector3(
                                  cosf(cameraRot.x) * sinf(cameraRot.y),
                                  0.0f, // keep movement horizontal
                                  cosf(cameraRot.x) * cosf(cameraRot.y)
                                  );
        forward.Normalize();
        
        Vector3 right = Vector3(
                                sinf(cameraRot.y - PI_F * 0.5f),
                                0.0f,
                                cosf(cameraRot.y - PI_F * 0.5f)
                                );
        right.Normalize();
        
        Vector3 up = -Vector3::Cross(forward, right);
        up.Normalize();
        
        // Movement speed
        const float speed = 0.25f;
        
        // Movement input
        if (event.Type == InputMapper::EventType::BEGIN)
        {
            switch (event.Code)
            {
                case InputCode::W:
                    event.SetHandled();
                    cameraPosition += forward * speed;
                    break;
                case InputCode::S:
                    event.SetHandled();
                    cameraPosition -= forward * speed;
                    break;
                case InputCode::A:
                    event.SetHandled();
                    cameraPosition -= right * speed;
                    break;
                case InputCode::D:
                    event.SetHandled();
                    cameraPosition += right * speed;
                    break;
                case InputCode::UP_ARROW:
                    event.SetHandled();
                    cameraPosition += up * speed;
                    break;
                case InputCode::DOWN_ARROW:
                    event.SetHandled();
                    cameraPosition -= up * speed;
                    break;
                default:
                    break;
            }
            _ViewStack[0].SetWorldPosition(cameraPosition);
        }
    }
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
    Camera cam{};
    cam.SetHFOV(50.f);
    cam.SetNearClip(0.2f);
    cam.SetFarClip(1000.f);
    context.GetWindowSize(cam._ScreenWidth, cam._ScreenHeight);
    cam.SetAspectRatio();
    
    cam.SetWorldPosition(cameraPosition);
    
    _ViewStack.push_back(cam);
    _Flags.Add(SceneFlags::RUNNING);
    
    // load an animation
    Handle<Animation> hAnim0{};
    //hAnim0.SetObject(context.GetResourceRegistry(), "thorn_face_angry.anm", false, true);
    // or 5421568.anm "thorn_face_happy.anm"  "cs_introthorn_sk03_thorn.anm"
    //testAnimController = PlayAnimation("Agent", hAnim0.GetObject(context.GetResourceRegistry(), true));
    //testAnimController->SetLooping(true);
    //testAnimController->Play();
}

// Called when the scene is being stopped
void Scene::OnAsyncRenderDetach(SceneRuntime& context)
{
    //test stuff
    testAnimController.reset();
}

void a(SceneRuntime& rtContext, RenderFrame& frame, Ptr<Node> node, RenderViewPass* pass, Camera* cam)
{
    Quaternion q{};
    q.SetEuler(cameraRot.x, cameraRot.y, cameraRot.z);
    Colour cl = Colour::Green;
    for(auto c = node->FirstChild; c; c = c->NextSibling)
    {
        Transform glob = Scene::GetNodeWorldTransform(c);
        RenderUtility::DrawWireBox(rtContext.GetRenderContext(), cam,
                                      MatrixRotation(q) * MatrixTransformation(Vector3(0.005f), glob._Rot, glob._Trans),
                                        cl, pass);
        a(rtContext, frame, c, pass, cam);
    }
}

// The role of this function is populate the renderer with draw commands for the scene, ie go through renderables and draw them
void Scene::PerformAsyncRender(SceneRuntime& rtContext, RenderFrame& frame, Float deltaTime)
{
    dt = deltaTime;
    
    // 1. UPDATE AGENTS AND SYSTEMS
    
    // UPDATE CONTROLLERS
    for(PlaybackController* pController: _Controllers)
        pController->Advance(deltaTime);
    
    Flags animFlags{}; // UPDATE ANIM MANAGERS
    animFlags.Add(AnimationManagerApplyMask::ALL);
    for(auto mgr: _AnimationMgrs)
        mgr->UpdateAnimation(frame.FrameNumber, animFlags, deltaTime);
    
    // 2. RENDER
    
    // CAMERA PARAMETERS (ALL OBJECTS SHARE COMMON CAMERA) AND BASE SETUP
    RenderContext& context = rtContext.GetRenderContext();
    
    ShaderParameter_Camera* cam = frame.Heap.NewNoDestruct<ShaderParameter_Camera>();
    RenderUtility::SetCameraParameters(context, cam, &_ViewStack.front());
    
    ShaderParametersGroup* camGroup = context.AllocateParameter(frame, ShaderParameterType::PARAMETER_CAMERA);
    context.SetParameterUniform(frame, camGroup, ShaderParameterType::PARAMETER_CAMERA, cam, sizeof(ShaderParameter_Camera));
    
    // SCENE MAIN VIEW
    RenderSceneViewParams viewParams{};
    viewParams.ViewType = RenderViewType::MAIN;
    RenderSceneView* pMainView = frame.PushView(viewParams, false);
    pMainView->PushViewParameters(context, camGroup); // attach camera parameters
    
    // MAIN DIFFUSE PASS
    RenderViewPassParams diffuseParams{};
    diffuseParams.PassFlags.Add(RenderViewPassParams::DO_CLEAR_COLOUR);
    diffuseParams.PassFlags.Add(RenderViewPassParams::DO_CLEAR_DEPTH);
    diffuseParams.PassFlags.Add(RenderViewPassParams::DO_CLEAR_STENCIL);
    diffuseParams.ClearDepth = 1.0f;
    diffuseParams.ClearStencil = 0;
    diffuseParams.ClearColour = Colour::Black;
    RenderViewPass* pDiffusePass = pMainView->PushPass(diffuseParams);
    pDiffusePass->SetRenderTarget(0, RenderTargetID(RenderTargetConstantID::BACKBUFFER), 0, 0);
    pDiffusePass->SetDepthTarget(RenderTargetID(RenderTargetConstantID::DEPTH), 0, 0);
    pDiffusePass->SetName("Diffuse");
    
    // this can be different per material but for now use constant
    RenderStateBlob globalRenderState{};
    globalRenderState.SetValue(RenderStateType::Z_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_WRITE_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_COMPARE_FUNC, SDL_GPU_COMPAREOP_LESS_OR_EQUAL);
    
    for(auto& renderable: _Renderables) // push draw for each material simple
    {
        Transform agentTransform = GetNodeWorldTransform(renderable.AgentNode);
        Matrix4 agentWorld = MatrixTransformation(agentTransform._Rot, agentTransform._Trans);
        for(auto& meshInstancePtr: renderable.Renderable.MeshList)
        {
            if(!context.TouchResource(meshInstancePtr))
                continue;
            auto& meshInstance = *meshInstancePtr.get();
            
            // TODO move below to setup mesh dirty
            for(auto& vstate: meshInstance.VertexStates)
            {
                if(vstate.IndexBuffer.BufferData && !vstate.RuntimeData.GPUIndexBuffer)
                {
                    vstate.RuntimeData.GPUIndexBuffer = context.CreateIndexBuffer(vstate.IndexBuffer.BufferSize);
                    frame.UpdateList->UpdateBufferMeta(vstate.IndexBuffer, vstate.RuntimeData.GPUIndexBuffer, 0);
                }
                for(U32 i = 0; i < vstate.Default.NumVertexBuffers; i++)
                {
                    if(vstate.VertexBuffers[i].BufferData && !vstate.RuntimeData.GPUVertexBuffers[i])
                    {
                        vstate.RuntimeData.GPUVertexBuffers[i] = context.CreateVertexBuffer(vstate.VertexBuffers[i].BufferSize);
                        frame.UpdateList->UpdateBufferMeta(vstate.VertexBuffers[i], vstate.RuntimeData.GPUVertexBuffers[i], 0);
                    }
                }
            }
            
            // TODO move to texture setup dirty
            Ptr<ResourceRegistry> reg = rtContext.GetResourceRegistry();
            for(auto& material: meshInstance.Materials)
            {
                Ptr<RenderTexture> tex = material.DiffuseTexture.GetObject(reg, true);
                if(tex && context.TouchResource(tex))
                {
                    tex->EnsureMip(&context, 0);
                }
            }
            
            // TODO render mesh sort out. BONE MATRIX
            Bool bDeformable = meshInstance.MeshFlags.Test(Mesh::MeshFlags::FLAG_DEFORMABLE);
            Bool bHasBoneMatrices = false;
            if(bDeformable)
            {
                Ptr<Node>& agentNode = renderable.AgentNode;
                SkeletonInstance* pSkeletonInstance = agentNode->GetObjDataByType<SkeletonInstance>();
                if(pSkeletonInstance)
                {
                    U32 numBones = (U32)pSkeletonInstance->GetSkeleton()->GetEntries().size();
                    if(!meshInstance.RuntimeData.BoneMatrixBuffer)
                    {
                        String name = agentNode->AgentName + " BoneBuffer";
                        meshInstance.RuntimeData.BoneMatrixBuffer = context.CreateGenericBuffer(64 * numBones, name);
                    }
                    Meta::BinaryBuffer uploadBuffer{};
                    uploadBuffer.BufferData = TTE_PROXY_PTR((U8*)pSkeletonInstance->GetCurrentPose(), U8);
                    uploadBuffer.BufferSize = 64 * numBones;
                    frame.UpdateList->UpdateBufferMeta(uploadBuffer, meshInstance.RuntimeData.BoneMatrixBuffer, 0);
                    bHasBoneMatrices = true;
                }
            }
            
            // For each LOD
            for(auto& lod : meshInstance.LODs)
            {
                // For default and shadow. For now skip shadow.
                for(U32 i = 0; i < 1; i++)
                {
                    // For each batch
                    for(auto& batch : lod.Batches[i])
                    {
                        
                        // EACH OBJECT PARAMETERS
                        
                        ShaderParameter_Object* obj = frame.Heap.NewNoDestruct<ShaderParameter_Object>();
                        Quaternion rot{};
                        rot.SetEuler(cameraRot.x, cameraRot.y, cameraRot.z);
                        
                        RenderUtility::SetObjectParameters(context, obj, MatrixRotation(rot) * agentWorld, Colour::White);
                        
                        // PARAMETER SETUP
                        ShaderParameterTypes required{};
                        required.Set(ShaderParameterType::PARAMETER_OBJECT, true);
                        required.Set(ShaderParameterType::PARAMETER_INDEX0IN, true);
                        required.Set(ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, true);
                        if(bHasBoneMatrices)
                            required.Set(ShaderParameterType::PARAMETER_GENERIC0, true);
                        for(U32 buf = 0; buf < meshInstance.VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
                        {
                            required.Set((ShaderParameterType)(ShaderParameterType::PARAMETER_VERTEX0IN + buf), true);
                        }
                        
                        ShaderParametersGroup* objGroup = context.AllocateParameters(frame, required);
                        
                        // BONE MATRIX
                        if(bHasBoneMatrices)
                        {
                            context.SetParameterGenericBuffer(frame, objGroup, ShaderParameterType::PARAMETER_GENERIC0,
                                                              meshInstance.RuntimeData.BoneMatrixBuffer, 0);
                        }
                        
                        // DIFFUSE TEXTURE
                        Ptr<RenderTexture> diffuseTex = meshInstance.Materials[batch.MaterialIndex].DiffuseTexture.GetObject(reg, true);
                        if(diffuseTex && context.TouchResource(diffuseTex))
                        {
                            context.SetParameterTexture(frame, objGroup,ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE,diffuseTex, nullptr);
                        }
                        else
                        { // else default to white tex
                            context.SetParameterDefaultTexture(frame, objGroup, ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, DefaultRenderTextureType::WHITE, 0);
                        }
                        
                        // OBJECT UNIFORM
                        context.SetParameterUniform(frame, objGroup, ShaderParameterType::PARAMETER_OBJECT, obj, sizeof(ShaderParameter_Object));
                        
                        // VERTEX AND INDEX BUFFERS
                        for(U32 buf = 0; buf < meshInstance.VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
                        {
                            context.SetParameterVertexBuffer(frame, objGroup,
                                                             (ShaderParameterType)(ShaderParameterType::PARAMETER_VERTEX0IN + buf),
                                                             meshInstance.VertexStates[lod.VertexStateIndex].RuntimeData.GPUVertexBuffers[buf], 0);
                        }
                        
                        context.SetParameterIndexBuffer(frame, objGroup, ShaderParameterType::PARAMETER_INDEX0IN,
                                                        meshInstance.VertexStates[lod.VertexStateIndex].RuntimeData.GPUIndexBuffer, 0);
                        
                        // DRAW
                        RenderInst inst{};
                        inst.SetVertexState(meshInstance.VertexStates[lod.VertexStateIndex].Default);
                        inst.SetShaderProgram(bHasBoneMatrices ? "MeshDeformable" : "Mesh");
                        inst.DrawPrimitives(RenderPrimitiveType::TRIANGLE_LIST, batch.StartIndex, batch.NumPrimitives, 1, batch.BaseIndex);
                        inst.GetRenderState() = globalRenderState;
                        pDiffusePass->PushRenderInst(context, std::move(inst), objGroup);
                    }
                }
            }
        }
        a(rtContext, frame, renderable.AgentNode->FirstChild, pDiffusePass, &_ViewStack.front());
    }
    
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
