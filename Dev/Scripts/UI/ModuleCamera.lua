function ModuleCamera_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Near Clip"] = {Class = "float", Key = "Clip Plane - Near"}
    dataTable["Near Clip"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Far Clip"] = {Class = "float", Key = "Clip Plane - Far"}
    dataTable["Far Clip"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["FOV"] = {Class = "float", Key = "Field of View"}
    dataTable["FOV"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}

    RegisterModuleUI("camera", "Module/Camera.png", dataTable)

end