function FileViewUI_GetIcons()

    local data = {}
    data["*.scene"] = {InfoLang = "fv.scene", Icon = "Scene_SCENE.png"}
    data["*.prop"] = {InfoLang = "fv.prop", Icon = "PropertySet_PROP.png"}
    data["*.dlg"] = {InfoLang = "fv.dialog", Icon = "Dialog_DLG_DLOG.png"}
    data["*.dlog"] = data["*.dlg"]
    data["*.anm"] = {InfoLang = "fv.anim", Icon = "Animation_ANM.png"}

    return data

end