function ModuleText_RegisterUI(moduleVersion)

    local dataTable = {}
    dataTable["Font"] = {Class = "class Handle<class Font>", Key = kTextFont}
    dataTable["Font"]["UI"] = {InputType = "handle:font", SubPath = "this.mHandle"}
    dataTable["Background Colour"] = {Class = "class Color", Key = kTextBackgroundColor}
    dataTable["Background Colour"]["UI"] = {InputType = "Colour", SubPath = "this"}
    dataTable["Background"] = {Class = "bool", Key = kTextBackground}
    dataTable["Background"]["UI"] = {InputType = kPropRenderBool, SubPath = "this"}
    dataTable["Foreground Colour"] = {Class = "class Color", Key = kTextColor}
    dataTable["Foreground Colour"]["UI"] = {InputType = kPropRenderColour, SubPath = "this"}
    dataTable["String"] = {Class = "class Color", Key = kTextString}
    dataTable["String"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
    dataTable["Extrude X"] = {Class = "float", Key = kTextExtrudeX}
    dataTable["Extrude X"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Extrude Y"] = {Class = "float", Key = kTextExtrudeY}
    dataTable["Extrude Y"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Kerning"] = {Class = "float", Key = kTextKerning}
    dataTable["Kerning"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Minimum Width"] = {Class = "float", Key = kTextMinWidth}
    dataTable["Minimum Width"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Minimum Height"] = {Class = "float", Key = kTextMinHeight}
    dataTable["Minimum Height"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Scale"] = {Class = "float", Key = kTextScale}
    dataTable["Scale"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Shadow Height"] = {Class = "float", Key = kTextShadowHeight}
    dataTable["Shadow Height"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Width"] = {Class = "float", Key = kTextWidth}
    dataTable["Width"]["UI"] = {InputType = kPropRenderFloat, SubPath = "this"}
    dataTable["Horizontal Align"] = {Class = "class TextAlignmentType", Key = kTextAlignmentHorizontal}
    dataTable["Horizontal Align"]["UI"] = {InputType = kPropRenderEnum, SubPath = "this.mAlignmentType"}
    dataTable["Vertical Align"] = {Class = "class TextAlignmentType", Key = kTextAlignmentVertical}
    dataTable["Vertical Align"]["UI"] = {InputType = kPropRenderEnum, SubPath = "this.mAlignmentType"}

    RegisterModuleUI("text", "Module/Text.png", dataTable)

end