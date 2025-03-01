#include <Renderer/RenderContext.hpp>
#include <Core/Context.hpp>

#include <Core/Thread.hpp>

#include <chrono>
#include <cfloat>
#include <sstream>
#include <iostream>

#define INTERNAL_START_PRIORITY 0x0F0C70FF

// ============================ SURFACE FORMAT ENUM MAPPINGS

static struct TextureFormatInfo {
	RenderSurfaceFormat Format;
	SDL_GPUTextureFormat SDLFormat;
}
SDL_FormatMappings[2]
{
	{RenderSurfaceFormat::RGBA8, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT},
	{RenderSurfaceFormat::UNKNOWN, SDL_GPU_TEXTUREFORMAT_INVALID}, // do not add below this, add above
};

inline TextureFormatInfo GetSDLFormatInfo(RenderSurfaceFormat format)
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

static struct AttributeFormatInfo {
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
}
SDL_PrimitiveMappings[3]
{
	{RenderPrimitiveType::TRIANGLE_LIST, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST},
	{RenderPrimitiveType::LINE_LIST, SDL_GPU_PRIMITIVETYPE_LINELIST},
	{RenderPrimitiveType::UNKNOWN, SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP}, // do not add below this, add above
};

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

RenderContext::RenderContext(String wName)
{
	TTE_ASSERT(JobScheduler::Instance, "Job scheduler has not been initialised. Ensure a ToolContext exists.");
	TTE_ASSERT(SDL3_Initialised, "SDL3 has not been initialised, or failed to.");
	
	_MainFrameIndex = 0;
	_PopulateJob = JobHandle();
	_Frame[0].Reset(0);
	_Frame[1].Reset(1);
	
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
	
	for(auto& sampler: _Samplers)
		SDL_ReleaseGPUSampler(_Device, sampler->_Handle);
	
	_DefaultMeshes.clear();
	_DefaultTextures.clear();
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

// USER CALL: called every frame by user to render the previous frame
Bool RenderContext::FrameUpdate(Bool isLastFrame)
{
	
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
		if((myTime - _StartTimeMicros) == 0)
		{
			// hardly doing anything (<1 micro) so wait for 1ms, which makes the fastest frame rate possible 1000 FPS!
			ThreadSleep(1);
			myTime = _StartTimeMicros + 1;
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
			pFrame->_BackBuffer = pFrame->_Heap.New<RenderTexture>();
			pFrame->_BackBuffer->_Context = this;
			pFrame->_BackBuffer->Format = FromSDLFormat(SDL_GetGPUSwapchainTextureFormat(_Device, _Window));
			TTE_ASSERT(SDL_WaitAndAcquireGPUSwapchainTexture(pMainCommandBuffer->_Handle,
												  _Window, &pFrame->_BackBuffer->_Handle,
															 &pFrame->_BackBuffer->Width,  &pFrame->_BackBuffer->Height),
					   "Failed to acquire backend swapchain texture at frame %lld: %s", pFrame->_FrameNumber, SDL_GetError());
			
			if(pFrame->_FrameNumber == 0)
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
			SDL_WaitForGPUFences(_Device, true, awaitingFences, activeCommandBuffers);
		for(auto& commandBuffer: pFrame->_CommandBuffers) // free the fences
		{
			if(commandBuffer->_SubmittedFence != nullptr)
			{
				SDL_ReleaseGPUFence(_Device, commandBuffer->_SubmittedFence);
				commandBuffer->_SubmittedFence = nullptr;
			}
		}
		
		// RECLAIM TRANSFER BUFFERS
		for(auto& cmd: pFrame->_CommandBuffers)
			_ReclaimTransferBuffers(*cmd);
		
		// CLEAR COMMAND BUFFERS.
		pFrame->_CommandBuffers.clear();
	}
	
	// 5. Wait for previous populater to complete if needed
	if(_PopulateJob)
	{
		JobScheduler::Instance->Wait(_PopulateJob);
		_PopulateJob.Reset(); // reset handle to empty
	}
	
	// 6. NO POPULATE THREAD JOB ACTIVE, SO NO LOCKING NEEDED UNTIL SUBMIT. Give our new processed frame, and swap.

	_MainFrameIndex ^= 1; // swap
	GetFrame(true).Reset(GetFrame(true)._FrameNumber + 2); // increase populater frame index (+2 from swap, dont worry)
	
	// free resources which are not needed
	if(_Flags & RENDER_CONTEXT_NEEDS_PURGE)
	{
		_Flags &= ~RENDER_CONTEXT_NEEDS_PURGE;
		PurgeResources();
	}
	_FreePendingDeletions(GetFrame(false)._FrameNumber);
	
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

// ============================ FRAME MANAGEMENT

void RenderFrame::Reset(U64 newFrameNumber)
{
	_CommandBuffers.clear();
	_Autorelease.clear(); // will free ptrs if needed
	_FrameNumber = newFrameNumber;
	NumDrawCalls = 0;
	_DrawCalls = nullptr;
	_Heap.Rollback(); // free all objects, but keep memory
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
	info.max_anisotropy = 1.0f;
	info.mip_lod_bias = desc.MipBias;
	info.mipmap_mode = (SDL_GPUSamplerMipmapMode)desc.MipMode;
	info.min_lod = 0.f;
	info.max_lod = 1000.f;
	desc._Handle = SDL_CreateGPUSampler(_Device, &info);
	std::shared_ptr<RenderSampler> pSampler = TTE_NEW_PTR(RenderSampler, MEMORY_TAG_RENDERER);
	*pSampler = desc;
	_Samplers.push_back(pSampler);
	return pSampler;
}

RenderTexture::~RenderTexture()
{
	if(_Handle && _Context)
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
std::shared_ptr<RenderShader> RenderContext::_FindShader(String name, RenderShaderType type)
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
	
	std::shared_ptr<RenderShader> sh = TTE_NEW_PTR(RenderShader, MEMORY_TAG_RENDERER);
	
	U32 BindsOf[3]{};
	
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
		const U8* map = inf + 8 + (2*binds);
		info.code += 2*binds;
		info.code_size -= 2*binds;
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
		if (std::getline(stream, firstLine)) {
			std::istringstream lineStream(firstLine);
			std::string token;
			if (!firstLine.empty() && firstLine[0] == '/' &&  firstLine[1] == '/') {
				while (lineStream >> token) {
					size_t colonPos = token.find(':');
					if (colonPos != std::string::npos) {
						std::string key = token.substr(0, colonPos);
						U32 value = std::stoi(token.substr(colonPos + 1));
						
						if (key == "Samplers") info.num_samplers = value;
						else if (key == "StorageTextures") info.num_storage_textures = value;
						else if (key == "StorageBuffers") info.num_storage_buffers = value;
						else if (key == "UniformBuffers") info.num_uniform_buffers = value;
						else
						{
							Bool Set = false;
							for(U32 i = 0; i < PARAMETER_COUNT; i++)
							{
								if(key == ShaderParametersInfo[i].Name)
								{
									TTE_ASSERT(sh->ParameterSlots[i] == 0xFF, "Invalid shader binding slot: already set");
									sh->ParameterSlots[i] = (U8)value;
									
									ShaderParameterTypeClass cls = ShaderParametersInfo[i].Class;
									if((cls != ShaderParameterTypeClass::UNIFORM_BUFFER && cls != ShaderParameterTypeClass::GENERIC_BUFFER &&
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
	
	sh->Name = name;
	sh->Handle = SDL_CreateGPUShader(_Device, &info);
	sh->Context = this;
	
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

std::shared_ptr<RenderPipelineState> RenderContext::_FindPipelineState(RenderPipelineState desc)
{
	U64 hash = _HashPipelineState(desc);
	{
		std::lock_guard<std::mutex> G{_Lock};
		for(auto& state: _Pipelines)
			if(state->Hash == hash)
				return state;
	}
	std::shared_ptr<RenderPipelineState> state = _AllocatePipelineState();
	*state = std::move(desc);
	state->Create();
	return state;
}

U64 RenderContext::_HashPipelineState(RenderPipelineState &state)
{
	U64 Hash = 0;
	
	Hash = CRC64LowerCase((const U8*)state.VertexSh.c_str(), (U32)state.VertexSh.length(), 0);
	Hash = CRC64LowerCase((const U8*)state.FragmentSh.c_str(), (U32)state.FragmentSh.length(), Hash);
	Hash = CRC64((const U8*)&state.VertexState.NumVertexBuffers, 4, Hash);
	Hash = CRC64((const U8*)&state.PrimitiveType, 4, Hash);
	
	for(U32 i = 0; i < state.VertexState.NumVertexAttribs; i++)
	{
		TTE_ASSERT(state.VertexState.Attribs[i].VertexBufferLocation < state.VertexState.NumVertexAttribs &&
				   state.VertexState.Attribs[i].VertexBufferIndex < state.VertexState.NumVertexBuffers, "Invalid slot/index");
		Hash = CRC64((const U8*)&state.VertexState.Attribs[i].Format, 4, state.VertexState.IsHalf ? ~Hash : Hash);
		Hash = CRC64((const U8*)&state.VertexState.Attribs[i].VertexBufferIndex, 2, Hash);
		Hash = CRC64((const U8*)&state.VertexState.Attribs[i].VertexBufferLocation, 2, Hash);
	}
	
	return Hash;
}

void RenderPipelineState::Create()
{
	if(_Internal._Handle == nullptr)
	{
		_Internal._Context->AssertMainThread();
		TTE_ASSERT(VertexState.NumVertexBuffers < 4 && VertexState.NumVertexAttribs < 32, "Vertex state has too many attributes / buffers");
		
		// calculate hash
		Hash = _Internal._Context->_HashPipelineState(*this);
		
		// create
		SDL_GPUGraphicsPipelineCreateInfo info{};
		
		// TARGETS: for now only use the swapchain render target
		info.target_info.num_color_targets = 1;
		SDL_GPUColorTargetDescription sc{};
		sc.format = SDL_GetGPUSwapchainTextureFormat(_Internal._Context->_Device, _Internal._Context->_Window);
		info.target_info.color_target_descriptions = &sc;
		
		info.vertex_shader = _Internal._Context->_FindShader(VertexSh, RenderShaderType::VERTEX)->Handle;
		info.fragment_shader = _Internal._Context->_FindShader(FragmentSh, RenderShaderType::FRAGMENT)->Handle;
		
		info.primitive_type = ToSDLPrimitiveType(PrimitiveType);
		
		info.vertex_input_state.num_vertex_buffers = VertexState.NumVertexBuffers;
		info.vertex_input_state.num_vertex_attributes = VertexState.NumVertexAttribs;
		
		SDL_GPUVertexBufferDescription desc[4]{};
		for(U32 i = 0; i < VertexState.NumVertexBuffers; i++)
		{
			desc[i].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
			desc[i].instance_step_rate = 0;
			desc[i].slot = i;
			desc[i].pitch = VertexState.BufferPitches[i];
		}
		SDL_GPUVertexAttribute attrib[32]{};
		U32 off[4] = {0,0,0,0};
		for(U32 i = 0; i < VertexState.NumVertexAttribs; i++)
		{
			attrib[i].buffer_slot = VertexState.Attribs[i].VertexBufferIndex;
			attrib[i].location = VertexState.Attribs[i].VertexBufferLocation;
			attrib[i].offset = off[VertexState.Attribs[i].VertexBufferIndex];
			AttributeFormatInfo inf = GetSDLAttribFormatInfo(VertexState.Attribs[i].Format);
			attrib[i].format = inf.SDLFormat;
			off[VertexState.Attribs[i].VertexBufferIndex] += inf.NumIntrinsics * inf.IntrinsicSize;
		}
		
		info.vertex_input_state.vertex_attributes = attrib;
		info.vertex_input_state.vertex_buffer_descriptions = desc;
		
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
	RenderCommandBuffer* pBuffer = frame._Heap.New<RenderCommandBuffer>();
	
	pBuffer->_Handle = SDL_AcquireGPUCommandBuffer(_Device);
	pBuffer->_Context = this;
	
	frame._CommandBuffers.push_back(pBuffer);
	
	return pBuffer;
}

void RenderCommandBuffer::StartCopyPass()
{
	_Context->AssertMainThread();
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass == nullptr, "Already within a pass. End the current pass before starting a new one.");
	
	RenderPass* pPass = _Context->GetFrame(false)._Heap.New<RenderPass>();
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
	swChain.texture = _Context->GetFrame(false)._BackBuffer->_Handle;
	swChain.load_op = SDL_GPU_LOADOP_CLEAR;
	swChain.store_op = SDL_GPU_STOREOP_STORE;
	swChain.clear_color.r = pass.ClearCol.r;
	swChain.clear_color.g = pass.ClearCol.g;
	swChain.clear_color.b = pass.ClearCol.b;
	swChain.clear_color.a = pass.ClearCol.a;

	RenderPass* pPass = _Context->GetFrame(false)._Heap.New<RenderPass>();
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
	
	std::shared_ptr<RenderShader> vsh = _Context->_FindShader(_BoundPipeline->VertexSh, RenderShaderType::VERTEX);
	std::shared_ptr<RenderShader> fsh = _Context->_FindShader(_BoundPipeline->FragmentSh, RenderShaderType::FRAGMENT);
	
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
				std::shared_ptr<RenderBuffer> proxy{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
				TTE_ASSERT(proxy.get(), "Required index buffer was not bound or was null");
				BindIndexBuffer(proxy, group->GetParameter(i).GenericValue.Offset, _BoundPipeline->VertexState.IsHalf);
				continue;
			}
			
			if(paramInfo.Type >= (U8)PARAMETER_FIRST_VERTEX_BUFFER && // bind vertex buffer
			   paramInfo.Type <= (U8)PARAMETER_LAST_VERTEX_BUFFER && !BoundVertex[paramInfo.Type - PARAMETER_FIRST_VERTEX_BUFFER])
			{
				// bind this vertex buffer
				BoundVertex[paramInfo.Type - PARAMETER_FIRST_VERTEX_BUFFER] = true;
				std::shared_ptr<RenderBuffer> proxy{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
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
						std::shared_ptr<RenderTexture> tex{group->GetParameter(i).SamplerValue.Texture, &NullDeleter};
						std::shared_ptr<RenderSampler> sam{group->GetParameter(i).SamplerValue.Sampler, &NullDeleter};
						TTE_ASSERT(tex.get(), "Required texture was not bound or was null");
						TTE_ASSERT(sam.get(), "Required sampler was not bound or was null");
						BindTextures(vsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::VERTEX, &tex, &sam);
					}
					if(fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
					{
						std::shared_ptr<RenderTexture> tex{group->GetParameter(i).SamplerValue.Texture, &NullDeleter};
						std::shared_ptr<RenderSampler> sam{group->GetParameter(i).SamplerValue.Sampler, &NullDeleter};
						TTE_ASSERT(tex.get(), "Required texture was not bound or was null");
						TTE_ASSERT(sam.get(), "Required sampler was not bound or was null");
						BindTextures(fsh->ParameterSlots[paramInfo.Type], 1, RenderShaderType::FRAGMENT, &tex, &sam);
					}
				}
				else if(paramInfo.Class == (U8)ShaderParameterTypeClass::GENERIC_BUFFER)
				{
					if(vsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
					{
						std::shared_ptr<RenderBuffer> buf{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
						TTE_ASSERT(buf.get(), "Required generic buffer was not bound or was null");
						BindGenericBuffers(vsh->ParameterSlots[paramInfo.Type], 1, &buf, RenderShaderType::VERTEX);
					}
					if(fsh->ParameterSlots[paramInfo.Type] != (U8)0xFF)
					{
						std::shared_ptr<RenderBuffer> buf{group->GetParameter(i).GenericValue.Buffer, &NullDeleter};
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
	
	std::shared_ptr<RenderSampler> defSampler = _Context->_FindSampler({});
	
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
				TTE_ASSERT("Parameter buffer %s was required for one of %s/%s but was not supplied.",
						   ShaderParametersInfo[i].Name, _BoundPipeline->VertexSh.c_str(),_BoundPipeline->FragmentSh.c_str());
			}
		}
	}
	
}

void RenderCommandBuffer::BindTextures(U32 slot, U32 num, RenderShaderType shader,
									   std::shared_ptr<RenderTexture>* pTextures, std::shared_ptr<RenderSampler>* pSamplers)
{
	_Context->AssertMainThread();
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass != nullptr, "No active pass");
	TTE_ASSERT(pTextures && pSamplers, "Invalid arguments passed into bind textures");
	TTE_ASSERT(num < 32, "Cannot bind more than 32 texture samplers at one time");
	
	SDL_GPUTextureSamplerBinding binds[32]{};
	for(U32 i = 0; i < num; i++)
	{
		binds[i].sampler = pSamplers[i]->_Handle;
		binds[i].texture = pTextures[i]->_Handle;
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
											 std::shared_ptr<RenderSampler> sampler, DefaultRenderTextureType type)
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

void RenderCommandBuffer::BindGenericBuffers(U32 slot, U32 num, std::shared_ptr<RenderBuffer>* pBuffers, RenderShaderType shaderSlot)
{
	_Context->AssertMainThread();
	TTE_ASSERT(_Handle != nullptr, "Render command buffer was not initialised properly");
	TTE_ASSERT(_CurrentPass != nullptr, "Already within a pass. End the current pass before starting a new one.");
	TTE_ASSERT(pBuffers, "Invalid arguments passed into bind generic buffers");
	TTE_ASSERT(num < 4, "Cannot bind more than 4 generic buffers at one time");
	
	SDL_GPUBuffer* buffers[4];
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

void RenderCommandBuffer::BindPipeline(std::shared_ptr<RenderPipelineState> &state)
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

std::shared_ptr<RenderBuffer> RenderContext::CreateGenericBuffer(U64 sizeBytes)
{
	std::shared_ptr<RenderBuffer> buffer = TTE_NEW_PTR(RenderBuffer, MEMORY_TAG_RENDERER);
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

void RenderCommandBuffer::BindIndexBuffer(std::shared_ptr<RenderBuffer> indexBuffer, U32 firstIndex, Bool isHalf)
{
	TTE_ASSERT(_Context && _Handle && _CurrentPass && _CurrentPass->_Handle, "Command buffer is not in a state to have bindings yet");
	TTE_ASSERT(indexBuffer && indexBuffer->_Handle, "Invalid arguments passed into BindIndexBuffer");
	
	SDL_GPUBufferBinding bind{};
	bind.offset = firstIndex * (isHalf ? 2 : 4);
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
		ShaderParametersStack* stack = frame._Heap.NewNoDestruct<ShaderParametersStack>();
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
	ShaderParametersGroup* pGroup = (ShaderParametersGroup*)frame._Heap.Alloc(allocSize);
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

void RenderContext::SetParameterTexture(RenderFrame& frame, ShaderParametersGroup* group, ShaderParameterType type,
						 std::shared_ptr<RenderTexture> tex, std::shared_ptr<RenderSampler> sampler)
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
	frame.PushAutorelease(sampler);
	group->GetParameter(ind).SamplerValue.Texture = tex.get();
	group->GetParameter(ind).SamplerValue.Sampler = sampler.get();
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
							   std::shared_ptr<RenderBuffer> buffer, U32 bufferOffset)
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
							  std::shared_ptr<RenderBuffer> buffer, U32 off)
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
							 std::shared_ptr<RenderBuffer> buffer, U32 startIndex)
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
	group->GetParameter(ind).GenericValue.Offset = startIndex;
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
			std::shared_ptr<RenderSampler> s = std::move(sampler);
			_PendingSDLResourceDeletions.push_back(PendingDeletion{std::move(s), GetFrame(false)._FrameNumber});
		}
		_Samplers.clear();
	}
	else
		_Flags |= RENDER_CONTEXT_NEEDS_PURGE;
}

ShaderParametersStack* RenderContext::AllocateParametersStack(RenderFrame& frame)
{
	return frame._Heap.NewNoDestruct<ShaderParametersStack>(); // on heap no destruct needed its very lightweight
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
						String sym = RuntimeSymbols.Find(msg.Agent);
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
		TTE_ASSERT(frame._Heap.GetPageIndexForAlloc(params) != -1, "Shader parameter stack must be allocated from render context!");

	RenderInst* instance = frame._Heap.New<RenderInst>();
	*instance = std::move(inst);
	instance->_Parameters = params;
	
	frame.NumDrawCalls++;
	instance->_Next = frame._DrawCalls;
	frame._DrawCalls = instance;
}


void RenderContext::_Render(Float dt, RenderCommandBuffer* pMainCommandBuffer)
{
	RenderFrame& frame = GetFrame(false);
	
	// LOW LEVEL RENDER (MAIN THREAD) THE PREVIOUS FRAME.
	
	// 1. temp alloc and resort draw calls to sorted order.
	
	RenderInst** sortedRenderInsts = (RenderInst**)frame._Heap.Alloc(sizeof(RenderInst*) * frame.NumDrawCalls);
	RenderInst* inst = frame._DrawCalls;
	for(U32 i = 0; i < frame.NumDrawCalls; i++)
	{
		sortedRenderInsts[i] = inst;
		inst = inst->_Next;
	}
	std::sort(sortedRenderInsts, sortedRenderInsts + frame.NumDrawCalls, RenderInstSorter{}); // sort by sort key
	
	// 2. execute draws
	
	RenderPass passdesc{};
	passdesc.ClearCol = Colour::Black;  // obviously if we want more complex passes we have to code them specifically higher level
	
	pMainCommandBuffer->StartPass(std::move(passdesc));
	
	for(U32 i = 0; i < frame.NumDrawCalls; i++)
		_Draw(frame, *sortedRenderInsts[i], *pMainCommandBuffer);
	
	pMainCommandBuffer->EndPass();
	
}

// renderer: perform draw.
void RenderContext::_Draw(RenderFrame& frame, RenderInst inst, RenderCommandBuffer& cmds)
{
	
	DefaultRenderMesh* pDefaultMesh = nullptr;
	
	std::shared_ptr<RenderPipelineState> state{};
	if(inst._DrawDefault == DefaultRenderMeshType::NONE)
	{
		TTE_ASSERT(inst.Vert.length() && inst.Frag.length(), "Vertex and fragment shaders not set");
		
		// Find pipeline state for draw
		RenderPipelineState pipelineDesc{};
		pipelineDesc.PrimitiveType = inst._PrimitiveType;
		pipelineDesc.VertexState = inst._VertexStateInfo;
		pipelineDesc.VertexSh = inst.Vert;
		pipelineDesc.FragmentSh = inst.Frag;
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
	cmds.DrawIndexed(inst._IndexCount, inst._InstanceCount, inst._StartIndex, 0, 0);
	
}
