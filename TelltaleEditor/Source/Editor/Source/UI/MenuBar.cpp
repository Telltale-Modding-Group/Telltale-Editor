#include <UI/ApplicationUI.hpp>

#include <imgui.h>

void MenuBar::Render()
{
    // BEGIN
    ImGui::SetNextWindowPos({0,0});
    SetNextWindowViewport(0.0f, 0.0f, 0, 0, 1.0f, 0.04f, 0, 40);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.0f, 0.0f, 0.0f, 1.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("#menubar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
                                    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);

    // BACKGROUND
    ImVec2 size = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor({0,0}, size, IM_COL32(107, 155, 199,255), 0xff000000u, 0xff000000u, IM_COL32(107, 155, 199, 255));

    // LOGO AND TITLE BAR
    DrawResourceTexture("LogoSquare.png", 0.f, 0.f, 40.f / size.x, 40.f / size.y);
    const String title = "The Telltale Editor (v" TTE_VERSION ")";
    ImGui::PushFont(GetApplication().GetEditorFontLarge());
    ImGui::SetCursorPos({ 50.0f, size.y * 0.5f - ImGui::CalcTextSize(title.c_str()).y * 0.5f });
    ImGui::Text(title.c_str());
    ImGui::PopFont();

    // END
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
}