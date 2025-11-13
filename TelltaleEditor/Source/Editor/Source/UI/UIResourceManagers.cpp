#include <UI/UIEditors.hpp>
#include <UI/ApplicationUI.hpp>
#include <imgui.h>
#include <imgui_internal.h>

DECL_VEC_ADDITION();

// ============ RESOURCE EDITOR BASE

Bool UIResourceEditorBase::SelectionBox(Float xPos, Float yPos, Float xSizePix, Float ySizePix, Bool state)
{
    ImVec2 posImage{ xPos, yPos };
    ImVec2 szImage{ xSizePix, ySizePix };
    Bool hov = ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage, false);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        state = !state;
    if (hov || state)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage, state ? IM_COL32(85, 197, 209, 100) : IM_COL32(65, 197, 209, 100));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage, state ? IM_COL32(74, 163, 173, 200) : IM_COL32(54, 163, 173, 200));
    }
    return state;
}

Bool UIResourceEditorBase::ArrowButton(Float xPos, Float yPos, Float xSizePix, Float ySizePix, U32 bgCol)
{
    ImVec2 wpos = ImGui::GetWindowPos();
    ImVec2 posImage{ xPos, yPos };
    posImage = posImage + wpos;
    ImVec2 szImage{ xSizePix, ySizePix };
    Bool clicked = false;
    Bool hov = ImGui::IsMouseHoveringRect(posImage, posImage + szImage, false);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        clicked = true;
    ImGui::GetWindowDrawList()->AddRectFilled(posImage, posImage + szImage, bgCol);
    ImGui::GetWindowDrawList()->AddRect(posImage, posImage + szImage, IM_COL32(200, 200, 200, 255));
    ImGui::GetWindowDrawList()->AddTriangleFilled(
                                                  posImage + ImVec2{szImage.x * 0.3f, szImage.y * 0.3f},
                                                  posImage + ImVec2{szImage.x * 0.5f, szImage.y * 0.6f},
                                                  posImage + ImVec2{szImage.x * 0.6f, szImage.y * 0.3f},
                                                  IM_COL32_BLACK);
    return clicked;
}

Bool UIResourceEditorBase::ImageButton(CString iconTex, Float xPos, Float yPos, Float xSizePix, Float ySizePix, U32 sc)
{
    ImVec2 posImage{ xPos, yPos };
    ImVec2 szImage{ xSizePix, ySizePix };
    Bool hov = ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + posImage, ImGui::GetWindowPos() + posImage + szImage);
    if(hov)
    {
        posImage.x = MAX(0.0f, posImage.x - 1.0f);
        posImage.y = MAX(0.0f, posImage.y - 1.0f);
        szImage.x += 2.0f;
        szImage.y += 2.0f;
    }
    DrawResourceTexturePixels(iconTex, posImage.x, posImage.y, szImage.x, szImage.y, sc);
    if (hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        return true;
    }
    return false;
}

// ============ TEXT FIELD POPUP

#define LINE_HEIGHT 19.0f

Bool TextFieldPopup::Render()
{
    ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2{ 10.0, 30.f });
    ImGui::TextUnformatted(_Prompt.c_str());
    ImGui::SetCursorScreenPos(ImVec2{ ImGui::GetWindowPos().x + 10.0f, ImGui::GetCursorScreenPos().y });
    ImGui::PushItemWidth(300.0f);
    ImGui::InputText("##input", &_InputText);
    ImGui::PopItemWidth();
    ImGui::SetCursorScreenPos(ImVec2{ ImGui::GetWindowPos().x + 10.0f, ImGui::GetCursorScreenPos().y });
    Bool dis = _InputText.empty(); Bool exit = false;
    if (dis)
        ImGui::BeginDisabled();
    if(ImGui::Button("OK"))
    {
        _Callback->CallErased(&_InputText, 0, 0, 0, 0, 0, 0, 0);
        exit = true;
    }
    if (dis)
        ImGui::EndDisabled();
    ImGui::SameLine();
    if(ImGui::Button("Cancel"))
    {
        exit = true;
    }
    return exit;
}

// ============ META INSTANCE POPUP

Bool MetaInstanceEditPopup::Render()
{
    if(!_Prompt.empty())
    {
        ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2{ 10.0, 30.f });
        ImGui::TextUnformatted(_Prompt.c_str());
    }
    ImGui::SetCursorScreenPos(ImVec2{ ImGui::GetWindowPos().x + 10.0f, ImGui::GetCursorScreenPos().y });
    ImGui::PushItemWidth(300.0f);
    const PropertyRenderInstruction* instruction = PropertyRenderInstructions;
    if(_Value)
    {
        const Meta::Class& valClass = Meta::GetClass(_Value.GetClassID());
        while(instruction->Name)
        {
            if(Symbol(instruction->Name).GetCRC64() == valClass.ToolTypeHash)
            {
                PropertyVisualAdapter adapter{};
                adapter.Flags = PropertyVisualAdapter::NO_REPOSITION;
                adapter.ClassID = _Value.GetClassID();
                adapter.RuntimeCache = _Cache;
                adapter.RenderInstruction = instruction;
                instruction->Render(_UI, adapter, _Value);
                _Cache = adapter.RuntimeCache;
                break;
            }
            instruction++;
        }
    }
    Bool ok = true;
    if(!_Value || instruction->Name == nullptr)
    {
        ok = false;
        ImGui::Text("<ERROR_NO_INSTRUCTION>:%s", _Value?Meta::GetClass(_Value.GetClassID()).Name.c_str():"NULL");
    }
    ImGui::PopItemWidth();
    ImGui::SetCursorScreenPos(ImVec2{ ImGui::GetWindowPos().x + 10.0f, ImGui::GetCursorScreenPos().y });
    Bool exit = false;
    if(ok && ImGui::Button("OK"))
    {
        _Callback->CallErased(&_Value, 0, &_ValueCollection, 0, 0, 0, 0, 0);
        exit = true;
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel"))
    {
        exit = true;
    }
    return exit;
}

// ============ RESOURCE PICKER POPUP

void ResourcePickerPopup::_RefreshItems(std::vector<String>& items)
{
    std::set<String> resources{};
    std::vector<String> locations{};
    Editor->GetApplication().GetRegistry()->GetResourceLocationNames(locations);
    for (const String& location : locations)
    {
        Editor->GetApplication().GetRegistry()->GetLocationResourceNames(location, resources, &Mask);
    }
    if (Mask.empty())
        items.reserve(resources.size());
    else
        items.reserve(100);
    for(const auto& value: resources)
    {
        if(Mask.empty() || StringMask::MatchSearchMask(value.c_str(), Mask.c_str(), StringMask::MASKMODE_ANY_SUBSTRING))
            items.push_back(value);
    }
}

void ResourcePickerPopup::_OnSelect(const String& selectedItem)
{
    String selected = selectedItem;
    CompletionCallback->CallErased(&selected, 0, 0, 0, 0, 0, 0, 0);
}

String ResourcePickerPopup::_GetToolTipText(const String& hoveredItem) 
{
    String loc = Editor->GetApplication().GetRegistry()->LocateConcreteResourceLocation(hoveredItem);
    if (!loc.empty())
    {
        return loc + hoveredItem;
    }
    return "";
}

// ============ META CLASS PICKER POPUP

void MetaClassPickerPopup::_OnSelect(const String& selectedItem)
{
    String selected = selectedItem;
    CompletionCallback->CallErased(&selected, 0, 0, 0, 0, 0, 0, 0);
}

String MetaClassPickerPopup::_GetToolTipText(const String& hoveredItem)
{
    // tool hash, version, hash, legacy hash
    const Meta::Class& cls = Meta::GetClass(Meta::FindClass(hoveredItem, 0, false));
    return "TH:0x" + SymbolToHexString(cls.ToolTypeHash) + ",V:0x" + StringFromInteger((I64)cls.VersionCRC, 16, false)
        + ",H:0x" + SymbolToHexString(cls.TypeHash) + ",LH:0x" + StringFromInteger((I64)cls.LegacyHash, 16, false);
}

struct ClassCompareIntrin
{
    Bool operator()(const String& lhs, const String& rhs) const
    {
        const Meta::Class& cl = Meta::GetClass(Meta::FindClass(lhs, 0, false));
        const Meta::Class& cr = Meta::GetClass(Meta::FindClass(rhs, 0, false));

        const Bool lhsIntrin = (cl.Flags & Meta::CLASS_INTRINSIC);
        const Bool rhsIntrin = (cr.Flags & Meta::CLASS_INTRINSIC);

        return lhsIntrin != rhsIntrin ? lhsIntrin : cl.Name < cr.Name;
    }
};

// for now only allow version number 0, can patch in future
void MetaClassPickerPopup::_RefreshItems(std::vector<String>& values)
{
    std::set<String, ClassCompareIntrin> classes{};
    std::vector<U32> classIDs = Meta::GetClassIDs();
    for(U32 id: classIDs)
    {
        const Meta::Class& cls = Meta::GetClass(id);
        if(cls.VersionNumber == 0 && (cls.Flags & Meta::CLASS_ABSTRACT) == 0 && !StringStartsWith(cls.Name, "__INTERNAL"))
        {
            classes.insert(MakeTypeName(cls.Name));
        }
    }
    values.reserve(classes.size());
    for (const auto& val : classes)
        values.push_back(val);
}

// ============ ABSTRACT LIST POPUP

Bool AbstractListSelectionPopup::Render()
{
    Bool exit = false;
    U32 rowNum = 0;
    U64 now = GetTimeStamp();
    if(_CacheTs == UINT64_MAX || (_RefreshRate > 0.0f && GetTimeStampDifference(_CacheTs, now) > _RefreshRate))
    {
        // update cache
        _ListItems.clear();
        _RefreshItems(_ListItems);
        _CacheTs = now;
    }
    ImVec2 posBase = ImGui::GetWindowPos() + ImVec2{ 5.0f, 65.0f - ImGui::GetScrollY() };
    ImGui::PushFont(ImGui::GetFont(), 10.0f);
    Float scrollSize = ImGui::GetWindowScrollbarRect(ImGui::GetCurrentWindow(), ImGuiAxis_Y).GetSize().x;
    ImVec2 boxSize = ImVec2{ ImGui::GetWindowSize().x - scrollSize - 5.0f - posBase.x + ImGui::GetWindowPos().x, LINE_HEIGHT };
    ImVec2 selPosBase{};
    U32 selIndex = 0;
    U32 curIndex = 0;
    for (const auto& resource : _ListItems)
    {
        if (StringMask::MatchSearchMask(resource.c_str(), UserMask.c_str(), StringMask::MASKMODE_ANY_SUBSTRING))
        {
            if ((rowNum++) & 1)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase + boxSize, IM_COL32(46, 46, 46, 255));
            }
            else
            {
                ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase + boxSize, IM_COL32(58, 58, 58, 255));
            }
            ImGui::SetCursorScreenPos(posBase + ImVec2{ 10.0f, 5.0f });
            ImGui::TextUnformatted(resource.c_str());
            if (posBase.y - ImGui::GetWindowPos().y >= 65.0f && ImGui::IsMouseHoveringRect(posBase, posBase + boxSize, false))
            {
                String tt = _GetToolTipText(resource);
                if (!tt.empty() && ImGui::BeginTooltip())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                    ImGui::TextUnformatted(tt.c_str());
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) || ImGui::IsKeyReleased(ImGuiKey_Enter))
                {
                    _SelectedItem = resource;
                    _OnSelect(_SelectedItem);
                    exit = true;
                }
                else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    _SelectedItem = resource;
                }
            }
            if (_SelectedItem == resource)
            {
                selIndex = curIndex;
                selPosBase = posBase;
            }
            posBase.y += LINE_HEIGHT;
        }
        curIndex++;
    }
    if(_ListItems.size())
    {
        if(ImGui::IsKeyReleased(ImGuiKey_DownArrow) || ImGui::IsKeyReleased(ImGuiKey_Tab))
        {
            selIndex = (selIndex + 1) % (U32)_ListItems.size();
        }
        else if(ImGui::IsKeyReleased(ImGuiKey_UpArrow))
        {
            selIndex = selIndex == 0 ? (U32)(_ListItems.size() - 1) : selIndex - 1;
        }
        _SelectedItem = _ListItems[selIndex];
    }
    ImGui::PopFont();
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + ImVec2{ 5.0f, 65.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - scrollSize - 5.0f, posBase.y - LINE_HEIGHT }, IM_COL32(120, 120, 120, 255));
    if (!_SelectedItem.empty() && selPosBase.y >= 65.0f)
        ImGui::GetWindowDrawList()->AddRect(selPosBase, selPosBase + boxSize, IM_COL32(100, 100, 200, 255));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x, 65.f }, IM_COL32(20, 20, 20, 255));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2{ 5.0f, 25.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - 20.0f, 60.0f }, IM_COL32(30, 30, 30, 255));
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowPos() + ImVec2{ 7.0f, 25.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - 22.0f, 58.0f }, IM_COL32(200,200,200, 255));
    ImGui::PushItemWidth(ImGui::GetWindowSize().x - 80.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2{ 10.0f, 32.0f });
    ImGui::InputTextWithHint("##input", Editor->GetLanguageText("misc.filter").c_str(), &UserMask);
    ImVec2 closeBase{ ImGui::GetWindowSize().x - 60.0f, 29.0f };
    ImVec2 closeSize{ 25.0f, 25.0f };
    Editor->DrawResourceTexturePixels("Misc/Close.png", closeBase.x, closeBase.y, closeSize.x, closeSize.y);
    if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + closeBase, ImGui::GetWindowPos() + closeBase + closeSize, false))
    {
        ImGui::GetWindowDrawList()->AddRect(closeBase + ImVec2{ -2.0f, -2.0f }, closeBase + closeSize + ImVec2{ 2.0f, 2.0f }, IM_COL32(130, 130, 190, 255));
        if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            exit = true;
        }
    }
    ImGui::SetCursorScreenPos(ImGui::GetWindowPos());
    ImGui::Dummy(ImVec2{ 1.0f,posBase.y * 0.5f });
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    return exit;
}
