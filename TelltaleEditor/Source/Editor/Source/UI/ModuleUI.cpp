#include <UI/ApplicationUI.hpp>
#include <Common/Scene.hpp>

#include <imgui.h>

using namespace ImGui;

DECL_VEC_ADDITION();

void SceneModule<SceneModuleType::RENDERABLE>::RenderUI(EditorUI& editor, SceneAgent& agent)
{
    Text("RENDERABLE");
    Separator();
}

void SceneModule<SceneModuleType::SKELETON>::RenderUI(EditorUI& editor, SceneAgent& agent)
{
    Text("SKELETON");
    Separator();
}