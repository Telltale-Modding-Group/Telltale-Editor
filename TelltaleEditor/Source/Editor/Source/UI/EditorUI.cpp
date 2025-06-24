#include <UI/EditorUI.hpp>
#include <UI/ApplicationUI.hpp>

#include <imgui.h>

using namespace ImGui;

EditorUI::EditorUI(ApplicationUI& app) : UIStackable(app), _MenuBar(app)
{
    SDL_SetWindowResizable(GetApplication()._Window, true);
}

void EditorUI::Render()
{
    _MenuBar.Render();
    Text("Helloworld");
}
