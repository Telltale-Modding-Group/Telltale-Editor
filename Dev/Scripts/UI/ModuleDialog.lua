function ModuleDialog_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Dialog Name"] = {Class = "class String", Key = kDialogName}
    dataTable["Dialog Name"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}

    if moduleVersion ~= 0 then

        dataTable["Dialog Resource"] = {Class = "class Handle<class DialogResource>", Key = kDialogResource}
        dataTable["Dialog Resource"]["UI"] = {InputType = "handle:dlg", SubPath = "this.mHandle"}

    end

    RegisterModuleUI("dialog", "Module/Dialog.png", dataTable)

end