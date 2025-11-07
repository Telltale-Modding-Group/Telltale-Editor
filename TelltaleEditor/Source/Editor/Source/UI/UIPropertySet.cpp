#include <UI/UIEditors.hpp>
#include <UI/ApplicationUI.hpp>
#include <imgui.h>
#include <imgui_internal.h>

DECL_VEC_ADDITION();

// BASE IMPL

Bool UIResourceEditorBase::SelectionBox(Float xPos, Float yPos, Float xSizePix, Float ySizePix, Bool state)
{
    ImVec2 posImage{ xPos, yPos };
    ImVec2 szImage{ xSizePix, ySizePix };
    Bool hov = ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        state = !state;
    if (hov || state)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage, state ? IM_COL32(85, 197, 209, 100) : IM_COL32(65, 197, 209, 100));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage, state ? IM_COL32(74, 163, 173, 200) : IM_COL32(54, 163, 173, 200));
    }
    return state;
}

Bool UIResourceEditorBase::ImageButton(CString iconTex, Float xPos, Float yPos, Float xSizePix, Float ySizePix, U32 sc)
{
    ImVec2 posImage{ xPos, yPos };
    ImVec2 szImage{ xSizePix, ySizePix };
    Bool hov = ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage);
    DrawResourceTexturePixels(iconTex, posImage.x, posImage.y, szImage.x, szImage.y, sc);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        return true;
    }
    return false;
}

// =========

UIPropertySet::UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop) : UIResourceEditor<>(ui, title, prop), _HandleTable(true)
{

}

UIPropertySet::UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop, Meta::ParentWeakReference p)
    : UIResourceEditor<>(ui, prop, p, title), _HandleTable(true)
{

}

UIPropertySet::UIPropertySet(EditorUI& ui, String agent, WeakPtr<ReferenceObjectInterface> scene) 
    : UIResourceEditor<>(ui, ui.GetApplication().GetLanguageText("misc.inspecting") + " " + agent), _HandleTable(true)
{
    Agent = agent;
    AgentScene = scene;
}

#define INDENT_SPACING 20.0f
#define LINE_HEIGHT 17.0f
#define VALUE_START_X 300.0f

Bool UIPropertySet::_RenderDropdown(Float& currentY, Float indentX, CString name, String fullPath, Bool bClassProp)
{
    ImVec2 posBase = ImGui::GetWindowPos() + ImVec2{ 0.0f, currentY - ImGui::GetScrollY() };
    if ((_RowNum++) & 1)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase +
            ImVec2{ ImGui::GetWindowSize().x, LINE_HEIGHT }, IM_COL32(46, 46, 46, 255));
    }
    else
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase +
            ImVec2{ ImGui::GetWindowSize().x, LINE_HEIGHT }, IM_COL32(58, 58, 58, 255));
    }
    ImVec2 posImage = posBase + ImVec2{ 2.0f + indentX, 0.0f };
    ImVec2 szImage = ImVec2{ INDENT_SPACING - 4.0f, INDENT_SPACING - 4.0f };
    Bool open = _OpenDrops.find(fullPath) != _OpenDrops.end();
    Bool hov = ImGui::IsMouseHoveringRect(posImage, posImage + szImage);
    DrawResourceTexturePixels(open ? "Misc/DownArrow.png" : "Misc/RightArrow.png", posImage.x - ImGui::GetWindowPos().x, posImage.y - ImGui::GetWindowPos().y, szImage.x, szImage.y, hov ? IM_COL32(125, 125, 125, 255) : 0xFFFFFFFFu);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (open)
            _OpenDrops.erase(_OpenDrops.find(fullPath));
        else
            _OpenDrops.insert(fullPath);
        open = !open;
    }
    if (bClassProp)
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 200, 20, 255));
    ImGui::SetCursorPos(ImVec2{ indentX + INDENT_SPACING, currentY + 4.0f });
    ImGui::TextUnformatted(name);
    if (bClassProp)
        ImGui::PopStyleColor();
    currentY += LINE_HEIGHT;
    return open;
}

PropGlobalAction UIPropertySet::_RenderProp(Float& currentY, Float indentX, CString name, CString propName, Meta::ClassInstance prop, String stackPath, Bool bClassProp)
{
    ImGui::PushID(name);
    PropGlobalAction action = PropGlobalAction::NONE;
    Bool open = _RenderDropdown(currentY, indentX, name, stackPath, false);
    if(bClassProp && ImGui::BeginPopupContextItem("##proprem"))
    {
        if(ImGui::MenuItem(GetApplication().GetLanguageText("misc.remove_global").c_str()))
        {
            action = PropGlobalAction::REMOVE;
        }
        if (ImGui::MenuItem(GetApplication().GetLanguageText("misc.remove_global_keys").c_str()))
        {
            action = PropGlobalAction::REMOVE_AND_LOCALS;
        }
        ImGui::EndPopup();
    }
    if(open)
    {
        Symbol actionKey{};
        PropAction action{};
        if(AgentMode == PropViewMode::SCENE || AgentMode == PropViewMode::CLASS)
        {
            std::set<Symbol> keys{};
            PropertySet::GetKeys(prop, keys, true, GetApplication().GetRegistry());
            for (const auto& entry : keys)
            {
                String propKName = GetRuntimeSymbols().FindOrHashString(entry);
                Symbol retreiveProp = PropertySet::GetPropertySetValueIsRetrievedFrom(prop, entry, GetApplication().GetRegistry(), true).GetObject();
                Bool bAllowEdit = false;
                Bool bGetFromAgentProps = false;
                Meta::ClassInstance propVal{};
                if (!bClassProp)
                {
                    if (AgentMode == PropViewMode::CLASS)
                    {
                        bAllowEdit = false;
                    }
                    else
                    {
                        bAllowEdit = retreiveProp == propName;
                    }
                }
                else if(AgentMode == PropViewMode::SCENE && _AgentProperties)
                {
                    bAllowEdit = PropertySet::ExistsKey(_AgentProperties, entry, false, GetApplication().GetRegistry());
                    bGetFromAgentProps = bAllowEdit;
                }
                if (!bGetFromAgentProps)
                    propVal = PropertySet::Get(prop, entry, true, GetApplication().GetRegistry());
                else
                    propVal = PropertySet::Get(_AgentProperties, entry, false, GetApplication().GetRegistry());
                PropAction action0 = _RenderPropItem(currentY, indentX + INDENT_SPACING, propKName.c_str(), propVal, false, !bAllowEdit, stackPath + propKName);
                if (action0 != PropAction::NONE)
                {
                    action = action0;
                    actionKey = entry;
                }
            }
        }
        else
        {
            for (const auto& entry : *prop._GetInternalChildrenRefs())
            {
                String propKName = GetRuntimeSymbols().FindOrHashString(entry.first);
                PropAction action0 = _RenderPropItem(currentY, indentX + INDENT_SPACING, propKName.c_str(), entry.second, false, bClassProp, stackPath + propKName);
                if (action0 != PropAction::NONE)
                {
                    action = action0;
                    actionKey = entry.first;
                }
            }
        }
        if (action == PropAction::REMOVE)
        {
            PropertySet::RemoveKey(_AgentProperties, actionKey);
        }
        else if (action == PropAction::MAKE_LOCAL)
        {
            PropertySet::PromoteKeyToLocal(_AgentProperties, actionKey, GetApplication().GetRegistry());
        }
    }
    ImGui::PopID();
    return action;
}

// globals => add, remove, open (specific)

Bool UIPropertySet::_RenderSinglePropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, String stackPath, Bool bDoRender)
{
    const Meta::Class& cls = Meta::GetClass(value.GetClassID());
    const PropertyRenderInstruction* Instruction = PropertyRenderInstructions;
    while (Instruction->Name)
    {
        CString iname = Instruction->Name;
        if (String(iname) == "Colour")
            iname = "Color"; // America WHY
        if (cls.ToolTypeHash == Symbol(iname))
        {
            if(bDoRender)
            {
                PropertyVisualAdapter adapter{};
                adapter.Flags = PropertyVisualAdapter::NO_REPOSITION;
                adapter.ClassID = cls.ClassID;
                adapter.RenderInstruction = Instruction;
                Instruction->Render(_EditorUI, adapter, value);
            }
            return true;
        }
        Instruction++;
    }
    if ((StringStartsWith(cls.Name, "class Handle<") || StringStartsWith(cls.Name, "Handle<")) && cls.RTSizeRaw == sizeof(Symbol))
    {
        if (bDoRender)
        {
            Symbol* handleValue = reinterpret_cast<Symbol*>(value._GetInternal());
            String handleStringValue = {};
            if (handleValue->GetCRC64() != 0)
            {
                handleStringValue = _HandleTable.FindLocal(*handleValue);
                if (handleStringValue.empty())
                {
                    if (_UnknownSymbols.find(*handleValue) != _UnknownSymbols.end())
                    {
                        handleStringValue = SymbolToHexString(*handleValue);
                    }
                    else
                    {
                        handleStringValue = SymbolTable::FindOrHashString(*handleValue);
                        if (SymbolTable::IsHashString(handleStringValue))
                        {
                            _UnknownSymbols.insert(*handleValue);
                        }
                        else
                        {
                            _HandleTable.Register(handleStringValue);
                        }
                    }
                }
            }
            ImGui::InputText("##inputH", &handleStringValue, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("TT_ASSET");
                if (pl)
                {
                    String nHandle = (CString)pl->Data;
                    U32 handledClass = Meta::FindClass(StringExtractTemplateParameter(MakeTypeName(cls.Name)), 0, false);
                    String ext{};
                    if(handledClass != 0 && StringEndsWith(nHandle, ext = Meta::GetClass(handledClass).Extension))
                    {
                        _HandleTable.Register(nHandle);
                        *handleValue = nHandle;
                        // on change
                    }
                    else if(!ext.empty())
                    {
                        PlatformMessageBoxAndWait(GetApplication().GetLanguageText("misc.invalid_type"), GetApplication().GetLanguageText("misc.only_xxx") + " " + ext);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::SetTooltip(GetApplication().GetLanguageText("misc.drag_here").c_str(), cls.Extension.c_str());
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
            if (ImGui::Button(GetApplication().GetLanguageText("misc.reset_lower").c_str()) && !handleStringValue.empty())
            {
                *handleValue = {};
                // on change
            }
        }
        return true;
    }
    if ((cls.Flags & Meta::CLASS_ENUM_WRAPPER) != 0)
    {
        for (const auto& mem : cls.Members)
        {
            if (mem.Flags & Meta::MEMBER_ENUM)
            {
                if (bDoRender)
                {
                    I32* enumVal = (I32*)(value._GetInternal() + mem.RTOffset);
                    CString curSelected = "UNKNOWN_VALUE";
                    for (const auto& enumDesc : mem.Descriptors)
                    {
                        if (*enumVal == enumDesc.Value)
                        {
                            curSelected = enumDesc.Name.c_str();
                            break;
                        }
                    }
                    if (ImGui::BeginCombo("##cmb", curSelected))
                    {
                        for (const auto& enumDesc : mem.Descriptors)
                        {
                            if (ImGui::Selectable(enumDesc.Name.c_str()))
                            {
                                *enumVal = enumDesc.Value;
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                return true;
            }
        }
    }
    return false;
}

void UIPropertySet::_RenderClassMembers(Float& currentY, Float indentX, String stackPath, const Meta::Class& cls, Meta::ClassInstance& value, CString mname, Bool bClassProp, PropAction& actionOut)
{
    if(_RenderDropdown(currentY, indentX, mname, stackPath, bClassProp))
    {
        actionOut = _RenderPropActionContext(cls, mname, bClassProp);
        for(const auto& member: cls.Members)
        {
            Meta::ClassInstance memVal = Meta::GetMember(value, member.Name, true);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, member.Name.c_str(), memVal, false, bClassProp, stackPath + member.Name, true);
        }
    }
    else
    {
        actionOut = _RenderPropActionContext(cls, mname, bClassProp);
    }
}

Bool UIPropertySet::_RenderDropdownPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, String stackPath, Bool bDoRender, Bool bClassProp, PropAction& actionOut)
{
    const Meta::Class& cls = Meta::GetClass(value.GetClassID());
    if (cls.ToolTypeHash == Symbol("Transform"))
    {
        if (bDoRender && _RenderDropdown(currentY, indentX, name, stackPath + name, bClassProp))
        {
            actionOut = _RenderPropActionContext(cls, name, bClassProp);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, GetApplication().GetLanguageText("misc.trans").c_str(), Meta::GetMember(value, "mTrans", true), false, bClassProp, stackPath + "T", true);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, GetApplication().GetLanguageText("misc.rot").c_str(), Meta::GetMember(value, "mRot", true), false, bClassProp, stackPath + "R", true);
        }
        else if(bDoRender)
        {
            actionOut = _RenderPropActionContext(cls, name, bClassProp);
        }
        return true;
    }

    // OTHERS, ENSURE *AS SOON AS* RENDER DROPDOWN IS CALLED, ACTION OUT IS UPDATED, REGARDLESS OF WETHER IT RETURNED TRUE/FALSE

    // FINALLY
    if(cls.Members.size() > 0)
    {
        if(bDoRender)
        {
            _RenderClassMembers(currentY, indentX, stackPath + name, cls, value, name, bClassProp, actionOut);
        }
        return true;
    }
    return false;
}

extern ImGuiContext* GImGui;

PropAction UIPropertySet::_RenderPropActionContext(const Meta::Class& cls, const String& keyName, Bool bClassProp)
{
    PropAction action = PropAction::NONE;
    Bool dis = GImGui->DisabledStackSize > 0;
    if (dis)
        ImGui::EndDisabled();
    if(ImGui::BeginPopupContextItem("##propitem", 1, ImGuiHoveredFlags_AllowWhenDisabled))
    { 
        String keyFrom = GetApplication().GetLanguageText("misc.keyfrom") +  " " +
            SymbolTable::Find(PropertySet::GetPropertySetKeyIsIntroducedFrom(_AgentProperties, keyName, GetApplication().GetRegistry()).GetObject());
        String valueFrom = GetApplication().GetLanguageText("misc.valfrom") + " " +
            SymbolTable::Find(PropertySet::GetPropertySetValueIsRetrievedFrom(_AgentProperties, keyName, GetApplication().GetRegistry(), true).GetObject());
        ImGui::TextUnformatted(keyName.c_str());
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        ImGui::TextUnformatted(cls.Name.c_str());
        ImGui::PopStyleColor();
        ImGui::TextUnformatted(keyFrom.c_str());
        ImGui::TextUnformatted(valueFrom.c_str());
        ImGui::Separator();
        if(bClassProp)
        {
            if (ImGui::MenuItem(GetApplication().GetLanguageText("misc.make_local").c_str()))
            {
                action = PropAction::MAKE_LOCAL;
            }
        }
        else
        {
            if (ImGui::MenuItem(GetApplication().GetLanguageText("misc.remove_local").c_str()))
            {
                action = PropAction::REMOVE;
            }
        }
        ImGui::EndPopup();
    }
    if (dis)
        ImGui::BeginDisabled();
    return action;
}

PropAction UIPropertySet::_RenderPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, Bool red, Bool bClassProp, String stackPath, Bool bIsClassMember)
{
    ImVec2 posBase = ImGui::GetWindowPos() + ImVec2{0.0f, currentY - ImGui::GetScrollY()};
    if((_RowNum++) & 1)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase + ImVec2{ImGui::GetWindowSize().x, LINE_HEIGHT},
                                                  red ? IM_COL32(100, 50, 50, 255) : IM_COL32(46, 46, 46, 255));
    }
    else
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase + ImVec2{ImGui::GetWindowSize().x, LINE_HEIGHT},
                                                  red ? IM_COL32(70, 30, 30, 255) : IM_COL32(58, 58, 58, 255));
    }
    ImGui::PushID(COERCE(&currentY, int));

    Bool bSingle = value && _RenderSinglePropItem(currentY, indentX, name, value, stackPath, false);
    if (bClassProp)
        ImGui::BeginDisabled();
    PropAction action{};
    if(bSingle || !(value && _RenderDropdownPropItem(currentY, indentX, name, value, stackPath, true, bClassProp, action)))
    {
        if (bClassProp)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 200, 20, 255));
        ImGui::SetCursorPos(ImVec2{ indentX + INDENT_SPACING, currentY + 4.0f });
        ImGui::TextUnformatted(name);
        if (bClassProp)
            ImGui::PopStyleColor();
        if(value)
        {
            if (bSingle)
            {
                if(!bIsClassMember)
                    action = _RenderPropActionContext(Meta::GetClass(value.GetClassID()), name, bClassProp);
                ImGui::SetCursorPos(ImVec2{ MAX(indentX + 10.0f, VALUE_START_X), currentY + 1.0f });
                _RenderSinglePropItem(currentY, indentX, name, value, stackPath, true);
            }
            else
            {
                ImGui::Text("Impl");
            }
        }
        currentY += LINE_HEIGHT;
    }
    if (bClassProp)
        ImGui::EndDisabled();
    ImGui::PopID();
    return action;
}

Bool UIPropertySet::IsAlive() const
{
    return UIResourceEditor::IsAlive() && (Agent.empty() ? true : !AgentScene.expired());
}

void UIPropertySet::_AddGlobalCallback(String globalProp)
{
    Meta::ClassInstance prop = GetMetaObject();
    if (!prop)
        prop = _GetRawScene()->GetAgentProps(Agent);
    if(prop)
    {
        if(PropertySet::IsMyParent(prop, globalProp, true, GetApplication().GetRegistry()))
        {
            PlatformMessageBoxAndWait("Error", GetApplication().GetLanguageText("misc.error_parent"));
        }
        else
        {
            PropertySet::AddParent(prop, globalProp, GetApplication().GetRegistry());
        }
    }
}

void UIPropertySet::_AddLocalCallback(String localName)
{
    Meta::ClassInstance prop = GetMetaObject();
    if (!prop)
        prop = _GetRawScene()->GetAgentProps(Agent);
    if(!_NewLocalType.empty())
    {
        if(PropertySet::ExistsKey(prop, localName, false, GetApplication().GetRegistry()))
        {
            PlatformMessageBoxAndWait("Error", GetApplication().GetLanguageText("misc.error_local"));
        }
        else
        {
            Meta::ClassInstance value = Meta::CreateInstance(Meta::FindClass(_NewLocalType, 0, false));
            if(value)
            {
                GetGameSymbols().Register(_NewLocalType);
                PropertySet::Set(prop, localName, value, GetApplication().GetRegistry(), PropertySet::SetPropertyMode::MOVE); // i think direct isnt needed...
            }
            else
            {
                PlatformMessageBoxAndWait("Error", "Could not allocate property value!!");
            }
        }
    }
    _NewLocalType = {};
}

void UIPropertySet::_SelectLocalTypeCallback(String localType)
{
    _NewLocalType = localType;
    GetApplication().QueuePopup(TTE_NEW_PTR(TextFieldPopup, MEMORY_TAG_EDITOR_UI, "Property Name", ALLOCATE_METHOD_CALLBACK_1(this, _AddLocalCallback, UIPropertySet, String), "Local Property Name"), _EditorUI);
}

Scene* UIPropertySet::_GetRawScene()
{
    Ptr<ReferenceObjectInterface> pScene = AgentScene.lock();
    Ptr<Scene> pConcreteScene = std::dynamic_pointer_cast<Scene, ReferenceObjectInterface>(pScene);
    Ptr<ReferenceObjectConcrete> pDeferredOwnershipScene = std::dynamic_pointer_cast<ReferenceObjectConcrete, ReferenceObjectInterface>(pScene);
    TTE_ASSERT(pScene || pDeferredOwnershipScene, "Invalid agent scene");
    return pConcreteScene ? pConcreteScene.get() : (Scene*)pDeferredOwnershipScene->Data$;
}

Bool UIPropertySet::RenderEditor()
{
    Bool closing = false;
    ImGui::SetNextWindowSizeConstraints(ImVec2{ 400.0f, 300.0f }, ImVec2{ 99999.0f, 99999.0f });
    if(ImGui::Begin(GetTitle().c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        if(ImGui::BeginMenuBar())
        {
            if(ImGui::BeginMenu("File"))
            {
                if(ImGui::MenuItem("Close"))
                {
                    closing = true;
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Locals"))
            {
                if(ImGui::MenuItem("Add"))
                {
                    GetApplication().SetCurrentPopup(TTE_NEW_PTR(MetaClassPickerPopup,
                        MEMORY_TAG_EDITOR_UI, "Choose Class", ALLOCATE_METHOD_CALLBACK_1(this, _SelectLocalTypeCallback, UIPropertySet, String)), _EditorUI);
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Globals"))
            {
                if(ImGui::MenuItem("Add"))
                {
                    GetApplication().SetCurrentPopup(TTE_NEW_PTR(ResourcePickerPopup, 
                        MEMORY_TAG_EDITOR_UI, "Choose Global", "*", ALLOCATE_METHOD_CALLBACK_1(this, _AddGlobalCallback, UIPropertySet, String)), _EditorUI);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::PushFont(ImGui::GetFont(), 11.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushItemWidth(230.0f);
        _RowNum = 0;
        Meta::ClassInstance propRoot = GetMetaObject();
        const Float iconSize = 30.0f, iconSpacing = 5.0f, barHeight = 40.0f, selHeight = 25.0f, startY = 35.0f;
        Float currentY = startY;
        if(!Agent.empty())
        {
            currentY += barHeight;
            currentY += selHeight;
        }
        else
        {
            currentY += 5.0f;
        }
        std::set<HandlePropertySet> parents{};
        String localPropName = "";
        if(!Agent.empty())
        {
            Scene* rawScene = _GetRawScene();
            // no parents for the next two! they are just each other
            if(AgentMode == PropViewMode::TRANSIENT)
            {
                propRoot = rawScene->GetAgentTransientProps(Agent); // i wont register, ill check at render
            }
            else if(AgentMode == PropViewMode::RUNTIME)
            {
                localPropName = Scene::GetAgentRuntimePropertiesName(rawScene->GetName(), Agent);
                propRoot = rawScene->GetAgentRuntimeProps(Agent);
            }
            else
            {
                propRoot = rawScene->GetAgentProps(Agent);
                localPropName = Scene::GetAgentScenePropertiesName(rawScene->GetName(), Agent);
                _AgentProperties = propRoot;
            }
            if (!_AgentProperties)
                _AgentProperties = rawScene->GetAgentProps(Agent);
        }
        else
        {
            _AgentProperties = propRoot;
        }
        if(AgentMode != PropViewMode::RUNTIME && AgentMode != PropViewMode::TRANSIENT)
        {
            PropertySet::GetParents(propRoot, parents, true, GetApplication().GetRegistry());
        }
        if(AgentMode == PropViewMode::POSITION)
        {
            // RENDER POS INFO, TODO ANIMS
        }
        else
        {
            if (AgentMode != PropViewMode::CLASS)
                _RenderProp(currentY, 0.0f, GetApplication().GetLanguageText(AgentMode == PropViewMode::SCENE ? "misc.natives" : "misc.locals").c_str(), localPropName.c_str(), propRoot, "LOCAL", false);
            Symbol globalRemove{};
            PropGlobalAction action = PropGlobalAction::NONE;
            for (auto parent : parents)
            {
                String name = parent.GetObjectResolved();
                if (!name.empty())
                {
                    Meta::ClassInstance parentProp = parent.GetObject(GetApplication().GetRegistry(), true);
                    if (parentProp)
                    {
                        PropGlobalAction action0{};
                        if ((action0 = _RenderProp(currentY, 0.0f, name.c_str(), name.c_str(), parentProp, name, true)), action0 != PropGlobalAction::NONE)
                        {
                            action = action0;
                            globalRemove = name;
                        }
                    }
                    else
                        _RenderPropItem(currentY, 0.0f, name.c_str(), {}, true, false, name);
                }
            }
            if(globalRemove)
            {
                if(PropertySet::IsMyParent(_AgentProperties, globalRemove, false, GetApplication().GetRegistry()))
                {
                    PropertySet::RemoveParent(_AgentProperties, globalRemove, action == PropGlobalAction::REMOVE_AND_LOCALS, GetApplication().GetRegistry());
                }
                else
                {
                    PlatformMessageBoxAndWait("Error", GetApplication().GetLanguageText("misc.global_remove_err"));
                }
            }
        }
        currentY = startY;
        if (!Agent.empty())
        {
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2{ 0.0f, currentY }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x, currentY + barHeight }, IM_COL32(100, 100, 100, 255));
            Float iconX = 5.0f;
            const PropViewModeDesc* selDesc = 0;
            for (U32 mode = 0; mode < (U32)PropViewMode::NUM; mode++)
            {
                const PropViewModeDesc& desc = ModeDescs[mode];
                SelectionBox(iconX, currentY + 8.0f, iconSize, iconSize, desc.ModeVal == AgentMode);
                if (ImageButton(desc.Icon, iconX, currentY + 6.0f, iconSize, iconSize))
                {
                    AgentMode = desc.ModeVal;
                }
                if (desc.ModeVal == AgentMode)
                    selDesc = &desc;
                iconX += iconSize + iconSpacing;
            }
            currentY += barHeight;
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2{ 0.0f, currentY }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x, currentY + selHeight }, IM_COL32(selDesc->R, selDesc->G, selDesc->B, 255));
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + ImVec2{ 0.0f, currentY }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x, currentY + selHeight }, IM_COL32(50, 50, 50, 255));
            String text = GetApplication().GetLanguageText(selDesc->Name) + " - " + Agent;
            ImVec2 tsize = ImGui::CalcTextSize(text.c_str());
            ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2{ 5.0f, currentY + (selHeight - tsize.y) * 0.5f });
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            ImGui::Text(text.c_str());
            ImGui::PopStyleColor();
            currentY += selHeight;
        }
        _AgentProperties = {};
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        ImGui::PopFont();
    }
    ImGui::End();
    return closing;
}
