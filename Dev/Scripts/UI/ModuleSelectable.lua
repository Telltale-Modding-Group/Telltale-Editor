function ModuleSelectable_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Extents Min"] = {Class = "class Vector3", Key = kSelectableExtentsMin}
    dataTable["Extents Min"]["UI"] = {InputType = "Vector3", SubPath = "this"}
    dataTable["Extents Max"] = {Class = "class Vector3", Key = kSelectableExtentsMax}
    dataTable["Extents Max"]["UI"] = {InputType = "Vector3", SubPath = "this"}
    dataTable["On Off"] = {Class = "bool", Key = kSelectableOnOff}
    dataTable["On Off"]["UI"] = {InputType = "bool", SubPath = "this"}

    RegisterModuleUI("selectable", "Module/Selectable.png", dataTable)

end