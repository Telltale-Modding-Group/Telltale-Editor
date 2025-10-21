function ModuleDialogChoice_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Choice"] = {Class = "class String", Key = kDialogChoiceChoice}
    dataTable["Choice"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}

    RegisterModuleUI("dlgChoice", "Module/DialogChoice.png", dataTable)

end