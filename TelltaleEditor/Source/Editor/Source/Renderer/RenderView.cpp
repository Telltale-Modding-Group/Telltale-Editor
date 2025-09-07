#include <Renderer/RenderContext.hpp>

RenderSceneView* RenderFrame::PushView(RenderSceneViewParams params, Bool bFront)
{
    RenderSceneView* view = Heap.NewNoDestruct<RenderSceneView>();
    view->Frame = this;
    view->Name = "View";
    view->Params = std::move(params);
    if(bFront)
    {
        if(Views)
            Views->Prev = view;
        view->Next = Views;
        view->Prev = nullptr;
        Views = view;
    }
    else
    {
        RenderSceneView* last = Views;
        if(last)
        {
            while (last->Next)
                last = last->Next;
            last->Next = view;
        } else Views = view;
        view->Prev = last;
        view->Next = nullptr;
    }
    return view;
}

void RenderViewPass::PushRenderInst(RenderContext& context, RenderInst &&inst)
{
    RenderFrame& frame = *SceneView->Frame;
    RenderInst& instance = DrawCalls.EmplaceBack(frame.Heap);
    instance = std::move(inst);
    instance._Parameters = nullptr;
}

void RenderViewPass::PushRenderInst(RenderContext& context, RenderInst &&inst, ShaderParametersGroup* params)
{
    if(!params)
    {
        PushRenderInst(context, std::move(inst));
        return;
    }
    RenderFrame& frame = *SceneView->Frame;
    
    TTE_ASSERT(frame.Heap.Contains(params), "Shader parameter stack must be allocated from render context!");
    if(inst._DrawDefault == DefaultRenderMeshType::NONE && (inst._VertexStateInfo.NumVertexBuffers == 0 || inst._VertexStateInfo.NumVertexAttribs == 0))
        TTE_ASSERT(false, "Cannot push a render instance draw if the vertex state has not been specified");
    
    RenderInst& instance = DrawCalls.EmplaceBack(frame.Heap);
    instance = std::move(inst);
    instance._Parameters = params;
}

RenderViewPass* RenderSceneView::PushPass(RenderViewPassParams params)
{
    RenderViewPass* pass = Frame->Heap.New<RenderViewPass>();
    pass->Params = params;
    pass->SceneView = this;
    pass->Name = "Pass";
    if(PassList == nullptr)
    {
        PassList = PassListTail = pass;
        pass->Next = pass->Prev = nullptr;
    }
    else
    {
        pass->Next = PassList;
        PassList->Prev = pass;
        pass->Prev = nullptr;
        PassList = pass;
    }
    return pass;
}

void RenderViewPass::PushPassParameters(RenderContext& context, ShaderParametersGroup* pGroup)
{
    context.PushParameterGroup(*SceneView->Frame, &Parameters, pGroup);
}

void RenderViewPass::SetViewport(RenderViewport vp)
{
    Viewport = vp;
}

void RenderSceneView::PushViewParameters(RenderContext &context, ShaderParametersGroup *pGroup)
{
    context.PushParameterGroup(*Frame, &Parameters, pGroup);
}

void RenderInst::SetDebugName(RenderViewPass* ScenePass, CString format, ...)
{
#ifdef DEBUG
    U8 Buf[0x200];
    va_list va{};
    va_start(va, format);
    U32 len = vsnprintf((char*)Buf, 0x200, format, va);
    va_end(va);
    U8* str = ScenePass->SceneView->Frame->Heap.Alloc(len + 1);
    str[len] = 0;
    memcpy(str, Buf, len);
    _DebugName = (CString)str;
#endif
}

void RenderViewPass::SetName(CString format, ...)
{
    U8 Buf[0x200];
    va_list va{};
    va_start(va, format);
    U32 len = vsnprintf((char*)Buf, 0x200, format, va);
    va_end(va);
    U8* str = SceneView->Frame->Heap.Alloc(len + 1);
    str[len] = 0;
    memcpy(str, Buf, len);
    Name = (CString)str;
}

void RenderSceneView::SetName(CString format, ...)
{
    U8 Buf[0x200];
    va_list va{};
    va_start(va, format);
    U32 len = vsnprintf((char*)Buf, 0x200, format, va);
    va_end(va);
    U8* str = Frame->Heap.Alloc(len + 1);
    str[len] = 0;
    memcpy(str, Buf, len);
    Name = (CString)str;
}

// RENDER TARGET MANAGEMENTS

void RenderViewPass::SetRenderTarget(U32 index, RenderTargetID id, U32 mip, U32 slice)
{
    TTE_ASSERT(index < 8, "Invalid index");
    TargetRefs.Target[index].ID = id;
    TargetRefs.Target[index].Mip = mip;
    TargetRefs.Target[index].Slice = slice;
}

void RenderViewPass::SetDepthTarget(RenderTargetID id, U32 mip, U32 slice)
{
    TargetRefs.Depth.ID = id;
    TargetRefs.Depth.Mip = mip;
    TargetRefs.Depth.Slice = slice;
}

const RenderTargetDesc& GetRenderTargetDesc(RenderTargetConstantID id)
{
    const RenderTargetDesc* pDesc = TargetDescs;
    while(pDesc)
    {
        if(pDesc->ID == id)
            return *pDesc;
        if(pDesc->ID == RenderTargetConstantID::NONE)
            return *pDesc;
        pDesc++;
    }
    TTE_ASSERT(false, "Render target descriptor enums not updated or ID is wrong");
    return TargetDescs[0]; // never happen
}

RenderTexture* RenderContext::_ResolveBackBuffers(RenderCommandBuffer& buf, 
                                                  SDL_GPUCommandBuffer* preAcquire, SDL_GPUTexture* bb)
{
    if(!_ConstantTargets[(U32)RenderTargetConstantID::BACKBUFFER])
        _ConstantTargets[(U32)RenderTargetConstantID::BACKBUFFER] = AllocateRuntimeTexture();
    Flags texFlags{};
    texFlags.Add(RenderTexture::TEXTURE_FLAG_DELEGATED);
    RenderSurfaceFormat fmt = FromSDLFormat(SDL_GetGPUSwapchainTextureFormat(_Device, _Window));
    U32 sw, sh{};
    SDL_GPUTexture* stex{};
    if(preAcquire && bb)
    {
        stex = bb;
    }
    else
    {
        TTE_ASSERT(SDL_WaitAndAcquireGPUSwapchainTexture(buf._Handle,
                   _Window, &stex, &sw, &sh),
           "Failed to acquire backend swapchain texture: %s", SDL_GetError());
    }
    _ConstantTargets[(U32)RenderTargetConstantID::BACKBUFFER]->CreateTarget(*this, texFlags, fmt, sw, sh, 1, 1, 1, false, stex);
    return _ConstantTargets[(U32)RenderTargetConstantID::BACKBUFFER].get();
}

void RenderContext::_ResolveTarget(RenderFrame& frame, const RenderTargetIDSurface &surface, RenderTargetResolvedSurface &resolved, U32 w, U32 h)
{
    TTE_ASSERT(IsCallingFromMain(), "Only render main call");
    resolved.Mip = surface.Mip;
    resolved.Slice = surface.Slice;
    if(surface.ID.IsDynamicTarget())
    {
        // Dynamic target. Ignore w & h.
        resolved.Target = _ResolveDynamicTarget(surface.ID).get();
        TTE_ASSERT(resolved.Target, "The dynamic target ID %d could not be resolved", surface.ID._Value);
    }
    else
    {
        // Constant target
        Ptr<RenderTexture>& texture = _ConstantTargets[surface.ID._Value];
        if((surface.ID._Value != (U32)RenderTargetConstantID::BACKBUFFER) && (!texture || texture->_Width != w || texture->_Height != h)) // do we need w and h here?
        {
            if(!texture)
                texture = AllocateRuntimeTexture();
            const RenderTargetDesc& desc = GetRenderTargetDesc((RenderTargetConstantID)surface.ID._Value);
            texture->_Name = GetRenderTargetDesc((RenderTargetConstantID)surface.ID._Value).Name;
            texture->CreateTarget(*this, {}, desc.Format, w, h, desc.NumMips, desc.NumSlices, desc.ArraySize, desc.DepthTarget);
        }
        resolved.Target = texture.get();
    }
}

void RenderContext::_ResolveTargets(RenderFrame& frame, const RenderTargetIDSet &ids, RenderTargetSet &set)
{
    U32 width{}, height{};
    SDL_GetWindowSize(_Window, (int*)&width, (int*)&height);
    U32 i = 0;
    while(i < 8 && ids.Target[i].ID.IsValid())
    {
        _ResolveTarget(frame, ids.Target[i], set.Target[i], width, height);
        i++;
    }
    if(ids.Depth.ID.IsValid())
        _ResolveTarget(frame, ids.Depth, set.Depth, width, height);
}

void RenderContext::_PrepareRenderPass(RenderFrame& frame, RenderPass& pass, const RenderViewPass& viewInfo)
{
    _ResolveTargets(frame, viewInfo.TargetRefs, pass.Targets);
    if (viewInfo.Viewport.w > 0.5f && viewInfo.Viewport.h > 0.5f)
    {
        pass.ClearViewport = viewInfo.Viewport;
    }
    pass.ClearColour = viewInfo.Params.ClearColour;
    pass.ClearDepth = viewInfo.Params.ClearDepth;
    pass.ClearStencil = viewInfo.Params.ClearStencil;
    pass.DoClearColour = viewInfo.Params.PassFlags.Test(RenderViewPassParams::DO_CLEAR_COLOUR);
    pass.DoClearDepth = viewInfo.Params.PassFlags.Test(RenderViewPassParams::DO_CLEAR_DEPTH);
    pass.DoClearStencil = viewInfo.Params.PassFlags.Test(RenderViewPassParams::DO_CLEAR_STENCIL);
}

void RenderContext::_ExecutePass(RenderFrame &frame, RenderSceneContext &context, RenderViewPass *pass)
{
    RenderCommandBuffer& cmds = *context.CommandBuf;
    
    // SORT DRAW CALLS
    RenderInst** sortedRenderInsts = (RenderInst**)frame.Heap.Alloc(sizeof(RenderInst*) * pass->DrawCalls.GetSize());
    U32 i = 0;
    for(auto it = pass->DrawCalls.Begin(); it != pass->DrawCalls.End(); i++, it++)
    {
        sortedRenderInsts[i] = &(*it);
    }
    std::sort(sortedRenderInsts, sortedRenderInsts + pass->DrawCalls.GetSize(), RenderInstSorter{}); // sort by sort key
    
    // SETUP RENDER PASS
    RenderPass passDesc{};
    _PrepareRenderPass(frame, passDesc, *pass);
    RenderSurfaceFormat depthFormat = passDesc.Targets.Depth.Target ? passDesc.Targets.Depth.Target->_Format : RenderSurfaceFormat::UNKNOWN;
    RenderSurfaceFormat colourFormats[8]{};
    U32 x = 0;
    while(x < 8 && passDesc.Targets.Target[x].Target)
    {
        colourFormats[x] = passDesc.Targets.Target[x].Target->_Format;
        x++;
    }
    U32 nTargets = x;
    while(x < 8)
        colourFormats[x++] = RenderSurfaceFormat::UNKNOWN;
    passDesc.Name = pass->Name;
    context.CommandBuf->StartPass(std::move(passDesc));

    if(pass->Viewport.w > 0.5f && pass->Viewport.h > 0.5f)
    {
        SDL_GPUViewport gpuViewport{};
        gpuViewport.x = pass->Viewport.x;
        gpuViewport.y = pass->Viewport.y;
        gpuViewport.w = pass->Viewport.w;
        gpuViewport.h = pass->Viewport.h;
        gpuViewport.min_depth = 0.0f; gpuViewport.max_depth = 1.0f;
        SDL_SetGPUViewport(context.CommandBuf->_CurrentPass->_Handle, &gpuViewport);
    }
    
    // DRAW
    for(i = 0; i < pass->DrawCalls.GetSize(); i++)
    {
        RenderInst& inst = *sortedRenderInsts[i];
        
        DefaultRenderMesh* pDefaultMesh = nullptr;
        RenderPipelineState pipelineDesc{};
        if(inst._DrawDefault == DefaultRenderMeshType::NONE)
        {
            TTE_ASSERT(inst.EffectRef, "Render instance effect variant not set");
            
            // Find pipeline state for draw
            pipelineDesc.PrimitiveType = inst._PrimitiveType;
            pipelineDesc.VertexState = inst._VertexStateInfo;
            pipelineDesc.EffectHash = inst.EffectRef.EffectHash;
        }
        else
        {
            for(auto& def: _DefaultMeshes)
            {
                if(def.Type == inst._DrawDefault)
                {
                    pipelineDesc = def.PipelineDesc; // use default
                    pDefaultMesh = &def;
                    break;
                }
            }
            TTE_ASSERT(pDefaultMesh, "Default mesh not found or not initialised yet");
            // set num indices
            inst._IndexCount = pDefaultMesh->NumIndices;
        }
        
        pipelineDesc.DepthFormat = depthFormat;
        x = 0;
        while(x < 8 && colourFormats[x] != RenderSurfaceFormat::UNKNOWN)
        {
            pipelineDesc.TargetFormat[x] = colourFormats[x];
            x++;
        }
        while(x < 8)
            pipelineDesc.TargetFormat[x++] = RenderSurfaceFormat::UNKNOWN;
        pipelineDesc.NumColourTargets = nTargets;
        
        pipelineDesc.RState = inst._RenderState;
        
        Ptr<RenderPipelineState> state = _FindPipelineState(std::move(pipelineDesc));
        TTE_ASSERT(state, "Pipeline state retrieval failed");
        
        // bind pipeline (if needed of course)
        if(cmds._BoundPipeline != state)
            cmds.BindPipeline(state);
        
        ShaderParametersStack stack{};
        if(inst._Parameters)
        {
            PushParameterGroup(frame, &stack, inst._Parameters);
        }
        
        // default meshes we set the index and vertex buffer
        if(inst._DrawDefault != DefaultRenderMeshType::NONE)
        {
            // push high level parameter stack with index and vertex buffer
            ShaderParameterTypes params{};
            params.Set(ShaderParameterType::PARAMETER_VERTEX0IN, true);
            params.Set(ShaderParameterType::PARAMETER_INDEX0IN, true);
            
            ShaderParametersGroup* inputGroup = AllocateParameters(frame, params);
            
            SetParameterVertexBuffer(frame, inputGroup, ShaderParameterType::PARAMETER_VERTEX0IN,
                                     pDefaultMesh->VertexBuffer, 0);
            SetParameterIndexBuffer(frame, inputGroup, ShaderParameterType::PARAMETER_INDEX0IN,
                                    pDefaultMesh->IndexBuffer, 0);
            
            PushParameterGroup(frame, &stack, inputGroup);
        }
        
        // bind shader inputs
        PushParameterStack(frame, &stack, &pass->Parameters);
        cmds.BindParameters(frame, &stack);
        
        // draw!
        cmds.DrawIndexed(inst._IndexCount, inst._InstanceCount, inst._StartIndex, inst._BaseIndex, 0);
    }
    
    // END
    context.CommandBuf->EndPass();
}

// execute the scene view
void RenderContext::_ExecuteFrame(RenderFrame &frame, RenderSceneContext& context)
{
    RenderSceneView* pCurrentView = frame.Views;
    while(pCurrentView)
    {
        
        for(RenderViewPass* pass = pCurrentView->PassListTail; pass; pass = pass->Prev)
        {
            PushParameterStack(frame, &pass->Parameters, &pCurrentView->Parameters);
            _ExecutePass(frame, context, pass);
        }
        
        pCurrentView = pCurrentView->Next;
    }
}
