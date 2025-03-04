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
	
	Vector3 cameraPosition = Vector3(0.0f, 0.0f, -10.0f); // Camera position in world space
	
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
	
	// CAMERA PARAMETERS AND BASE SETUP
	
	ShaderParametersStack* paramStack = context.AllocateParametersStack(frame);
	
	ShaderParameter_Camera* cam = frame._Heap.NewNoDestruct<ShaderParameter_Camera>();
	RenderUtility::SetCameraParameters(context, cam, &_ViewStack.front());
	
	ShaderParametersGroup* camGroup = context.AllocateParameter(frame, ShaderParameterType::PARAMETER_CAMERA);
	context.SetParameterUniform(frame, camGroup, ShaderParameterType::PARAMETER_CAMERA, cam, sizeof(ShaderParameter_Camera));
	
	context.PushParameterGroup(frame, paramStack, camGroup);
	
	// test rotating sphere
	
	Colour sphereColour {255.f/255.f, 218.0f/255.f, 115.f/255.f, 1.0f};
	
	// rotating
	angle += deltaTime * 2 * M_PI * 0.1f; // speed 0.1rads-1
	sphereColour.r = 0.5f * (sinf(angle) + 1.0f);
	
	Quaternion rotAxis{Vector3::Normalize({1.0f, 1.0f, 0.0f}), angle};
	
	for(auto& renderable: _Renderables)
	{
		for(auto& meshInstance: renderable.Renderable.MeshList)
		{
			// Draw bounding box
			Matrix4 m = RenderUtility::CreateBoundingBoxModelMatrix(meshInstance.BBox);
			m = RenderUtility::CreateSphereModelMatrix(meshInstance.BSphere) * MatrixRotation(rotAxis);
			RenderUtility::DrawWireBox(context, nullptr, m, sphereColour, paramStack);
			
			for(auto& vstate: meshInstance.VertexStates)
			{
				if(vstate.IndexBuffer.BufferData && !vstate.RuntimeData.GPUIndexBuffer)
				{
					// TODO FRAME UPDATE LIST.
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
						
					}
				}
			}
		}
	}
	
}
