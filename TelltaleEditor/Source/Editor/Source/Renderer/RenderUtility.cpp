#include <Renderer/RenderContext.hpp>

namespace RenderUtility
{
    
    void SetObjectParameters(RenderContext& context, ShaderParameter_Object* obj, Matrix4 objectWorldMatrix, Colour c)
    {
        FinalisePlatformMatrix(context, obj->WorldMatrix, objectWorldMatrix);
        obj->Diffuse = c;
        obj->Alpha = c.a;
    }
    
    void SetCameraParameters(RenderContext& context, ShaderParameter_Camera* cam, Camera* ac)
    {
        Matrix4 vp = (ac->GetProjectionMatrix() * ac->GetViewMatrix());
        if(!context.IsLeftHanded())
        {
            vp.SetRow(2, -vp.GetRow(2));
        }
        FinalisePlatformMatrix(context, cam->ViewProj, vp); // swap rows/cols if needed
        cam->HFOVParam = ac->_HFOV * ac->_HFOVScale;
        cam->VFOVParam = 2.0f * atanf(ac->GetAspectRatio() * tanf(ac->_HFOV * ac->_HFOVScale * 0.5f));
        cam->Aspect = ac->GetAspectRatio();
        cam->CameraNear = ac->_NearClip;
        cam->CameraFar = ac->_FarClip;
    }
    
    void _DrawInternal(RenderContext& context, Camera* ac, Matrix4 model, Colour col, DefaultRenderMeshType primitive, RenderViewPass* pass, RenderStateBlob* pRenderState)
    {
        TTE_ASSERT(pass, "Pass is not specified!");
        RenderFrame& frame = context.GetFrame(true);
        
        // SETUP CAMERA UNIFORM
        ShaderParameter_Camera* cam = nullptr;
        if(ac)
        {
            cam = frame.Heap.NewNoDestruct<ShaderParameter_Camera>();
            SetCameraParameters(context, cam, ac);
        }
        
        // SETUP MODEL UNIFORM
        ShaderParameter_Object& obj = *frame.Heap.NewNoDestruct<ShaderParameter_Object>();
        FinalisePlatformMatrix(context, obj.WorldMatrix, model);
        
        obj.Alpha = col.a;
        obj.Diffuse = Vector3(col.r, col.g, col.b);
        
        // SET PARAMETERS REQUIRED. (we are drawing a default mesh)
        ShaderParameterTypes types{};
        if(ac)
            types.Set(ShaderParameterType::PARAMETER_CAMERA, true);
        types.Set(ShaderParameterType::PARAMETER_OBJECT, true);
        
        // Parameter stack abstracts lots of groups, binding top down until all required from the shaders have been bound.
        ShaderParametersGroup* group = context.AllocateParameters(frame, types);
        
        // Link the uniforms to be uploaded. Note the uniform data when taking the address is on the frame heap, NOT this stack frame.
        if(ac)
            context.SetParameterUniform(frame, group, ShaderParameterType::PARAMETER_CAMERA, cam, sizeof(ShaderParameter_Camera));
        context.SetParameterUniform(frame, group, ShaderParameterType::PARAMETER_OBJECT, &obj, sizeof(ShaderParameter_Object));
        
        // Queue the draw command
        RenderInst draw {};
        draw.DrawDefaultMesh(primitive);
        if (pRenderState)
            draw.GetRenderState() = *pRenderState;
        pass->PushRenderInst(context, std::move(draw), group);
    }
    
}
