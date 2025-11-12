function ModuleWalkAnimator_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Forward Animation"] = {Class = "class Handle<class Animation>", Key = kWalkAnimatorForwardAnimation}
    dataTable["Forward Animation"]["UI"] = {InputType = "handle:anm", SubPath = "this.mHandle"}

    dataTable["Idle Animation"] = {Class = "class Handle<class Animation>", Key = kWalkAnimatorIdleAnimation}
    dataTable["Idle Animation"]["UI"] = {InputType = "handle:anm", SubPath = "this.mHandle"}

    RegisterModuleUI("walkAnimator", "Module/WalkAnimator.png", dataTable)

end