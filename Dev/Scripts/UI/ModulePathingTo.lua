function ModulePathingTo_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Walk Radius"] = {Class = "float", Key = kPathToWalkRadius}
    dataTable["Walk Radius"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}

    RegisterModuleUI("pathTo", "Module/PathTo.png", dataTable)

end