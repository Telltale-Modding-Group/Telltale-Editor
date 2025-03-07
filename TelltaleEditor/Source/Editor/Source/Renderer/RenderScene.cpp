#include <Renderer/RenderContext.hpp>

#include <Common/Scene.hpp>

// Called whenever a message for the scene needs to be processed.
void Scene::AsyncProcessRenderMessage(RenderContext& context, SceneMessage message, const SceneAgent* pAgent)
{
	
}

// Called when the scene is being started. here just push one view camera
void Scene::OnAsyncRenderAttach(RenderContext& context)
{
	Camera cam{};
	cam.SetHFOV(45);
	cam.SetNearClip(0.1f);
	cam.SetFarClip(1000.f);
	SDL_GetWindowSize(context._Window, (int*)&cam._ScreenWidth, (int*)&cam._ScreenHeight);
	cam.SetAspectRatio();
	
	Vector3 cameraPosition = Vector3(0.0f, 0.0f, -90.0f); // Camera position in world space
	
	cam.SetWorldPosition(cameraPosition);
	
	_ViewStack.push_back(cam);
}

// Called when the scene is being stopped
void Scene::OnAsyncRenderDetach(RenderContext& context)
{
	
}

static float angle = 0.0f;

// The role of this function is populate the renderer with draw commands for the scene, ie go through renderables and draw them
void Scene::PerformAsyncRender(RenderContext& context, RenderFrame& frame, Float deltaTime)
{
	
	// CAMERA PARAMETERS (ALL OBJECTS SHARE COMMON CAMERA) AND BASE SETUP
	
	ShaderParametersStack* paramStack = context.AllocateParametersStack(frame);
	
	ShaderParameter_Camera* cam = frame.Heap.NewNoDestruct<ShaderParameter_Camera>();
	RenderUtility::SetCameraParameters(context, cam, &_ViewStack.front());
	
	ShaderParametersGroup* camGroup = context.AllocateParameter(frame, ShaderParameterType::PARAMETER_CAMERA);
	context.SetParameterUniform(frame, camGroup, ShaderParameterType::PARAMETER_CAMERA, cam, sizeof(ShaderParameter_Camera));
	
	context.PushParameterGroup(frame, paramStack, camGroup);
	
	// test rotating sphere
	
	Colour sphereColour {255.f/255.f, 218.0f/255.f, 115.f/255.f, 1.0f};
	
	// rotating
	angle += deltaTime * 2 * PI_F * 0.1f; // speed 0.1rads-1
	
	Quaternion rotAxis{Vector3::Normalize({1.0f, 1.0f, 0.0f}), angle};
	
	for(auto& renderable: _Renderables)
	{
		for(auto& meshInstance: renderable.Renderable.MeshList)
		{
			
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
						
						ShaderParametersStack* objStack = context.AllocateParametersStack(frame);
						
						ShaderParameter_Object* obj = frame.Heap.NewNoDestruct<ShaderParameter_Object>();
						RenderUtility::SetObjectParameters(context, obj, MatrixRotation(rotAxis), Colour::DarkCyan);
						
						ShaderParameterTypes required{};
						required.Set(ShaderParameterType::PARAMETER_OBJECT, true);
						required.Set(ShaderParameterType::PARAMETER_INDEX0IN, true);
						required.Set(ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, true);
						for(U32 buf = 0; buf < meshInstance.VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
						{
							required.Set((ShaderParameterType)(ShaderParameterType::PARAMETER_VERTEX0IN + buf), true);
						}
						
						ShaderParametersGroup* objGroup = context.AllocateParameters(frame, required);
						
						context.SetParameterDefaultTexture(frame, objGroup, ShaderParameterType::PARAMETER_SAMPLER_DIFFUSE, DefaultRenderTextureType::WHITE, 0);
						
						context.SetParameterUniform(frame, objGroup, ShaderParameterType::PARAMETER_OBJECT, obj, sizeof(ShaderParameter_Object));
						
						for(U32 buf = 0; buf < meshInstance.VertexStates[lod.VertexStateIndex].Default.NumVertexBuffers; buf++)
						{
							context.SetParameterVertexBuffer(frame, objGroup,
															 (ShaderParameterType)(ShaderParameterType::PARAMETER_VERTEX0IN + buf),
															 meshInstance.VertexStates[lod.VertexStateIndex].RuntimeData.GPUVertexBuffers[buf], 0);
						}

						context.SetParameterIndexBuffer(frame, objGroup, ShaderParameterType::PARAMETER_INDEX0IN,
														 meshInstance.VertexStates[lod.VertexStateIndex].RuntimeData.GPUIndexBuffer, 0);
						
						context.PushParameterGroup(frame, objStack, objGroup);
						// add camera parameters to obj parameters
						context.PushParameterStack(frame, objStack, paramStack);
						
						RenderInst inst{};
						inst.SetVertexState(meshInstance.VertexStates[lod.VertexStateIndex].Default);
						inst.SetShaderProgram("Mesh");
						
						inst.DrawPrimitives(RenderPrimitiveType::TRIANGLE_LIST, batch.StartIndex, batch.NumPrimitives, 1, batch.BaseIndex);
						context.PushRenderInst(frame, objStack, std::move(inst));
					}
				}
			}
		}
	}
	
}
