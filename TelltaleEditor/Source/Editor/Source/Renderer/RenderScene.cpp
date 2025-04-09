#include <Common/Scene.hpp>
#include <Runtime/SceneRuntime.hpp>

// Called whenever a message for the scene needs to be processed.
void Scene::AsyncProcessRenderMessage(SceneRuntime& context, SceneMessage message, const SceneAgent* pAgent)
{
    
}

static Vector3 cameraPosition = Vector3(0.0f, 0.0f, -3.0f); // Camera position in world space
static Vector3 cameraRot{};
static Float dt = 0.0f;
static Vector2 lastMouse{};
static Vector2 mouseAmount{};
// hmm

void Scene::AsyncProcessEvents(SceneRuntime& context, const std::vector<RuntimeInputEvent>& events)
{
    for(auto& event: events)
    {
        if(event.Code == InputCode::MOUSE_MOVE)
        {
            // Only move the camera if the left mouse button is held down
            if(context.IsKeyDown(InputCode::MOUSE_LEFT_BUTTON))
            {
                // Mouse look adjustments
                Float xCursorDifference = event.X - lastMouse.x;
                Float yCursorDifference = event.Y - lastMouse.y;
                
                // Apply mouse movement to yaw and pitch
                mouseAmount.x -= (PI_F * xCursorDifference * 0.6f); // Sensitivity factor
                mouseAmount.y -= (PI_F * yCursorDifference * 0.6f); // Fix y difference usage
                
                // Clamping the pitch to prevent flipping
                mouseAmount.y = fmaxf(-PI_F * 0.5f, fminf(mouseAmount.y, PI_F * 0.5f));
                
                // Update camera rotation
                Vector3 newRotation = Vector3(mouseAmount.y, mouseAmount.x, 0.0f);
                cameraRot = newRotation;
            }
            lastMouse = Vector2(event.X, event.Y);
        }
        else if(event.Code == InputCode::A && event.Type == InputMapper::EventType::BEGIN)
        {
            event.SetHandled();
            Vector3 forward = Vector3(cosf(cameraRot.y), 0.0f, sinf(cameraRot.y));
            cameraPosition += forward * 0.5f;
            _ViewStack[0].SetWorldPosition(cameraPosition);
        }
        else if(event.Code == InputCode::W && event.Type == InputMapper::EventType::BEGIN)
        {
            event.SetHandled();
            Vector3 right = Vector3(-sinf(cameraRot.y), 0.0f, cosf(cameraRot.y));
            cameraPosition += right * 0.5f;
            _ViewStack[0].SetWorldPosition(cameraPosition);
        }
        else if(event.Code == InputCode::D && event.Type == InputMapper::EventType::BEGIN)
        {
            event.SetHandled();
            Vector3 forward = Vector3(cosf(cameraRot.y), 0.0f, sinf(cameraRot.y));
            cameraPosition -= forward * 0.5f;
            _ViewStack[0].SetWorldPosition(cameraPosition);
        }
        else if(event.Code == InputCode::S && event.Type == InputMapper::EventType::BEGIN)
        {
            event.SetHandled();
            Vector3 right = Vector3(-sinf(cameraRot.y), 0.0f, cosf(cameraRot.y));
            cameraPosition -= right * 0.5f;
            _ViewStack[0].SetWorldPosition(cameraPosition);
        }

        
    }
}

// Called when the scene is being started. here just push one view camera
void Scene::OnAsyncRenderAttach(SceneRuntime& context)
{
    Camera cam{};
    cam.SetHFOV(50.f);
    cam.SetNearClip(1.f);
    cam.SetFarClip(1000.f);
    context.GetWindowSize(cam._ScreenWidth, cam._ScreenHeight);
    cam.SetAspectRatio();
    
    cam.SetWorldPosition(cameraPosition);
    
    _ViewStack.push_back(cam);
}

// Called when the scene is being stopped
void Scene::OnAsyncRenderDetach(SceneRuntime& context)
{
    
}

// The role of this function is populate the renderer with draw commands for the scene, ie go through renderables and draw them
void Scene::PerformAsyncRender(SceneRuntime& rtContext, RenderFrame& frame, Float deltaTime)
{
    dt = deltaTime;
    
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
                        RenderUtility::SetObjectParameters(context, obj, MatrixRotation(rot), Colour::White);
                        
                        ShaderParameterTypes required{};
                        required.Set(ShaderParameterType::PARAMETER_OBJECT, true);
                        required.Set(ShaderParameterType::PARAMETER_INDEX0IN, true);
                        required.Set(ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, true);
                        for(U32 buf = 0; buf < meshInstance.VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
                        {
                            required.Set((ShaderParameterType)(ShaderParameterType::PARAMETER_VERTEX0IN + buf), true);
                        }
                        
                        ShaderParametersGroup* objGroup = context.AllocateParameters(frame, required);
                        
                        Ptr<RenderTexture> diffuseTex = meshInstance.Materials[batch.MaterialIndex].DiffuseTexture.GetObject(reg, true);
                        if(diffuseTex && context.TouchResource(diffuseTex))
                        {
                            context.SetParameterTexture(frame, objGroup,ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE,diffuseTex, nullptr);
                        }
                        else
                        { // else default to white tex
                            context.SetParameterDefaultTexture(frame, objGroup, ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, DefaultRenderTextureType::WHITE, 0);
                        }
                        
                        // BIND BUFFERS AND CAMERA OBJECT UNIFORM
                        context.SetParameterUniform(frame, objGroup, ShaderParameterType::PARAMETER_OBJECT, obj, sizeof(ShaderParameter_Object));
                        
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
                        inst.SetShaderProgram("Mesh");
                        inst.DrawPrimitives(RenderPrimitiveType::TRIANGLE_LIST, batch.StartIndex, batch.NumPrimitives, 1, batch.BaseIndex);
                        inst.GetRenderState() = globalRenderState;
                        pDiffusePass->PushRenderInst(context, std::move(inst), objGroup);
                    }
                }
            }
        }
    }
    
}
