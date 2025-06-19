#include <Renderer/RenderContext.hpp>
#include <Core/Context.hpp>
#include <Common/InputMapper.hpp>

#include <chrono>
#include <cfloat>
#include <sstream>
#include <iostream>
#include <algorithm>

#if defined(PLATFORM_WINDOWS) && defined(DEBUG)
#define USE_PIX
#include <Windows.h>
#include <d3d12.h>
#include <WinPixEventRuntime/pix3.h>
#include <ShlObj.h>
#include <strsafe.h>
#undef USE_PIX
#endif

// ============================ ENUM MAPPINGS

const TextureFormatInfo& GetSDLFormatInfo(RenderSurfaceFormat format)
{
    U32 i = 0;
    while (SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        if (SDL_FormatMappings[i].Format == format)
            return SDL_FormatMappings[i];
        i++;
    }
    return SDL_FormatMappings[i];
}

RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format)
{
    U32 i = 0;
    while (SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        if (SDL_FormatMappings[i].SDLFormat == format)
            return SDL_FormatMappings[i].Format;
        i++;
    }
    return RenderSurfaceFormat::UNKNOWN;
}

// ============================ BUFFER ATTRIBUTE FORMAT ENUM MAPPINGS

inline const AttributeFormatInfo& GetSDLAttribFormatInfo(RenderBufferAttributeFormat format)
{
    U32 i = 0;
    while (SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
    {
        if (SDL_VertexAttributeMappings[i].Format == format)
            return SDL_VertexAttributeMappings[i];
        i++;
    }
    return SDL_VertexAttributeMappings[i];
}

inline RenderBufferAttributeFormat FromSDLAttribFormat(SDL_GPUVertexElementFormat format)
{
    U32 i = 0;
    while (SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
    {
        if (SDL_VertexAttributeMappings[i].SDLFormat == format)
            return SDL_VertexAttributeMappings[i].Format;
        i++;
    }
    return RenderBufferAttributeFormat::UNKNOWN;
}

// ================================== PRIMITIVE TYPE MAPPINGS

void RegisterRenderConstants(LuaFunctionCollection& Col)
{
    U32 i = 0;
    Bool bEnd = false;
    while (!bEnd)
    {
        PUSH_GLOBAL_I(Col, SDL_VertexAttributeMappings[i].ConstantName, (U32)SDL_VertexAttributeMappings[i].Format, "Vertex attribute formats");
        bEnd = SDL_VertexAttributeMappings[i].Format == RenderBufferAttributeFormat::UNKNOWN;
        i++;
    }
    i = 0;
    while (SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, SDL_PrimitiveMappings[i].ConstantName, (U32)SDL_PrimitiveMappings[i].Type, "Primitive types");
        i++;
    }
    i = 0;
    while (SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
    {
        PUSH_GLOBAL_I(Col, SDL_FormatMappings[i].ConstantName, (U32)SDL_FormatMappings[i].Format, "Surface formats");
        i++;
    }
    i = 0;
    bEnd = false;
    while (!bEnd)
    {
        PUSH_GLOBAL_I(Col, AttribInfoMap[i].ConstantName, (U32)AttribInfoMap[i].Type, "Vertex attributes");
        bEnd = AttribInfoMap[i].Type == RenderAttributeType::UNKNOWN;
        i++;
    }
    bEnd = false;
}

inline SDL_GPUPrimitiveType ToSDLPrimitiveType(RenderPrimitiveType format)
{
    U32 i = 0;
    while (SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        if (SDL_PrimitiveMappings[i].Type == format)
            return SDL_PrimitiveMappings[i].SDLType;
        i++;
    }
    return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
}

inline RenderPrimitiveType FromSDLPrimitiveType(SDL_GPUPrimitiveType format)
{
    U32 i = 0;
    while (SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        if (SDL_PrimitiveMappings[i].SDLType == format)
            return SDL_PrimitiveMappings[i].Type;
        i++;
    }
    return RenderPrimitiveType::UNKNOWN;
}

// ============================ DESCRIPTOR GETTERS

template<typename _E, typename _D>
const _D& _AbstractGetDescriptor(_E en, const _D* Descriptors)
{
    if ((U32)en >= (U32)_E::COUNT)
        return Descriptors[(U32)_E::COUNT];
    return Descriptors[(U32)en];
}

const AttribInfo& RenderContext::GetVertexAttributeDesc(RenderAttributeType attrib)
{
    return _AbstractGetDescriptor(attrib, AttribInfoMap);
}

const ShaderParameterTypeInfo& RenderContext::GetParameterDesc(ShaderParameterType param)
{
    return _AbstractGetDescriptor(param, ShaderParametersInfo);
}

const RenderEffectDesc& RenderContext::GetRenderEffectDesc(RenderEffect effect)
{
    return _AbstractGetDescriptor(effect, RenderEffectDescriptors);
}

const RenderEffectFeatureDesc& RenderContext::GetRenderEffectFeatureDesc(RenderEffectFeature feature)
{
    return _AbstractGetDescriptor(feature, RenderEffectFeatureDescriptors);
}

const RenderTargetDesc& RenderContext::GetRenderTargetDesc(RenderTargetConstantID target)
{
    return _AbstractGetDescriptor(target, TargetDescs);
}

const TextureFormatInfo& RenderContext::GetTextureFormatDesc(RenderSurfaceFormat surfaceFormat)
{
    return GetSDLFormatInfo(surfaceFormat);
}

const AttributeFormatInfo& RenderContext::GetVertexAttributeFormatDesc(RenderBufferAttributeFormat format)
{
    return GetSDLAttribFormatInfo(format);
}

const PrimitiveTypeInfo& RenderContext::GetPrimitiveTypeInfoDesc(RenderPrimitiveType prim)
{
    U32 i = 0;
    while (SDL_PrimitiveMappings[i].Type != RenderPrimitiveType::UNKNOWN)
    {
        if (SDL_PrimitiveMappings[i].Type == prim)
            return SDL_PrimitiveMappings[i];
        i++;
    }
    return SDL_PrimitiveMappings[i];
}

// ============================ LIFETIME RENDER MANAGEMENT

static Bool SDL3_Initialised = false;

#if defined(PLATFORM_WINDOWS) && defined(DEBUG)

static Bool _Win32PIXModuleLoaded = false;
static HMODULE _Win32PIXModule = {};

static std::wstring Win32_GetCapturePath() // https://devblogs.microsoft.com/pix/taking-a-capture/
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::wstring pixSearchPath = programFilesPath + std::wstring(L"\\Microsoft PIX\\*");

    WIN32_FIND_DATAW findData;
    bool foundPixInstallation = false;
    wchar_t newestVersionFound[MAX_PATH];

    HANDLE hFind = FindFirstFileW(pixSearchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                 (findData.cFileName[0] != '.'))
            {
                if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                {
                    foundPixInstallation = true;
                    StringCchCopyW(newestVersionFound, _countof(newestVersionFound), findData.cFileName);
                }
            }
        }
        while (FindNextFileW(hFind, &findData) != 0);
    }

    FindClose(hFind);

    if (!foundPixInstallation)
    {
        TTE_LOG("WARNING: Could not locate Microsoft PIX installation: D3D12 debugging won't be enabled. Please download it to enable Telltale Editor GPU captures.");
        return std::wstring();
    }

    wchar_t output[MAX_PATH];
    StringCchCopyW(output, pixSearchPath.length(), pixSearchPath.data());
    StringCchCatW(output, MAX_PATH, &newestVersionFound[0]);
    StringCchCatW(output, MAX_PATH, L"\\WinPixGpuCapturer.dll");

    return std::wstring(&output[0]);
}
#endif

Bool RenderContext::Initialise()
{
    if (!SDL3_Initialised)
    {
        SDL3_Initialised = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
#if defined(PLATFORM_WINDOWS) && defined(DEBUG)
        if (GetModuleHandleW(L"WinPixGpuCapturer.dll") == 0)
        {
            std::wstring pth = Win32_GetCapturePath();
            if(pth.length())
            {
                _Win32PIXModule = LoadLibraryW(pth.c_str());
                _Win32PIXModuleLoaded = true;
            }
        }
#endif
    }
    return SDL3_Initialised;
}

void RenderContext::Shutdown()
{
    if (SDL3_Initialised)
    {
        SDL_Quit();
        SDL3_Initialised = false;
#if defined(PLATFORM_WINDOWS) && defined(DEBUG)
        if(_Win32PIXModuleLoaded)
        {
            _Win32PIXModuleLoaded = false;
            FreeLibrary(_Win32PIXModule);
            _Win32PIXModule = {};
        }
#endif
    }
}

Bool RenderContext::IsLeftHanded()
{
    CString device = SDL_GetGPUDeviceDriver(_Device);
    if (CompareCaseInsensitive(device, "metal"))
    {
        return true; // LEFT
    }
    else if (CompareCaseInsensitive(device, "vulkan"))
    {
        return false; // RIGHT
    }
    else if (CompareCaseInsensitive(device, "direct3d12"))
    {
        return true; // LEFT
    }
    else
    {
        TTE_ASSERT(false, "Unknown graphics device driver: %s", device);
        return false;
    }
}

RenderEffectRef RenderContext::GetEffectRef(RenderEffect effect, RenderEffectFeaturesBitSet variantFeatures)
{
    return _FXCache.GetEffectRef(effect, variantFeatures);
}

RenderDeviceType RenderContext::GetDeviceType()
{
    CString device = SDL_GetGPUDeviceDriver(_Device);
    if (CompareCaseInsensitive(device, "metal"))
    {
        return RenderDeviceType::METAL;
    }
    else if (CompareCaseInsensitive(device, "vulkan"))
    {
        return RenderDeviceType::VULKAN;
    }
    else if (CompareCaseInsensitive(device, "direct3d12"))
    {
        return RenderDeviceType::D3D12;
    }
    else
    {
        TTE_ASSERT(false, "Unknown graphics device driver: %s", device);
        return RenderDeviceType::UNKNOWN;
    }
}

Bool RenderContext::IsRowMajor()
{
    CString device = SDL_GetGPUDeviceDriver(_Device);
    if (CompareCaseInsensitive(device, "metal"))
    {
        return false; // COLUMN MAJOR
    }
    else if (CompareCaseInsensitive(device, "vulkan"))
    {
        return false; // COLUMN MAJOR
    }
    else if (CompareCaseInsensitive(device, "direct3d12"))
    {
        return false; // COLUMN MAJOR
    }
    else
    {
        TTE_ASSERT(false, "Unknown graphics device driver: %s", device);
        return false;
    }
}

RenderContext::RenderContext(SDL_GPUDevice* pDevice, SDL_Window* pWindow, Ptr<ResourceRegistry> pEditorResourceSystem) : _FXCache(pEditorResourceSystem, this)
{
    TTE_ASSERT(pDevice && pWindow, "Invalid arguments");

    _HotResourceThresh = DEFAULT_HOT_RESOURCE_THRESHOLD;
    _HotLockThresh = DEFAULT_LOCKED_HANDLE_THRESHOLD;

    _MainFrameIndex = 0;
    _PopulateJob = JobHandle();
    _Frame[0].Reset(*this, 1);
    _Frame[1].Reset(*this, 2);

    _Window = pWindow;
    _Device = pDevice;

    _Flags |= RENDER_CONTEXT_AGGREGATED;

}

RenderContext::RenderContext(String wName, Ptr<ResourceRegistry> pEditorResourceSystem, U32 cap) : _FXCache(pEditorResourceSystem, this)
{
    TTE_ASSERT(JobScheduler::Instance, "Job scheduler has not been initialised. Ensure a ToolContext exists.");
    TTE_ASSERT(SDL3_Initialised, "SDL3 has not been initialised, or failed to.");

    CapFrameRate(cap);
    _HotResourceThresh = DEFAULT_HOT_RESOURCE_THRESHOLD;
    _HotLockThresh = DEFAULT_LOCKED_HANDLE_THRESHOLD;

    _MainFrameIndex = 0;
    _PopulateJob = JobHandle();
    _Frame[0].Reset(*this, 1); // start on frame 1, not 0, 0 was imaginary
    _Frame[1].Reset(*this, 2);

    // Create window and renderer
   // _Window = SDL_CreateWindow(wName.c_str(), 780, 326, 0); (boneville size)
    _Window = SDL_CreateWindow(wName.c_str(), 1920 >> 1, 1080 >> 1, 0);
    Bool bDebug = false;
#ifdef DEBUG
    bDebug = true;
#endif
    _Device = SDL_CreateGPUDevice(RENDER_CONTEXT_SHADER_FORMAT, bDebug, nullptr);
    SDL_ClaimWindowForGPUDevice(_Device, _Window);
    _BackBuffer = SDL_GetWindowSurface(_Window);

    _FXCache.Initialise();
}

RenderLayer::RenderLayer(String name, RenderContext& context) : _Context(context), _Name(std::move(name))
{
#ifdef DEBUG
    TTE_ASSERT(dynamic_cast<HandleLockOwner*>(this) == nullptr, "Render layers cannot derive from handle lock owners. The owner must be the render context.");
#endif
}

void RenderLayer::GetWindowSize(U32& width, U32& height)
{
    SDL_GetWindowSize(_Context._Window, (int*)&width, (int*)&height);
}

RenderContext::~RenderContext()
{

    // END JOBS & END SCENES

    if (_PopulateJob)
    {
        // wait for populate job. ignore what it said
        JobScheduler::Instance->Wait(_PopulateJob);
        _PopulateJob.Reset();
    }

    _LayerStack.clear(); // clear all layer operations as they are tied with resources
    _DeltaLayers.clear();

    for (auto& locked : _LockedResources)
        locked->Unlock(*this);

    for(auto& owned: _OwnedResources)
    {
        if(!owned.expired())
            owned.lock()->Release();
    }
    _OwnedResources.clear();

    // CLEANUP

    for (U32 i = 0; i < (U32)RenderTargetConstantID::COUNT; i++)
        _ConstantTargets[i].reset();

    for (auto& transfer : _AvailTransferBuffers)
        SDL_ReleaseGPUTransferBuffer(_Device, transfer.Handle);

    _LockedResources.clear();
    _DefaultMeshes.clear();
    _DefaultTextures.clear();
    _Samplers.clear();
    _Pipelines.clear();
    _AvailTransferBuffers.clear();
    _FXCache.Release();

    _Frame[0].Reset(*this, UINT64_MAX);
    _Frame[1].Reset(*this, UINT64_MAX);
    _Frame[0].Heap.ReleaseAll();
    _Frame[1].Heap.ReleaseAll();

    if ((_Flags & RENDER_CONTEXT_AGGREGATED) == 0)
    {
        SDL_ReleaseWindowFromGPUDevice(_Device, _Window);
        SDL_DestroyWindow(_Window);
        SDL_DestroyGPUDevice(_Device);
    }

}

// ============================ HIGH LEVEL RENDER FUNCTIONS

void RenderContext::PopLayer()
{
    std::lock_guard<std::mutex> G{ _Lock };
    _DeltaLayers.push_back(Ptr<RenderLayer>(nullptr));
}

void RenderContext::_PushLayer(Ptr<RenderLayer> pLayer)
{
    std::lock_guard<std::mutex> G{ _Lock };
    _DeltaLayers.push_back(std::move(pLayer));
}

// USER CALL: called every frame by user to render the previous frame
Bool RenderContext::FrameUpdate(Bool isLastFrame)
{

    // 1. locals
    Bool bUserWantsQuit = false;

    // 2. poll SDL events
    SDL_Event e{ 0 };
    std::vector<RuntimeInputEvent> events{};
    I32 w, h{};
    SDL_GetWindowSize(_Window, &w, &h);
    Vector2 windowSize((Float)w, (Float)h);
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            bUserWantsQuit = true;
        }
        else
        {
            InputMapper::ConvertRuntimeEvents(e, events, (SDL_GetWindowFlags(_Window) & SDL_WINDOW_INPUT_FOCUS) != 0, windowSize);
        }
    }

    // 3. Calculate delta time
    Float dt = 0.0f;
    if (_StartTimeMicros != 0)
    {
        U64 myTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        U64 dtMicros = myTime - _StartTimeMicros;
        if (dtMicros <= 1000 * _MinFrameTimeMS)
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
            _ResolveBackBuffers(*pMainCommandBuffer);

            if (pFrame->FrameNumber == 1)
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
        for (auto& commandBuffer : pFrame->CommandBuffers)
        {
            if (!commandBuffer->_Submitted)
                commandBuffer->Submit(); // submit if needed

            if (commandBuffer->_SubmittedFence && !commandBuffer->Finished())
                awaitingFences[activeCommandBuffers++] = commandBuffer->_SubmittedFence; // add waiting fences
        }

        // WAIT ON ALL ACTIVE COMMAND BUFFERS TO FINISH GPU EXECUTION

        if (activeCommandBuffers > 0)
            SDL_WaitForGPUFences(_Device, true, awaitingFences, activeCommandBuffers);
        for (auto& commandBuffer : pFrame->CommandBuffers) // free the fences
        {
            if (commandBuffer->_SubmittedFence != nullptr)
            {
                SDL_ReleaseGPUFence(_Device, commandBuffer->_SubmittedFence);
                commandBuffer->_SubmittedFence = nullptr;
            }
        }

        // FREE ANY COLD RESOURCES PAST THE THRESHOLD
        _PurgeColdResources(pFrame);

        // RECLAIM TRANSFER BUFFERS
        for (auto& cmd : pFrame->CommandBuffers)
            _ReclaimTransferBuffers(*cmd);

        // CLEAR COMMAND BUFFERS.
        pFrame->CommandBuffers.clear();
    }

    // 5. Wait for previous populater to complete if needed
    if (_PopulateJob)
    {
        JobScheduler::Instance->Wait(_PopulateJob);
        _PopulateJob.Reset(); // reset handle to empty
    }

    // 6. NO POPULATE THREAD JOB ACTIVE, SO NO LOCKING NEEDED UNTIL SUBMIT. Give our new processed frame, and swap.

    _MainFrameIndex ^= 1; // swap
    GetFrame(true).Reset(*this, GetFrame(true).FrameNumber + 2); // increase populater frame index (+2 from swap, dont worry)
    GetFrame(true)._Events = std::move(events);

    // free resources which are not needed
    if (_Flags & RENDER_CONTEXT_NEEDS_PURGE)
    {
        _Flags &= ~RENDER_CONTEXT_NEEDS_PURGE;
        PurgeResources();
    }
    _FreePendingDeletions(GetFrame(false).FrameNumber);

    // Purge any cold locks. This will free any unused resources by the renderer. They will be destroyed after
    // the populater job is fired off, as we may have the last reference to these resources so freeing them might take a while
    std::vector<Ptr<Handleable>> freedLocks = _PurgeColdLocks();

    // 7. Kick off populater job to render the last thread data (if needed)
    if (!isLastFrame && !bUserWantsQuit)
    {
        JobDescriptor job{};
        job.AsyncFunction = &_Populate;
        job.Priority = JOB_PRIORITY_HIGHEST;
        job.UserArgA = this;
        union
        {
            void* a; Float b;
        } _cvt{};
        _cvt.b = dt;
        job.UserArgB = _cvt.a;
        _PopulateJob = JobScheduler::Instance->Post(job);
    }

    freedLocks.clear();

    return !bUserWantsQuit && !isLastFrame;
}

std::vector<Ptr<Handleable>> RenderContext::_PurgeColdLocks()
{
    TTE_ASSERT(IsCallingFromMain(), "Can only be called from main thread"); // No lock needed as called between
    U64 ts = GetRenderFrameNumber();
    std::vector<Ptr<Handleable>> toUnlock{};
    for (auto it = _LockedResources.begin(); it != _LockedResources.end();)
    {
        if (GetTimeStampDifference((*it)->GetLockedTimeStamp(*this, true), ts) >= _HotLockThresh)
        {
            (*it)->Unlock(*this);
            toUnlock.push_back(std::move(*it));
            _LockedResources.erase(it);
        }
        else ++it;
    }
    return std::move(toUnlock);
}

void RenderContext::_PurgeColdResources(RenderFrame* pFrame)
{
    for (auto it = _AvailTransferBuffers.begin(); it != _AvailTransferBuffers.end();)
    {
        if (it->LastUsedFrame + _HotResourceThresh >= pFrame->FrameNumber)
        {
            SDL_ReleaseGPUTransferBuffer(_Device, it->Handle);
            it = _AvailTransferBuffers.erase(it);
        }
        else ++it;
    }
    _PurgeDeadResources();
}

void RenderContext::_PurgeDeadResources()
{
    for(auto it = _OwnedResources.begin(); it != _OwnedResources.end();)
    {
        if(it->expired())
        {
            it->reset();
            _OwnedResources.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void RenderContext::_AttachContextDependentResource(WeakPtr<RenderResource> pResource, Bool bLock)
{
    if(bLock)
        _Lock.lock();

    for(auto& r: _OwnedResources)
    {
        if(r.lock() == pResource.lock())
        {
            if (bLock)
                _Lock.unlock();
            return;
        }
    }
    _OwnedResources.push_back(std::move(pResource));
    if (bLock)
        _Lock.unlock();
}

// ============================ FRAME MANAGEMENT

void RenderFrame::Reset(RenderContext& context, U64 newFrameNumber)
{
    CommandBuffers.clear();
    _Autorelease.clear(); // will free ptrs if needed
    _Events.clear();
    FrameNumber = newFrameNumber;
    Views = nullptr;
    Heap.Rollback(); // free all objects, but keep memory
    UpdateList = Heap.New<RenderFrameUpdateList>(context, *this);
    PreRenderCallbacks.Clear();
    PostRenderCallbacks.Clear();
}

// ============================ TEXTURES & RENDER TARGET

void RenderTexture::SetName(CString name)
{
    _Name = name;
}

void RenderTexture::EnsureMip(RenderContext* c, U32 mipIndex)
{
    _Context = c;
    if (GetHandle(_Context))
    {
        if (!_UploadedMips.Test(1u << mipIndex))
        {
            Ptr<RenderTexture> myself = std::dynamic_pointer_cast<RenderTexture>(shared_from_this());
            for (U32 slice = 0; slice < _Depth; slice++)
            {
                for (U32 face = 0; face < _ArraySize; face++)
                {
                    _Context->GetFrame(true).UpdateList->UpdateTexture(myself, mipIndex, slice, face);
                }
            }
            _UploadedMips.Add(1u << mipIndex);
        }
    }
}

SDL_GPUTexture* RenderTexture::GetHandle(RenderContext* context)
{
    _Context = context;
    if (_Handle == nullptr)
    {
        SDL_GPUTextureCreateInfo info{};
        info.width = _Width;
        info.height = _Height;
        info.format = GetSDLFormatInfo(_Format).SDLFormat;
        info.num_levels = _NumMips;
        info.layer_count_or_depth = _Depth * _ArraySize;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.type = SDL_GPU_TEXTURETYPE_2D; // TODO
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.props = SDL_CreateProperties();
        SDL_SetStringProperty(info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, _Name.length() ? _Name.c_str() : "Unnamed TTE Texture");
        _Handle = SDL_CreateGPUTexture(context->_Device, &info);
        context->_AttachContextDependentResource(std::static_pointer_cast<RenderTexture>(shared_from_this()), true);
        SDL_DestroyProperties(info.props);
    }
    return _Handle;
}

void RenderTexture::Release()
{
    if (_Handle)
    {
        if (!_TextureFlags.Test(TEXTURE_FLAG_DELEGATED))
        {
            SDL_ReleaseGPUTexture(_Context->_Device, _Handle);
        }
        _Handle = nullptr;
        _TextureFlags.Remove(TEXTURE_FLAG_DELEGATED);
    }
    _Handle = nullptr;
    _Context = nullptr;
}

void RenderTexture::CreateTarget(RenderContext& context, Flags flags, RenderSurfaceFormat format, U32 width,
                                 U32 height, U32 nMips, U32 nSlices, U32 arraySize, Bool bIsDepth, SDL_GPUTexture* handle)
{
    // CAUTION: IF Release() called, set pContext and handle again.
    Release();
    _Context = &context;
    _Width = width;
    _Height = height;
    _Format = format;
    _Depth = nSlices;
    _NumMips = nMips;
    _ArraySize = arraySize;
    _TextureFlags = flags;
    _Handle = handle;
    if (!handle)
    {
        SDL_GPUTextureCreateInfo info{};
        info.width = width;
        info.height = height;
        info.format = GetSDLFormatInfo(format).SDLFormat;
        info.num_levels = nMips;
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1; // increase eventually
        info.layer_count_or_depth = _Depth * _ArraySize;
        info.usage = bIsDepth ? SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET : SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.props = SDL_CreateProperties();
        SDL_SetStringProperty(info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, _Name.length() ? _Name.c_str() : "Unnamed TTE Target");
        _Handle = SDL_CreateGPUTexture(_Context->_Device, &info);
        context._AttachContextDependentResource(std::static_pointer_cast<RenderTexture>(shared_from_this()), true);
        SDL_DestroyProperties(info.props);
    }
}

void RenderTexture::Create2D(RenderContext* pContext, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips)
{
    // CAUTION: IF Release() called, set pContext and handle again.
    _Context = pContext;
    if (_Context && !_Handle)
    {
        _Width = width;
        _Height = height;
        _Format = format;
        _Depth = _ArraySize = 1;
        _NumMips = nMips;

        TTE_ASSERT(_NumMips == 1, "TODO");
        _Images.clear();
        Image img{};
        img.Width = width;
        img.Height = height;
        img.RowPitch = (U32)(GetSDLFormatInfo(format).BytesPerPixel * img.Width);
        img.SlicePitch = img.RowPitch * height;
        _Images.push_back(std::move(img));

        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D; // TODO 2d/3d arrays. also fix TWO above versions
        info.width = width;
        info.height = height;
        info.format = GetSDLFormatInfo(format).SDLFormat;
        info.num_levels = nMips;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.layer_count_or_depth = 1;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.props = SDL_CreateProperties();
        SDL_SetStringProperty(info.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, _Name.length() ? _Name.c_str() : "Unnamed TTE Texture");
        _Handle = SDL_CreateGPUTexture(pContext->_Device, &info);
        pContext->_AttachContextDependentResource(std::static_pointer_cast<RenderTexture>(shared_from_this()), true);
        SDL_DestroyProperties(info.props);
    }
}

Ptr<RenderSampler> RenderContext::_FindSampler(RenderSampler desc)
{
    std::lock_guard<std::mutex> L{ _Lock };
    for (auto& sampler : _Samplers)
    {
        if (desc == *sampler)
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
    info.props = SDL_CreateProperties();
    SDL_SetStringProperty(info.props, SDL_PROP_GPU_SAMPLER_CREATE_NAME_STRING, "TTE Sampler");
    desc._Handle = SDL_CreateGPUSampler(_Device, &info);
    SDL_DestroyProperties(info.props);
    desc._Context = this;
    Ptr<RenderSampler> pSampler = TTE_NEW_PTR(RenderSampler, MEMORY_TAG_RENDERER);
    *pSampler = std::move(desc);
    desc._Context = nullptr;
    desc._Handle = nullptr; // sometimes compiler doesnt do move:(
    _Samplers.push_back(pSampler);
    _AttachContextDependentResource(pSampler, false);
    return pSampler;
}

RenderTexture::~RenderTexture()
{
    Release();
}

void RenderContext::_FreePendingDeletions(U64 frame)
{
    AssertMainThread();
    std::lock_guard<std::mutex> G{ _Lock };
    for (auto it = _PendingSDLResourceDeletions.begin(); it != _PendingSDLResourceDeletions.end();)
    {
        if (frame - it->_LastUsedFrame > 1)
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

void RenderCommandBuffer::UploadTextureDataSlow(Ptr<RenderTexture>& texture,
                                                DataStreamRef src, U64 srcOffset, U32 mip, U32 slice, U32 face, U32 dataZ)
{
    if (_CurrentPass)
        TTE_ASSERT(_CurrentPass->_CopyHandle != nullptr, "A render pass cannot be active unless its a copy pass if uploading textures.");
    texture->_Context = _Context;

    texture->_LastUsedFrame = _Context->GetFrame(false).FrameNumber;

    _RenderTransferBuffer tBuffer = _Context->_AcquireTransferBuffer(dataZ, *this);

    U8* mappedMemorySeg = (U8*)SDL_MapGPUTransferBuffer(_Context->_Device, tBuffer.Handle, false);
    src->SetPosition(srcOffset);
    src->Read(mappedMemorySeg, dataZ);
    SDL_UnmapGPUTransferBuffer(_Context->_Device, tBuffer.Handle);

    Bool bStartPass = _CurrentPass == nullptr;

    if (bStartPass)
        StartCopyPass();

    U32 w, h, r, s{};
    texture->GetImageInfo(mip, slice, face, w, h, r, s);

    SDL_GPUTextureTransferInfo srcinf{};
    SDL_GPUTextureRegion dst{};
    srcinf.offset = 0;
    srcinf.rows_per_layer = h;
    srcinf.pixels_per_row = w;
    srcinf.transfer_buffer = tBuffer.Handle;
    dst.texture = texture->GetHandle(_Context);
    dst.mip_level = mip;
    dst.layer = (slice * texture->_ArraySize) + face;
    dst.x = dst.y = dst.z = 0;
    dst.w = w;
    dst.h = h;
    dst.d = 1;

    SDL_UploadToGPUTexture(_CurrentPass->_CopyHandle, &srcinf, &dst, false);

    if (bStartPass)
        EndPass();
}

// ================ PIPELINE STATES, SHADER MGR

Bool RenderContext::_ResolveEffectRef(RenderEffectRef ref, Ptr<RenderShader>& vert, Ptr<RenderShader>& frag)
{
    TTE_ASSERT(ref, "Invalid effect variant ref!");
    return _FXCache.ResolveEffectRef(ref, vert, frag);
}

/* // OLD VERSION!
Ptr<RenderShader> RenderContext::_FindShader(String name, RenderShaderType type)
{
    _Lock.lock();
    for (auto& sh : _LoadedShaders)
    {
        if (CompareCaseInsensitive(name, sh->Name))
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

    if (fmts & SDL_GPU_SHADERFORMAT_MSL) // raw metal shader
    {
        rawSh = GetToolContext()->LoadLibraryStringResource("Shader/" + name + ".msl");
        if (rawSh.length() == 0)
        {
            TTE_LOG("ERROR: %s [METAL] could not be loaded. Shader file empty or not found.", name.c_str());
            return nullptr;
        }
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = (const Uint8*)rawSh.c_str();
        info.code_size = rawSh.length();
    }
    // TODO. identify if the .GLSL or .HLSL version exists, if so then compile and save (if dev) as .dxbc/.spv
    else if (fmts & SDL_GPU_SHADERFORMAT_DXBC) // compiled hlsl
    {
        bin = true;
        info.format = SDL_GPU_SHADERFORMAT_DXBC;
        DataStreamRef stream = GetToolContext()->LoadLibraryResource("Shader/" + name + ".dxbc");
        if (!stream || stream->GetSize() == 0)
        {
            TTE_LOG("ERROR: %s [D3D] could not be loaded. Shader file empty or not found.", name.c_str());
            return nullptr;
        }
        info.code_size = stream->GetSize();
        temp = TTE_ALLOC(info.code_size, MEMORY_TAG_TEMPORARY);
        stream->Read(temp, (U64)info.code_size);
    }
    else if (fmts & SDL_GPU_SHADERFORMAT_SPIRV) // compiled glsl
    {
        bin = true;
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        DataStreamRef stream = GetToolContext()->LoadLibraryResource("Shader/" + name + ".spv");
        if (!stream || stream->GetSize() <= 8)
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
    if (bin) // if compiled read the binary version
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
        U32 binds = info.num_samplers + info.num_storage_buffers + info.num_uniform_buffers + info.num_storage_textures;
        memcpy(set.Words(), inf + 8, 4);
        const U8* map = inf + 12 + (2 * binds);
        info.code += 2 * binds + 4;
        info.code_size -= (2 * binds + 12);
        for (U32 i = 0; i < binds; i++)
        {
            TTE_ASSERT(sh->ParameterSlots[map[0]] == 0xFF, "Corrupt shader binary"); // must have no binding
            sh->ParameterSlots[map[0]] = map[1];
            ShaderParameterTypeClass cls = ShaderParametersInfo[map[0]].Class;
            if ((cls != ShaderParameterTypeClass::UNIFORM_BUFFER && cls != ShaderParameterTypeClass::GENERIC_BUFFER &&
                cls != ShaderParameterTypeClass::SAMPLER) || cls >= (ShaderParameterTypeClass)3)
            {
                TTE_ASSERT(false, "The shader parameter '%s' cannot have its binding slot specified", ShaderParametersInfo[map[0]].Name);
            }
            BindsOf[(U32)cls]++;
            map += 2;
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
                        else
                        {
                            Bool Set = false;
                            for (U32 i = 0; i < PARAMETER_COUNT; i++)
                            {
                                if (key == ShaderParametersInfo[i].Name)
                                {
                                    TTE_ASSERT(sh->ParameterSlots[i] == 0xFF, "Invalid shader binding slot: already set");
                                    sh->ParameterSlots[i] = (U8)std::stoi(valueStr);

                                    ShaderParameterTypeClass cls = ShaderParametersInfo[i].Class;
                                    if ((cls != ShaderParameterTypeClass::UNIFORM_BUFFER && cls != ShaderParameterTypeClass::GENERIC_BUFFER &&
                                        cls != ShaderParameterTypeClass::SAMPLER) || cls >= (ShaderParameterTypeClass)3)
                                    {
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
    if (type == RenderShaderType::VERTEX)
        TTE_ASSERT(set.CountBits() > 0, "No vertex attributes specified for vertex shader %s", name.c_str());

    sh->Name = name;
    info.props = SDL_CreateProperties();
    SDL_SetStringProperty(info.props, SDL_PROP_GPU_SHADER_CREATE_NAME_STRING, name.c_str());
    sh->Handle = SDL_CreateGPUShader(_Device, &info);
    SDL_DestroyProperties(info.props);
    sh->Context = this;
    sh->Attribs = set;

    if (temp)
    {
        TTE_FREE(temp);
        temp = nullptr; // free temp shader
    }

    TTE_ASSERT(sh->Handle, "Could not create shader %s", name.c_str());

    if (sh->Handle)
    {
        _Lock.lock();
        SDL_GPUShader* ret = sh->Handle;
        _LoadedShaders.push_back(sh);
        _Lock.unlock();
        return sh;
    }

    return nullptr;
} */

// ======================= PIPELINE STATES

Ptr<RenderPipelineState> RenderContext::_AllocatePipelineState()
{
    auto val = TTE_NEW_PTR(RenderPipelineState, MEMORY_TAG_RENDERER);
    val->_Internal._Context = this;

    // add to array
    {
        std::lock_guard<std::mutex> G{ _Lock };
        _Pipelines.push_back(val);
    }

    return val;
}

Ptr<RenderPipelineState> RenderContext::_FindPipelineState(RenderPipelineState desc)
{
    U64 hash = _HashPipelineState(desc);
    {
        std::lock_guard<std::mutex> G{ _Lock };
        for (auto& state : _Pipelines)
            if (state->Hash == hash)
                return state;
    }
    Ptr<RenderPipelineState> state = _AllocatePipelineState();
    desc._Internal._Context = this;
    *state = std::move(desc);
    state->Create();
    return state;
}

U64 RenderContext::_HashPipelineState(RenderPipelineState& state)
{
    U64 Hash = 0;

    Hash = state.EffectHash;
    Hash = CRC64((const U8*)&state.VertexState.NumVertexBuffers, 4, Hash);
    Hash = CRC64((const U8*)&state.PrimitiveType, 4, Hash);

    for (U32 i = 0; i < state.VertexState.NumVertexAttribs; i++)
    {
        TTE_ASSERT(state.VertexState.Attribs[i].VertexBufferIndex < state.VertexState.NumVertexBuffers, "Invalid buffer index");
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].Format, 4, state.VertexState.IsHalf ? ~Hash : Hash);
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].VertexBufferIndex, 4, Hash);
        Hash = CRC64((const U8*)&state.VertexState.Attribs[i].Attrib, 4, Hash);
    }

    return Hash;
}

RenderStateBlob::Entry RenderStateBlob::_GetEntry(RenderStateType type)
{
    TTE_ASSERT((U32)type < (U32)RenderStateType::NUM, "Invalid entry type");
    TTE_ASSERT(_Entries[(U32)type].Type == type, "Render state blob entries static array is modified and indices do not match enums");
    return _Entries[(U32)type];
}

U32 RenderStateBlob::GetValue(RenderStateType type) const
{
    const Entry& entry = _Entries[(U32)type];
    U32 value = (_Words[entry.WordIndex] >> entry.WordShift) & ((1u << entry.BitWidth) - 1);
    return value;
}

void RenderStateBlob::SetValue(RenderStateType type, U32 value)
{
    const Entry& entry = _Entries[(U32)type];
    U32 mask = (1u << entry.BitWidth) - 1;
    TTE_ASSERT((value & (mask ^ 0xFFFFFFFFu)) == 0, "When setting render state %s, the value %d is out of range", entry.Name, value);
    _Words[entry.WordIndex] &= ~(mask << entry.WordShift);
    _Words[entry.WordIndex] |= (value << entry.WordShift);
}

RenderStateBlob::RenderStateBlob()
{
    *this = Default;
}

RenderStateBlob RenderStateBlob::Default{};

void RenderStateBlob::Initialise()
{
    U32 wordBitIndex = 0;
    U32 wordIndex = 0;
    for (U32 i = 0; i < (U32)RenderStateType::NUM; i++)
    {
        if (wordBitIndex + _Entries[i].BitWidth >= 32)
        {
            _Entries[i].WordShift = 0;
            _Entries[i].WordIndex = ++wordIndex;
            wordBitIndex = _Entries[i].BitWidth;
        }
        else
        {
            _Entries[i].WordIndex = wordIndex;
            _Entries[i].WordShift = wordBitIndex;
            wordBitIndex += _Entries[i].BitWidth;
        }
    }
    for (U32 i = 0; i < (U32)RenderStateType::NUM; i++)
    {
        Default.SetValue((RenderStateType)i, _Entries[i].Default);
    }
}

static SDL_GPUBlendFactor GetColourBlendAlphaEquiv(SDL_GPUBlendFactor in)
{
    switch (in)
    {
    case SDL_GPU_BLENDFACTOR_ZERO:            return SDL_GPU_BLENDFACTOR_ZERO;
    case SDL_GPU_BLENDFACTOR_ONE:             return SDL_GPU_BLENDFACTOR_ONE;
    case SDL_GPU_BLENDFACTOR_SRC_COLOR:       return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case SDL_GPU_BLENDFACTOR_DST_COLOR:       return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case SDL_GPU_BLENDFACTOR_SRC_ALPHA:       return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case SDL_GPU_BLENDFACTOR_DST_ALPHA:       return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case SDL_GPU_BLENDFACTOR_CONSTANT_COLOR:  return SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
    case SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
    case SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    default: return in;
    }
}


void RenderPipelineState::Create()
{
    if (_Internal._Handle == nullptr)
    {
        _Internal._Context->AssertMainThread();
        TTE_ASSERT(VertexState.NumVertexBuffers < 32 && VertexState.NumVertexAttribs < 32, "Vertex state has too many attributes / buffers");
        
        String effectName = _Internal._Context->_FXCache.LocateEffectName(RenderEffectRef{ EffectHash });

        // calculate hash
        Hash = _Internal._Context->_HashPipelineState(*this);

        SDL_GPUGraphicsPipelineCreateInfo info{};

        // RENDER TARGETS
        info.target_info.num_color_targets = NumColourTargets;
        TTE_ASSERT(NumColourTargets <= 8, "Too many colour targets");
        SDL_GPUColorTargetDescription sc[8]{};
        for (U32 i = 0; i < NumColourTargets; i++)
        {
            sc[i].format = GetSDLFormatInfo(TargetFormat[i]).SDLFormat;
        }
        info.target_info.color_target_descriptions = sc;

        // DEPTH TARGET

        if (DepthFormat != RenderSurfaceFormat::UNKNOWN)
        {
            info.target_info.has_depth_stencil_target = true;
            info.target_info.depth_stencil_format = GetSDLFormatInfo(DepthFormat).SDLFormat;
        }

        // RASTERISER STATE
        info.rasterizer_state.cull_mode = (SDL_GPUCullMode)RState.GetValue(RenderStateType::CULL_MODE);
        info.rasterizer_state.fill_mode = (SDL_GPUFillMode)RState.GetValue(RenderStateType::FILL_MODE);
        info.rasterizer_state.front_face = (SDL_GPUFrontFace)RState.GetValue(RenderStateType::VERTEX_WINDING);
        info.rasterizer_state.depth_bias_constant_factor = RState.GetValue(RenderStateType::Z_OFFSET) == 1 ? 16.0f : 0.0f;
        info.rasterizer_state.depth_bias_clamp = 0.0f; // no clamp
        Bool bClip = (Bool)RState.GetValue(RenderStateType::Z_CLIPPING);
        Bool bInvert = (Bool)RState.GetValue(RenderStateType::Z_INVERSE);
        Bool bBias = (Bool)RState.GetValue(RenderStateType::Z_BIAS);
        if (bBias)
            info.rasterizer_state.depth_bias_slope_factor = bInvert ? -1.0f : 1.0f;
        else
            info.rasterizer_state.depth_bias_slope_factor = 0.0f;
        info.rasterizer_state.enable_depth_bias = true;
        info.rasterizer_state.enable_depth_clip = bClip;

        // DEPTH STENCIL STATE
        info.depth_stencil_state.enable_depth_test = (Bool)RState.GetValue(RenderStateType::Z_ENABLE);
        info.depth_stencil_state.enable_depth_write = (Bool)RState.GetValue(RenderStateType::Z_WRITE_ENABLE);
        info.depth_stencil_state.enable_stencil_test = (Bool)RState.GetValue(RenderStateType::STENCIL_ENABLE);
        info.depth_stencil_state.compare_mask = (U8)RState.GetValue(RenderStateType::STENCIL_READ_MASK);
        info.depth_stencil_state.write_mask = (U8)RState.GetValue(RenderStateType::STENCIL_WRITE_MASK);
        info.depth_stencil_state.front_stencil_state.compare_op =
            info.depth_stencil_state.back_stencil_state.compare_op = (SDL_GPUCompareOp)RState.GetValue(RenderStateType::STENCIL_COMPARE_FUNC);
        info.depth_stencil_state.front_stencil_state.depth_fail_op =
            info.depth_stencil_state.back_stencil_state.depth_fail_op = (SDL_GPUStencilOp)RState.GetValue(RenderStateType::STENCIL_DEPTH_FAIL_OP);
        info.depth_stencil_state.front_stencil_state.fail_op =
            info.depth_stencil_state.back_stencil_state.fail_op = (SDL_GPUStencilOp)RState.GetValue(RenderStateType::STENCIL_FAIL_OP);
        info.depth_stencil_state.front_stencil_state.pass_op =
            info.depth_stencil_state.back_stencil_state.pass_op = (SDL_GPUStencilOp)RState.GetValue(RenderStateType::STENCIL_PASS_OP);
        SDL_GPUCompareOp zcompare = (SDL_GPUCompareOp)RState.GetValue(RenderStateType::Z_COMPARE_FUNC);
        if (bInvert)
        {
            if (zcompare == SDL_GPU_COMPAREOP_LESS)
                zcompare = SDL_GPU_COMPAREOP_GREATER;
            else if (zcompare == SDL_GPU_COMPAREOP_LESS_OR_EQUAL)
                zcompare = SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
            else if (zcompare == SDL_GPU_COMPAREOP_GREATER)
                zcompare = SDL_GPU_COMPAREOP_LESS;
            else if (zcompare == SDL_GPU_COMPAREOP_GREATER_OR_EQUAL)
                zcompare = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        }
        info.depth_stencil_state.compare_op = zcompare;

        // BLEND STATE
        Bool bSeparate = (Bool)RState.GetValue(RenderStateType::SEPARATE_ALPHA_BLEND);
        sc[0].blend_state.enable_color_write_mask = true;
        sc[0].blend_state.enable_blend = (Bool)RState.GetValue(RenderStateType::BLEND_ENABLE);
        sc[0].blend_state.color_write_mask = (SDL_GPUColorComponentFlags)RState.GetValue(RenderStateType::COLOUR_WRITE_ENABLE_MASK);
        sc[0].blend_state.color_blend_op = (SDL_GPUBlendOp)RState.GetValue(RenderStateType::BLEND_OP);
        sc[0].blend_state.src_color_blendfactor = (SDL_GPUBlendFactor)RState.GetValue(RenderStateType::SOURCE_BLEND);
        sc[0].blend_state.dst_color_blendfactor = (SDL_GPUBlendFactor)RState.GetValue(RenderStateType::DEST_BLEND);
        if (bSeparate)
        {
            sc[0].blend_state.alpha_blend_op = (SDL_GPUBlendOp)RState.GetValue(RenderStateType::ALPHA_BLEND_OP);
            sc[0].blend_state.src_alpha_blendfactor = (SDL_GPUBlendFactor)RState.GetValue(RenderStateType::ALPHA_SOURCE_BLEND);
            sc[0].blend_state.dst_alpha_blendfactor = (SDL_GPUBlendFactor)RState.GetValue(RenderStateType::ALPHA_DEST_BLEND);
        }
        else
        {
            sc[0].blend_state.alpha_blend_op = (SDL_GPUBlendOp)RState.GetValue(RenderStateType::BLEND_OP);
            sc[0].blend_state.src_alpha_blendfactor = GetColourBlendAlphaEquiv((SDL_GPUBlendFactor)RState.GetValue(RenderStateType::SOURCE_BLEND));
            sc[0].blend_state.dst_alpha_blendfactor = GetColourBlendAlphaEquiv((SDL_GPUBlendFactor)RState.GetValue(RenderStateType::DEST_BLEND));
        }

        // TODO MULTISAMPLE
        info.multisample_state.enable_mask = false;

        // SHADER PROGRAM

        Ptr<RenderShader> vert, frag{};
        TTE_ASSERT(_Internal._Context->_ResolveEffectRef(RenderEffectRef{EffectHash}, vert, frag), "Could not fetch effect shader program: %s", 
                   effectName.c_str());

        info.vertex_shader = vert->Handle;
        info.fragment_shader = frag->Handle;

        info.primitive_type = ToSDLPrimitiveType(PrimitiveType);

        // VERTEX STATE

        info.vertex_input_state.num_vertex_buffers = VertexState.NumVertexBuffers;
        info.vertex_input_state.num_vertex_attributes = VertexState.NumVertexAttribs;

        SDL_GPUVertexBufferDescription desc[32]{};
        for (U32 i = 0; i < VertexState.NumVertexBuffers; i++)
        {
            desc[i].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            desc[i].instance_step_rate = 0;
            desc[i].slot = i;
            desc[i].pitch = VertexState.BufferPitches[i];
        }

        SDL_GPUVertexAttribute attrib[32]{};
        U32 off[32] = { 0 };
        Bool attribsHave[(U32)RenderAttributeType::COUNT] = { 0 };
        for (U32 i = 0; i < VertexState.NumVertexAttribs; i++)
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
        for (U32 attrib = 0; attrib < (U32)RenderAttributeType::COUNT; attrib++)
        {
            Bool bVertRequires = vert->Attribs[(RenderAttributeType)attrib];
            if (bVertRequires && !attribsHave[attrib])
                TTE_ASSERT(false, "Vertex attribute %s is required for effect variant %s, but was not supplied!",
                           AttribInfoMap[attrib].ConstantName, effectName.c_str());
            // if it does not require but we have bound it, its ok we aren't using it anyway
        }

        info.vertex_input_state.vertex_attributes = attrib;
        info.vertex_input_state.vertex_buffer_descriptions = desc;
        info.props = SDL_CreateProperties();
#ifdef DEBUG
        String dbgName = "TTE_PIPELINE " + _Internal._Context->_FXCache.LocateEffectName(RenderEffectRef{EffectHash});
        SDL_SetStringProperty(info.props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, dbgName.c_str());
#else
        SDL_SetStringProperty(info.props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, "TTE Pipeline");
#endif
        _Internal._Handle = SDL_CreateGPUGraphicsPipeline(_Internal._Context->_Device, &info);
        // dont need to attach to context, owned by it intrinsically
        SDL_DestroyProperties(info.props);
        TTE_ASSERT(_Internal._Handle != nullptr, "Could not create pipeline state: %s", SDL_GetError());

    }
}

void RenderPipelineState::Release()
{
    if (_Internal._Handle && _Internal._Context)
    {
        _Internal._Context->AssertMainThread();
        SDL_ReleaseGPUGraphicsPipeline(_Internal._Context->_Device, _Internal._Handle);
        _Internal._Handle = nullptr;
    }
    _Internal._Context = nullptr;
}

RenderPipelineState::~RenderPipelineState()
{
    Release();
}

void RenderShader::Release()
{
    if (Handle && Context)
    {
        Context->AssertMainThread();
        SDL_ReleaseGPUShader(Context->_Device, Handle);
        Handle = nullptr;
    }
    Handle = nullptr;
    Context = nullptr;
}

RenderShader::~RenderShader()
{
    Release();
}

void RenderSampler::Release()
{
    if (_Handle && _Context)
    {
        _Context->AssertMainThread();
        SDL_ReleaseGPUSampler(_Context->_Device, _Handle);
        _Handle = nullptr;
    }
    _Context = nullptr;
}

RenderSampler::~RenderSampler()
{
    Release();
}

// ===================== COMMAND BUFFERS

RenderCommandBuffer* RenderContext::_NewCommandBuffer()
{
    AssertMainThread();
    RenderFrame& frame = GetFrame(false);
    RenderCommandBuffer* pBuffer = frame.Heap.New<RenderCommandBuffer>();

    pBuffer->_Handle = SDL_AcquireGPUCommandBuffer(_Device);
    TTE_ASSERT(pBuffer->_Handle, "Could not acquire a new command buffer: %s", SDL_GetError());
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

RenderPass RenderCommandBuffer::EndPass()
{
    _Context->AssertMainThread();
    TTE_ASSERT(_CurrentPass, "No active pass");

    if (_CurrentPass->_Handle)
        SDL_EndGPURenderPass(_CurrentPass->_Handle);
    else
        SDL_EndGPUCopyPass(_CurrentPass->_CopyHandle);
    RenderPass pass = std::move(*_CurrentPass);
    _Context->_PopDebugGroup(*this);
    pass._Handle = nullptr;
    pass._CopyHandle = nullptr;
    _CurrentPass = nullptr;
    _BoundPipeline.reset();
    return pass;
}

void RenderContext::_PushDebugGroup(RenderCommandBuffer& buf, const String& name)
{
#ifdef PLATFORM_WINDOWS
#ifdef DEBUG
    // PIX API
    PIXBeginEvent(*((ID3D12GraphicsCommandList**)buf._Handle), PIX_COLOR(167, 22, 224), name.c_str());
#endif
#else
    SDL_PushGPUDebugGroup(buf._Handle, name.c_str());
#endif
}

void RenderContext::_PopDebugGroup(RenderCommandBuffer& buf)
{
#ifdef PLATFORM_WINDOWS
#ifdef DEBUG
    PIXEndEvent(*((ID3D12GraphicsCommandList**)buf._Handle));
#endif
#else
    SDL_PopGPUDebugGroup(buf._Handle);
#endif
}

void RenderCommandBuffer::StartPass(RenderPass&& pass)
{
    _Context->AssertMainThread();
    TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
    TTE_ASSERT(pass._Handle == nullptr, "Duplicate render pass!");
    TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");

    SDL_GPUColorTargetInfo targets[8]{};
    U32 nTargets = 0;
    Bool hasDepth = pass.Targets.Depth.Target != nullptr;
    while (nTargets < 8 && pass.Targets.Target[nTargets].Target != nullptr)
    {
        RenderTargetResolvedSurface& src = pass.Targets.Target[nTargets];
        SDL_GPUColorTargetInfo& target = targets[nTargets++];
        target.cycle = target.cycle_resolve_texture = false;
        if (pass.DoClearColour)
        {
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.clear_color = SDL_FColor{ pass.ClearColour.r, pass.ClearColour.g, pass.ClearColour.b, pass.ClearColour.a };
        }
        else
        {
            target.load_op = SDL_GPU_LOADOP_LOAD;
        }
        target.mip_level = src.Mip;
        target.layer_or_depth_plane = src.Slice;
        target.store_op = SDL_GPU_STOREOP_STORE;
        target.resolve_layer = target.resolve_mip_level = 0;
        target.resolve_texture = nullptr;
        target.texture = src.Target->GetHandle(_Context);
    }

    SDL_GPUDepthStencilTargetInfo depth{};
    if (hasDepth)
    {
        depth.cycle = false;
        if (pass.DoClearDepth)
        {
            depth.clear_depth = pass.ClearDepth;
            depth.load_op = SDL_GPU_LOADOP_CLEAR;
        }
        else
        {
            depth.load_op = SDL_GPU_LOADOP_LOAD;
        }
        if (pass.DoClearStencil)
        {
            depth.clear_stencil = pass.ClearStencil;
            depth.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        }
        else
        {
            depth.stencil_load_op = SDL_GPU_LOADOP_LOAD;
        }
        depth.texture = pass.Targets.Depth.Target->GetHandle(_Context);
        depth.store_op = SDL_GPU_STOREOP_STORE;
        depth.stencil_store_op = SDL_GPU_STOREOP_STORE;
    }

    if (pass.Name)
    {
        _Context->_PushDebugGroup(*this, pass.Name);
    }
    else
    {
        _Context->_PushDebugGroup(*this, "Unnamed TTE Pass");
    }

    RenderPass* pPass = _Context->GetFrame(false).Heap.New<RenderPass>();
    *pPass = std::move(pass);
    pPass->_Handle = SDL_BeginGPURenderPass(_Handle, nTargets ? targets : 0, nTargets, hasDepth ? &depth : nullptr);

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
    if (shader == RenderShaderType::VERTEX)
        SDL_PushGPUVertexUniformData(_Handle, slot, data, size);
    else if (shader == RenderShaderType::FRAGMENT)
        SDL_PushGPUFragmentUniformData(_Handle, slot, data, size);
}

void RenderCommandBuffer::BindParameters(RenderFrame& frame, ShaderParametersStack* stack)
{
    TTE_ASSERT(_BoundPipeline.get() != nullptr, "No currently bound pipeline.");

    ShaderParameterTypes parametersToSet{};

    Ptr<RenderShader> vsh, fsh{};
    TTE_ASSERT(_Context->_FXCache.ResolveEffectRef(RenderEffectRef{_BoundPipeline->EffectHash}, vsh, fsh), "Shader effect variant could not be loaded!");

    for (U8 i = 0; i < (U8)ShaderParameterType::PARAMETER_COUNT; i++)
    {
        if (vsh->ParameterSlots[i] != (U8)0xFF || fsh->ParameterSlots[i] != (U8)0xFF)
            parametersToSet.Set((ShaderParameterType)i, true);
    }

    // bind the latest index and vertex buffers pushed always
    Bool BoundIndex = false;
    Bool BoundVertex[(U32)ShaderParameterType::PARAMETER_LAST_VERTEX_BUFFER - (U32)ShaderParameterType::PARAMETER_FIRST_VERTEX_BUFFER + 1]{};

    do
    {

        ShaderParametersGroup* group = stack->Group;

        for (U32 i = 0; i < group->NumParameters; i++)
        {
            ShaderParametersGroup::ParameterHeader& paramInfo = group->GetHeader(i);

            if (paramInfo.Type == (U8)ShaderParameterType::PARAMETER_INDEX0IN && !BoundIndex) // bind index buffer
            {
                BoundIndex = true;
                Ptr<RenderBuffer> proxy{ group->GetParameter(i).GenericValue.Buffer, &NullDeleter };
                TTE_ASSERT(proxy.get(), "Required index buffer %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);
                BindIndexBuffer(proxy, group->GetParameter(i).GenericValue.Offset, _BoundPipeline->VertexState.IsHalf);
                continue;
            }

            if (paramInfo.Type >= (U8)ShaderParameterType::PARAMETER_FIRST_VERTEX_BUFFER && // bind vertex buffer
               paramInfo.Type <= (U8)ShaderParameterType::PARAMETER_LAST_VERTEX_BUFFER && !BoundVertex[paramInfo.Type - (U8)ShaderParameterType::PARAMETER_FIRST_VERTEX_BUFFER])
            {
                // bind this vertex buffer
                BoundVertex[paramInfo.Type - (U8)ShaderParameterType::PARAMETER_FIRST_VERTEX_BUFFER] = true;
                Ptr<RenderBuffer> proxy{ group->GetParameter(i).GenericValue.Buffer, &NullDeleter };
                TTE_ASSERT(proxy.get(), "Required vertex buffer %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);
                U32 offset = group->GetParameter(i).GenericValue.Offset;
                BindVertexBuffers(&proxy, &offset, (U32)(paramInfo.Type - (U32)ShaderParameterType::PARAMETER_FIRST_VERTEX_BUFFER), 1);
                continue;
            }

            if (parametersToSet[(ShaderParameterType)paramInfo.Type])
            {
                if (paramInfo.Class == (U8)ShaderParameterTypeClass::UNIFORM_BUFFER)
                {
                    TTE_ASSERT(group->GetParameter(i).UniformValue.Data, "Required uniform buffer %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);
                    if (vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                        BindUniformData(vsh->ParameterSlots[paramInfo.Type], RenderShaderType::VERTEX,
                                        group->GetParameter(i).UniformValue.Data, group->GetParameter(i).UniformValue.Size);
                    if (fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                        BindUniformData(fsh->ParameterSlots[paramInfo.Type], RenderShaderType::FRAGMENT,
                                        group->GetParameter(i).UniformValue.Data, group->GetParameter(i).UniformValue.Size);
                }
                else if (paramInfo.Class == (U8)ShaderParameterTypeClass::SAMPLER)
                {
                    if (vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderTexture> tex{ group->GetParameter(i).SamplerValue.Texture, &NullDeleter };
                        TTE_ASSERT(tex.get(), "Required texture sampler %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);

                        RenderSampler* samDesc = group->GetParameter(i).SamplerValue.SamplerDesc;
                        Ptr<RenderSampler> resolvedSampler = _Context->_FindSampler(samDesc ? *samDesc : RenderSampler{});

                        BindTextures(vsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::VERTEX, &tex, &resolvedSampler);
                    }
                    if (fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderTexture> tex{ group->GetParameter(i).SamplerValue.Texture, &NullDeleter };
                        TTE_ASSERT(tex.get(), "Required texture sampler %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);

                        RenderSampler* samDesc = group->GetParameter(i).SamplerValue.SamplerDesc;
                        Ptr<RenderSampler> resolvedSampler = _Context->_FindSampler(samDesc ? *samDesc : RenderSampler{});

                        BindTextures(fsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::FRAGMENT, &tex, &resolvedSampler);
                    }
                }
                else if (paramInfo.Class == (U8)ShaderParameterTypeClass::GENERIC_BUFFER)
                {
                    if (vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderBuffer> buf{ group->GetParameter(i).GenericValue.Buffer, &NullDeleter };
                        TTE_ASSERT(buf.get(), "Required generic buffer %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);
                        BindGenericBuffers(vsh->ParameterSlots[paramInfo.Type], 1, &buf, RenderShaderType::VERTEX);
                    }
                    if (fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
                    {
                        Ptr<RenderBuffer> buf{ group->GetParameter(i).GenericValue.Buffer, &NullDeleter };
                        TTE_ASSERT(buf.get(), "Required generic buffer %s was not bound or was null", RenderContext::GetParameterDesc((ShaderParameterType)paramInfo.Type).Name);
                        BindGenericBuffers(fsh->ParameterSlots[paramInfo.Type], 1, &buf, RenderShaderType::FRAGMENT);
                    }
                }
                parametersToSet.Set((ShaderParameterType)paramInfo.Type, false); // done this parameter
            }
        }
        stack = stack->Parent;
    }
    while (stack);

    // any that are not bound use defaults for textures (Could do buffers? not much for such a small thing)

    Ptr<RenderSampler> defSampler = _Context->_FindSampler({});

    for (U32 i = 0; i < (U32)ShaderParameterType::PARAMETER_COUNT; i++)
    {
        if (parametersToSet[(ShaderParameterType)i])
        {
            if (ShaderParametersInfo[i].Class == ShaderParameterTypeClass::SAMPLER)
            {
                // use default tex
                if (vsh->ParameterSlots[i] != (U8)0xFF)
                {
                    BindDefaultTexture(vsh->ParameterSlots[i], RenderShaderType::VERTEX, defSampler, ShaderParametersInfo[i].DefaultTex);
                }
                if (fsh->ParameterSlots[i] != (U8)0xFF)
                {
                    BindDefaultTexture(fsh->ParameterSlots[i], RenderShaderType::FRAGMENT, defSampler, ShaderParametersInfo[i].DefaultTex);
                }
            }
            else
            {
                String fx = _Context->_FXCache.LocateEffectName(RenderEffectRef{_BoundPipeline->EffectHash});
                TTE_ASSERT(false, "Parameter buffer %s was required for effect variant %s but was not supplied.",
                           ShaderParametersInfo[i].Name, fx.c_str());
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
    for (U32 i = 0; i < num; i++)
    {
        binds[i].sampler = pSamplers[i]->_Handle;
        binds[i].texture = pTextures[i]->_Handle;
        pTextures[i]->_LastUsedFrame = frame;
        pSamplers[i]->LastUsedFrame = frame;
    }

    if (shader == RenderShaderType::FRAGMENT)
        SDL_BindGPUFragmentSamplers(_CurrentPass->_Handle, slot, binds, num);
    else if (shader == RenderShaderType::VERTEX)
        SDL_BindGPUVertexSamplers(_CurrentPass->_Handle, slot, binds, num);

}

void RenderCommandBuffer::DrawDefaultMesh(DefaultRenderMeshType type)
{
    _Context->AssertMainThread();
    for (auto& def : _Context->_DefaultMeshes)
    {
        if (def.Type == type)
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
    for (auto& def : _Context->_DefaultTextures)
    {
        if (def.Type == type)
        {
            BindTextures(slot, 1, shaderType, &def.Texture, &sampler);
            return;
        }
    }
    TTE_ASSERT(false, "Default texture could not be found");
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
    for (U32 i = 0; i < num; i++)
    {
        buffers[i] = pBuffers[i]->_Handle;
        pBuffers[i]->LastUsedFrame = frame;
    }

    if (shaderSlot == RenderShaderType::VERTEX)
        SDL_BindGPUVertexStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
    else if (shaderSlot == RenderShaderType::FRAGMENT)
        SDL_BindGPUFragmentStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
    else
        TTE_ASSERT(false, "Unknown render shader type");

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
    if (_Submitted && _SubmittedFence && _Context)
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
    if (_Context && _SubmittedFence)
    {
        Bool result = SDL_QueryGPUFence(_Context->_Device, _SubmittedFence);

        // if done, free fence
        if (result)
        {
            SDL_ReleaseGPUFence(_Context->_Device, _SubmittedFence);
            _SubmittedFence = nullptr;
            _Context->_ReclaimTransferBuffers(*this);
        }
        return result;
    }
    else return true; // nothing submitted
}

void RenderCommandBuffer::BindPipeline(Ptr<RenderPipelineState>& state)
{
    TTE_ASSERT(state && state->_Internal._Handle, "Invalid pipeline state");
    if (_Context)
    {
        TTE_ASSERT(_CurrentPass && _CurrentPass->_Handle, "No active non copy pass");
        _Context->AssertMainThread();
        SDL_BindGPUGraphicsPipeline(_CurrentPass->_Handle, state->_Internal._Handle);
        _BoundPipeline = state;
    }
}

// ============================ BUFFERS

Ptr<RenderBuffer> RenderContext::CreateVertexBuffer(U64 sizeBytes, String N)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::VERTEX;

    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_VERTEX;

    inf.props = SDL_CreateProperties();
    SDL_SetStringProperty(inf.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, N.c_str());
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    _AttachContextDependentResource(buffer, true);
    SDL_DestroyProperties(inf.props);

    return buffer;
}

Ptr<RenderBuffer> RenderContext::CreateGenericBuffer(U64 sizeBytes, String N)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::UNIFORM;

    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;

    inf.props = SDL_CreateProperties();
    SDL_SetStringProperty(inf.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, N.c_str());
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    SDL_DestroyProperties(inf.props);

    _AttachContextDependentResource(buffer, true);
    return buffer;
}

Ptr<RenderBuffer> RenderContext::CreateIndexBuffer(U64 sizeBytes, String N)
{
    Ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
    buffer->_Context = this;
    buffer->SizeBytes = sizeBytes;
    buffer->Usage = RenderBufferUsage::INDICES;

    SDL_GPUBufferCreateInfo inf{};
    inf.size = (U32)sizeBytes;
    inf.props = 0;
    inf.usage = SDL_GPU_BUFFERUSAGE_INDEX;

    inf.props = SDL_CreateProperties();
    SDL_SetStringProperty(inf.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, N.c_str());
    buffer->_Handle = SDL_CreateGPUBuffer(_Device, &inf);
    SDL_DestroyProperties(inf.props);

    _AttachContextDependentResource(buffer, true);
    return buffer;
}

void RenderCommandBuffer::BindVertexBuffers(Ptr<RenderBuffer>* buffers, U32* offsets, U32 first, U32 num)
{
    TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
    TTE_ASSERT(buffers && first + num <= 32, "Invalid arguments passed into BindVertexBuffers");

    U64 frame = _Context->GetFrame(false).FrameNumber;

    SDL_GPUBufferBinding binds[32]{};
    for (U32 i = 0; i < num; i++)
    {
        TTE_ASSERT(buffers[i] && buffers[i]->_Handle, "Invalid buffer passed in for binding slot %d: is null", first + i);
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

    if (_CurrentPass)
        TTE_ASSERT(_CurrentPass->_CopyHandle != nullptr, "A render pass cannot be active unless its a copy pass if uploading buffers.");

    _RenderTransferBuffer tBuffer = _Context->_AcquireTransferBuffer(numBytes, *this);

    U8* mappedMemorySeg = (U8*)SDL_MapGPUTransferBuffer(_Context->_Device, tBuffer.Handle, false);
    srcStream->SetPosition(src);
    srcStream->Read(mappedMemorySeg, numBytes);
    SDL_UnmapGPUTransferBuffer(_Context->_Device, tBuffer.Handle);

    Bool bStartPass = _CurrentPass == nullptr;

    if (bStartPass)
        StartCopyPass();

    SDL_GPUTransferBufferLocation srcinf{};
    SDL_GPUBufferRegion dst{};
    srcinf.offset = 0;
    srcinf.transfer_buffer = tBuffer.Handle;
    dst.size = numBytes;
    dst.offset = destOffset;
    dst.buffer = buffer->_Handle;

    SDL_UploadToGPUBuffer(_CurrentPass->_CopyHandle, &srcinf, &dst, false);

    if (bStartPass)
        EndPass();

}

void RenderContext::_ReclaimTransferBuffers(RenderCommandBuffer& cmds)
{
    std::lock_guard<std::mutex> L{ _Lock };
    for (auto& buffer : cmds._AcquiredTransferBuffers)
    {
        buffer.Size = 0;
        _AvailTransferBuffers.insert(std::move(buffer));
    }
    cmds._AcquiredTransferBuffers.clear();
}

_RenderTransferBuffer RenderContext::_AcquireTransferBuffer(U32 size, RenderCommandBuffer& cmds)
{
    std::lock_guard<std::mutex> L{ _Lock };
    for (auto it = _AvailTransferBuffers.begin(); it != _AvailTransferBuffers.end(); it++)
    {

        if (size >= it->Size)
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
    inf.props = SDL_CreateProperties();
    SDL_SetStringProperty(inf.props, SDL_PROP_GPU_TRANSFERBUFFER_CREATE_NAME_STRING, "Acquired TTE Transfer Buffer");
    buffer.Handle = SDL_CreateGPUTransferBuffer(_Device, &inf);
    SDL_DestroyProperties(inf.props);
    buffer.LastUsedFrame = cmds._Context->GetFrame(false).FrameNumber;
    cmds._AcquiredTransferBuffers.push_back(buffer);

    return buffer;
}

void RenderBuffer::Release()
{
    if (_Handle && _Context)
        SDL_ReleaseGPUBuffer(_Context->_Device, _Handle);
    _Handle = nullptr;
    _Context = nullptr;
}

RenderBuffer::~RenderBuffer()
{
    Release();
}

// ================================== PARAMETERS

void RenderContext::PushParameterStack(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersStack* stack)
{
    if (stack->Parent)
    {
        PushParameterStack(frame, self, stack->Parent);
    }
    if (stack->Group)
    {
        PushParameterGroup(frame, self, stack->Group);
        self->ParameterTypes.Import(stack->ParameterTypes);
    }
}

void RenderContext::PushParameterGroup(RenderFrame& frame, ShaderParametersStack* self, ShaderParametersGroup* group)
{
    if (self->Group)
    {
        ShaderParametersStack* stack = frame.Heap.NewNoDestruct<ShaderParametersStack>();
        stack->Group = self->Group;
        stack->Parent = self->Parent;
        stack->ParameterTypes = self->ParameterTypes;
        self->Parent = stack;
    }
    self->Group = group;
    for (U32 i = 0; i < group->NumParameters; i++)
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
    for (U32 i = 0; i < N; i++)
    {
        param = types.FindFirstBit(param);
        pGroup->GetHeader(i).Class = (U8)ShaderParametersInfo[(U8)param].Class;
        pGroup->GetHeader(i).Type = (U8)param;
        param = (ShaderParameterType)((U8)param + 1);
    }
    return pGroup;
}

void RenderContext::SetParameterDefaultTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                               DefaultRenderTextureType tex, RenderSampler* sampler)
{
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::SAMPLER,
                       "Parameter %s is not a sampler!", ShaderParametersInfo[(U32)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    ShaderParametersGroup::Parameter& param = group->GetParameter(ind);
    param.SamplerValue.Texture = _GetDefaultTexture(tex).get(); // no autorelease needed, its constant
    param.SamplerValue.SamplerDesc = sampler;
}

void RenderContext::SetParameterTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                        Ptr<RenderTexture> tex, RenderSampler* sampler)
{
    if (!DbgCheckOwned(tex))
        return;
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::SAMPLER,
                       "Parameter %s is not a sampler!", ShaderParametersInfo[(U8)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    frame.PushAutorelease(tex);
    group->GetParameter(ind).SamplerValue.Texture = tex.get();
    group->GetParameter(ind).SamplerValue.SamplerDesc = sampler;
}

void RenderContext::SetParameterUniform(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                        const void* uniformData, U32 size)
{
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::UNIFORM_BUFFER,
                       "Parameter %s is not a uniform!", ShaderParametersInfo[(U8)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    UniformParameterBinding& param = group->GetParameter(ind).UniformValue;
    param.Data = uniformData;
    param.Size = size;
}

void RenderContext::SetParameterGenericBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                              Ptr<RenderBuffer> buffer, U32 bufferOffset)
{
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::GENERIC_BUFFER,
                       "Parameter %s is not a generic buffer!", ShaderParametersInfo[(U8)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = bufferOffset;
}

void RenderContext::SetParameterVertexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                             Ptr<RenderBuffer> buffer, U32 off)
{
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::VERTEX_BUFFER,
                       "Parameter %s is not a vertex buffer!", ShaderParametersInfo[(U8)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = off;
}

// Set an index buffer parameter input.
void RenderContext::SetParameterIndexBuffer(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
                                            Ptr<RenderBuffer> buffer, U32 startIndex)
{
    U32 ind = (U32)-1;
    for (U32 i = 0; i < group->NumParameters; i++)
    {
        if (group->GetHeader(i).Type == (U8)type)
        {
            TTE_ASSERT(group->GetHeader(i).Class == (U8)ShaderParameterTypeClass::INDEX_BUFFER,
                       "Parameter %s is not an index buffer!", ShaderParametersInfo[(U8)type].Name);
            ind = i;
            break;
        }
    }
    TTE_ASSERT(ind != (U32)-1, "Shader parameter %s does not exist in parameter group", ShaderParametersInfo[(U8)type].Name);
    frame.PushAutorelease(buffer);
    group->GetParameter(ind).GenericValue.Buffer = buffer.get();
    group->GetParameter(ind).GenericValue.Offset = (U32)startIndex;
}

// ================================== RENDERING BULK

void RenderContext::PurgeResources()
{
    if (IsCallingFromMain())
    {
        std::lock_guard<std::mutex> G{ _Lock };
        for (auto& b : _AvailTransferBuffers)
            SDL_ReleaseGPUTransferBuffer(_Device, b.Handle);
        _AvailTransferBuffers.clear();
        for (auto& sampler : _Samplers)
        {
            Ptr<RenderSampler> s = std::move(sampler);
            _PendingSDLResourceDeletions.push_back(PendingDeletion{ std::move(s), GetFrame(false).FrameNumber });
        }
        _Samplers.clear();
    }
    else
        _Flags |= RENDER_CONTEXT_NEEDS_PURGE;
}

Bool RenderContext::DbgCheckOwned(const Ptr<Handleable> handle) const
{
    Bool bOwned = handle->OwnedBy(*this, false); // dont lock, it should have been. important!!
#if DEBUG
    TTE_ASSERT(bOwned, "Common resource used in render context was not locked before use."); // c++ error
#endif
    return bOwned;
}

Bool RenderContext::TouchResource(const Ptr<Handleable> handle)
{
    if (handle->OwnedBy(*this, false))
    {
        // ideal case
        handle->OverrideLockedTimeStamp(*this, true, IsCallingFromMain() ? GetRenderFrameNumber() : GetCurrentFrameNumber());
        return true;
    }
    Bool bResult = handle->Lock(*this);
    if (bResult)
    {
        std::lock_guard<std::mutex> G{ _Lock };
        handle->OverrideLockedTimeStamp(*this, false, IsCallingFromMain() ? GetRenderFrameNumber() : GetCurrentFrameNumber());
        _LockedResources.push_back(handle);
    }
    return bResult;
}

ShaderParametersStack* RenderContext::AllocateParametersStack(RenderFrame& frame)
{
    return frame.Heap.NewNoDestruct<ShaderParametersStack>(); // on heap no destruct needed its very lightweight
}

// NON-MAIN THREAD CALL: perform all higher level render calls and do any skinning / animation updates, etc.
Bool RenderContext::_Populate(const JobThread& jobThread, void* pRenderCtx, void* pDT)
{

    Float dt = *((Float*)&pDT);
    RenderContext* pContext = static_cast<RenderContext*>(pRenderCtx);
    RenderFrame& frame = pContext->GetFrame(true);

    {
        // update delta layers
        std::lock_guard<std::mutex> G{ pContext->_Lock };
        for (auto& dl : pContext->_DeltaLayers)
        {
            if (dl)
            {
                pContext->_LayerStack.push_back(std::move(dl));
            }
            else
            {
                TTE_ASSERT(pContext->_LayerStack.size(), "Trying to pop layer but stack is empty");
                pContext->_LayerStack.pop_back();
            }
        }
        pContext->_DeltaLayers.clear();
    }

    // process events
    if (frame._Events.size())
    {
        for (auto it = pContext->_LayerStack.rbegin(); it != pContext->_LayerStack.rend(); it++)
            (*it)->AsyncProcessEvents(frame._Events);
    }

    RenderNDCScissorRect rect{}; // full screen for now
    for (auto& layer : pContext->_LayerStack)
        rect = layer->AsyncUpdate(frame, rect, dt);

    return true;
}

void RenderContext::_Render(Float dt, RenderCommandBuffer* pMainCommandBuffer)
{
    RenderFrame& frame = GetFrame(false);

    // LOW LEVEL RENDER (MAIN THREAD) THE PREVIOUS FRAME.

    RenderSceneContext context{};
    context.DeltaTime = dt;
    context.CommandBuf = pMainCommandBuffer;

    // perform any uploads. Right now execute before any draws. In future we could optimise and use fences interleaved. Dont think even telltale do that.
    //RenderCommandBuffer* pCopyCommands = _NewCommandBuffer();
    frame.UpdateList->BeginRenderFrame(pMainCommandBuffer);

    RenderFrame* pFrame = &frame;
    frame.PreRenderCallbacks.CallErased(&pFrame, 0, nullptr, 0, nullptr, 0, nullptr, 0);

    _ExecuteFrame(frame, context);

    frame.PostRenderCallbacks.CallErased(&pFrame, 0, nullptr, 0, nullptr, 0, nullptr, 0);

    frame.UpdateList->EndRenderFrame();

}

// ===================================== RENDER FRAME UPDATE LIST =====================================

void RenderFrameUpdateList::BeginRenderFrame(RenderCommandBuffer* pCopyCommands)
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

        if (_TotalNumUploads > 32)
        {
            U64 average = _TotalBufferUploadSize / _TotalNumUploads;
            U64 stagingSize = _TotalBufferUploadSize > (1024 * 1024 * 256) ? 1024 * 1024 * 256 : (MAX(average, 1024 * 1024 * 32));
            groupStaging = _Context._AcquireTransferBuffer((U32)stagingSize, *pCopyCommands);
            stagingGroupBuffer = (U8*)SDL_MapGPUTransferBuffer(_Context._Device, groupStaging.Handle, false);
        }

        GroupTransferBuffer* deferred = nullptr; // these are done after as we cant map memory and push copy commands at the same time

        // META BUFFERS
        for (MetaBufferUpload* upload = _MetaUploads; upload; upload = upload->Next)
        {
            _RenderTransferBuffer local{};
            _RenderTransferBuffer* myBuffer = nullptr;
            U8* staging = stagingGroupBuffer;
            U64 offset = 0;

            if (groupStagingOffset + upload->Data.BufferSize > groupStaging.Size || upload->Data.BufferSize > 1024 * 1024 * 64)
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

            if (local.Handle)
                SDL_UnmapGPUTransferBuffer(_Context._Device, local.Handle);

            SDL_GPUTransferBufferLocation src{};
            src.offset = (U32)offset;
            src.transfer_buffer = myBuffer->Handle;
            SDL_GPUBufferRegion dst{};
            dst.size = upload->Data.BufferSize;
            dst.offset = (U32)upload->DestPosition;
            dst.buffer = upload->Buffer.get()->_Handle;

            if (local.Handle)
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
        for (DataStreamBufferUpload* upload = _StreamUploads; upload; upload = upload->Next)
        {
            _RenderTransferBuffer local{};
            _RenderTransferBuffer* myBuffer = nullptr;
            U8* staging = stagingGroupBuffer;
            U64 offset = 0;

            if (groupStagingOffset + upload->UploadSize > groupStaging.Size || upload->UploadSize > 1024 * 1024 * 64)
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

            if (local.Handle)
                SDL_UnmapGPUTransferBuffer(_Context._Device, local.Handle);

            SDL_GPUTransferBufferLocation src{};
            src.offset = (U32)offset;
            src.transfer_buffer = myBuffer->Handle;
            SDL_GPUBufferRegion dst{};
            dst.size = (U32)upload->UploadSize;
            dst.offset = (U32)upload->DestPosition;
            dst.buffer = upload->Buffer.get()->_Handle;

            if (local.Handle)
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

        // TEXTURES
        for (MetaTextureUpload* upload = _MetaTexUploads; upload; upload = upload->Next)
        {
            U32 w, h, r, s{};
            U32 totalw, totalh, depth, arrz{};
            upload->Texture->GetImageInfo(upload->Mip, upload->Slice, upload->Face, w, h, r, s);

            U32 bytes = (U32)upload->Data.BufferSize;
            if (!bytes)
                bytes = RenderTexture::CalculateSlicePitch(upload->Texture->_Format, w, h);

            _RenderTransferBuffer local = _Context._AcquireTransferBuffer(bytes, *pCopyCommands);

            U8* staging = (U8*)SDL_MapGPUTransferBuffer(_Context._Device, local.Handle, false);

            //memcpy(staging, upload->Data.BufferData.get(), upload->Data.BufferSize);
            if (!upload->Data.BufferData.get() || upload->Data.BufferSize == 0)
                memset(staging, 0x00, bytes); // black empty tex
            else
                memcpy(staging, upload->Data.BufferData.get(), bytes);

            SDL_UnmapGPUTransferBuffer(_Context._Device, local.Handle);

            SDL_GPUTextureTransferInfo srcinf{};
            SDL_GPUTextureRegion dst{};
            srcinf.offset = 0;
            srcinf.rows_per_layer = h;
            srcinf.pixels_per_row = w; // assume width tight packing no padding.
            srcinf.transfer_buffer = local.Handle;
            dst.texture = upload->Texture->GetHandle(&_Context);
            dst.mip_level = upload->Mip;
            upload->Texture->GetDimensions(totalw, totalh, depth, arrz);
            dst.layer = (upload->Slice * arrz) + upload->Face;
            dst.x = dst.y = dst.z = 0; // upload whole subimage for any slice/mip for now.
            dst.w = w;
            dst.h = h;
            dst.d = 1; // upload one depth

            SDL_UploadToGPUTexture(pCopyCommands->_CurrentPass->_CopyHandle, &srcinf, &dst, false);

        }

        if (groupStaging.Handle)
        {
            SDL_UnmapGPUTransferBuffer(_Context._Device, groupStaging.Handle);

            for (GroupTransferBuffer* tr = deferred; tr; tr = tr->Next)
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
void RenderFrameUpdateList::_DismissTexture(const Ptr<RenderTexture>& tex, U32 mip, U32 slice, U32 face)
{
    {
        MetaTextureUpload* uploadPrev = nullptr;
        for (MetaTextureUpload* upload = _MetaTexUploads; upload; upload = upload->Next)
        {
            if (upload->Texture == tex && upload->Mip == mip && upload->Slice == slice && upload->Face == face)
            {
                _TotalBufferUploadSize -= upload->Data.BufferSize;
                _TotalNumUploads--;
                if (uploadPrev)
                    uploadPrev->Next = upload->Next;
                else
                    _MetaTexUploads = upload->Next;
                *upload = MetaTextureUpload{}; // reset
                break;
            }
            uploadPrev = upload;
        }
    }
}

void RenderFrameUpdateList::_DismissBuffer(const Ptr<RenderBuffer>& buf)
{
    {
        MetaBufferUpload* uploadPrev = nullptr;
        for (MetaBufferUpload* upload = _MetaUploads; upload; upload = upload->Next)
        {
            if (upload->Buffer == buf)
            {
                _TotalBufferUploadSize -= upload->Data.BufferSize;
                _TotalNumUploads--;
                if (uploadPrev)
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
        for (DataStreamBufferUpload* upload = _StreamUploads; upload; upload = upload->Next)
        {
            if (upload->Buffer == buf)
            {
                _TotalBufferUploadSize -= upload->UploadSize;
                _TotalNumUploads--;
                if (uploadPrev)
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

void RenderFrameUpdateList::UpdateBufferMeta(const Meta::BinaryBuffer& metaBuffer, const Ptr<RenderBuffer>& destBuffer, U64 dst)
{
    if (destBuffer->LastUpdatedFrame == _Frame.FrameNumber)
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

void RenderFrameUpdateList::UpdateBufferDataStream(const DataStreamRef& srcStream, const Ptr<RenderBuffer>& destBuffer, U64 srcPos, U64 nBytes, U64 dst)
{
    if (destBuffer->LastUpdatedFrame == _Frame.FrameNumber)
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

void RenderFrameUpdateList::UpdateTexture(const Ptr<RenderTexture>& destTexture, U32 mip, U32 slice, U32 face)
{
    if (!_Context.DbgCheckOwned(destTexture))
        return;
    destTexture->_Context = &_Context;
    if (destTexture->_LastUpdatedFrame == _Frame.FrameNumber)
    {
        _DismissTexture(destTexture, mip, slice, face);
    }
    else
    {
        destTexture->_LastUpdatedFrame = _Frame.FrameNumber;
    }
    MetaTextureUpload* upload = _Frame.Heap.New<MetaTextureUpload>();
    upload->Data = destTexture->_Images[destTexture->GetImageIndex(mip, slice, face)].Data;
    upload->Mip = mip;
    upload->Texture = destTexture;
    upload->Next = _MetaTexUploads;
    _MetaTexUploads = upload;
    _TotalNumUploads++;
    _TotalBufferUploadSize += upload->Data.BufferSize;
}
