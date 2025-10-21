function ModuleRollover_RegisterUI(moduleVersion)

    local dataTable = {}

    if moduleVersion == 0 then

        dataTable["Texture"] = {Class = "class Handle<class D3DTexture>", Key = kRolloverTexture}
        dataTable["Texture"]["UI"] = {InputType = "handle:d3dtx", SubPath = "this.mHandle"}

    else

        dataTable["Text"] = {Class = "float", Key = kRolloverText}
        dataTable["Text"]["UI"] = {InputType = kPropRenderString, SubPath = "this"}
        dataTable["Text Colour"] = {Class = "class Color", Key = kRolloverTextColour}
        dataTable["Text Colour"]["UI"] = {InputType = kPropRenderColour, SubPath = "this"}
        dataTable["Text Background Colour"] = {Class = "float", Key = kRolloverTextBackgroundColour}
        dataTable["Text Background Colour"]["UI"] = {InputType = kPropRenderColour, SubPath = "this"}
        dataTable["Cursor Mesh"] = {Class = "class Handle<class D3DMesh>", Key = kRolloverMesh}
        dataTable["Cursor Mesh"]["UI"] = {InputType = "handle:d3dmesh", SubPath = "this.mHandle"}
        dataTable["Cursor Properties"] = {Class = "class Handle<class PropertySet>", Key = kRolloverCursorProps}
        dataTable["Cursor Properties"]["UI"] = {InputType = "handle:prop", SubPath = "this.mHandle"}

    end

    RegisterModuleUI("rollover", "Module/Rollover.png", dataTable)

end