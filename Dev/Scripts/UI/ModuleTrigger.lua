function ModuleTrigger_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Enter Callback"] = {Class = "class String", Key = kTriggerEnterCallback}
    dataTable["Enter Callback"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
    dataTable["Exit Callback"] = {Class = "class String", Key = kTriggerExitCallback}
    dataTable["Exit Callback"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
    dataTable["Enabled"] = {Class = "bool", Key = kTriggerEnabled}
    dataTable["Enabled"]["UI"] = {InputType = kPropRenderBool, SubPath = "this"}

    RegisterModuleUI("trigger", "Module/Trigger.png", dataTable)

end