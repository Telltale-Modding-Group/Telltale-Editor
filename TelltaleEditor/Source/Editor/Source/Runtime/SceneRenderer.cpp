#include <Runtime/SceneRenderer.hpp>

SceneRenderer::SceneRenderer(Ptr<RenderContext> pRenderContext) : _Renderer(pRenderContext), _CallbackTag(0)
{
    _CallbackTag = GetTimeStamp() ^ ((U64)((std::uintptr_t)this));
    Ptr<FunctionBase> pPostCallback = ALLOCATE_METHOD_CALLBACK_1(this, _PostRenderCallback, SceneRenderer, RenderFrame*);
    pRenderContext->GetPostRenderMainThreadCallbacks().PushCallback(pPostCallback);
}

SceneRenderer::~SceneRenderer()
{
    if(_Renderer)
        _Renderer->GetPostRenderMainThreadCallbacks().RemoveCallbacks(_CallbackTag);
}

void SceneRenderer::SetEditMode(Bool bOnOff)
{
    _Flags.Set(SceneRendererFlag::EDIT_MODE, bOnOff);
}

void SceneRenderer::_SwitchSceneState(Ptr<Scene> pScene)
{
    if (_CurrentScene.MyScene.lock() != pScene)
    {
        // Switch scene state
        for (auto it = _PersistentScenes.begin(); it != _PersistentScenes.end(); it++)
        {
            if (it->MyScene.expired())
            {
                it = _PersistentScenes.erase(it);
            }
            else
            {
                Ptr<Scene> pCur = it->MyScene.lock();
                if(pCur == pScene)
                {
                    SceneState cache = std::move(_CurrentScene);
                    _CurrentScene = std::move(*it);
                    _PersistentScenes.erase(it);
                    if(!cache.MyScene.expired())
                    {
                        _PersistentScenes.push_back(std::move(cache));
                    }
                    break;
                }
            }
        }
        // New
        SceneState cache = std::move(_CurrentScene);
        _InitSceneState(_CurrentScene, pScene);
        if (!cache.MyScene.expired())
        {
            _PersistentScenes.push_back(std::move(cache));
        }
    }
}

void SceneRenderer::_CopyMainTarget(SceneState& state, RenderFrame* pRenderFrame, SDL_GPUCommandBuffer* buf)
{
    if(state.StateFlags.Test(SceneRendererFlag::SCENE_TARGET_COPY_NEEDED))
    {
        state.StateFlags.Remove(SceneRendererFlag::SCENE_TARGET_COPY_NEEDED);
        RenderCommandBuffer copier{};
        copier._Handle = buf;
        copier._Context = _Renderer.get();

        Vector2 backBufferSize = pRenderFrame->BackBufferSize;
        U32 backBufferX = (U32)backBufferSize.x, backBufferY = (U32)backBufferSize.y;
        Vector2 min, max;
        state.Scissor.GetFractional(min.x, min.y, max.x, max.y);
        
        DefaultRenderMesh& quad = _Renderer->_GetDefaultMesh(DefaultRenderMeshType::QUAD);
        RenderPipelineState quadState = quad.PipelineDesc;
        quadState.NumColourTargets = 1;
        quadState.TargetFormat[0] = pRenderFrame->BackBuffer->GetFormat();
        
        RenderTargetIDSurface srcSurface{state.Target, 0, 0};
        RenderTargetResolvedSurface resolvedSrc{};
        _Renderer->_ResolveTarget(*pRenderFrame, srcSurface, resolvedSrc, backBufferX, backBufferY);
        RenderTexture* tex = resolvedSrc.Target;
        U32 sW = 0, sH = 0, sD = 0, sA = 0;
        resolvedSrc.Target->GetDimensions(sW, sH, sD, sA);
        
        // PASS
        RenderPass pass{};
        pass.Name = "Copy Main Target";
        pass.Targets.Target[0] = {tex, 0, 0};
        copier.StartPass(std::move(pass));
        
        // PIPELINE
        SDL_BindGPUGraphicsPipeline(copier._CurrentPass->_Handle, _Renderer->_FindPipelineState(quadState)->_Internal._Handle);
        
        // PARAMS
        ShaderParameter_Camera param_cam{};
        memset(&param_cam, 0, sizeof(param_cam));
        FinalisePlatformMatrix(*_Renderer.get(), param_cam.ViewProj, Matrix4::Identity());
        SDL_PushGPUVertexUniformData(copier._Handle, (U32)ShaderParameterType::PARAMETER_CAMERA - (U32)ShaderParameterType::PARAMETER_FIRST_UNIFORM, &param_cam, sizeof(param_cam));
        ShaderParameter_Object param_obj{};
        RenderUtility::SetObjectParameters(*_Renderer.get(), &param_obj, Matrix4::Identity(), Colour::White);
        SDL_PushGPUVertexUniformData(copier._Handle, (U32)ShaderParameterType::PARAMETER_OBJECT - (U32)ShaderParameterType::PARAMETER_FIRST_UNIFORM, &param_obj, sizeof(param_obj));
        
        // SAMPLER
        SDL_GPUTextureSamplerBinding b{};
        b.texture = tex->GetHandle(_Renderer.get());
        b.sampler = _Renderer->_FindSampler({})->_Handle;
        SDL_BindGPUFragmentSamplers(copier._CurrentPass->_Handle, (U32)ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE - (U32)ShaderParameterType::PARAMETER_FIRST_SAMPLER, &b, 1);
        
        // BUFFERS
        SDL_GPUBufferBinding bind{};
        bind.offset = 0;
        bind.buffer = quad.IndexBuffer->_Handle;
        SDL_BindGPUIndexBuffer(copier._CurrentPass->_Handle, &bind, quad.PipelineDesc.VertexState.IsHalf ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
        bind.buffer = quad.VertexBuffer->_Handle;
        SDL_BindGPUVertexBuffers(copier._CurrentPass->_Handle, 0, &bind, 1);
        
        // DRAW
        SDL_DrawGPUIndexedPrimitives(copier._CurrentPass->_Handle, quad.NumIndices, 1, 0, 0, 0);
        
        copier.EndPass();
    }
}

void SceneRenderer::_PostRenderCallback(RenderFrame* pFrame)
{
    _CopyMainTarget(_CurrentScene, pFrame, pFrame->SDL3MainCommandBuffer->_Handle);
    for(auto& s: _PersistentScenes)
        _CopyMainTarget(s, pFrame, pFrame->SDL3MainCommandBuffer->_Handle);
}

void SceneRenderer::_InitSceneState(SceneState& state, Ptr<Scene> pScene)
{
    state.MyScene = WeakPtr<Scene>(pScene);

    // Push a default view camera bottom of the view stack (so at least always one cam)
    Camera cam{};
    cam.SetHFOV(50.f);
    cam.SetNearClip(0.2f);
    cam.SetFarClip(1000.f);
    cam.SetWorldPosition({0,0, 10.f});
    pScene->_ViewStack.push_back(cam);
}

void SceneRenderer::ResetScene(Ptr<Scene> pScene)
{
    if(_CurrentScene.MyScene.lock() == pScene)
    {
        _CurrentScene = {};
    }
    else
    {
        for(auto it = _PersistentScenes.begin(); it != _PersistentScenes.end();)
        {
            if(it->MyScene.lock() == pScene)
            {
                it = _PersistentScenes.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
}

SceneRenderer::SceneState::VertexStateData::VertexStateData()
{
    Dirty.Clear(true);
}

void SceneRenderer::_RenderMeshBatch(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, Mesh::LODInstance& lod, Mesh::MeshBatch& batch, RenderViewPass* pass, RenderStateBlob blob)
{
    WeakPtr<Mesh::MeshInstance> wk = WeakPtr<Mesh::MeshInstance>(pMeshInstance);
    RenderContext& context = *_Renderer.get();

    ShaderParameter_Object* obj = frame.Heap.NewNoDestruct<ShaderParameter_Object>();

    // PARAMETER SETUP
    ShaderParameterTypes required{};
    required.Set(ShaderParameterType::PARAMETER_OBJECT, true);
    required.Set(ShaderParameterType::PARAMETER_INDEX0IN, true);
    required.Set(ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, true);
    for (U32 buf = 0; buf < pMeshInstance->VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
    {
        required.Set((ShaderParameterType)((U32)ShaderParameterType::PARAMETER_VERTEX0IN + buf), true);
    }
    ShaderParametersGroup* objGroup = context.AllocateParameters(frame, required);
    context.SetParameterUniform(frame, objGroup, ShaderParameterType::PARAMETER_OBJECT, obj, sizeof(ShaderParameter_Object));
    RenderUtility::SetObjectParameters(context, obj, MatrixTransformation(model._Rot, model._Trans), Colour::White);

    // DIFFUSE TEXTURE
    Ptr<RenderTexture> diffuseTex = pMeshInstance->Materials[batch.MaterialIndex].DiffuseTexture.GetObject(pScene->GetRegistry(), true);
    if (diffuseTex && context.TouchResource(diffuseTex))
    {
        context.SetParameterTexture(frame, objGroup, ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, diffuseTex, nullptr);
    }
    else
    { // else default to white tex
        context.SetParameterDefaultTexture(frame, objGroup, ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, DefaultRenderTextureType::WHITE, 0);
    }

    // OBJECT UNIFORM
    context.SetParameterUniform(frame, objGroup, ShaderParameterType::PARAMETER_OBJECT, obj, sizeof(ShaderParameter_Object));

    // VERTEX AND INDEX BUFFERS
    for (U32 buf = 0; buf < pMeshInstance->VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
    {
        context.SetParameterVertexBuffer(frame, objGroup,
                                         (ShaderParameterType)((U32)ShaderParameterType::PARAMETER_VERTEX0IN + buf),
                                         _CurrentScene.MeshData[wk].VertexState[lod.VertexStateIndex].GPUVertexBuffers[buf], 0);
    }

    context.SetParameterIndexBuffer(frame, objGroup, ShaderParameterType::PARAMETER_INDEX0IN, 
                                    _CurrentScene.MeshData[wk].VertexState[lod.VertexStateIndex].GPUIndexBuffer, 0);

    // Effect setup
    RenderEffectFeaturesBitSet variants{};

    // DRAW
    RenderInst inst{};
    inst.SetVertexState(pMeshInstance->VertexStates[lod.VertexStateIndex].Default);
    inst.SetEffectRef(context.GetEffectRef(RenderEffect::MESH, variants));
    inst.DrawPrimitives(RenderPrimitiveType::TRIANGLE_LIST, batch.StartIndex, batch.NumPrimitives, 1, batch.BaseIndex);
    inst.GetRenderState() = blob;
    pass->PushRenderInst(context, std::move(inst), objGroup);
}

void SceneRenderer::_RenderMeshLOD(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, Mesh::LODInstance& lod, RenderViewPass* pass, RenderStateBlob blob)
{
    // Skip shadow for now
    for(Mesh::MeshBatch& batch: lod.Batches[0])
    {
        _RenderMeshBatch(pScene, frame, pMeshInstance, model, lod, batch, pass, blob);
    }
}

void SceneRenderer::_RenderMeshInstance(Ptr<Scene> pScene, RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance, Transform model, RenderViewPass* pass, RenderStateBlob blob)
{
    if(_Renderer->TouchResource(pMeshInstance))
    {
        _UpdateMeshBuffers(frame, pMeshInstance);

        // TODO MOVE THIS (MATERIAL SYS)
        for(auto& material: pMeshInstance->Materials)
        {
            Ptr<RenderTexture> tex = material.DiffuseTexture.GetObject(pScene->GetRegistry(), true);
            if(tex && _Renderer->TouchResource(tex))
            {
                tex->EnsureMip(_Renderer.get(), 0);
            }
        }

        // TODO fix deformable
        if(!pMeshInstance->MeshFlags.Test(Mesh::FLAG_DEFORMABLE))
        {

            for (Mesh::LODInstance& LOD : pMeshInstance->LODs)
            {
                _RenderMeshLOD(pScene, frame, pMeshInstance, model, LOD, pass, blob);
            }

        }

    }
}

void SceneRenderer::_UpdateMeshBuffers(RenderFrame& frame, const Ptr<Mesh::MeshInstance> pMeshInstance)
{
    WeakPtr<Mesh::MeshInstance> wk = WeakPtr<Mesh::MeshInstance>(pMeshInstance);
    auto& data = _CurrentScene.MeshData[wk];
    Bool bIsEmpty = data.VertexState.empty();
    TTE_ASSERT(bIsEmpty || (data.VertexState.size() == pMeshInstance->VertexStates.size()), "Mesh %s was modified when rendering", pMeshInstance->Name.c_str());
    U32 index = 0;
    for(Mesh::VertexState& vertexState: pMeshInstance->VertexStates)
    {
        if(bIsEmpty)
            data.VertexState.emplace_back();
        SceneState::VertexStateData& renderVertexState = data.VertexState[index++];
        if(renderVertexState.Dirty[32]) // 0-31 are vertex, 32 is index
        {
            renderVertexState.GPUIndexBuffer = _Renderer->CreateIndexBuffer(vertexState.IndexBuffer.BufferSize);
            frame.UpdateList->UpdateBufferMeta(vertexState.IndexBuffer, renderVertexState.GPUIndexBuffer, 0);
            renderVertexState.Dirty.Set(32, false);
        }
        for(U32 i = 0; i < 31; i++)
        {
            if (vertexState.VertexBuffers[i].BufferData && renderVertexState.Dirty[i])
            {
                renderVertexState.GPUVertexBuffers[i] = _Renderer->CreateVertexBuffer(vertexState.VertexBuffers[i].BufferSize);
                frame.UpdateList->UpdateBufferMeta(vertexState.VertexBuffers[i], renderVertexState.GPUVertexBuffers[i], 0);
                renderVertexState.Dirty.Set(i, false);
            }
        }
    }
}

void SceneRenderer::RenderScene(Ptr<Scene> pScene, RenderTargetID target, RenderNDCScissorRect sc, U32 tW, U32 tH) // Scissor is the final copied size. render tW and tH to target as normal.
{
    // SETUP SCENE RENDER STATE ASSOCIATED WITH pScenes
    if(!pScene)
        return;
    _SwitchSceneState(pScene);
    _CurrentScene.Target = target;
    _CurrentScene.Scissor = sc;
    if(_CurrentScene.Target != RenderTargetID(RenderTargetConstantID::BACKBUFFER))
    {
        _CurrentScene.StateFlags.Add(SceneRendererFlag::SCENE_TARGET_COPY_NEEDED);
    }

    // CAMERA PARAMETERS (ALL OBJECTS SHARE COMMON CAMERA) AND BASE SETUP
    RenderContext& context = *_Renderer;
    RenderFrame& frame = context.GetFrame(true);

    ShaderParameter_Camera* cam = frame.Heap.NewNoDestruct<ShaderParameter_Camera>();
    Camera& drawCam = pScene->_ViewStack.front(); // Top most camera
    drawCam._ScreenWidth = tW, drawCam._ScreenHeight = tH;
    drawCam.SetAspectRatio();
    RenderUtility::SetCameraParameters(context, cam, &drawCam);

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
    pDiffusePass->SetRenderTarget(0, target, 0, 0);
    pDiffusePass->SetDepthTarget(RenderTargetID(RenderTargetConstantID::DEPTH), 0, 0);
    pDiffusePass->SetName("%s Diffuse", pScene->GetName().c_str());

    // this can be different per material but for now use constant
    RenderStateBlob globalRenderState{};
    globalRenderState.SetValue(RenderStateType::Z_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_WRITE_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_COMPARE_FUNC, SDL_GPU_COMPAREOP_LESS_OR_EQUAL);

    for(SceneModule<SceneModuleType::RENDERABLE>& renderable: pScene->_Renderables)
    {
        Transform agentWorld = Scene::GetNodeWorldTransform(renderable.AgentNode);
        for(Ptr<Mesh::MeshInstance>& meshInstance: renderable.Renderable.MeshList)
        {
            _RenderMeshInstance(pScene, frame, meshInstance, agentWorld, pDiffusePass, globalRenderState);
        }
    }
    for(auto it = _CurrentScene.MeshData.begin(); it != _CurrentScene.MeshData.end();)
    {
        if(it->first.expired())
        {
            it = _CurrentScene.MeshData.erase(it);
        }
        else
        {
            it++;
        }
    }
}
