#include <UI/UIEditors.hpp>
#include <UI/ApplicationUI.hpp>
#include <imgui.h>
DECL_VEC_ADDITION();

UIPropertySet::UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop) : UIResourceEditor<>(ui, prop, nullptr, title)
{

}

UIPropertySet::UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop, Meta::ParentWeakReference p)
    : UIResourceEditor<>(ui, prop, p, nullptr, title)
{

}

#define INDENT_SPACING 20.0f
#define LINE_HEIGHT 17.0f
#define VALUE_START_X 300.0f

void UIPropertySet::_RenderProp(Float& currentY, Float indentX, CString name, Meta::ClassInstance prop)
{
    ImGui::PushID(name);
    ImVec2 posBase = ImGui::GetWindowPos() + ImVec2{0.0f, currentY - ImGui::GetScrollY()};
    if((_RowNum++) & 1)
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase +
                                                  ImVec2{ImGui::GetWindowSize().x, LINE_HEIGHT}, IM_COL32(46, 46, 46, 255));
    }
    else
    {
        ImGui::GetWindowDrawList()->AddRectFilled(posBase, posBase +
                                                  ImVec2{ImGui::GetWindowSize().x, LINE_HEIGHT}, IM_COL32(58, 58, 58, 255));
    }
    ImVec2 posImage = posBase + ImVec2{ 2.0f, 2.0f };
    ImVec2 szImage = ImVec2{ INDENT_SPACING - 4.0f, INDENT_SPACING - 4.0f };
    Bool open = _OpenDrops.find(name) != _OpenDrops.end();
    Bool hov = ImGui::IsMouseHoveringRect(posImage, posImage + szImage);
    DrawResourceTexturePixels(open ? "Misc/DownArrow.png" : "Misc/RightArrow.png", posImage.x - ImGui::GetWindowPos().x, posImage.y - ImGui::GetWindowPos().y, szImage.x, szImage.y, hov ? IM_COL32(125, 125, 125, 255) : 0xFFFFFFFFu);
    if(hov && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (open)
            _OpenDrops.erase(_OpenDrops.find(name));
        else
            _OpenDrops.insert(name);
        open = !open;
    }
    ImGui::SetCursorPos(ImVec2{indentX + INDENT_SPACING, currentY + 4.0f});
    ImGui::TextUnformatted(name);
    currentY += LINE_HEIGHT;
    if(open)
    {
        for (const auto& entry : *prop._GetInternalChildrenRefs())
        {
            String propName = GetRuntimeSymbols().FindOrHashString(entry.first);
            _RenderPropItem(currentY, indentX + INDENT_SPACING, propName.c_str(), entry.second, propName.empty());
        }
    }
    ImGui::PopID();
}

void UIPropertySet::_RenderPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, Bool red)
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
    ImGui::SetCursorPos(ImVec2{indentX + INDENT_SPACING, currentY + 4.0f});
    ImGui::TextUnformatted(name);
    if(value)
    {

        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted(Meta::GetClass(value.GetClassID()).Name.c_str());
            ImGui::EndTooltip();
        }


        ImGui::SetCursorPos(ImVec2{MAX(indentX + 10.0f, VALUE_START_X), currentY + 1.0f});
        const Meta::Class& cls = Meta::GetClass(value.GetClassID());
        const PropertyRenderInstruction* Instruction = PropertyRenderInstructions;
        Bool ok = false;
        ImGui::PushID(COERCE(&currentY, int));
        while(Instruction->Name)
        {
            CString iname = Instruction->Name;
            if (String(iname) == "Colour")
                iname = "Color"; // America WHY
            if(cls.ToolTypeHash == Symbol(iname))
            {
                PropertyVisualAdapter adapter{};
                adapter.Flags = PropertyVisualAdapter::NO_REPOSITION;
                adapter.ClassID = cls.ClassID;
                adapter.RenderInstruction = Instruction;
                Instruction->Render(_EditorUI, adapter, value);
                ok = true;
                break;
            }
            Instruction++;
        }
        if(!ok)
        {
            ImGui::Text("I need some help");
        }
        ImGui::PopID();
    }
    
    currentY += LINE_HEIGHT;
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
            if(ImGui::BeginMenu("Add Locals"))
            {
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Globals"))
            {
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
        Float currentY = 40.0f;
        _RenderProp(currentY, 0.0f, GetApplication().GetLanguageText("misc.locals").c_str(), propRoot);
        std::set<HandlePropertySet> parents{};
        PropertySet::GetParents(propRoot, parents, true, GetApplication().GetRegistry());
        for(auto parent: parents)
        {
            String name = parent.GetObjectResolved();
            if(!name.empty())
            {
                Meta::ClassInstance parentProp = parent.GetObject(GetApplication().GetRegistry(), true);
                if(parentProp)
                    _RenderProp(currentY, 0.0f, name.c_str(), parentProp);
                else
                    _RenderPropItem(currentY, 0.0f, name.c_str(), {}, true);
            }
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        ImGui::PopFont();
    }
    ImGui::End();
    return closing;
}
