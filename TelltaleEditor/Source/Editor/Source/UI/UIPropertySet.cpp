#include <UI/UIEditors.hpp>
#include <UI/ApplicationUI.hpp>
#include <imgui.h>
#include <imgui_internal.h>

DECL_VEC_ADDITION();

// ========= PROPERTY SET UI IMPL

// these two are needed, they arent implemented as they are overriden
template<>
void UIResourceEditor<>::OnExit()
{
}

template<>
Bool UIResourceEditor<>::RenderEditor()
{
    return true;
}

UIPropertySet::UIPropertySet(String propName, EditorUI& ui, String title, Meta::ClassInstance prop) :
    UIResourceEditor<>(propName, ui, title, prop), _HandleTable(true)
{

}

UIPropertySet::UIPropertySet(String propName, EditorUI& ui, String title, Meta::ClassInstance prop, Meta::ParentWeakReference p)
    : UIResourceEditor<>(propName, ui, prop, p, title), _HandleTable(true)
{

}

UIPropertySet::UIPropertySet(EditorUI& ui, String agent, WeakPtr<ReferenceObjectInterface> scene)
    : UIResourceEditor<>(agent, ui, ui.GetApplication().GetLanguageText("misc.inspecting") + " " + agent), _HandleTable(true)
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
                // TODO ADD CACHE
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

void UIPropertySet::_RenderClassMembers(Float& currentY, Float indentX, String stackPath, const Meta::Class& cls, Meta::ClassInstance& value, CString mname, Bool bClassProp, PropAction& actionOut, Bool bCollection)
{
    if(_RenderDropdown(currentY, indentX, mname, stackPath, bClassProp))
    {
        actionOut = _RenderPropActionContext(cls, mname, bClassProp, bCollection);
        for(const auto& member: cls.Members)
        {
            Meta::ClassInstance memVal = Meta::GetMember(value, member.Name, true);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, member.Name.c_str(), memVal, false, bClassProp, stackPath + member.Name, true);
        }
    }
    else
    {
        actionOut = _RenderPropActionContext(cls, mname, bClassProp, bCollection);
    }
}

void UIPropertySet::_AddElementCallback(Meta::ClassInstance newValue, Meta::ClassInstance collectionV)
{
    if(newValue && collectionV)
    {
        Meta::ClassInstanceCollection& collection = Meta::CastToCollection(collectionV);
        Bool bKeyed = collection.IsKeyedCollection();
        U8 Accel[128]{};
        Bool bNeedCheck = bKeyed || ((Meta::GetClass(collection.GetValueClass()).Flags & Meta::CLASS_COLLECTION_SORTED) != 0);
        Bool bNeedWarn = false;
        if(bNeedCheck)
        {
            U32 nelem = collection.GetSize();
            for(U32 i = 0; i < nelem; i++)
            {
                Meta::ClassInstance comparand = bKeyed ? collection.GetKey(i) : collection.GetValue(i);
                if(Meta::PerformEquality(newValue, comparand))
                {
                    bNeedWarn = true;
                    break;
                }
            }
        }
        if(bNeedWarn)
        {
            PlatformMessageBoxAndWait("Warning", GetApplication().GetLanguageText("misc.warn_existing").c_str());
        }
        else
        {
            if(bKeyed)
            {
                collection.Push(newValue, Meta::CreateTemporaryInstance(Accel, 128, collection.GetValueClass()), false, false);
            }
            else
            {
                // insert at index
                I32 index = MIN(collection.GetSize(), *(I32*)newValue._GetInternal());
                collection.Insert({}, Meta::CreateTemporaryInstance(Accel, 128, collection.GetValueClass()), index, false, false);
            }
        }
    }
}

Bool UIPropertySet::_RenderDropdownPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, String stackPath, Bool bDoRender, Bool bClassProp, PropAction& actionOut, Bool bCollection)
{
    const Meta::Class& cls = Meta::GetClass(value.GetClassID());
    if (cls.ToolTypeHash == Symbol("Transform"))
    {
        if (bDoRender && _RenderDropdown(currentY, indentX, name, stackPath + name, bClassProp))
        {
            actionOut = _RenderPropActionContext(cls, name, bClassProp, bCollection);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, GetApplication().GetLanguageText("misc.trans").c_str(), Meta::GetMember(value, "mTrans", true), false, bClassProp, stackPath + "T", true);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, GetApplication().GetLanguageText("misc.rot").c_str(), Meta::GetMember(value, "mRot", true), false, bClassProp, stackPath + "R", true);
        }
        else if(bDoRender)
        {
            actionOut = _RenderPropActionContext(cls, name, bClassProp, bCollection);
        }
        return true;
    }
    
    if(cls.Flags & Meta::CLASS_CONTAINER)
    {
        if(bDoRender)
        {
            String myPath = stackPath + name;
            Bool open = _RenderDropdown(currentY, indentX, name, myPath, bClassProp);
            actionOut = _RenderPropActionContext(cls, name, bClassProp, bCollection);
            
            // COLLECTION STATE
            Meta::ClassInstanceCollection& collection = Meta::CastToCollection(value);
            std::pair<U32, U32> myState = std::make_pair(0, MIN(5, collection.GetSize())); // start index, number of elements showing
            if(_CollectionViewState.find(myPath) != _CollectionViewState.end())
            {
                myState = _CollectionViewState.find(myPath)->second;
            }
            if(myState.second > collection.GetSize())
            {
                myState.second = MIN(5, collection.GetSize());
            }
            if(myState.first >= collection.GetSize())
            {
                myState.first = 0;
            }
            if(myState.second == 0 && collection.GetSize() > 0)
                myState.second = 1; // when adding items
            
            // COLLECTION HEADER
            ImVec2 wpos = ImGui::GetWindowPos();
            _RenderSetCursorForInlineValue(indentX, currentY - LINE_HEIGHT);
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            U32 buttonCol = !bClassProp ? IM_COL32(160, 160, 160, 255) : IM_COL32(130, 130, 130, 255);
            ImGui::SetCursorScreenPos(screenPos);
            if(ArrowButton(screenPos.x + 2.0f - wpos.x, screenPos.y + 2.0f - wpos.y, 11.0f, 11.0f, buttonCol))
            {
                ImGui::OpenPopup("Col0");
            }
            if(ImGui::IsPopupOpen("Col0"))
                ImGui::SetNextWindowPos(screenPos + ImVec2{5.0f, 20.0f});
            if(ImGui::BeginPopup("Col0", ImGuiWindowFlags_NoTitleBar))
            {
                // empty dropdown for now if class prop, will add more in future maybe anyway
                if(!bClassProp && !collection.IsStaticArray() && ImGui::Button("Add Element"))
                {
                    Meta::ClassInstance keyValue{};
                    
                    // COLLECTION TYPES: MAP, ARRAY, SET, DEQUE, LIST,
                    String title = {}, prompt{};
                    const Meta::Class& colClass = Meta::GetClass(value.GetClassID());
                    Bool bKeyed = collection.IsKeyedCollection();
                    Bool bSorted = (colClass.Flags & Meta::CLASS_COLLECTION_SORTED) != 0;
                    Bool bNeedKeyWindow = false;
                    if(bKeyed)
                    {
                        bNeedKeyWindow = true;
                        keyValue = Meta::CreateInstance(collection.GetKeyClass());
                        title = "Map Key";
                        prompt = "New K value for Map<K,V>";
                        // Map.
                    }
                    else if(bSorted)
                    {
                        bNeedKeyWindow = false; // sorted, index is determined by value (when saving prop)
                    }
                    else
                    {
                        keyValue = Meta::CreateInstance(Meta::FindClass("int", 0));
                        *(I32*)keyValue._GetInternal() = collection.GetSize();
                        title = "Insertion Index";
                        prompt = "Index to insert into collection";
                        bNeedKeyWindow = true; // user can select index where to insert
                    }
                    
                    if(bNeedKeyWindow)
                    {
                        GetApplication().SetCurrentPopup(TTE_NEW_PTR(MetaInstanceEditPopup, MEMORY_TAG_EDITOR_UI, _EditorUI, title,
                                ALLOCATE_METHOD_CALLBACK_2(this, _AddElementCallback, UIPropertySet,
                                        Meta::ClassInstance, Meta::ClassInstance), prompt, keyValue, value), _EditorUI);
                    }
                    else
                    {
                        U8 Accel[128]{};
                        // add now
                        collection.PushValue(Meta::CreateTemporaryInstance(Accel, 128, collection.GetValueClass()), false);
                    }
                    ImGui::CloseCurrentPopup();
                }
                // options
                ImGui::EndPopup();
            }
            screenPos.x += 17.0f;
            
            // INFO TEXT
            ImGui::SetCursorScreenPos(screenPos + ImVec2{0.0f, 4.0f});
            ImGui::PushFont(ImGui::GetFont(), 10.0f);
            String showText = "[" + StringFromInteger((I64)collection.GetSize(), 10, false) + (collection.GetSize() == 1 ?
                            " element] showing " : " elements] showing ") + StringFromInteger((I64)myState.second, 10, false);
            ImGui::TextUnformatted(showText.c_str());
            screenPos.x += ImGui::CalcTextSize(showText.c_str()).x + 3.0f;
            
            // SHOWING XX
            if(ArrowButton(screenPos.x + 2.0f - wpos.x, screenPos.y + 2.0f - wpos.y, 11.0f, 11.0f, buttonCol))
            {
                ImGui::OpenPopup("Col1");
            }
            if(ImGui::IsPopupOpen("Col1"))
                ImGui::SetNextWindowPos(screenPos + ImVec2{5.0f, 20.0f});
            if(ImGui::BeginPopup("Col1", ImGuiWindowFlags_NoTitleBar))
            {
                U32 remaining = collection.GetSize() - myState.first;
                if(remaining != 0)
                {
                    for(U32 i = 0; i < remaining; i++)
                    {
                        String stri = std::to_string(i+1);
                        if(ImGui::Button(stri.c_str()))
                        {
                            myState.second = i+1;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            screenPos.x += 15.0f;
            
            // STARTING AT
            showText = "starting at: " + StringFromInteger((I64)myState.first, 10, false);
            ImGui::SetCursorScreenPos(screenPos + ImVec2{5.0f, 4.0f});
            ImGui::TextUnformatted(showText.c_str());
            screenPos.x += ImGui::CalcTextSize(showText.c_str()).x + 7.0f;
            ImGui::PopFont();
            
            if(ArrowButton(screenPos.x + 2.0f - wpos.x, screenPos.y + 2.0f - wpos.y, 11.0f, 11.0f, buttonCol))
            {
                ImGui::OpenPopup("Col2");
            }
            if(ImGui::IsPopupOpen("Col2"))
                ImGui::SetNextWindowPos(screenPos + ImVec2{5.0f, 20.0f});
            if(ImGui::BeginPopup("Col2", ImGuiWindowFlags_NoTitleBar))
            {
                if(collection.GetSize())
                {
                    for(U32 i = 0; i < collection.GetSize(); i++)
                    {
                        String stri = std::to_string(i);
                        if(ImGui::Button(stri.c_str()))
                        {
                            ImGui::CloseCurrentPopup();
                            myState.first = i;
                            break;
                        }
                    }
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            if(open)
            {
                U32 removeindex = collection.GetSize();
                if(myState.first + myState.second >= collection.GetSize())
                {
                    myState.second = MIN(collection.GetSize() - myState.first, 10000000);
                    if(myState.second == 0)
                    {
                        myState.first = myState.second = 0;
                    }
                }
                for(U32 i = myState.first; i < myState.first + myState.second; i++)
                {
                    String key{};
                    if(collection.IsKeyedCollection())
                    {
                        Meta::ClassInstance keyi = collection.GetKey(i);
                        key = Meta::PerformToString(keyi);
                        if(Meta::Is(keyi, "String") || Meta::Is(keyi, "class String"))
                        {
                            key = "\"" + key + "\"";
                        }
                    }
                    else
                    {
                        key = std::to_string(i);
                    }
                    PropAction action = _RenderPropItem(currentY, indentX + INDENT_SPACING, key.c_str(),
                                    collection.GetValue(i), false, bClassProp, stackPath + key, false, true);
                    if(action == PropAction::REMOVE)
                    {
                        removeindex = i;
                    }
                }
                if(removeindex != collection.GetSize() && !collection.IsStaticArray())
                {
                    Meta::ClassInstance _{};
                    collection.Pop(removeindex, _, _);
                }
            }
            
            // save state
            _CollectionViewState[myPath] = myState;
            
        }
        return true;
    }

    // OTHERS, ENSURE *AS SOON AS* RENDER DROPDOWN IS CALLED, ACTION OUT IS UPDATED, REGARDLESS OF WETHER IT RETURNED TRUE/FALSE

    // FINALLY
    if(cls.Members.size() > 0)
    {
        if(bDoRender)
        {
            _RenderClassMembers(currentY, indentX, stackPath + name, cls, value, name, bClassProp, actionOut, bCollection);
        }
        return true;
    }
    return false;
}

extern ImGuiContext* GImGui;

PropAction UIPropertySet::_RenderPropActionContext(const Meta::Class& cls, const String& keyName, Bool bClassProp, Bool bCollectionVal)
{
    PropAction action = PropAction::NONE;
    if(bClassProp && bCollectionVal)
        return action;
    Bool dis = GImGui->DisabledStackSize > 0;
    if (dis)
        ImGui::EndDisabled();
    if(ImGui::BeginPopupContextItem("##propitem", 1, ImGuiHoveredFlags_AllowWhenDisabled))
    {
        if(bCollectionVal)
        {
            if (ImGui::MenuItem(GetApplication().GetLanguageText("misc.remove_element").c_str()))
            {
                action = PropAction::REMOVE;
            }
        }
        else
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
                Meta::ClassInstance actualProp = GetMetaObject();
                if(!actualProp)
                    actualProp = _GetRawScene()->GetAgentProps(Agent);
                
                if (!PropertySet::IsKeyLocal(actualProp, keyName, GetApplication().GetRegistry()) && ImGui::MenuItem(GetApplication().GetLanguageText("misc.make_local").c_str()))
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
        }
        ImGui::EndPopup();
    }
    if (dis)
        ImGui::BeginDisabled();
    return action;
}

void UIPropertySet::_RenderSetCursorForInlineValue(Float indentX, Float currentY)
{
    ImGui::SetCursorPos(ImVec2{ MAX(indentX + 10.0f, VALUE_START_X), currentY + 1.0f });
}

PropAction UIPropertySet::_RenderPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, Bool red, Bool bClassProp, String stackPath, Bool bIsClassMember, Bool bIsCollectionMem)
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
    Bool bHandledByDropdown = !bSingle && value;
    if(bHandledByDropdown)
    {
        _RowNum--;
        bHandledByDropdown = _RenderDropdownPropItem(currentY, indentX, name, value, stackPath, true, bClassProp, action, false);
    }
    if(bSingle || !bHandledByDropdown)
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
                    action = _RenderPropActionContext(Meta::GetClass(value.GetClassID()), name, bClassProp, bIsCollectionMem);
                _RenderSetCursorForInlineValue(indentX, currentY);
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
                GetRuntimeSymbols().Register(localName);
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
            ImGui::TextUnformatted(text.c_str());
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
