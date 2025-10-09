#include <UI/ApplicationUI.hpp>

#include <imgui.h>

DECL_VEC_ADDITION();

MenuBar::MenuBar(EditorUI& ui) : UIComponent(ui.GetApplication()), _Editor(ui) {}

void MenuBar::Render()
{
    // BEGIN
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_Always);
    SetNextWindowViewport(0.0f, 0.0f, 0, 0, 1.0f, 0.04f, 0, 40, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.0f, 0.0f, 0.0f, 1.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("#menubar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
                                    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);

    // BACKGROUND
    ImVec2 size = ImGui::GetWindowSize();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(ImGui::GetWindowViewport()->Pos, ImGui::GetWindowViewport()->Pos + size,
                                                        IM_COL32(120, 14, 57,255), 0xff000000u, 0xff000000u, IM_COL32(120, 14, 57, 255));

    // LOGO AND TITLE BAR
    DrawResourceTexture("LogoSquare.png", 2.f / size.x, 2.f / size.y, 36.f / size.x, 36.f / size.y);
    const String title = "Telltale Editor";
    ImGui::PushFont(GetApplication().GetEditorFont(), 20.0f);
    ImGui::SetCursorPos({ 50.0f, size.y * 0.5f - ImGui::CalcTextSize(title.c_str()).y * 0.5f });
    ImGui::TextUnformatted(title.c_str());
    ImGui::PopFont();
    ImGui::PushFont(GetApplication().GetEditorFont(), 10.0f);
    String proj = GetApplication().GetProjectManager().GetHeadProject()->ProjectName + " [" + GetApplication().GetProjectManager().GetHeadProject()->ProjectFile.filename().string() + "]";
    if(!_Editor.GetActiveScene().GetName().empty())
    {
        proj = proj + " | " + _Editor.GetActiveScene().GetName();
    }
    else if(!_Editor._LoadingAsyncScene.empty())
    {
        proj = proj + " | Loading " + _Editor._LoadingAsyncScene + "...";
    }
    else
    {
        proj = proj + " | No Scene Loaded";
    }
    ImVec2 projNameSize = ImGui::CalcTextSize(proj.c_str());
    ImGui::SetCursorPos({ size.x * 0.5f - 0.5f * projNameSize.x, size.y * 0.5f - projNameSize.y * 0.5f });
    ImGui::TextUnformatted(proj.c_str());
    ImGui::PopFont();

    // END
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
}
