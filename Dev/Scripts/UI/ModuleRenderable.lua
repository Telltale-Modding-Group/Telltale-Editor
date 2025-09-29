function ModuleRenderable_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Mesh"] = {Class = "class Handle<class D3DMesh>", Key = "D3D Mesh" }
    dataTable["Mesh"]["UI"] = {InputType = "handle:d3dmesh", SubPath = "this.mHandle"}
    dataTable["Axis Scale"] = {Class = "class Vector3", Key = "Render Axis Scale", UI = {InputType = kPropRenderVector3, SubPath = "this"}}
    dataTable["Global Scale"] = {Class = "float", Key = "Render Global Scale", UI = {InputType = kPropRenderFloat, SubPath = "this"}}

    RegisterModuleUI("renderable", "Module/Renderable.png", dataTable)

end