#include <Renderer/RenderContext.hpp>
#include <Core/Context.hpp>

#include <chrono>
#include <cfloat>

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

// ============================ BUFFER ATTRIBUTE FORMAT ENUM MAPPINGS

static struct {
	RenderBufferAttributeFormat Format;
	SDL_GPUVertexElementFormat SDLFormat;
	U32 NumIntrinsics = 0; // for float4, this is 4
	U32 IntrinsicSize = 0; // for four ints, three ints, 1 int, etc, this is 4.
	RenderBufferAttributeFormat IntrinsicType = RenderBufferAttributeFormat::UNKNOWN;
}
SDL_VertexAttributeMappings[13]
{
	{RenderBufferAttributeFormat::F32x1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, 1, 4, RenderBufferAttributeFormat::F32x1},
	{RenderBufferAttributeFormat::F32x2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 2, 4, RenderBufferAttributeFormat::F32x1},
	{RenderBufferAttributeFormat::F32x3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 3, 4, RenderBufferAttributeFormat::F32x1},
	{RenderBufferAttributeFormat::F32x4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 4, 4, RenderBufferAttributeFormat::F32x1},
	{RenderBufferAttributeFormat::I32x1, SDL_GPU_VERTEXELEMENTFORMAT_INT, 1, 4, RenderBufferAttributeFormat::I32x1},
	{RenderBufferAttributeFormat::I32x2, SDL_GPU_VERTEXELEMENTFORMAT_INT2, 2, 4, RenderBufferAttributeFormat::I32x1},
	{RenderBufferAttributeFormat::I32x3, SDL_GPU_VERTEXELEMENTFORMAT_INT3, 3, 4, RenderBufferAttributeFormat::I32x1},
	{RenderBufferAttributeFormat::I32x4, SDL_GPU_VERTEXELEMENTFORMAT_INT4, 4, 4, RenderBufferAttributeFormat::I32x1},
	{RenderBufferAttributeFormat::U32x1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 1, 4, RenderBufferAttributeFormat::U32x1},
	{RenderBufferAttributeFormat::U32x2, SDL_GPU_VERTEXELEMENTFORMAT_UINT2, 2, 4, RenderBufferAttributeFormat::U32x1},
	{RenderBufferAttributeFormat::U32x3, SDL_GPU_VERTEXELEMENTFORMAT_UINT3, 3, 4, RenderBufferAttributeFormat::U32x1},
	{RenderBufferAttributeFormat::U32x4, SDL_GPU_VERTEXELEMENTFORMAT_UINT4, 4, 4, RenderBufferAttributeFormat::U32x1},
	{RenderBufferAttributeFormat::UNKNOWN, SDL_GPU_VERTEXELEMENTFORMAT_INVALID, 0, 0}, // add above this!
};

inline SDL_GPUVertexElementFormat ToSDLAttribFormat(RenderBufferAttributeFormat format)
{
	U32 i = 0;
	while(SDL_VertexAttributeMappings[i].Format != RenderBufferAttributeFormat::UNKNOWN)
	{
		if(SDL_VertexAttributeMappings[i].Format == format)
			return SDL_VertexAttributeMappings[i].SDLFormat;
		i++;
	}
	return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
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
	
	for(auto& transfer: _AvailTransferBuffers)
		SDL_ReleaseGPUTransferBuffer(_Device, transfer.Handle);
	
	for(auto& sampler: _Samplers)
		SDL_ReleaseGPUSampler(_Device, sampler->_Handle);
	
	for(auto& pipeline: _Pipelines)
		SDL_ReleaseGPUGraphicsPipeline(_Device, pipeline->_Internal._Handle);
	
	_Samplers.clear();
	_Pipelines.clear();
	_LoadedShaders.clear();
	_AvailTransferBuffers.clear();
	
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
	
	// RECLAIM TRANSFER BUFFERS
	for(auto& cmd: pFrame->_CommandBuffers)
		pContext->_ReclaimTransferBuffers(*cmd);
	
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

std::shared_ptr<RenderSampler> RenderContext::_FindSampler(RenderSampler desc)
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
	info.max_anisotropy = 0.0f;
	info.mip_lod_bias = desc.MipBias;
	info.mipmap_mode = (SDL_GPUSamplerMipmapMode)desc.MipMode;
	info.min_lod = 0.f;
	info.max_lod = FLT_MAX;
	desc._Handle = SDL_CreateGPUSampler(_Device, &info);
	std::shared_ptr<RenderSampler> pSampler = TTE_NEW_PTR(RenderSampler, MEMORY_TAG_RENDERER);
	*pSampler = desc;
	return pSampler;
}

RenderTexture::~RenderTexture()
{
	if(_Handle && _Context)
		SDL_ReleaseGPUTexture(_Context->_Device, _Handle);
	_Handle = nullptr;
}


void RenderCommandBuffer::UploadTextureData(std::shared_ptr<RenderTexture> &texture,
											DataStreamRef src, U64 srcOffset, U32 mip, U32 slice, U32 dataZ)
{
	if(_CurrentPass)
		TTE_ASSERT(_CurrentPass->_CopyHandle != nullptr, "A render pass cannot be active unless its a copy pass if uploading textures.");
	
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
		TTE_ASSERT(VertexState.NumVertexBuffers < 4 && VertexState.NumVertexAttribs < 32, "Vertex state has too many attributes / buffers");
		
		// calculate hash
		Hash = CRC64LowerCase((const U8*)VertexSh.c_str(), (U32)VertexSh.length(), 0);
		Hash = CRC64LowerCase((const U8*)FragmentSh.c_str(), (U32)FragmentSh.length(), Hash);
		Hash = CRC64((const U8*)&VertexState.NumVertexBuffers, 4, Hash);
		
		for(U32 i = 0; i < VertexState.NumVertexAttribs; i++)
		{
			TTE_ASSERT(VertexState.Attribs[i].VertexBufferLocation < VertexState.NumVertexAttribs &&
					   VertexState.Attribs[i].VertexBufferIndex < VertexState.NumVertexBuffers, "Invalid slot/index");
			Hash = CRC64((const U8*)&VertexState.Attribs[i].Format, 4, Hash);
			Hash = CRC64((const U8*)&VertexState.Attribs[i].VertexBufferIndex, 2, Hash);
			Hash = CRC64((const U8*)&VertexState.Attribs[i].VertexBufferLocation, 2, Hash);
		}
		
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

void RenderCommandBuffer::StartCopyPass()
{
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
	
	RenderPass* pPass = _Context->GetFrame(true)._Heap.New<RenderPass>();
	pPass->_CopyHandle = SDL_BeginGPUCopyPass(_Handle);
	
	_CurrentPass = pPass;
}

void RenderCommandBuffer::StartPass(RenderPass&& pass, RenderTexture& sw)
{
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(pass._Handle == nullptr, "Duplicate render pass!");
	TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
	
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
	
	_CurrentPass = pPass;
}

void RenderCommandBuffer::BindTextures(U32 slot, U32 num, std::shared_ptr<RenderTexture>* pTextures, std::shared_ptr<RenderSampler>* pSamplers)
{
	
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
	TTE_ASSERT(pTextures && pSamplers, "Invalid arguments passed into bind textures");
	TTE_ASSERT(num < 32, "Cannot bind more than 32 texture samplers at one time");
	
	SDL_GPUTextureSamplerBinding binds[32]{};
	for(U32 i = 0; i < num; i++)
	{
		binds[i].sampler = pSamplers[i]->_Handle;
		binds[i].texture = pTextures[i]->_Handle;
	}
	
	SDL_BindGPUFragmentSamplers(_CurrentPass->_Handle, slot, binds, num);
	
}

void RenderCommandBuffer::BindGenericBuffers(U32 slot, U32 num, std::shared_ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot)
{
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
	TTE_ASSERT(pBuffers, "Invalid arguments passed into bind generic buffers");
	TTE_ASSERT(num < 32, "Cannot bind more than 32 generic buffers at one time");
	
	SDL_GPUBuffer* buffers[32];
	for(U32 i = 0; i < num; i++)
		buffers[i] = pBuffers[i]->_Handle;
	
	if(shaderSlot == RenderShaderType::VERTEX)
		SDL_BindGPUVertexStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
	else if(shaderSlot == RenderShaderType::FRAGMENT)
		SDL_BindGPUFragmentStorageBuffers(_CurrentPass->_Handle, slot, buffers, num);
	else
		TTE_ASSERT(false, "Unknown render shader type");
	
}

RenderPass RenderCommandBuffer::EndPass()
{
	TTE_ASSERT(_CurrentPass, "No active pass");
	
	SDL_EndGPURenderPass(_CurrentPass->_Handle);
	RenderPass pass = std::move(*_CurrentPass);
	pass._Handle = nullptr;
	pass._CopyHandle = nullptr;
	_CurrentPass = nullptr;
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

void RenderCommandBuffer::BindPipeline(std::shared_ptr<RenderPipelineState> &state)
{
	TTE_ASSERT(state && state->_Internal._Handle, "Invalid pipeline state");
	if(_Context)
	{
		TTE_ASSERT(_CurrentPass && _CurrentPass->_Handle, "No active non copy pass");
		_Context->AssertRenderThread();
		SDL_BindGPUGraphicsPipeline(_CurrentPass->_Handle, state->_Internal._Handle);
	}
}

// ============================ BUFFERS

std::shared_ptr<RenderBuffer> RenderContext::CreateVertexBuffer(U64 sizeBytes)
{
	std::shared_ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
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

std::shared_ptr<RenderBuffer> RenderContext::CreateIndexBuffer(U64 sizeBytes)
{
	std::shared_ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
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

void RenderCommandBuffer::BindVertexBuffers(std::shared_ptr<RenderBuffer> *buffers, U32 *offsets, U32 first, U32 num)
{
	TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
	TTE_ASSERT(buffers && first + num <= 4, "Invalid arguments passed into BindVertexBuffers");
	
	SDL_GPUBufferBinding binds[4]{};
	for(U32 i = 0; i < num; i++)
	{
		TTE_ASSERT(buffers[i] && buffers[i]->_Handle, "Invalid buffer passed in for binding slot %d: is null", first+i);
		binds[i].offset = offsets ? offsets[i] : 0;
		binds[i].buffer = buffers[i]->_Handle;
	}
	
	SDL_BindGPUVertexBuffers(_CurrentPass->_Handle, first, binds, num);
	
}

void RenderCommandBuffer::BindIndexBuffer(std::shared_ptr<RenderBuffer> indexBuffer, U32 off, Bool isHalf)
{
	TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
	TTE_ASSERT(indexBuffer && indexBuffer->_Handle, "Invalid arguments passed into BindIndexBuffer");
	
	SDL_GPUBufferBinding bind{};
	bind.offset = off;
	bind.buffer = indexBuffer->_Handle;
	
	SDL_BindGPUIndexBuffer(_CurrentPass->_Handle, &bind, isHalf ? SDL_GPU_INDEXELEMENTSIZE_16BIT : SDL_GPU_INDEXELEMENTSIZE_32BIT);
	
}

void RenderCommandBuffer::UploadBufferData(std::shared_ptr<RenderBuffer>& buffer, DataStreamRef srcStream, U64 src, U32 destOffset, U32 numBytes)
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
	cmds._AcquiredTransferBuffers.push_back(buffer);
	
	return buffer;
}

RenderBuffer::~RenderBuffer()
{
	if(_Handle && _Context)
		SDL_ReleaseGPUBuffer(_Context->_Device, _Handle);
	_Handle = nullptr;
}
