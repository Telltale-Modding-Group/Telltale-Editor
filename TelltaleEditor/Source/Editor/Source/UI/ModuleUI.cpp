#include <UI/ApplicationUI.hpp>
#include <Common/Scene.hpp>

#include <imgui.h>
#include <imgui_internal.h>

using namespace ImGui;

DECL_VEC_ADDITION();

static Bool BeginModule(EditorUI& editor, const char* title, Float bodyHeight, Bool& isOpen, const char* icon, Bool* pAddModule = nullptr)
{
    ImVec2 wpos = ImGui::GetWindowPos() + ImVec2{ 0.0f, editor.InspectorViewY } + ImVec2{ 1.0f, 20.0f };
    ImVec2 wsize = ImGui::GetWindowSize() + ImVec2{ -15.0f, -20.0f };
    ImDrawList* list = ImGui::GetWindowDrawList();
    list->PushClipRect(wpos, wpos + wsize);

    const Float headerHeight = 20.0f;

    // Header background
    list->AddRectFilled(
        wpos,
        wpos + ImVec2(wsize.x, headerHeight),
        IM_COL32(70, 70, 70, 255)
    );

    // Header bottom border
    list->AddLine(
        wpos + ImVec2(0.0f, headerHeight),
        wpos + ImVec2(wsize.x, headerHeight),
        IM_COL32(30, 30, 30, 255)
    );

    // Arrow icon
    if (ImGui::IsMouseHoveringRect(wpos, wpos + ImVec2{ 18.0f, 18.0f }, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        isOpen = !isOpen;

    if (isOpen)
        list->AddTriangleFilled(
        wpos + ImVec2(6.0f, 6.0f),
        wpos + ImVec2(14.0f, 6.0f),
        wpos + ImVec2(10.0f, 14.0f),
        IM_COL32(120, 120, 120, 255)
        );
    else
        list->AddTriangleFilled(
        wpos + ImVec2(6.0f, 6.0f),
        wpos + ImVec2(6.0f, 14.0f),
        wpos + ImVec2(14.0f, 10.0f),
        IM_COL32(120, 120, 120, 255)
        );

    if(icon)
    {
        editor.DrawResourceTexturePixels(icon, wpos.x + 15.0f - ImGui::GetWindowPos().x, wpos.y + 1.0f - ImGui::GetWindowPos().y, 18.0f, 18.0f);
    }

    // Title text
    list->AddText(
        wpos + ImVec2(icon ? 35.0f : 25.0f, 4.0f),
        IM_COL32(190, 190, 190, 255),
        title
    );

    if(pAddModule)
    {
        ImVec2 pos{ wpos.x + wsize.x - 20.0f - ImGui::GetWindowPos().x, wpos.y + 1.0f - ImGui::GetWindowPos().y };
        editor.DrawResourceTexturePixels("Misc/AddModule.png", pos.x, pos.y, 18.0f, 18.0f);
        if (ImGui::IsMouseHoveringRect(pos, pos + ImVec2{18.0f, 18.0f}, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            *pAddModule = true;
        }
    }

    if (isOpen)
    {
        // Body BG
        list->AddRectFilled(
            wpos + ImVec2(0.0f, headerHeight),
            wpos + ImVec2(wsize.x, bodyHeight + headerHeight),
            IM_COL32(60, 60, 60, 255)
        );
        list->AddLine(
            wpos + ImVec2(0.0f, headerHeight),
            wpos + ImVec2(wsize.x, headerHeight),
            IM_COL32(45, 45, 45, 255)
        );
        // Full border
        list->AddRect(
            wpos,
            wpos + ImVec2(wsize.x, bodyHeight + headerHeight),
            IM_COL32(40, 40, 40, 255)
        );
    }
    else
    {
        // Full border
        list->AddRect(
            wpos,
            wpos + ImVec2(wsize.x, headerHeight),
            IM_COL32(40, 40, 40, 255)
        );
    }

    list->PopClipRect();
    
    ImGui::SetCursorScreenPos(wpos + ImVec2{5.0f, headerHeight + 8.0f});
    editor.InspectorViewY += headerHeight + (isOpen ? bodyHeight : 0.0f);

    return isOpen;
}

static Bool DrawVector3Input(const char* label, float values[3])
{
    ImGui::PushID(label);

    ImGui::Text("%s", label);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Compact style for inputs
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

    bool changed = false;
    const float item_width = 55.0f;
    const ImVec2 label_size = ImGui::CalcTextSize("X");
    const char Txt[] = "X\0Y\0Z\0";

    // Colors for RGB
    ImU32 colors[3] = {
        IM_COL32(200,  40,  40, 255),  // red background
        IM_COL32(40, 200,  40, 255),   // green background
        IM_COL32(40,  40, 200, 255)    // blue background
    };

    ImGui::SetCursorPos(ImVec2(8.0f, ImGui::GetCursorPosY())); // start x=8, keep y

    for (int i = 0; i < 3; i++)
    {
        ImVec2 pos = ImGui::GetCursorPos();  // relative to window content

        ImVec2 rect_size = ImVec2(label_size.x + 8, label_size.y + 4);

        // Convert pos to screen pos for drawing rect/text
        ImVec2 screen_pos = ImGui::GetCursorScreenPos();

        drawList->AddRectFilled(screen_pos, screen_pos + rect_size, colors[i], 0.0f);

        ImVec2 text_pos = ImVec2(
            screen_pos.x + (rect_size.x - label_size.x) * 0.5f,
            screen_pos.y + (rect_size.y - label_size.y) * 0.5f + 1.0f
        );

        drawList->AddText(text_pos, IM_COL32(255, 255, 255, 255), Txt + (i << 1));

        // Move cursor right after colored box (relative)
        ImGui::SetCursorPosX(pos.x + rect_size.x);

        ImGui::PushItemWidth(item_width);

        if (ImGui::InputFloat(
            (std::string("##") + label + (char)('X' + i)).c_str(),
            &values[i],
            0.0f, 0.0f, "%.3f",
            ImGuiInputTextFlags_CharsDecimal))
        {
            changed = true;
        }
        ImGui::PopItemWidth();

        if (i < 2)
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2.0f); // gap between groups
        }
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();

    return changed;
}

void InspectorView::RenderNode()
{
    static Bool IsOpen = true;
    Ptr<Node> pNode = _EditorUI.InspectingNode.lock();
    Bool clicked = false;
    if(pNode && BeginModule(_EditorUI, "Node", 100.0f, IsOpen, "Module/TransformGizmo.png", &clicked))
    {
        if(clicked)
        {
            TTE_LOG("new mod");
        }
        else
        {
            Transform orig = pNode->LocalTransform;
            Float* pos = (Float*)&pNode->LocalTransform._Trans;
            Bool bChanged = DrawVector3Input("Position", (Float*)&pNode->LocalTransform._Trans);
            Vector3 Euler{};
            pNode->LocalTransform._Rot.GetEuler(Euler);
            Euler *= Vector3(180.0f / 3.14159265f);
            if (DrawVector3Input("Rotation", (Float*)&Euler))
                bChanged = true;
            if (bChanged)
            {
                Transform newTransform = pNode->LocalTransform;
                Euler /= Vector3(180.0f / 3.14159265f);
                newTransform._Rot.SetEuler(Euler.x, Euler.y, Euler.z);
                pNode->LocalTransform = orig;
                Scene::UpdateNodeLocalTransform(pNode, newTransform, true);
            }
        }
    }
}

void SceneModule<SceneModuleType::RENDERABLE>::RenderUI(EditorUI& editor, SceneAgent& agent)
{
    static Bool IsOpen = false;
    if (BeginModule(editor, "Renderable", 50.0f, IsOpen, "Module/Renderable.png"))
    {
        Text("RENDERABLE");
    }
}

void SceneModule<SceneModuleType::SKELETON>::RenderUI(EditorUI& editor, SceneAgent& agent)
{
    Text("SKELETON");
}