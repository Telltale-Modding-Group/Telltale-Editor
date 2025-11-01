function ModuleNavCam_RegisterUI(moduleVersion)

    -- this module might need some revision in terms of UI. i think the mode determines which of the properties
    -- below are used: eg eOrbit should ignore most of the others apart from orbit properties, same for eAnimationTime, etc
    local dataTable = {}
    dataTable["Mode"] = {Class = "struct NavCam::EnumMode", Key = kNavCamMode}
    dataTable["Mode"]["UI"] = {InputType = kPropRenderEnum, SubPath = "this.mVal.mVal"}
    dataTable["Animation"] = {Class = "class Handle<class Animation>", Key = kNavCamAnimation}
    dataTable["Animation"]["UI"] = {InputType = "handle:anm", SubPath = "this.mHandle"}
    dataTable["Animation Time"] = {Class = "float", Key = kNavCamAnimationTime}
    dataTable["Animation Time"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Dampen"] = {Class = "float", Key = kNavCamDampen}
    dataTable["Dampen"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Trigger Horizontal %"] = {Class = "float", Key = kNavCamTriggerHPercent}
    dataTable["Trigger Horizontal %"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Trigger Vertical %"] = {Class = "float", Key = kNavCamTriggerVPercent}
    dataTable["Trigger Vertical %"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Target Agent"] = {Class = "class String", Key = kNavCamTargetAgent}
    dataTable["Target Agent"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
    dataTable["Target Offset"] = {Class = "class Vector3", Key = kNavCamTargetOffset}
    dataTable["Target Offset"]["UI"] = {InputType = kPropRenderVector3, SubPath = "this"}
    dataTable["Orbit Min"] = {Class = "class Polar", Key = kNavCamOrbitMin}
    dataTable["Orbit Min"]["UI"] = {InputType = kPropRenderPolar, SubPath = "this"}
    dataTable["Orbit Max"] = {Class = "class Polar", Key = kNavCamOrbitMax}
    dataTable["Orbit Max"]["UI"] = {InputType = kPropRenderPolar, SubPath = "this"}
    dataTable["Orbit Offset"] = {Class = "class Polar", Key = kNavCamOrbitOffset}
    dataTable["Orbit Offset"]["UI"] = {InputType = kPropRenderPolar, SubPath = "this"}

    RegisterModuleUI("navCam", "Module/NavCam.png", dataTable)

end