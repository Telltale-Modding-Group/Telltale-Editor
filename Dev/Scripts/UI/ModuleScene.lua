function ModuleScene_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Walk Boxes"] = {Class = "class Handle<class WalkBoxes>", Key = "Walk Boxes"}
    dataTable["Walk Boxes"]["UI"] = {InputType = "handle:wbox", SubPath = "this.mHandle"}
    dataTable["Ambient Colour"] = {Class = "class Color", Key = "Ambient Color"}
    dataTable["Ambient Colour"]["UI"] = {InputType = "Colour", SubPath = "this"}

    RegisterModuleUI("scene", "Module/Scene.png", dataTable)

end