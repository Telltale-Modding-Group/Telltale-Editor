#include <Renderer/RenderContext.hpp>
#include <Core/Context.hpp>

#include <chrono>

// ============================ SURFACE FORMAT ENUM MAPPINGS

static struct {
	RenderSurfaceFormat Format;
	SDL_GPUTextureFormat SDLFormat;
}
SDL_FormatMappings[2]
{
	{RenderSurfaceFormat::RGBA8, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT},
	{RenderSurfaceFormat::UNKNOWN, SDL_GPU_TEXTUREFORMAT_INVALID}, // do not add below this, add above
};

inline SDL_GPUTextureFormat ToSDLFormat(RenderSurfaceFormat format)
{
	U32 i = 0;
	while(SDL_FormatMappings[i].Format != RenderSurfaceFormat::UNKNOWN)
	{
		if(SDL_FormatMappings[i].Format == format)
			return SDL_FormatMappings[i].SDLFormat;
		i++;
	}
	return SDL_GPU_TEXTUREFORMAT_INVALID;
}

inline RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format)
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

RenderContext::RenderContext(String wName)
{
    TTE_ASSERT(JobScheduler::Instance, "Job scheduler has not been initialised. Ensure a ToolContext exists.");
    TTE_ASSERT(SDL3_Initialised, "SDL3 has not been initialised, or failed to.");
    
    _MainFrameIndex = 0;
    _RenderJob = JobHandle();
    _Frame[0].Reset(0);
    _Frame[1].Reset(0);
    
    // Create window and renderer
	_Window = SDL_CreateWindow(wName.c_str(), 780, 326, 0);
	_Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC, false, nullptr);
    _BackBuffer = SDL_GetWindowSurface(_Window);
}

RenderContext::~RenderContext()
{
	
	// CLEANUP
	
	_Pipelines.clear();
	_LoadedShaders.clear();
	
    _Frame[0]._Heap.ReleaseAll();
    _Frame[1]._Heap.ReleaseAll();
	
	SDL_ReleaseWindowFromGPUDevice(_Device, _Window);
	SDL_DestroyWindow(_Window);
	SDL_DestroyGPUDevice(_Device);
	
}

// ============================ HIGH LEVEL RENDER FUNCTIONS

// RENDER THREAD CALL: take in high level calls to render objects, eg sphere, mesh
Bool RenderContext::_Render(const JobThread& jobThread, void* pRenderCtx, void* pRenderFrame)
{
	
	// LOCALS
	
    RenderContext* pContext = static_cast<RenderContext*>(pRenderCtx);
    RenderFrame* pFrame = static_cast<RenderFrame*>(pRenderFrame);
    
	// Translate high level render operations, eg draw text, draw verts, into command buffer operations for GPU optimised.
	
	{
		
		RenderCommandBuffer* pMainCommandBuffer = pContext->_NewCommandBuffer();
		
		// setup swapchain tex
		pFrame->_BackBuffer = pFrame->_Heap.New<RenderTexture>();
		pFrame->_BackBuffer->_Context = pContext;
		pFrame->_BackBuffer->Format = FromSDLFormat(SDL_GetGPUSwapchainTextureFormat(pContext->_Device, pContext->_Window));
		SDL_WaitAndAcquireGPUSwapchainTexture(pMainCommandBuffer->_Handle,
											  pContext->_Window, &pFrame->_BackBuffer->_Handle,
											  &pFrame->_BackBuffer->Width,  &pFrame->_BackBuffer->Height);
		
		// RENDER
		
		// command buffer is submitted below
		
	}
    
    // SUBMIT ALL UNSUBMITTED COMMAND buffers
	
	U32 activeCommandBuffers = 0;
	SDL_GPUFence** awaitingFences = pFrame->_Heap.NewArray<SDL_GPUFence*>((U32)pFrame->_CommandBuffers.size());
	for(auto& commandBuffer: pFrame->_CommandBuffers)
	{
		if(!commandBuffer->_Submitted)
			commandBuffer->Submit(); // submit if needed
		
		if(commandBuffer->_SubmittedFence && !commandBuffer->Finished())
			awaitingFences[activeCommandBuffers++] = commandBuffer->_SubmittedFence; // add waiting fences
	}
	
	// WAIT ON ALL ACTIVE COMMAND BUFFERS TO FINISH GPU EXECUTION
	
	if(activeCommandBuffers > 0)
		SDL_WaitForGPUFences(pContext->_Device, true, awaitingFences, activeCommandBuffers);
	for(auto& commandBuffer: pFrame->_CommandBuffers) // free the fences
	{
		if(commandBuffer->_SubmittedFence != nullptr)
		{
			SDL_ReleaseGPUFence(pContext->_Device, commandBuffer->_SubmittedFence);
			commandBuffer->_SubmittedFence = nullptr;
		}
	}
	
	// CLEAR COMMAND BUFFERS. DONE.
	pFrame->_CommandBuffers.clear();
	
    return true;
}

// ============================ MAIN THREAD UPDATERS

// MAIN THREAD CALL: take in scene information and perform anim, updates, call mesh renders, etc
void RenderContext::_RenderMain(Float deltaSeconds)
{
	
    ;
	
}

// USER CALL: called every frame by user to update render and kick off render job
Bool RenderContext::FrameUpdate(Bool isLastFrame)
{
    Bool bUserWantsQuit = false;
    
    // 1. increase frame number
    _Frame[_MainFrameIndex].Reset(_Frame[_MainFrameIndex]._FrameNumber + 1);
    
    // 2. poll SDL events
    SDL_Event e {0};
    while( SDL_PollEvent( &e ) )
    {
        if( e.type == SDL_EVENT_QUIT )
        {
            bUserWantsQuit = true;
        }
    }
    
    // 3. Do rendering and calculate delta time
	
	Float dt = 0.0f;
	if(_StartTimeMicros != 0)
	{
		U64 myTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		dt = ((Float)(myTime - _StartTimeMicros)) * 1e-6F; // convert to seconds
		_StartTimeMicros = myTime; // reset timer for next frame
	}
	
    _RenderMain(dt); // DO RENDER!

    // 4. Wait for previous render to complete if needed
    if(_RenderJob)
    {
        JobScheduler::Instance->Wait(_RenderJob);
        _RenderJob.Reset(); // reset handle to empty
    }
    
    // 5. NO RENDER THREAD JOB ACTIVE, SO NO LOCKING NEEDED UNTIL SUBMIT. Give render frame our new processed frame to render, and swap.
    _MainFrameIndex ^= 1;
    
    // 6. Kick off render job to render the last thread data (if needed)
    if(!isLastFrame && !bUserWantsQuit)
    {
        JobDescriptor job{};
        job.AsyncFunction = &_Render;
        job.Priority = JOB_PRIORITY_HIGHEST;
        job.UserArgA = this;
        job.UserArgB = &_Frame[_MainFrameIndex ^ 1]; // render thread frame
        _RenderJob = JobScheduler::Instance->Post(job);
        // in the future, maybe we can queue the next render job if we want more than just 2 snapshots of render state.
    }
    
    return bUserWantsQuit;
}

// ============================ FRAME MANAGEMENT

void RenderFrame::Reset(U64 newFrameNumber)
{	
	_CommandBuffers.clear();
	_FrameNumber = newFrameNumber;
	_Heap.FreeAll(); // free all objects, but keep memory
	_SceneStack = nullptr;
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
		info.format = ToSDLFormat(format);
		info.num_levels = nMips;
		info.sample_count = SDL_GPU_SAMPLECOUNT_1;
		info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		_Handle = SDL_CreateGPUTexture(pContext->_Device, &info);
	}
}

// ================ PIPELINE STATES, SHADER MGR

// find a loaded shader, if not found its loaded.
SDL_GPUShader* RenderContext::_FindShader(String name, RenderShaderType type)
{
	_Lock.lock();
	for(auto& sh: _LoadedShaders)
	{
		if(CompareCaseInsensitive(name, sh.Name))
		{
			_Lock.unlock();
			return sh.Handle;
		}
	}
	_Lock.unlock();
	
	// create, not found
	String rawSh{};
	U8* temp = nullptr;
	SDL_GPUShaderFormat fmts = SDL_GetGPUShaderFormats(_Device);
	SDL_GPUShaderCreateInfo info{};
	info.stage = type == RenderShaderType::FRAGMENT ? SDL_GPU_SHADERSTAGE_FRAGMENT : SDL_GPU_SHADERSTAGE_VERTEX;
	info.entrypoint = "main";
	info.num_samplers = info.num_storage_buffers = info.num_uniform_buffers = info.num_storage_textures = 0; // todo!
	
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
		info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		DataStreamRef stream = GetToolContext()->LoadLibraryResource("Shader/" + name + ".spv");
		if(!stream || stream->GetSize() == 0)
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
	
	RenderShader sh{};
	sh.Name = name;
	sh.Handle = SDL_CreateGPUShader(_Device, &info);
	sh.Context = this;
	
	if(temp)
	{
		TTE_FREE(temp);
		temp = nullptr; // free temp shader
	}
	
	TTE_ASSERT(sh.Handle, "Could not create shader %s", name.c_str());
	
	if(sh.Handle)
	{
		_Lock.lock();
		SDL_GPUShader* ret = sh.Handle;
		_LoadedShaders.push_back(sh);
		_Lock.unlock();
		return ret;
	}
	
	return nullptr;
}

// ======================= PIPELINE STATES

std::shared_ptr<RenderPipelineState> RenderContext::_AllocatePipelineState()
{
	auto val = TTE_NEW_PTR(RenderPipelineState, MEMORY_TAG_RENDERER);
	val->_Internal._Context = this;
	
	// add to array
	{
		std::lock_guard<std::mutex>{_Lock};
		_Pipelines.push_back(val);
	}
	
	return val;
}

void RenderPipelineState::Create()
{
	if(_Internal._Handle == nullptr)
	{
		_Internal._Context->AssertRenderThread();
		
		// calculate hash
		Hash = CRC64LowerCase((const U8*)VertexSh.c_str(), (U32)VertexSh.length(), 0);
		Hash = CRC64LowerCase((const U8*)FragmentSh.c_str(), (U32)FragmentSh.length(), Hash);
		
		// create
		SDL_GPUGraphicsPipelineCreateInfo info{};
		
		
		
		_Internal._Handle = SDL_CreateGPUGraphicsPipeline(_Internal._Context->_Device, &info);
		
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
	if(Handle)
	{
		SDL_ReleaseGPUShader(Context->_Device, Handle);
		Handle = nullptr;
	}
}

// ===================== COMMAND BUFFERS

RenderCommandBuffer* RenderContext::_NewCommandBuffer()
{
	AssertRenderThread();
	RenderFrame& frame = GetFrame(true);
	RenderCommandBuffer* pBuffer = frame._Heap.New<RenderCommandBuffer>();
	
	pBuffer->_Handle = SDL_AcquireGPUCommandBuffer(_Device);
	pBuffer->_Context = this;
	
	frame._CommandBuffers.push_back(pBuffer);
	
	return pBuffer;
}

void RenderCommandBuffer::PushPass(RenderPass&& pass, RenderTexture& sw)
{
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(pass._Handle == nullptr, "Duplicate render pass!");
	
	// todo improve this. allow different target descs.
	SDL_GPUColorTargetInfo swChain{};
	swChain.texture = sw._Handle;
	swChain.load_op = SDL_GPU_LOADOP_LOAD;
	swChain.store_op = SDL_GPU_STOREOP_STORE;
	swChain.clear_color.r = pass.ClearCol.r;
	swChain.clear_color.g = pass.ClearCol.g;
	swChain.clear_color.b = pass.ClearCol.b;
	swChain.clear_color.a = pass.ClearCol.a;
	
	RenderPass* pPass = _Context->GetFrame(true)._Heap.New<RenderPass>();
	*pPass = std::move(pass);
	pPass->_Handle = SDL_BeginGPURenderPass(_Handle, &swChain, 1, nullptr);
	
	pPass->_Next = _PassStack; // push to stack LL
	_PassStack = pPass;
}

RenderPass RenderCommandBuffer::PopPass()
{
	TTE_ASSERT(_PassStack, "No passes currently in stack");
	RenderPass top = std::move(*_PassStack);
	_PassStack = top._Next;
	
	SDL_EndGPURenderPass(top._Handle);
	top._Handle = nullptr;
	top._Next = nullptr;
	return top;
}

void RenderCommandBuffer::Submit()
{
	TTE_ASSERT(_Handle && !_SubmittedFence && !_Submitted, "Command buffer state is invalid");
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
		}
		return result;
	}
	else return true; // nothing submitted
}

void RenderCommandBuffer::BindPipeline(std::shared_ptr<RenderPipelineState> &state)
{
	TTE_ASSERT(state && state->_Internal._Handle, "Invalid pipeline state");
	if(_Context)
	{
		TTE_ASSERT(_PassStack, "Pass stack is empty");
		_Context->AssertRenderThread();
		SDL_BindGPUGraphicsPipeline(_PassStack->_Handle, state->_Internal._Handle);
	}
}
