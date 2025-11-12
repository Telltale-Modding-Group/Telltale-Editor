function ModuleCamera_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Near Clip"] = {Class = "float", Key = kClipPlaneNear}
    dataTable["Near Clip"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Far Clip"] = {Class = "float", Key = kClipPlaneFar}
    dataTable["Far Clip"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["FOV"] = {Class = "float", Key = kFieldOfView}
    dataTable["FOV"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}

    RegisterModuleUI("camera", "Module/Camera.png", dataTable)

end