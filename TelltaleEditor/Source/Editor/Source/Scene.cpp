#include <Scene.hpp>
#include <Renderer/RenderContext.hpp>

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
	
	Vector3 cameraPosition = Vector3(0.0f, 0.0f, -20.0f); // Camera position in world space
	
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
	
	// here we setup the parameters (camera and object) for rendering a default simple model with just vertices.
	
	// SETUP CAMERA UNIFORM
	ShaderParameter_Camera& cam = *frame._Heap.NewNoDestruct<ShaderParameter_Camera>();
	Camera& ac = _ViewStack.front();
	cam.ViewProj = (ac.GetProjectionMatrix() * ac.GetViewMatrix()).Transpose();
	cam.TransposeViewZ = ac.GetViewMatrix().GetRow(2);
	cam.HFOVParam = ac._HFOV * ac._HFOVScale;
	cam.VFOVParam = ac._HFOV * ac._HFOVScale;
	cam.Aspect = ac.GetAspectRatio();
	cam.CameraNear = ac._NearClip;
	cam.CameraFar = ac._FarClip;
	
	// SETUP TEST MODEL UNIFORM
	angle += deltaTime * 2 * M_PI * /*freq*/ 0.1f;
	ShaderParameter_Object& obj = *frame._Heap.NewNoDestruct<ShaderParameter_Object>();
	Vector3 axis = Vector3(1.0f,1.0f,0.0f);
	axis.Normalize();
	Matrix4 model = MatrixTransformation(Vector3(1.f,1.f,1.f), Quaternion(axis, angle), Vector3::Zero);
	obj.WorldMatrix[0] = model.GetRow(0);
	obj.WorldMatrix[1] = model.GetRow(1);
	obj.WorldMatrix[2] = model.GetRow(2);
	
	// SET PARAMETERS REQUIRED. (we are drawing a default mesh)
	ShaderParameterTypes types{};
	types.Set(PARAMETER_CAMERA, true);
	types.Set(PARAMETER_OBJECT, true);
	
	// Parameter stack abstracts lots of groups, binding top down until all required from the shaders have been bound.
	ShaderParametersStack* paramStack = context.AllocateParametersStack(frame);
	ShaderParametersGroup* group = context.AllocateParameters(frame, types);
	
	// Link the uniforms to be uploaded. Note the uniform data when taking the address is on the frame heap, NOT this stack frame.
	context.SetParameterUniform(frame, group, PARAMETER_CAMERA, &cam, sizeof(ShaderParameter_Camera));
	context.SetParameterUniform(frame, group, PARAMETER_OBJECT, &obj, sizeof(ShaderParameter_Object));
	
	// Push the group to the stack
	context.PushParameterGroup(frame, paramStack, group); // push the group to this param stack
	
	// Queue the draw command
	RenderInst draw {};
	draw.DrawDefaultMesh(DefaultRenderMeshType::WIREFRAME_CAPSULE);
	context.PushRenderInst(frame, paramStack, std::move(draw)); // push draw command
	
}
