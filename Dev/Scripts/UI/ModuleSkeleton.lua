function ModuleSkeleton_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Skeleton"] = {Class = "class Handle<class Skeleton>", Key = "Skeleton File", Default = ""}
    dataTable["Skeleton"]["UI"] = {InputType = "handle:skl", SubPath = "this.mHandle"}

    RegisterModuleUI("skeleton", "Module/Skeleton.png", dataTable)

end