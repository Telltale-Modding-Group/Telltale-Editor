#include <Renderer/RenderContext.hpp>
#include <Core/Context.hpp>

#include <Core/Thread.hpp>

#include <chrono>
#include <cfloat>
#include <sstream>
#include <iostream>
#include <algorithm>

#define INTERNAL_START_PRIORITY 0x0F0C70FF

// ============================ ENUM MAPPINGS

TextureFormatInfo GetSDLFormatInfo(RenderSurfaceFormat format)
{
    U32 i = 0;
    while(SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        if(SDL_FormatMappings[i].Format == format)
            return SDL_FormatMappings[i];
        i++;
    }
    return {};
}

RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format)
{
    U32 i = 0;
    while(SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        if(SDL_FormatMappings[i].SDLFormat == format)
            return SDL_FormatMappings[i].Format;
        i++;
    }
    return RenderSurfaceFormat::UNKNOWN;
}

// ============================ BUFFER ATTRIBUTE FORMAT ENUM MAPPINGS

static struct AttributeFormatInfo {
    RenderBufferAttributeFormat Format;
    SDL_GPUVertexElementFormat SDLFormat;
    U32 NumIntrinsics = 0; // for float4, this is 4
    U32 IntrinsicSize = 0; // for four ints, three ints, 1 int, etc, this is 4.
    RenderBufferAttributeFormat IntrinsicType = RenderBufferAttributeFormat::UNKNOWN;
    CString ConstantName;
}
SDL_VertexAttributeMappings[13]
{
    {RenderBufferAttributeFormat::F32x1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, 1, 4, RenderBufferAttributeFormat::F32x1, "kCommonMeshFloat1"},
    {RenderBufferAttributeFormat::F32x2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 2, 4, RenderBufferAttributeFormat::F32x1,"kCommonMeshFloat2"},
    {RenderBufferAttributeFormat::F32x3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 3, 4, RenderBufferAttributeFormat::F32x1,"kCommonMeshFloat3"},
    {RenderBufferAttributeFormat::F32x4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 4, 4, RenderBufferAttributeFormat::F32x1,"kCommonMeshFloat4"},
    {RenderBufferAttributeFormat::I32x1, SDL_GPU_VERTEXELEMENTFORMAT_INT, 1, 4, RenderBufferAttributeFormat::I32x1, "kCommonMeshInt1"},
    {RenderBufferAttributeFormat::I32x2, SDL_GPU_VERTEXELEMENTFORMAT_INT2, 2, 4, RenderBufferAttributeFormat::I32x1,"kCommonMeshInt2"},
    {RenderBufferAttributeFormat::I32x3, SDL_GPU_VERTEXELEMENTFORMAT_INT3, 3, 4, RenderBufferAttributeFormat::I32x1,"kCommonMeshInt3"},
    {RenderBufferAttributeFormat::I32x4, SDL_GPU_VERTEXELEMENTFORMAT_INT4, 4, 4, RenderBufferAttributeFormat::I32x1,"kCommonMeshInt4"},
    {RenderBufferAttributeFormat::U32x1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 1, 4, RenderBufferAttributeFormat::U32x1,"kCommonMeshUInt1"},
    {RenderBufferAttributeFormat::U32x2, SDL_GPU_VERTEXELEMENTFORMAT_UINT2, 2, 4, RenderBufferAttributeFormat::U32x1,"kCommonMeshUInt2"},
    {RenderBufferAttributeFormat::U32x3, SDL_GPU_VERTEXELEMENTFORMAT_UINT3, 3, 4, RenderBufferAttributeFormat::U32x1,"kCommonMeshUInt3"},
    {RenderBufferAttributeFormat::U32x4, SDL_GPU_VERTEXELEMENTFORMAT_UINT4, 4, 4, RenderBufferAttributeFormat::U32x1,"kCommonMeshUInt4"},
    {RenderBufferAttributeFormat::UNKNOWN, SDL_GPU_VERTEXELEMENTFORMAT_INVALID, 0, 0, RenderBufferAttributeFormat::F32x1, "kCommonMeshFormatUnknown"},
    // add above this!
};

inline AttributeFormatInfo GetSDLAttribFormatInfo(RenderBufferAttributeFormat format)
{
    U32 i = 0;
    while(SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
    {
        if(SDL_VertexAttributeMappings[i].Format == format)
            return SDL_VertexAttributeMappings[i];
        i++;
    }
    return {};
}

inline RenderBufferAttributeFormat FromSDLAttribFormat(SDL_GPUVertexElementFormat format)
{
    U32 i = 0;
    while(SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
    {
        if(SDL_VertexAttributeMappings[i].SDLFormat == format)
            return SDL_VertexAttributeMappings[i].Format;
        i++;
    }
    return RenderBufferAttributeFormat::UNKNOWN;
}

// ================================== PRIMITIVE TYPE MAPPINGS

static struct PrimitiveTypeInfo {
    RenderPrimitiveType Type;
    SDL_GPUPrimitiveType SDLType;
    CString ConstantName;
}
SDL_PrimitiveMappings[3]
{
    {RenderPrimitiveType::TRIANGLE_LIST, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, "kCommonMeshTriangleList"},
    {RenderPrimitiveType::LINE_LIST, SDL_GPU_PRIMITIVETYPE_LINELIST, "kCommonMeshLineList"},
    {RenderPrimitiveType::UNKNOWN, SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP, "kCommonMeshTriangleStrip"}, // do not add below this, add above
};

void RegisterRenderConstants(LuaFunctionCollection& Col)
{
    U32 i = 0;
    while(SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, SDL_VertexAttributeMappings[i].ConstantName, (U32)SDL_VertexAttributeMappings[i].Format);
        i++;
    }
    i = 0;
    while(SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, SDL_PrimitiveMappings[i].ConstantName, (U32)SDL_PrimitiveMappings[i].Type);
        i++;
    }
    i = 0;
    while(SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, SDL_FormatMappings[i].ConstantName, (U32)SDL_FormatMappings[i].Format);
        i++;
    }
    i = 0;
    while(AttribInfoMap[i].Type != RenderAttributeType::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, AttribInfoMap[i].ConstantName, (U32)AttribInfoMap[i].Type);
        i++;
    }
}

inline SDL_GPUPrimitiveType ToSDLPrimitiveType(RenderPrimitiveType format)
{
    U32 i = 0;
    while(SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        if(SDL_PrimitiveMappings[i].Type == format)
            return SDL_PrimitiveMappings[i].SDLType;
        i++;
    }
    return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

inline RenderPrimitiveType FromSDLPrimitiveType(SDL_GPUPrimitiveType format)
{
    U32 i = 0;
    while(SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        if(SDL_PrimitiveMappings[i].SDLType == format)
            return SDL_PrimitiveMappings[i].Type;
        i++;
    }
    return RenderPrimitiveType::UNKNOWN;
}

// ============================ LIFETIME RENDER MANAGEMENT

static Bool SDL3_Initialised = false;

Bool RenderContext::Initialise()
{
    if(!SDL3_Initialised)
    {
        SDL3_Initialised = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
    }
    return SDL3_Initialised;
}

void RenderContext::Shutdown()
{
    if(SDL3_Initialised)
    {
        SDL_Quit();
        SDL3_Initialised = false;
    }
}

Bool RenderContext::IsRowMajor()
{
    CString device = SDL_GetGPUDeviceDriver(_Device);
    if(CompareCaseInsensitive(device, "metal"))
    {
        return false; // COLUMN MAJOR
    }
    else if(CompareCaseInsensitive(device, "vulkan"))
    {
        return false; // COLUMN MAJOR
    }
    else if(CompareCaseInsensitive(device, "direct3d12"))
    {
        return true; // ROW MAJOR
    }
    else
    {
        TTE_ASSERT(false, "Unknown graphics device driver: %s", device);
        return false;
    }
}

RenderContext::RenderContext(String wName, U32 cap)
{
    TTE_ASSERT(JobScheduler::Instance, "Job scheduler has not been initialised. Ensure a ToolContext exists.");
    TTE_ASSERT(SDL3_Initialised, "SDL3 has not been initialised, or failed to.");
    
    CapFrameRate(cap);
    _HotResourceThresh = DEFAULT_HOT_RESOURCE_THRESHOLD;
    
    _MainFrameIndex = 0;
    _PopulateJob = JobHandle();
    _Frame[0].Reset(*this, 1); // start on frame 1, not 0, 0 was imaginary
    _Frame[1].Reset(*this, 2);
    
    // Create window and renderer
    _Window = SDL_CreateWindow(wName.c_str(), 780, 326, 0);
    _Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC, false, nullptr);
    SDL_ClaimWindowForGPUDevice(_Device, _Window);
    _BackBuffer = SDL_GetWindowSurface(_Window);
    
}

RenderContext::~RenderContext()
{
    
    // END JOBS & END SCENES
    
    if(_PopulateJob)
    {
        // wait for populate job. ignore what it said
        JobScheduler::Instance->Wait(_PopulateJob);
        _PopulateJob.Reset();
    }
    
    for(auto& scene: _AsyncScenes)
    {
        scene.OnAsyncRenderDetach(*this);
    }
    
    _AsyncScenes.clear();
    
    // CLEANUP
    
    for(auto& transfer: _AvailTransferBuffers)
        SDL_ReleaseGPUTransferBuffer(_Device, transfer.Handle);
    
    _DefaultMeshes.clear();
    _DefaultTextures.clear();
    _Samplers.clear();
    _Pipelines.clear();
    _LoadedShaders.clear();
    _AvailTransferBuffers.clear();
    
    _Frame[0].Reset(*this, UINT64_MAX);
    _Frame[1].Reset(*this, UINT64_MAX);
    _Frame[0].Heap.ReleaseAll();
    _Frame[1].Heap.ReleaseAll();
    
    SDL_ReleaseWindowFromGPUDevice(_Device, _Window);
    SDL_DestroyWindow(_Window);
    SDL_DestroyGPUDevice(_Device);
    
}

// ============================ HIGH LEVEL RENDER FUNCTIONS

// USER CALL: called every frame by user to render the previous frame
Bool RenderContext::FrameUpdate(Bool isLastFrame)
{
    TTE_ASSERT(_AttachedRegistry, "No attached resource registry! Please attach one into the render context before rendering.");
    
    // 1. locals
    Bool bUserWantsQuit = false;
    
    // 2. poll SDL events
    SDL_Event e {0};
    while( SDL_PollEvent( &e ) )
    {
        if( e.type == SDL_EVENT_QUIT )
        {
            bUserWantsQuit = true;
        }
    }
    
    // 3. Calculate delta time
    Float dt = 0.0f;
    if(_StartTimeMicros != 0)
    {
        U64 myTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        U64 dtMicros =myTime - _StartTimeMicros;
        if(dtMicros <= 1000 * _MinFrameTimeMS)
        {
            U32 waitMS = (1000 * _MinFrameTimeMS - (U32)dtMicros) / 1000;
            ThreadSleep(waitMS);
            myTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }
        dt = ((Float)(myTime - _StartTimeMicros)) * 1e-6F; // convert to seconds
        _StartTimeMicros = myTime; // reset timer for next frame
    }
    else
    {
        _StartTimeMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                                                                                 std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    // 4. Perform rendering. Create main command buffer to acquire swapchain texture. Then wait on all to finish.
    RenderFrame* pFrame = &GetFrame(false);
    {
        
        // Translate high level render operations, eg draw text, draw verts, into command buffer operations for GPU optimised
        
        {
            
            RenderCommandBuffer* pMainCommandBuffer = _NewCommandBuffer();
            
            // setup swapchain tex
            pFrame->BackBuffer = pFrame->Heap.New<RenderTexture>();
            pFrame->BackBuffer->TextureFlags += RenderTexture::TEXTURE_FLAG_DELEGATED;
            pFrame->BackBuffer->_Context = this;
            pFrame->BackBuffer->Format = FromSDLFormat(SDL_GetGPUSwapchainTextureFormat(_Device, _Window));
            TTE_ASSERT(SDL_WaitAndAcquireGPUSwapchainTexture(pMainCommandBuffer->_Handle,
                                                             _Window, &pFrame->BackBuffer->_Handle,
                                                             &pFrame->BackBuffer->Width,  &pFrame->BackBuffer->Height),
                       "Failed to acquire backend swapchain texture at frame %lld: %s", pFrame->FrameNumber, SDL_GetError());
            
            if(pFrame->FrameNumber == 1)
            {
                // Create defaults
                RegisterDefaultMeshes(*this, pMainCommandBuffer, _DefaultMeshes);
                RegisterDefaultTextures(*this, pMainCommandBuffer, _DefaultTextures);
            }
            else
            {
                _Render(dt, pMainCommandBuffer);
            }
            // command buffer is submitted below
            
        }
        
        // SUBMIT ALL UNSUBMITTED COMMAND buffers
        
        U32 activeCommandBuffers = 0;
        SDL_GPUFence** awaitingFences = pFrame->Heap.NewArray<SDL_GPUFence*>((U32)pFrame->CommandBuffers.size());
        for(auto& commandBuffer: pFrame->CommandBuffers)
        {
            if(!commandBuffer->_Submitted)
                commandBuffer->Submit(); // submit if needed
            
            if(commandBuffer->_SubmittedFence && !commandBuffer->Finished())
                awaitingFences[activeCommandBuffers++] = commandBuffer->_SubmittedFence; // add waiting fences
        }
        
        // WAIT ON ALL ACTIVE COMMAND BUFFERS TO FINISH GPU EXECUTION
        
        if(activeCommandBuffers > 0)
            SDL_WaitForGPUFences(_Device, true, awaitingFences, activeCommandBuffers);
        for(auto& commandBuffer: pFrame->CommandBuffers) // free the fences
        {
            if(commandBuffer->_SubmittedFence != nullptr)
            {
                SDL_ReleaseGPUFence(_Device, commandBuffer->_SubmittedFence);
                commandBuffer->_SubmittedFence = nullptr;
            }
        }
        
        // FREE ANY COLD RESOURCES PAST THE THRESHOLD
        _PurgeColdResources(pFrame);
        
        // RECLAIM TRANSFER BUFFERS
        for(auto& cmd: pFrame->CommandBuffers)
            _ReclaimTransferBuffers(*cmd);
        
        // CLEAR COMMAND BUFFERS.
        pFrame->CommandBuffers.clear();
    }
    
    // 5. Wait for previous populater to complete if needed
    if(_PopulateJob)
    {
        JobScheduler::Instance->Wait(_PopulateJob);
        _PopulateJob.Reset(); // reset handle to empty
    }
    
    // 6. NO POPULATE THREAD JOB ACTIVE, SO NO LOCKING NEEDED UNTIL SUBMIT. Give our new processed frame, and swap.
    
    _MainFrameIndex ^= 1; // swap
    GetFrame(true).Reset(*this, GetFrame(true).FrameNumber + 2); // increase populater frame index (+2 from swap, dont worry)
    
    // free resources which are not needed
    if(_Flags & RENDER_CONTEXT_NEEDS_PURGE)
    {
        _Flags &= ~RENDER_CONTEXT_NEEDS_PURGE;
        PurgeResources();
    }
    _FreePendingDeletions(GetFrame(false).FrameNumber);
    
    // 7. Kick off populater job to render the last thread data (if needed)
    if(!isLastFrame && !bUserWantsQuit)
    {
        JobDescriptor job{};
        job.AsyncFunction = &_Populate;
        job.Priority = JOB_PRIORITY_HIGHEST;
        job.UserArgA = this;
        union {void* a; Float b;} _cvt{};
        _cvt.b = dt;
        job.UserArgB = _cvt.a;
        _PopulateJob = JobScheduler::Instance->Post(job);
    }
    
    return !bUserWantsQuit && !isLastFrame;
}

void RenderContext::_PurgeColdResources(RenderFrame* pFrame)
{
    for(auto it = _AvailTransferBuffers.begin(); it != _AvailTransferBuffers.end();)
    {
        if(it->LastUsedFrame + _HotResourceThresh >= pFrame->FrameNumber)
        {
            SDL_ReleaseGPUTransferBuffer(_Device, it->Handle);
            it = _AvailTransferBuffers.erase(it);
        }
        else ++it;
    }
}

// ============================ FRAME MANAGEMENT

void RenderFrame::Reset(RenderContext& context, U64 newFrameNumber)
{
    CommandBuffers.clear();
    _Autorelease.clear(); // will free ptrs if needed
    FrameNumber = newFrameNumber;
    NumDrawCalls = 0;
    DrawCalls = nullptr;
    Heap.Rollback(); // free all objects, but keep memory
    UpdateList = Heap.New<RenderFrameUpdateList>(context, *this);
}

// ============================ TEXTURES & RENDER TARGET

void RenderTexture::Create2D(RenderContext* pContext, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips)
{
    _Context = pContext;
    if(_Context && !_Handle)
    {
        Width = width;
        Height = height;
        Format = format;
        
        // TODO mip maps.
        
        SDL_GPUTextureCreateInfo info{};
        info.width = width;
        info.height = height;
        info.format = GetSDLFormatInfo(format).SDLFormat;
        info.num_levels = nMips;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        _Handle = SDL_CreateGPUTexture(pContext->_Device, &info);
    }
}

Ptr<RenderSampler> RenderContext::_FindSampler(RenderSampler desc)
{
    std::lock_guard<std::mutex> L{_Lock};
    for(auto& sampler: _Samplers)
    {
        if(desc == *sampler)
            return sampler;
    }
    SDL_GPUSamplerCreateInfo info{};
    info.props = 0;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    info.address_mode_u = (SDL_GPUSamplerAddressMode)desc.WrapU;
    info.address_mode_v = (SDL_GPUSamplerAddressMode)desc.WrapV;
    info.compare_op = SDL_GPU_COMPAREOP_NEVER;
    info.enable_anisotropy = info.enable_compare = false;
    info.min_filter = info.mag_filter = SDL_GPU_FILTER_NEAREST;
    info.max_anisotropy = 1.0f;
    info.mip_lod_bias = desc.MipBias;
    info.mipmap_mode = (SDL_GPUSamplerMipmapMode)desc.MipMode;
    info.min_lod = 0.f;
    info.max_lod = 1000.f;
    desc._Handle = SDL_CreateGPUSampler(_Device, &info);
    desc._Context = this;
    Ptr<RenderSampler> pSampler = TTE_NEW_PTR(RenderSampler, MEMORY_TAG_RENDERER);
    *pSampler = std::move(desc);
    desc._Context = nullptr;
    desc._Handle = nullptr; // sometimes compiler doesnt do move:(
    _Samplers.push_back(pSampler);
    return pSampler;
}

RenderTexture::~RenderTexture()
{
    if(_Handle && _Context && !TextureFlags.Test(TEXTURE_FLAG_DELEGATED))
        SDL_ReleaseGPUTexture(_Context->_Device, _Handle);
    _Handle = nullptr;
}

void RenderContext::_FreePendingDeletions(U64 frame)
{
    AssertMainThread();
    std::lock_guard<std::mutex> G{_Lock};
    for(auto it = _PendingSDLResourceDeletions.begin(); it != _PendingSDLResourceDeletions.end();)
    {
        if(frame - it->_LastUsedFrame > 1)
        {
            TTE_ASSERT(it->_Resource.use_count() == 1, "SDL GPU Resource has been cached when it should not be.");
            _PendingSDLResourceDeletions.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void RenderCommandBuffer::UploadTextureDataSlow(Ptr<RenderTexture> &texture,
                                                DataStreamRef src, U64 srcOffset, U32 mip, U32 slice, U32 dataZ)
{
    if(_CurrentPass)
        TTE_ASSERT(_CurrentPass->_CopyHandle != nullptr, "A render pass cannot be active unless its a copy pass if uploading textures.");
    
    texture->LastUsedFrame = _Context->GetFrame(false).FrameNumber;
    
    _RenderTransferBuffer tBuffer = _Context->_AcquireTransferBuffer(dataZ, *this);
    
    U8* mappedMemorySeg = (U8*)SDL_MapGPUTransferBuffer(_Context->_Device, tBuffer.Handle, false);
    src->SetPosition(srcOffset);
    src->Read(mappedMemorySeg, dataZ);
    SDL_UnmapGPUTransferBuffer(_Context->_Device, tBuffer.Handle);
    
    Bool bStartPass = _CurrentPass == nullptr;
    
    if(bStartPass)
        StartCopyPass();
    
    SDL_GPUTextureTransferInfo srcinf{};
    SDL_GPUTextureRegion dst{};
    srcinf.offset = 0;
    srcinf.rows_per_layer = texture->Height; // 3D textures this will change
    srcinf.pixels_per_row = texture->Width; // assume width tight packing no padding.
    srcinf.transfer_buffer = tBuffer.Handle;
    dst.texture = texture->_Handle;
    dst.mip_level = mip;
    dst.layer = slice;
    dst.x = dst.y = dst.z = 0; // upload whole subimage for any slice/mip for now.
    dst.w = texture->Width;
    dst.h = texture->Height;
    dst.d = 1; // depth count one for now.
    
    SDL_UploadToGPUTexture(_CurrentPass->_CopyHandle, &srcinf, &dst, false);
    
    if(bStartPass)
        EndPass();
}

// ================ PIPELINE STATES, SHADER MGR

Bool RenderContext::_FindProgram(String name, Ptr<RenderShader> &vert, Ptr<RenderShader> &frag)
{
    vert = _FindShader(name + "/Vertex", RenderShaderType::VERTEX);
    frag = _FindShader(name + "/Frag", RenderShaderType::FRAGMENT);
    return vert != nullptr && frag != nullptr;
}

// find a loaded shader, if not found its loaded.
Ptr<RenderShader> RenderContext::_FindShader(String name, RenderShaderType type)
{
    _Lock.lock();
    for(auto& sh: _LoadedShaders)
    {
        if(CompareCaseInsensitive(name, sh->Name))
        {
            _Lock.unlock();
            return sh;
        }
    }
    _Lock.unlock();
    
    // create, not found
    String rawSh{};
    U8* temp = nullptr;
    SDL_GPUShaderFormat fmts = SDL_GetGPUShaderFormats(_Device);
    SDL_GPUShaderCreateInfo info{};
    info.entrypoint = "main0";
    info.stage = type == RenderShaderType::FRAGMENT ? SDL_GPU_SHADERSTAGE_FRAGMENT : SDL_GPU_SHADERSTAGE_VERTEX;
    Bool bin = false;
    
    if(fmts & SDL_GPU_SHADERFORMAT_MSL) // raw metal shader
    {
        rawSh = GetToolContext()->LoadLibraryStringResource("Shader/" + name + ".msl");
        if(rawSh.length() == 0)
        {
            TTE_LOG("ERROR: %s [METAL] could not be loaded. Shader file empty or not found.", name.c_str());
            return nullptr;
        }
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = (const Uint8*)rawSh.c_str();
        info.code_size = rawSh.length();
    }
    // TODO. identify if the .GLSL or .HLSL version exists, if so then compile and save (if dev) as .dxbc/.spv
    else if(fmts & SDL_GPU_SHADERFORMAT_DXBC) // compiled hlsl
    {
        info.format = SDL_GPU_SHADERFORMAT_DXBC;
        DataStreamRef stream = GetToolContext()->LoadLibraryResource("Shader/" + name + ".dxbc");
        if(!stream || stream->GetSize() == 0)
        {
            TTE_LOG("ERROR: %s [D3D] could not be loaded. Shader file empty or not found.", name.c_str());
            return nullptr;
        }
        info.code_size = stream->GetSize();
        temp = TTE_ALLOC(info.code_size, MEMORY_TAG_TEMPORARY);
        stream->Read(temp, (U64)info.code_size);
    }
    else if(fmts & SDL_GPU_SHADERFORMAT_SPIRV) // compiled glsl
    {
        bin = false;
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        DataStreamRef stream = GetToolContext()->LoadLibraryResource("Shader/" + name + ".spv");
        if(!stream || stream->GetSize() <= 8)
        {
            TTE_LOG("ERROR: %s [GL] could not be loaded. Shader file empty or not found.", name.c_str());
            return nullptr;
        }
        info.code_size = stream->GetSize();
        temp = TTE_ALLOC(info.code_size, MEMORY_TAG_TEMPORARY);
        stream->Read(temp, (U64)info.code_size);
    }
    else
    {
        TTE_LOG("ERROR: %s [??] render device is unknown (not DXBC/METAL/SPIRV)", name.c_str());
        return nullptr;
    }
    
    Ptr<RenderShader> sh = TTE_NEW_PTR(RenderShader, MEMORY_TAG_RENDERER);
    
    U32 BindsOf[3]{};
    VertexAttributesBitset set{};
    
    // read binding information. we need to store this.
    // in text, first line of format 'Samplers:10 StorageTextures:1 StorageBuffers:2 GenericBuffers:1
    // in binary, first 4 bytes are 0xFEEDFACE, then the 4 one for each, followed by sampler,bufs,tex,uni map (U8=>U8)
    if(bin) // if compiled read the binary version
    {
        const U8* inf = info.code;
        info.code += 8;
        info.code_size -= 8;
        TTE_ASSERT(!strncmp((const char*)inf, (const char*)"\xFE\xED\xFA\xCE", 4) ||
                   !strncmp((const char*)inf, (const char*)"\xCE\xFA\xED\xFE", 4), "Compiled shader does not have correct magic");
        info.num_samplers = (U32)inf[4];
        info.num_storage_buffers = (U32)inf[5];
        info.num_storage_textures = (U32)inf[6];
        info.num_uniform_buffers = (U32)inf[7];
        U32 running = 0;
        U32 binds = info.num_samplers+info.num_storage_buffers+info.num_uniform_buffers+info.num_storage_textures;
        memcpy(set.Words(), inf + 8, 4);
        const U8* map = inf + 12 + (2*binds);
        info.code += 2*binds + 4;
        info.code_size -= (2*binds + 12);
        for(U32 i = 0; i < binds; i++)
        {
            TTE_ASSERT(sh->ParameterSlots[map[0]] == 0xFF, "Corrupt shader binary"); // must have no binding
            sh->ParameterSlots[map[0]] = map[1];
            ShaderParameterTypeClass cls = ShaderParametersInfo[map[0]].Class;
            if((cls != ShaderParameterTypeClass::UNIFORM_BUFFER && cls != ShaderParameterTypeClass::GENERIC_BUFFER &&
                cls != ShaderParameterTypeClass::SAMPLER) || cls >= (ShaderParameterTypeClass)3)
            {
                TTE_ASSERT(false, "The shader parameter '%s' cannot have its binding slot specified", ShaderParametersInfo[map[0]].Name);
            }
            BindsOf[(U32)cls]++;
            map+=2;
        }
        
    }
    else
    {
        std::string shaderfull = (CString)info.code;
        std::istringstream stream(shaderfull);
        std::string firstLine{};
        
        if (std::getline(stream, firstLine))
        {
            std::istringstream lineStream(firstLine);
            std::string token;
            
            if (!firstLine.empty() && firstLine[0] == '/' && firstLine[1] == '/')
            {
                while (lineStream >> token)
                {
                    size_t colonPos = token.find(':');
                    if (colonPos != std::string::npos)
                    {
                        std::string key = token.substr(0, colonPos);
                        std::string valueStr = token.substr(colonPos + 1);
                        
                        if (key == "Samplers") info.num_samplers = std::stoi(valueStr);
                        else if (key == "StorageTextures") info.num_storage_textures = std::stoi(valueStr);
                        else if (key == "StorageBuffers") info.num_storage_buffers = std::stoi(valueStr);
                        else if (key == "UniformBuffers") info.num_uniform_buffers = std::stoi(valueStr);
                        else if (key == "Attribs")
                        {
                            std::istringstream attribStream(valueStr);
                            std::string attribToken;
                            while (std::getline(attribStream, attribToken, ','))
                            {
                                U32 val = std::stoi(attribToken);
                                set.Set((RenderAttributeType)val, true);
                            }
                        }
                        else {
                            Bool Set = false;
                            for (U32 i = 0; i < PARAMETER_COUNT; i++)
                            {
                                if (key == ShaderParametersInfo[i].Name)
                                {
                                    TTE_ASSERT(sh->ParameterSlots[i] == 0xFF, "Invalid shader binding slot: already set");
                                    sh->ParameterSlots[i] = (U8)std::stoi(valueStr);
                                    
                                    ShaderParameterTypeClass cls = ShaderParametersInfo[i].Class;
                                    if ((cls != ShaderParameterTypeClass::UNIFORM_BUFFER && cls != ShaderParameterTypeClass::GENERIC_BUFFER &&
                                         cls != ShaderParameterTypeClass::SAMPLER) || cls >= (ShaderParameterTypeClass)3) {
                                        TTE_ASSERT(false, "The shader parameter '%s' cannot have its binding slot specified",
                                                   ShaderParametersInfo[i].Name);
                                    }
                                    BindsOf[(U32)cls]++;
                                    
                                    Set = true;
                                    break;
                                }
                            }
                            TTE_ASSERT(Set, "Shader input specification: unknown parameter '%s': see available in RenderParameters.h",
                                       key.c_str());
                        }
                    }
                }
            }
        }
    }
    TTE_ASSERT(BindsOf[(U32)ShaderParameterTypeClass::UNIFORM_BUFFER] == info.num_uniform_buffers &&
               BindsOf[(U32)ShaderParameterTypeClass::GENERIC_BUFFER] == info.num_storage_buffers &&
               BindsOf[(U32)ShaderParameterTypeClass::SAMPLER] == info.num_samplers, "Too many or too little binding slots specified");
    if(type == RenderShaderType::VERTEX)
        TTE_ASSERT(set.CountBits() > 0, "No vertex attributes specified for vertex shader %s", name.c_str());
    
    sh->Name = name;
    sh->Handle = SDL_CreateGPUShader(_Device, &info);
    sh->Context = this;
    sh->Attribs = set;
    
    if(temp)
    {
        TTE_FREE(temp);
        temp = nullptr; // free temp shader
    }
    
    TTE_ASSERT(sh->Handle, "Could not create shader %s", name.c_str());
    
    if(sh->Handle)
    {
        _Lock.lock();
        SDL_GPUShader* ret = sh->Handle;
        _LoadedShaders.push_back(sh);
        _Lock.unlock();
        return sh;
    }
    
    return nullptr;
}

// ======================= PIPELINE STATES

Ptr<RenderPipelineState> RenderContext::_AllocatePipelineState()
{
    auto val = TTE_NEW_PTR(RenderPipelineState, MEMORY_TAG_RENDERER);
    val->_Internal._Context = this;
    
    // add to array
    {
        std::lock_guard<std::mutex> G{_Lock};
        _Pipelines.push_back(val);
    }
    
    return val;
}

Ptr<RenderPipelineState> RenderContext::_FindPipelineState(RenderPipelineState desc)
{
    U64 hash = _HashPipelineState(desc);
    {
        std::lock_guard<std::mutex> G{_Lock};
        for(auto& state: _Pipelines)
            if(state->Hash == hash)
                return state;
    }
    Ptr<RenderPipelineState> state = _AllocatePipelineState();
    desc._Internal._Context = this;
    *state = std::move(desc);
    state->Create();
    return state;
}

U64 RenderContext::_HashPipelineState(RenderPipelineState &state)
{
    U64 Hash = 0;
    
    Hash = CRC64LowerCase((const U8*)state.ShaderProgram.c_str(), (U32)state.ShaderProgram.length(), 0);
    Hash = CRC64((const U8*)&state.VertexState.NumVertexBuffers, 4, Hash);
    Hash = CRC64((const U8*)&state.PrimitiveType, 4, Hash);
    
    for(U32 i = 0; i < state.VertexState.NumVertexAttribs; i++)
    {
        TTE_ASSERT(state.VertexState.Attribs[i].VertexBufferIndex < state.VertexState.NumVertexBuffers, "Invalid buffer index");
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].Format, 4, state.VertexState.IsHalf ? ~Hash : Hash);
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].VertexBufferIndex, 4, Hash);
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].Attrib, 4, Hash);
    }
    
    return Hash;
}

void RenderPipelineState::Create()
{
    if(_Internal._Handle == nullptr)
    {
        _Internal._Context->AssertMainThread();
        TTE_ASSERT(VertexState.NumVertexBuffers < 32 && VertexState.NumVertexAttribs < 32, "Vertex state has too many attributes / buffers");
        
        // calculate hash
        Hash = _Internal._Context->_HashPipelineState(*this);
        
        // create
        SDL_GPUGraphicsPipelineCreateInfo info{};
        
        // TARGETS: for now only use the swapchain render target
        info.target_info.num_color_targets = 1;
        SDL_GPUColorTargetDescription sc{};
        sc.format = SDL_GetGPUSwapchainTextureFormat(_Internal._Context->_Device, _Internal._Context->_Window);
        info.target_info.color_target_descriptions = &sc;
        
        Ptr<RenderShader> vert, frag{};
        TTE_ASSERT(_Internal._Context->_FindProgram(ShaderProgram, vert, frag), "Could not fetch shader program: %s", ShaderProgram.c_str());
        
        info.vertex_shader = vert->Handle;
        info.fragment_shader = frag->Handle;
        
        info.primitive_type = ToSDLPrimitiveType(PrimitiveType);
        
        info.depth_stencil_state.enable_depth_test = false;
        info.depth_stencil_state.enable_depth_write = false;
        info.depth_stencil_state.enable_stencil_test = false;
        
        info.rasterizer_state.enable_depth_clip = false;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        
        info.multisample_state.enable_mask = false;
        
        info.vertex_input_state.num_vertex_buffers = VertexState.NumVertexBuffers;
        info.vertex_input_state.num_vertex_attributes = VertexState.NumVertexAttribs;
        
        SDL_GPUVertexBufferDescription desc[32]{};
        for(U32 i = 0; i < VertexState.NumVertexBuffers; i++)
        {
            desc[i].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            desc[i].instance_step_rate = 0;
            desc[i].slot = i;
            desc[i].pitch = VertexState.BufferPitches[i];
        }
        
        SDL_GPUVertexAttribute attrib[32]{};
        U32 off[32] = {0};
        Bool attribsHave[(U32)RenderAttributeType::COUNT] = {0};
        for(U32 i = 0; i < VertexState.NumVertexAttribs; i++)
        {
            attrib[i].buffer_slot = VertexState.Attribs[i].VertexBufferIndex;
            attrib[i].location = (U32)VertexState.Attribs[i].Attrib;
            attrib[i].offset = off[VertexState.Attribs[i].VertexBufferIndex];
            AttributeFormatInfo inf = GetSDLAttribFormatInfo(VertexState.Attribs[i].Format);
            attrib[i].format = inf.SDLFormat;
            off[VertexState.Attribs[i].VertexBufferIndex] += inf.NumIntrinsics * inf.IntrinsicSize;
            attribsHave[(U32)VertexState.Attribs[i].Attrib] = true;
        }
        
        // check attribs match vertex
        for(U32 attrib = 0; attrib < (U32)RenderAttributeType::COUNT; attrib++)
        {
            Bool bVertRequires = vert->Attribs[(RenderAttributeType)attrib];
            if(bVertRequires && !attribsHave[attrib])
                TTE_ASSERT(false, "Vertex attribute %s is required for program %s, but was not supplied!",
                           AttribInfoMap[attrib].ConstantName, ShaderProgram.c_str());
            // if it does not require but we have bound it, its ok we aren't using it anyway
        }
        
        info.vertex_input_state.vertex_attributes = attrib;
        info.vertex_input_state.vertex_buffer_descriptions = desc;
        
        _Internal._Handle = SDL_CreateGPUGraphicsPipeline(_Internal._Context->_Device, &info);
        TTE_ASSERT(_Internal._Handle != nullptr, "Could not create pipeline state: %s", SDL_GetError());
        
    }
}

RenderPipelineState::~RenderPipelineState()
{
    if(_Internal._Handle)
    {
        _Internal._Context->AssertMainThread();
        SDL_ReleaseGPUGraphicsPipeline(_Internal._Context->_Device, _Internal._Handle);
        _Internal._Handle = nullptr;
    }
}

RenderShader::~RenderShader()
{
    if(Handle && Context)
    {
        Context->AssertMainThread();
        SDL_ReleaseGPUShader(Context->_Device, Handle);
        Handle = nullptr;
    }
}


RenderSampler::~RenderSampler()
{
    if(_Handle && _Context)
    {
        _Context->AssertMainThread();
        SDL_ReleaseGPUSampler(_Context->_Device, _Handle);
        _Handle = nullptr;
    }
}

// ===================== COMMAND BUFFERS

RenderCommandBuffer* RenderContext::_NewCommandBuffer()
{
    AssertMainThread();
    RenderFrame& frame = GetFrame(false);
    RenderCommandBuffer* pBuffer = frame.Heap.New<RenderCommandBuffer>();
    
    pBuffer->_Handle = SDL_AcquireGPUCommandBuffer(_Device);
    pBuffer->_Context = this;
    
    frame.CommandBuffers.push_back(pBuffer);
    
    return pBuffer;
}

void RenderCommandBuffer::StartCopyPass()
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
    
    RenderPass* pPass = _Context->GetFrame(false).Heap.New<RenderPass>();
    pPass->_CopyHandle = SDL_BeginGPUCopyPass(_Handle);
    
    _BoundPipeline.reset();
    _CurrentPass = pPass;
}

void RenderCommandBuffer::StartPass(RenderPass&& pass)
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(pass._Handle == nullptr, "Duplicate render pass!");
    TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
    
    // todo improve this. allow different target descs.
    SDL_GPUColorTargetInfo swChain{};
    swChain.texture = _Context->GetFrame(false).BackBuffer->_Handle;
    swChain.load_op = SDL_GPU_LOADOP_CLEAR;
    swChain.store_op = SDL_GPU_STOREOP_STORE;
    swChain.clear_color.r = pass.ClearCol.r;
    swChain.clear_color.g = pass.ClearCol.g;
    swChain.clear_color.b = pass.ClearCol.b;
    swChain.clear_color.a = pass.ClearCol.a;
    
    RenderPass* pPass = _Context->GetFrame(false).Heap.New<RenderPass>();
    *pPass = std::move(pass);
    pPass->_Handle = SDL_BeginGPURenderPass(_Handle, &swChain, 1, nullptr);
    
    _BoundPipeline.reset();
    _CurrentPass = pPass;
}

void RenderCommandBuffer::DrawIndexed(U32 numIndices, U32 numInstances, U32 indexStart, I32 vertexIndexOffset, U32 firstInstanceIndex)
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(_CurrentPass != nullptr, "Not within a pass. Start a pass before drawing");
    SDL_DrawGPUIndexedPrimitives(_CurrentPass->_Handle, numIndices, numInstances, indexStart, vertexIndexOffset, firstInstanceIndex);
}

void RenderCommandBuffer::BindUniformData(U32 slot, RenderShaderType shader, const void* data, U32 size)
{
    _Context->AssertMainThread();
    TTE_ASSERT(size < 0x10000, "Uniform buffer too large. Consider using a generic buffer");
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    if(shader == RenderShaderType::VERTEX)
        SDL_PushGPUVertexUniformData(_Handle, slot, data, size);
    else if(shader == RenderShaderType::FRAGMENT)
        SDL_PushGPUFragmentUniformData(_Handle, slot, data, size);
}

void RenderCommandBuffer::BindParameters(RenderFrame& frame, ShaderParametersStack *stack)
{
    TTE_ASSERT(_BoundPipeline.get() != nullptr, "No currently bound pipeline.");
    
    ShaderParameterTypes parametersToSet{};
    
    Ptr<RenderShader> vsh, fsh{};
    TTE_ASSERT(_Context->_FindProgram(_BoundPipeline->ShaderProgram, vsh, fsh), "Shader program not found");
    
    for(U8 i = 0; i < (U8)ShaderParameterType::PARAMETER_COUNT; i++)
    {
        if(vsh->ParameterSlots[i] != (U8)0xFF || fsh->ParameterSlots[i] != (U8)0xFF)
            parametersToSet.Set((ShaderParameterType)i, true);
    }
    
    // bind the latest index and vertex buffers pushed always
    Bool BoundIndex = false;
    Bool BoundVertex[PARAMETER_LAST_VERTEX_BUFFER - PARAMETER_FIRST_VERTEX_BUFFER + 1]{};
    
    do{
        
        ShaderParametersGroup* group = stack->Group;
        
        for(U32 i = 0; i < group->NumParameters; i++)
        {
            ShaderParametersGroup::ParameterHeader& paramInfo = group->GetHeader(i);
            
            if(paramInfo.Type == (U8)PARAMETER_INDEX0IN && !BoundIndex) // bind index buffer
            {
                BoundIndex = true;
                Ptr<RenderBuffer> proxy{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
                TTE_ASSERT(proxy.get(), "Required index buffer was not bound or was null");
                BindIndexBuffer(proxy, group->GetParameter(i).GenericValue.Offset, _BoundPipeline->VertexState.IsHalf);
                continue;
            }
            
            if(paramInfo.Type >= (U8)PARAMETER_FIRST_VERTEX_BUFFER && // bind vertex buffer
               paramInfo.Type <= (U8)PARAMETER_LAST_VERTEX_BUFFER && !BoundVertex[paramInfo.Type - PARAMETER_FIRST_VERTEX_BUFFER])
            {
                // bind this vertex buffer
                BoundVertex[paramInfo.Type - PARAMETER_FIRST_VERTEX_BUFFER] = true;
                Ptr<RenderBuffer> proxy{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
                TTE_ASSERT(proxy.get(), "Required vertex buffer was not bound or was null");
                U32 offset = group->GetParameter(i).GenericValue.Offset;
                BindVertexBuffers(&proxy, &offset, (U32)(paramInfo.Type - PARAMETER_FIRST_VERTEX_BUFFER), 1);
                continue;
            }
            
            if(parametersToSet[(ShaderParameterType)paramInfo.Type])
            {
                if(paramInfo.Class == (U8)ShaderParameterTypeClass::UNIFORM_BUFFER)
                {
                    TTE_ASSERT(group->GetParameter(i).UniformValue.Data, "Required uniform buffer was not bound or was null");
                    if(vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                        BindUniformData(vsh->ParameterSlots[paramInfo.Type], RenderShaderType::VERTEX,
                                        group->GetParameter(i).UniformValue.Data, group->GetParameter(i).UniformValue.Size);
                    if(fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                        BindUniformData(fsh->ParameterSlots[paramInfo.Type], RenderShaderType::FRAGMENT,
                                        group->GetParameter(i).UniformValue.Data, group->GetParameter(i).UniformValue.Size);
                }
                else if(paramInfo.Class == (U8)ShaderParameterTypeClass::SAMPLER)
                {
                    if(vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderTexture> tex{group->GetParameter(i).SamplerValue.Texture, &NullDeleter};
                        TTE_ASSERT(tex.get(), "Required texture was not bound or was null");
                        
                        RenderSampler* samDesc = group->GetParameter(i).SamplerValue.SamplerDesc;
                        Ptr<RenderSampler> resolvedSampler = _Context->_FindSampler(samDesc ? *samDesc : RenderSampler{});
                        
                        BindTextures(vsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::VERTEX, &tex, &resolvedSampler);
                    }
                    if(fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderTexture> tex{group->GetParameter(i).SamplerValue.Texture, &NullDeleter};
                        TTE_ASSERT(tex.get(), "Required texture was not bound or was null");
                        
                        RenderSampler* samDesc = group->GetParameter(i).SamplerValue.SamplerDesc;
                        Ptr<RenderSampler> resolvedSampler = _Context->_FindSampler(samDesc ? *samDesc : RenderSampler{});
                        
                        BindTextures(fsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::FRAGMENT, &tex, &resolvedSampler);
                    }
                }
                else if(paramInfo.Class == (U8)ShaderParameterTypeClass::GENERIC_BUFFER)
                {
                    if(vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderBuffer> buf{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
                        TTE_ASSERT(buf.get(), "Required generic buffer was not bound or was null");
                        BindGenericBuffers(vsh->ParameterSlots[paramInfo.Type], 1, &buf, RenderShaderType::VERTEX);
                    }
                    if(fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderBuffer> buf{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
                        TTE_ASSERT(buf.get(), "Required generic buffer was not bound or was null");
                        BindGenericBuffers(fsh->ParameterSlots[paramInfo.Type], 1, &buf, RenderShaderType::FRAGMENT);
                    }
                }
                parametersToSet.Set((ShaderParameterType)paramInfo.Type, false); // done this parameter
            }
        }
        stack = stack->Parent;
    }while(stack);
    
    // any that are not bound use defaults for textures (Could do buffers? not much for such a small thing)
    
    Ptr<RenderSampler> defSampler = _Context->_FindSampler({});
    
    for(U32 i = 0; i < ShaderParameterType::PARAMETER_COUNT; i++)
    {
        if(parametersToSet[(ShaderParameterType)i])
        {
            if(ShaderParametersInfo[i].Class == ShaderParameterTypeClass::SAMPLER)
            {
                // use default tex
                if(vsh->ParameterSlots[i] != (U8)0xFF)
                {
                    BindDefaultTexture(vsh->ParameterSlots[i], RenderShaderType::VERTEX, defSampler, ShaderParametersInfo[i].DefaultTex);
                }
                if(fsh->ParameterSlots[i] != (U8)0xFF)
                {
                    BindDefaultTexture(fsh->ParameterSlots[i], RenderShaderType::FRAGMENT, defSampler, ShaderParametersInfo[i].DefaultTex);
                }
            }
            else
            {
                TTE_ASSERT(false, "Parameter buffer %s was required for %s but was not supplied.",
                           ShaderParametersInfo[i].Name, _BoundPipeline->ShaderProgram.c_str());
            }
        }
    }
    
}

void RenderCommandBuffer::BindTextures(U32 slot, U32 num, RenderShaderType shader,
                                       Ptr<RenderTexture>* pTextures, Ptr<RenderSampler>* pSamplers)
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(_CurrentPass != nullptr, "No active pass");
    TTE_ASSERT(pTextures && pSamplers, "Invalid arguments passed into bind textures");
    TTE_ASSERT(num < 32, "Cannot bind more than 32 texture samplers at one time");
    
    U64 frame = _Context->GetFrame(false).FrameNumber;
    
    SDL_GPUTextureSamplerBinding binds[32]{};
    for(U32 i = 0; i < num; i++)
    {
        binds[i].sampler = pSamplers[i]->_Handle;
        binds[i].texture = pTextures[i]->_Handle;
        pTextures[i]->LastUsedFrame = frame;
        pSamplers[i]->LastUsedFrame = frame;
    }
    
    if(shader == RenderShaderType::FRAGMENT)
        SDL_BindGPUFragmentSamplers(_CurrentPass->_Handle, slot, binds, num);
    else if(shader == RenderShaderType::VERTEX)
        SDL_BindGPUVertexSamplers(_CurrentPass->_Handle, slot, binds, num);
    
}

void RenderCommandBuffer::DrawDefaultMesh(DefaultRenderMeshType type)
{
    _Context->AssertMainThread();
    for(auto& def: _Context->_DefaultMeshes)
    {
        if(def.Type == type)
        {
            DrawIndexed(def.NumIndices, 1, 0, 0, 0);
            return;
        }
    }
    TTE_ASSERT(false, "Default mesh could not be found");
}

void RenderCommandBuffer::BindDefaultTexture(U32 slot, RenderShaderType shaderType,
                                             Ptr<RenderSampler> sampler, DefaultRenderTextureType type)
{
    _Context->AssertMainThread();
    for(auto& def: _Context->_DefaultTextures)
    {
        if(def.Type == type)
        {
            BindTextures(slot, 1, shaderType, &def.Texture, &sampler);
            return;
        }
    }
    TTE_ASSERT(false, "Default texture could not be found");
}

void RenderCommandBuffer::BindDefaultMesh(DefaultRenderMeshType type)
{
    _Context->AssertMainThread();
    for(auto& def: _Context->_DefaultMeshes)
    {
        if(def.Type == type)
        {
            BindPipeline(def.PipelineState);
            BindIndexBuffer(def.IndexBuffer, 0, true);
            BindVertexBuffers(&def.VertexBuffer, nullptr, 0, 1);
            return;
        }
    }
    TTE_ASSERT(false, "Default mesh could not be found");
}

void RenderCommandBuffer::BindGenericBuffers(U32 slot, U32 num, Ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot)
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(_CurrentPass != nullptr, "Already within a pass. End the current pass before starting a new one.");
    TTE_ASSERT(pBuffers, "Invalid arguments passed into bind generic buffers");
    TTE_ASSERT(num < 4, "Cannot bind more than 4 generic buffers at one time");
    
    U64 frame = _Context->GetFrame(false).FrameNumber;
    
    SDL_GPUBuffer* buffers[4];
    for(U32 i = 0; i < num; i++)
    {
        buffers[i] = pBuffers[i]->_Handle;
        pBuffers[i]->LastUsedFrame = frame;
    }
    
    if(shaderSlot == RenderShaderType::VERTEX)
        SDL_BindGPUVertexStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
    else if(shaderSlot == RenderShaderType::FRAGMENT)
        SDL_BindGPUFragmentStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
    else
        TTE_ASSERT(false, "Unknown render shader type");
    
}

RenderPass RenderCommandBuffer::EndPass()
{
    _Context->AssertMainThread();
    TTE_ASSERT(_CurrentPass, "No active pass");
    
    if(_CurrentPass->_Handle)
        SDL_EndGPURenderPass(_CurrentPass->_Handle);
    else
        SDL_EndGPUCopyPass(_CurrentPass->_CopyHandle);
    RenderPass pass = std::move(*_CurrentPass);
    pass._Handle = nullptr;
    pass._CopyHandle = nullptr;
    _CurrentPass = nullptr;
    _BoundPipeline.reset();
    return pass;
}

void RenderCommandBuffer::Submit()
{
    TTE_ASSERT(_Handle && !_SubmittedFence && !_Submitted, "Command buffer state is invalid");
    TTE_ASSERT(_CurrentPass == nullptr, "Cannot submit command buffer: EndPass() not called before submission");
    
    _SubmittedFence = SDL_SubmitGPUCommandBufferAndAcquireFence(_Handle);
    _Submitted = true;
}

void RenderCommandBuffer::Wait()
{
    if(_Submitted && _SubmittedFence && _Context)
    {
        // wait
        SDL_WaitForGPUFences(_Context->_Device, true, &_SubmittedFence, 1);
        SDL_ReleaseGPUFence(_Context->_Device, _SubmittedFence);
        _SubmittedFence = nullptr;
        _Context->_ReclaimTransferBuffers(*this);
    }
}

Bool RenderCommandBuffer::Finished()
{
    if(_Context && _SubmittedFence)
    {
        Bool result = SDL_QueryGPUFence(_Context->_Device, _SubmittedFence);
        
        // if done, free fence
        if(result)
        {
            SDL_ReleaseGPUFence(_Context->_Device, _SubmittedFence);
            _SubmittedFence = nullptr;
            _Context->_ReclaimTransferBuffers(*this);
        }
        return result;
    }
    else return true; // nothing submitted
}

void RenderCommandBuffer::BindPipeline(Ptr<RenderPipelineState> &state)
{
    TTE_ASSERT(state && state->_Internal._Handle, "Invalid pipeline state");
    if(_Context)
    {
        TTE_ASSERT(_CurrentPass && _CurrentPass->_Handle, "No active non copy pass");
        _Context->AssertMainThread();
        SDL_BindGPUGraphicsPipeline(_CurrentPass->_Handle, state->_Internal._Handle);
        _BoundPipeline = state;
    }
}

// ============================ BUFFERS

Ptr<RenderBuffer> RenderContext::CreateVertexBuffer(U64 sizeBytes)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::VERTEX;
    
    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    
    return buffer;
}

Ptr<RenderBuffer> RenderContext::CreateGenericBuffer(U64 sizeBytes)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::UNIFORM;
    
    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    
    return buffer;
}

Ptr<RenderBuffer> RenderContext::CreateIndexBuffer(U64 sizeBytes)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::INDICES;
    
    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    
    return buffer;
}

void RenderCommandBuffer::BindVertexBuffers(Ptr<RenderBuffer> *buffers, U32 *offsets, U32 first, U32 num)
{
    TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
    TTE_ASSERT(buffers && first + num <= 32, "Invalid arguments passed into BindVertexBuffers");
    
    U64 frame = _Context->GetFrame(false).FrameNumber;
    
    SDL_GPUBufferBinding binds[32]{};
    for(U32 i = 0; i < num; i++)
    {
        TTE_ASSERT(buffers[i] && buffers[i]->_Handle, "Invalid buffer passed in for binding slot %d: is null", first+i);
        binds[i].offset = offsets ? offsets[i] : 0;
        binds[i].buffer = buffers[i]->_Handle;
        buffers[i]->LastUsedFrame = frame;
    }
    
    SDL_BindGPUVertexBuffers(_CurrentPass->_Handle, first, binds, num);
    
}

void RenderCommandBuffer::BindIndexBuffer(Ptr<RenderBuffer> indexBuffer, U32 firstIndex, Bool isHalf)
{
    TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
    TTE_ASSERT(indexBuffer && indexBuffer->_Handle, "Invalid arguments passed into BindIndexBuffer");
    
    SDL_GPUBufferBinding bind{};
    bind.offset = firstIndex * (isHalf ? 2 : 4);
    bind.buffer = indexBuffer->_Handle;
    indexBuffer->LastUsedFrame = _Context->GetFrame(false).FrameNumber;
    
    SDL_BindGPUIndexBuffer(_CurrentPass->_Handle, &bind, isHalf ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
    
}

void RenderCommandBuffer::UploadBufferDataSlow(Ptr<RenderBuffer>& buffer, DataStreamRef srcStream, U64 src, U32 destOffset, U32 numBytes)
{
    
    if(_CurrentPass)
        TTE_ASSERT(_CurrentPass->_CopyHandle != nullptr, "A render pass cannot be active unless its a copy pass if uploading buffers.");
    
    _RenderTransferBuffer tBuffer = _Context->_AcquireTransferBuffer(numBytes, *this);
    
    U8* mappedMemorySeg = (U8*)SDL_MapGPUTransferBuffer(_Context->_Device, tBuffer.Handle, false);
    srcStream->SetPosition(src);
    srcStream->Read(mappedMemorySeg, numBytes);
    SDL_UnmapGPUTransferBuffer(_Context->_Device, tBuffer.Handle);
    
    Bool bStartPass = _CurrentPass == nullptr;
    
    if(bStartPass)
        StartCopyPass();
    
    SDL_GPUTransferBufferLocation srcinf{};
    SDL_GPUBufferRegion dst{};
    srcinf.offset = 0;
    srcinf.transfer_buffer = tBuffer.Handle;
    dst.size = numBytes;
    dst.offset = destOffset;
    dst.buffer = buffer->_Handle;
    
    SDL_UploadToGPUBuffer(_CurrentPass->_CopyHandle, &srcinf, &dst, false);
    
    if(bStartPass)
        EndPass();
    
}

void RenderContext::_ReclaimTransferBuffers(RenderCommandBuffer& cmds)
{
    std::lock_guard<std::mutex> L{_Lock};
    for(auto& buffer: cmds._AcquiredTransferBuffers)
    {
        buffer.Size = 0;
        _AvailTransferBuffers.insert(std::move(buffer));
    }
    cmds._AcquiredTransferBuffers.clear();
}

_RenderTransferBuffer RenderContext::_AcquireTransferBuffer(U32 size, RenderCommandBuffer &cmds)
{
    std::lock_guard<std::mutex> L{_Lock};
    for(auto it = _AvailTransferBuffers.begin(); it != _AvailTransferBuffers.end(); it++)
    {
        
        if(size >= it->Size)
            continue;
        
        // remove it, use this one
        _RenderTransferBuffer buffer = std::move(*it);
        buffer.Size = size;
        buffer.LastUsedFrame = cmds._Context->GetFrame(false).FrameNumber;
        _AvailTransferBuffers.erase(it);
        cmds._AcquiredTransferBuffers.push_back(buffer);
        return buffer;
        
    }
    
    // make new
    _RenderTransferBuffer buffer{};
    TTE_ROUND_UPOW2_U32(buffer.Capacity, size); // keep to powers of two.
    buffer.Size = size;
    SDL_GPUTransferBufferCreateInfo inf{};
    inf.size = buffer.Capacity;
    inf.props = 0;
    inf.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    buffer.Handle = SDL_CreateGPUTransferBuffer(_Device, &inf);
    buffer.LastUsedFrame = cmds._Context->GetFrame(false).FrameNumber;
    cmds._AcquiredTransferBuffers.push_back(buffer);
    
    return buffer;
}

RenderBuffer::~RenderBuffer()
{
    if(_Handle && _Context)
        SDL_ReleaseGPUBuffer(_Context->_Device, _Handle);
    _Handle = nullptr;
}

// ================================== PARAMETERS

void RenderContext::PushParameterStack(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersStack* stack)
{
    if(stack->Parent)
    {
        PushParameterStack(frame, self, stack->Parent);
    }
    if(stack->Group)
    {
        PushParameterGroup(frame, self, stack->Group);
        self->ParameterTypes.Import(stack->ParameterTypes);
    }
}

void RenderContext::PushParameterGroup(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersGroup* group)
{
    if(self->Group)
    {
        ShaderParametersStack* stack = frame.Heap.NewNoDestruct<ShaderParametersStack>();
        stack->Group = self->Group;
        stack->Parent = self->Parent;
        stack->ParameterTypes = self->ParameterTypes;
        self->Parent = stack;
    }
    self->Group = group;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        self->ParameterTypes.Set((ShaderParameterType)group->GetHeader(i).Type, true);
    }
}

ShaderParametersGroup* RenderContext::_CreateParameterGroup(RenderFrame& frame, ShaderParameterTypes types)
{
    U32 N = types.CountBits();
    U32 allocSize = sizeof(ShaderParametersGroup) + ((sizeof(ShaderParametersGroup::ParameterHeader) +
                                                      sizeof(ShaderParametersGroup::Parameter)) * N);
    ShaderParametersGroup* pGroup = (ShaderParametersGroup*)frame.Heap.Alloc(allocSize);
    pGroup->NumParameters = N;
    ShaderParameterType param = (ShaderParameterType)0;
    for(U32 i = 0; i < N; i++)
    {
        param = types.FindFirstBit(param);
        pGroup->GetHeader(i).Class = (U8)ShaderParametersInfo[param].Class;
        pGroup->GetHeader(i).Type = (U8)param;
        param = (ShaderParameterType)(param + 1);
    }
    return pGroup;
}

void RenderContext::SetParameterDefaultTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                               DefaultRenderTextureType tex, RenderSampler* sampler)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::SAMPLER,
                       "Parameter %s is not a sampler!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    ShaderParametersGroup::Parameter& param = group->GetParameter(ind);
    param.SamplerValue.Texture = _GetDefaultTexture(tex).get(); // no autorelease needed, its constant
    param.SamplerValue.SamplerDesc = sampler;
}

void RenderContext::SetParameterTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                        Ptr<RenderTexture> tex, RenderSampler* sampler)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::SAMPLER,
                       "Parameter %s is not a sampler!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    frame.PushAutorelease(tex);
    group->GetParameter(ind).SamplerValue.Texture = tex.get();
    group->GetParameter(ind).SamplerValue.SamplerDesc = sampler;
}

void RenderContext::SetParameterUniform(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                        const void* uniformData, U32 size)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::UNIFORM_BUFFER,
                       "Parameter %s is not a uniform!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    UniformParameterBinding& param =group->GetParameter(ind).UniformValue;
    param.Data = uniformData;
    param.Size = size;
}

void RenderContext::SetParameterGenericBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                              Ptr<RenderBuffer> buffer, U32 bufferOffset)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::GENERIC_BUFFER,
                       "Parameter %s is not a generic buffer!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = bufferOffset;
}

void RenderContext::SetParameterVertexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                             Ptr<RenderBuffer> buffer, U32 off)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::VERTEX_BUFFER,
                       "Parameter %s is not a vertex buffer!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = off;
}

// Set an index buffer parameter input.
void RenderContext::SetParameterIndexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                            Ptr<RenderBuffer> buffer, U32 startIndex)
{
    U32 ind = (U32)-1;
    for(U32 i = 0; i < group->NumParameters; i++)
    {
        if(group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::INDEX_BUFFER,
                       "Parameter %s is not an index buffer!", ShaderParametersInfo[type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = (U32)startIndex;
}

// ================================== SCENE MANAGEMENT & RENDERING BULK

void* RenderContext::AllocateMessageArguments(U32 sz)
{
    return TTE_ALLOC(sz, MEMORY_TAG_TEMPORARY);
}

void RenderContext::SendMessage(SceneMessage message)
{
    if(message.Type == SceneMessageType::START_INTERNAL)
        TTE_ASSERT(message.Priority == INTERNAL_START_PRIORITY, "Cannot post a start scene message"); // priority set internally
    std::lock_guard<std::mutex> _L{_MessagesLock};
    _AsyncMessages.push(std::move(message));
}

void RenderContext::PushScene(Scene&& scene)
{
    Scene* pTemporary = TTE_NEW(Scene, MEMORY_TAG_TEMPORARY, std::move(scene)); // should be TTE_ALLOC, but handled separately.
    SceneMessage msg{};
    msg.Arguments = pTemporary;
    msg.Priority = INTERNAL_START_PRIORITY;
    msg.Type = SceneMessageType::START_INTERNAL;
    SendMessage(msg);
}

void RenderContext::PurgeResources()
{
    if(IsCallingFromMain())
    {
        std::lock_guard<std::mutex> G{_Lock};
        for(auto& b: _AvailTransferBuffers)
            SDL_ReleaseGPUTransferBuffer(_Device, b.Handle);
        _AvailTransferBuffers.clear();
        for(auto& sampler: _Samplers)
        {
            Ptr<RenderSampler> s = std::move(sampler);
            _PendingSDLResourceDeletions.push_back(PendingDeletion{std::move(s), GetFrame(false).FrameNumber});
        }
        _Samplers.clear();
    }
    else
        _Flags |= RENDER_CONTEXT_NEEDS_PURGE;
}

ShaderParametersStack* RenderContext::AllocateParametersStack(RenderFrame& frame)
{
    return frame.Heap.NewNoDestruct<ShaderParametersStack>(); // on heap no destruct needed its very lightweight
}

// NON-MAIN THREAD CALL: perform all higher level render calls and do any skinning / animation updates, etc.
Bool RenderContext::_Populate(const JobThread& jobThread, void* pRenderCtx, void* pDT)
{
    
    // LOCALS
    
    Float dt = *((Float*)&pDT);
    RenderContext* pContext = static_cast<RenderContext*>(pRenderCtx);
    RenderFrame& frame = pContext->GetFrame(true);
    
    // PERFORM SCENE MESSAGES
    
    pContext->_MessagesLock.lock();
    std::priority_queue<SceneMessage> messages = std::move(pContext->_AsyncMessages);
    pContext->_MessagesLock.unlock();
    
    while(!messages.empty())
    {
        SceneMessage msg = messages.top(); messages.pop();
        if(msg.Type == SceneMessageType::START_INTERNAL)
        {
            // Start a scene
            pContext->_AsyncScenes.emplace_back(std::move(*((Scene*)msg.Arguments)));
            pContext->_AsyncScenes.back().OnAsyncRenderAttach(*pContext); // on attach. ready to prepare this frame.
            TTE_DEL((Scene*)msg.Arguments); // delete the temp alloc
            continue;
        }
        for(auto sceneit = pContext->_AsyncScenes.begin(); sceneit != pContext->_AsyncScenes.end(); sceneit++)
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
                        scene.OnAsyncRenderDetach(*pContext);
                        pContext->_AsyncScenes.erase(sceneit); // remove the scene. done.
                    }
                    else
                    {
                        scene.AsyncProcessRenderMessage(*pContext, msg, pAgent);
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
    for(auto& scene : pContext->_AsyncScenes)
        scene.PerformAsyncRender(*pContext, frame, dt);
    
    return true;
}

void RenderContext::PushRenderInst(RenderFrame& frame, ShaderParametersStack* params, RenderInst &&inst)
{
    if(params != nullptr)
        TTE_ASSERT(frame.Heap.Contains(params), "Shader parameter stack must be allocated from render context!");
    if(inst._DrawDefault == DefaultRenderMeshType::NONE && (inst._VertexStateInfo.NumVertexBuffers == 0 || inst._VertexStateInfo.NumVertexAttribs == 0))
        TTE_ASSERT(false, "Cannot push a render instance draw if the vertex state has not been specified");
    
    RenderInst* instance = frame.Heap.New<RenderInst>();
    *instance = std::move(inst);
    instance->_Parameters = params;
    
    frame.NumDrawCalls++;
    instance->_Next = frame.DrawCalls;
    frame.DrawCalls = instance;
}


void RenderContext::_Render(Float dt, RenderCommandBuffer* pMainCommandBuffer)
{
    RenderFrame& frame = GetFrame(false);
    
    // LOW LEVEL RENDER (MAIN THREAD) THE PREVIOUS FRAME.
    
    // 1. temp alloc and resort draw calls to sorted order.
    
    RenderInst** sortedRenderInsts = (RenderInst**)frame.Heap.Alloc(sizeof(RenderInst*) * frame.NumDrawCalls);
    RenderInst* inst = frame.DrawCalls;
    for(U32 i = 0; i < frame.NumDrawCalls; i++)
    {
        sortedRenderInsts[i] = inst;
        inst = inst->_Next;
    }
    std::sort(sortedRenderInsts, sortedRenderInsts + frame.NumDrawCalls, RenderInstSorter{}); // sort by sort key
    
    // 2. perform any uploads. Right now execute before any draws. In future we could optimise and use fences interleaved. Dont think even telltale do that.
    //RenderCommandBuffer* pCopyCommands = _NewCommandBuffer();
    frame.UpdateList->BeginRenderFrame(pMainCommandBuffer);
    
    // 3. execute draws
    
    // TEMP PASS. NEED MORE COMPLEX PASS DESCRIPTORS IN FUTURE.
    RenderPass passdesc{};
    passdesc.ClearCol = Colour::Black;  // obviously if we want more complex passes we have to code them specifically higher level
    
    pMainCommandBuffer->StartPass(std::move(passdesc));
    
    for(U32 i = 0; i < frame.NumDrawCalls; i++)
        _Draw(frame, *sortedRenderInsts[i], *pMainCommandBuffer);
    
    pMainCommandBuffer->EndPass();
    
    frame.UpdateList->EndRenderFrame();
    
}

// renderer: perform draw.
void RenderContext::_Draw(RenderFrame& frame, RenderInst inst, RenderCommandBuffer& cmds)
{
    
    DefaultRenderMesh* pDefaultMesh = nullptr;
    
    Ptr<RenderPipelineState> state{};
    if(inst._DrawDefault == DefaultRenderMeshType::NONE)
    {
        TTE_ASSERT(inst.Program.c_str(), "Render instance shader program not set");
        
        // Find pipeline state for draw
        RenderPipelineState pipelineDesc{};
        pipelineDesc.PrimitiveType = inst._PrimitiveType;
        pipelineDesc.VertexState = inst._VertexStateInfo;
        pipelineDesc.ShaderProgram = std::move(inst.Program);
        // more stuff in the future.
        
        state = _FindPipelineState(std::move(pipelineDesc));
    }
    else
    {
        for(auto& def: _DefaultMeshes)
        {
            if(def.Type == inst._DrawDefault)
            {
                state = def.PipelineState; // use default
                pDefaultMesh = &def;
                break;
            }
        }
        TTE_ASSERT(state.get() && pDefaultMesh, "Default mesh not found or not initialised yet");
        // set num indices
        inst._IndexCount = pDefaultMesh->NumIndices;
    }
    
    // bind pipeline
    cmds.BindPipeline(state);
    
    // if no parameters allocate now
    if(!inst._Parameters)
        inst._Parameters = AllocateParametersStack(frame);
    
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
        
        PushParameterGroup(frame, inst._Parameters, inputGroup);
    }
    
    // bind shader inputs
    cmds.BindParameters(frame, inst._Parameters);
    
    // draw!
    cmds.DrawIndexed(inst._IndexCount, inst._InstanceCount, inst._StartIndex, inst._BaseIndex, 0);
}

// ===================================== RENDER FRAME UPDATE LIST =====================================

void RenderFrameUpdateList::BeginRenderFrame(RenderCommandBuffer *pCopyCommands)
{
    pCopyCommands->StartCopyPass();
    
    // UPLOADS. Strategy: if we need to upload a large number of bytes OR we have lots of little ones, use one big transfer buffer, else acquire.
    {
        _RenderTransferBuffer groupStaging{};
        U64 groupStagingOffset = 0;
        U8* stagingGroupBuffer = nullptr;
        
        struct GroupTransferBuffer
        {
            GroupTransferBuffer* Next = nullptr;
            SDL_GPUBufferRegion Dst;
            SDL_GPUTransferBufferLocation Src;
        };
        
        if(_TotalNumUploads > 32)
        {
            U64 average = _TotalBufferUploadSize / _TotalNumUploads;
            U64 stagingSize = _TotalBufferUploadSize > (1024 * 1024 * 256) ? 1024 * 1024 * 256 : (MAX(average, 1024 * 1024 * 32));
            groupStaging = _Context._AcquireTransferBuffer((U32)stagingSize, *pCopyCommands);
            stagingGroupBuffer = (U8*)SDL_MapGPUTransferBuffer(_Context._Device, groupStaging.Handle, false);
        }
        
        GroupTransferBuffer* deferred = nullptr; // these are done after as we cant map memory and push copy commands at the same time
        
        // META BUFFERS
        for(MetaBufferUpload* upload = _MetaUploads; upload; upload = upload->Next)
        {
            _RenderTransferBuffer local{};
            _RenderTransferBuffer* myBuffer = nullptr;
            U8* staging = stagingGroupBuffer;
            U64 offset = 0;
            
            if(groupStagingOffset + upload->Data.BufferSize > groupStaging.Size || upload->Data.BufferSize > 1024 * 1024 * 64)
            {
                local = _Context._AcquireTransferBuffer(upload->Data.BufferSize, *pCopyCommands);
                myBuffer = &local;
                staging = (U8*)SDL_MapGPUTransferBuffer(_Context._Device, local.Handle, false);
            }
            else
            {
                myBuffer = &groupStaging;
                offset = groupStagingOffset;
                groupStagingOffset += upload->Data.BufferSize;
            }
            
            memcpy(staging + offset, upload->Data.BufferData.get(), upload->Data.BufferSize);
            
            if(local.Handle)
                SDL_UnmapGPUTransferBuffer(_Context._Device, local.Handle);
            
            SDL_GPUTransferBufferLocation src{};
            src.offset = (U32)offset;
            src.transfer_buffer = local.Handle;
            SDL_GPUBufferRegion dst{};
            dst.size = upload->Data.BufferSize;
            dst.offset = (U32)upload->DestPosition;
            dst.buffer = upload->Buffer.get()->_Handle;
            
            if(local.Handle)
            {
                SDL_UploadToGPUBuffer(pCopyCommands->_CurrentPass->_CopyHandle, &src, &dst, false);
            }
            else
            {
                GroupTransferBuffer* tr = _Frame.Heap.NewNoDestruct<GroupTransferBuffer>();
                tr->Dst = dst;
                tr->Src = src;
                tr->Next = deferred;
                deferred = tr;
            }
        }
        
        // DATA STREAM BUFFERS
        for(DataStreamBufferUpload* upload = _StreamUploads; upload; upload = upload->Next)
        {
            _RenderTransferBuffer local{};
            _RenderTransferBuffer* myBuffer = nullptr;
            U8* staging = stagingGroupBuffer;
            U64 offset = 0;
            
            if(groupStagingOffset + upload->UploadSize > groupStaging.Size || upload->UploadSize > 1024 * 1024 * 64)
            {
                local = _Context._AcquireTransferBuffer((U32)upload->UploadSize, *pCopyCommands);
                myBuffer = &local;
                staging = (U8*)SDL_MapGPUTransferBuffer(_Context._Device, local.Handle, false);
            }
            else
            {
                myBuffer = &groupStaging;
                offset = groupStagingOffset;
                groupStagingOffset += upload->UploadSize;
            }
            
            upload->Src->SetPosition(upload->Position);
            upload->Src->Read(staging + offset, upload->UploadSize);
            
            if(local.Handle)
                SDL_UnmapGPUTransferBuffer(_Context._Device, local.Handle);
            
            SDL_GPUTransferBufferLocation src{};
            src.offset = (U32)offset;
            src.transfer_buffer = local.Handle;
            SDL_GPUBufferRegion dst{};
            dst.size = (U32)upload->UploadSize;
            dst.offset = (U32)upload->DestPosition;
            dst.buffer = upload->Buffer.get()->_Handle;
            
            if(local.Handle)
            {
                SDL_UploadToGPUBuffer(pCopyCommands->_CurrentPass->_CopyHandle, &src, &dst, false);
            }
            else
            {
                GroupTransferBuffer* tr = _Frame.Heap.NewNoDestruct<GroupTransferBuffer>();
                tr->Dst = dst;
                tr->Src = src;
                tr->Next = deferred;
                deferred = tr;
            }
        }
        
        if(groupStaging.Handle)
        {
            SDL_UnmapGPUTransferBuffer(_Context._Device, groupStaging.Handle);
            
            for(GroupTransferBuffer* tr = deferred; tr; tr = tr->Next)
            {
                SDL_UploadToGPUBuffer(pCopyCommands->_CurrentPass->_CopyHandle, &tr->Src, &tr->Dst, false);
            }
        }
    }
    
    pCopyCommands->EndPass();
}

void RenderFrameUpdateList::EndRenderFrame()
{
    
}

void RenderFrameUpdateList::_DismissBuffer(const Ptr<RenderBuffer> &buf)
{
    {
        MetaBufferUpload* uploadPrev = nullptr;
        for(MetaBufferUpload* upload = _MetaUploads; upload; upload = upload->Next)
        {
            if(upload->Buffer == buf)
            {
                _TotalBufferUploadSize -= upload->Data.BufferSize;
                _TotalNumUploads--;
                if(uploadPrev)
                    uploadPrev->Next = upload->Next;
                else
                    _MetaUploads = upload->Next;
                *upload = MetaBufferUpload{}; // reset it (hopefully delete pointers if needed)
                break;
            }
            uploadPrev = upload;
        }
    }
    {
        DataStreamBufferUpload* uploadPrev = nullptr;
        for(DataStreamBufferUpload* upload = _StreamUploads; upload; upload = upload->Next)
        {
            if(upload->Buffer == buf)
            {
                _TotalBufferUploadSize -= upload->UploadSize;
                _TotalNumUploads--;
                if(uploadPrev)
                    uploadPrev->Next = upload->Next;
                else
                    _StreamUploads = upload->Next;
                *upload = DataStreamBufferUpload{}; // reset it (hopefully delete pointers if needed)
                break;
            }
            uploadPrev = upload;
        }
    }
}

void RenderFrameUpdateList::UpdateBufferMeta(const Meta::BinaryBuffer &metaBuffer, const Ptr<RenderBuffer> &destBuffer, U64 dst)
{
    if(destBuffer->LastUpdatedFrame == _Frame.FrameNumber)
    {
        _DismissBuffer(destBuffer);
    }
    else
        destBuffer->LastUpdatedFrame = _Frame.FrameNumber;
    MetaBufferUpload* upload = _Frame.Heap.New<MetaBufferUpload>();
    upload->Data = metaBuffer;
    upload->Buffer = destBuffer;
    upload->DestPosition = dst;
    upload->Next = _MetaUploads;
    _MetaUploads = upload;
    _TotalBufferUploadSize += metaBuffer.BufferSize;
    _TotalNumUploads++;
}

void RenderFrameUpdateList::UpdateBufferDataStream(const DataStreamRef &srcStream, const Ptr<RenderBuffer> &destBuffer, U64 srcPos, U64 nBytes, U64 dst)
{
    if(destBuffer->LastUpdatedFrame == _Frame.FrameNumber)
    {
        _DismissBuffer(destBuffer);
    }
    else
        destBuffer->LastUpdatedFrame = _Frame.FrameNumber;
    DataStreamBufferUpload* upload = _Frame.Heap.New<DataStreamBufferUpload>();
    upload->Buffer = destBuffer;
    upload->DestPosition = dst;
    upload->Src = srcStream;
    upload->Position = srcPos;
    upload->UploadSize = nBytes;
    upload->Next = _StreamUploads;
    _StreamUploads = upload;
    _TotalBufferUploadSize += nBytes;
    _TotalNumUploads++;
}
