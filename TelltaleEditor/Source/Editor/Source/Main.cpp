#include <TelltaleEditor.hpp>
#include <Common/Scene.hpp> 
#include <Renderer/RenderContext.hpp>
#include <Runtime/SceneRuntime.hpp>
#include <UI/ApplicationUI.hpp>

static void _TestScene(TelltaleEditor* editor)
{
    // This simple examples loads a scene and runs it
    RenderContext context("Bone: Out from Boneville");
    
    // Create resource system and attach to render context for runtime
    Ptr<ResourceRegistry> registry = editor->CreateResourceRegistry();
    registry->MountSystem("<Archives>/", "/Users/lucassaragosa/Desktop/Game/Bone", true);
    registry->PrintLocations();
    
    // Add a scene runtime layer to run the scene
    auto runtimeLayer = context.PushLayer<SceneRuntime>(registry);
    
    // 2. load a mesh
    //Handle<Mesh::MeshInstance> hMesh{};
    //hMesh.SetObject(registry, "sk03_thorn.d3dmesh", false, true);
    
    // 3. create a dummy scene, add an agent, attach a renderable module to it.
    Handle<Scene> hScene{};
    hScene.SetObject(registry, "ui_adventure.scene", false, false);
    Ptr<Scene> pScene = hScene.GetObject(registry, true);
    
    
    // 5. push scene to renderer
    runtimeLayer.lock()->PushScene(std::move(*pScene)); // push scene to render
    
    // 6. Run renderer and show the mesh!
    context.CapFrameRate(40); // 40 FPS cap
    Bool running = true;
    while((running = context.FrameUpdate(!running)))
        ;
}

// Run full application
I32 CommandLine::Executor_Editor(const std::vector<TaskArgument>& args)
{
    ApplicationUI App{};
    return App.Run(args);
}

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv);
    Memory::DumpTrackedMemory();
    return exit;
}
