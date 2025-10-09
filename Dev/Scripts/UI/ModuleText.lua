function ModuleText_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Font"] = {Class = "class Handle<class Font>", Key = "Text Font"}
    dataTable["Font"]["UI"] = {InputType = "handle:font", SubPath = "this.mHandle"}
    dataTable["Background Colour"] = {Class = "class Color", Key = "Text Background Color"}
    dataTable["Background Colour"]["UI"] = {InputType = "Colour", SubPath = "this"}
    dataTable["Background"] = {Class = "bool", Key = "Text Background"}
    dataTable["Background"]["UI"] = {InputType = kPropRenderBool, SubPath = "this"}
    dataTable["Foreground Colour"] = {Class = "class Color", Key = "Text Color"}
    dataTable["Foreground Colour"]["UI"] = {InputType = kPropRenderColour, SubPath = "this"}
    dataTable["String"] = {Class = "class Color", Key = "Text String"}
    dataTable["String"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
    dataTable["Extrude X"] = {Class = "float", Key = "Text Extrude X"}
    dataTable["Extrude X"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Extrude Y"] = {Class = "float", Key = "Text Extrude Y"}
    dataTable["Extrude Y"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Kerning"] = {Class = "float", Key = "Text Kerning"}
    dataTable["Kerning"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Minimum Width"] = {Class = "float", Key = "Text Min Width"}
    dataTable["Minimum Width"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Minimum Height"] = {Class = "float", Key = "Text Min Height"}
    dataTable["Minimum Height"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Scale"] = {Class = "float", Key = "Text Scale"}
    dataTable["Scale"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Shadow Height"] = {Class = "float", Key = "Text Shadow Height"}
    dataTable["Shadow Height"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Width"] = {Class = "float", Key = "Text Width"}
    dataTable["Width"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Horizontal Align"] = {Class = "class TextAlignmentType", Key = "Text Alignment Horizontal"}
    dataTable["Horizontal Align"]["UI"] = {InputType = kPropRenderEnum, SubPath = "this.mAlignmentType"}
    dataTable["Vertical Align"] = {Class = "class TextAlignmentType", Key = "Text Alignment Vertical"}
    dataTable["Vertical Align"]["UI"] = {InputType = kPropRenderEnum, SubPath = "this.mAlignmentType"}

    RegisterModuleUI("text", "Module/Text.png", dataTable)

end