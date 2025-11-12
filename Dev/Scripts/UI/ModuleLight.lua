function ModuleLight_RegisterUI(moduleVersion)

    local dataTable = {}

    dataTable["Dimmer"] = {Class = "float", Key = kLightDimmer}
    dataTable["Dimmer"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Colour"] = {Class = "class Color", Key = kLightColour}
    dataTable["Colour"]["UI"] = {InputType = kPropRenderColour, SubPath = "this"}
    dataTable["Intensity"] = {Class = "float", Key = kLightIntensity}
    dataTable["Intensity"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Max Distance"] = {Class = "float", Key = kLightMaxDistance}
    dataTable["Max Distance"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}

    -- plus light type TODO

    RegisterModuleUI("light", "Module/Light.png", dataTable)

end