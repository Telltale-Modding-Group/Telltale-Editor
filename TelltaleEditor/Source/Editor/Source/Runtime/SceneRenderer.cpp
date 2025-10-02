#include <Runtime/SceneRenderer.hpp>

SceneRenderer::SceneRenderer(Ptr<RenderContext> pRenderContext) : _Renderer(pRenderContext), _CallbackTag(0)
{
    _CallbackTag = GetTimeStamp() ^ ((U64)((std::uintptr_t)this));
    //Ptr<FunctionBase> pPostCallback = ALLOCATE_METHOD_CALLBACK_1(this, _PostRenderCallback, SceneRenderer, RenderFrame*);
    //pRenderContext->GetPostRenderMainThreadCallbacks().PushCallback(pPostCallback);
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
    if(pScene != _CurrentScene.MyScene.lock())
    {
        for (auto it = _PersistentScenes.begin(); it != _PersistentScenes.end(); )
        {
            if (it->MyScene.expired())
            {
                it = _PersistentScenes.erase(it);
                continue; // skip
            }

            Ptr<Scene> pCur = it->MyScene.lock();
            if (pCur == pScene)
            {
                SceneState cache = std::move(_CurrentScene);
                _CurrentScene = std::move(*it);
                it = _PersistentScenes.erase(it);
                if (!cache.MyScene.expired())
                {
                    _PersistentScenes.push_back(std::move(cache));
                }
                return;
            }
            else
            {
                ++it;
            }
        }
        SceneState cache = std::move(_CurrentScene);
        _InitSceneState(_CurrentScene, pScene);
        if (!cache.MyScene.expired())
        {
            _PersistentScenes.push_back(std::move(cache));
        }
    }
}

void SceneRenderer::_PostRenderCallback(RenderFrame* pFrame)
{
    // Disabled for now
}

void SceneRenderer::_InitSceneState(SceneState& state, Ptr<Scene> pScene)
{
    state.MyScene = WeakPtr<Scene>(pScene);

    if(!state.DefaultCam)
    {
        state.DefaultCam = TTE_NEW_PTR(Camera, MEMORY_TAG_RENDERER);
    }

    // Push a default view camera bottom of the view stack (so at least always one cam)
    state.DefaultCam->SetHFOV(50.f);
    state.DefaultCam->SetNearClip(0.2f);
    state.DefaultCam->SetFarClip(1000.f);
    state.DefaultCam->SetWorldPosition({0,0, 10.f});
    pScene->_ViewStack.push_back(state.DefaultCam);
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
        //if(!pMeshInstance->MeshFlags.Test(Mesh::FLAG_DEFORMABLE))
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

void SceneRenderer::RenderScene(const SceneFrameRenderParams& frameRender) // Scissor is the final copied size. render tW and tH to target as normal.
{
    // SETUP SCENE RENDER STATE ASSOCIATED WITH pScenes
    if(!frameRender.RenderScene)
        return;
    _SwitchSceneState(frameRender.RenderScene);
    _CurrentScene.Viewport = frameRender.Viewport;

    RenderViewport vp = frameRender.Viewport.GetAsViewport();
    vp.x *= (Float)frameRender.TargetWidth;  vp.w *= (Float)frameRender.TargetWidth;
    vp.y *= (Float)frameRender.TargetHeight;  vp.h *= (Float)frameRender.TargetHeight;

    // CAMERA PARAMETERS (ALL OBJECTS SHARE COMMON CAMERA) AND BASE SETUP
    RenderContext& context = *_Renderer;
    RenderFrame& frame = context.GetFrame(true);

    ShaderParameter_Camera* cam = frame.Heap.NewNoDestruct<ShaderParameter_Camera>();
    Camera& drawCam = *frameRender.RenderScene->_ViewStack.front().lock(); // Top most camera
    drawCam._ScreenWidth = (U32)vp.w, drawCam._ScreenHeight = (U32)vp.h;
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
    pDiffusePass->SetViewport(vp);
    pDiffusePass->SetRenderTarget(0, frameRender.Target, 0, 0);
    pDiffusePass->SetDepthTarget(RenderTargetID(RenderTargetConstantID::DEPTH), 0, 0);
    pDiffusePass->SetName("%s Diffuse", frameRender.RenderScene->GetName().c_str());

    // this can be different per material but for now use constant
    RenderStateBlob globalRenderState{};
    globalRenderState.SetValue(RenderStateType::Z_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_WRITE_ENABLE, true);
    globalRenderState.SetValue(RenderStateType::Z_COMPARE_FUNC, SDL_GPU_COMPAREOP_LESS_OR_EQUAL);

    for(SceneModule<SceneModuleType::RENDERABLE>& renderable: frameRender.RenderScene->_Modules.GetModuleArray<SceneModuleType::RENDERABLE>())
    {
        Transform agentWorld = Scene::GetNodeWorldTransform(renderable.AgentNode);
        for(Ptr<Mesh::MeshInstance>& meshInstance: renderable.Renderable.MeshList)
        {
            _RenderMeshInstance(frameRender.RenderScene, frame, meshInstance, agentWorld, pDiffusePass, globalRenderState);
        }
    }

    if (frameRender.RenderPost != nullptr)
        frameRender.RenderPost(frameRender.UserData, frameRender, pMainView);

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
