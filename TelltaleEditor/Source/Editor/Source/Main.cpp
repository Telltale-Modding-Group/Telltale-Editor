#include <TelltaleEditor.hpp>
#include <Common/Scene.hpp> 
#include <Renderer/RenderContext.hpp>
#include <Runtime/SceneRuntime.hpp>
#include <UI/ApplicationUI.hpp>

static void _TestScene(TelltaleEditor* editor)
{
    
    // Create resource system and attach to render context for runtime
    Ptr<ResourceRegistry> registry = editor->CreateResourceRegistry();
    registry->MountSystem("<Archives>/", "D:/Games/Bone - Out from Boneville/26_06_2007/Pack/data", true);
    registry->PrintLocations();

    // This simple examples loads a scene and runs it
    RenderContext* pRenderContext = TTE_NEW(RenderContext, MEMORY_TAG_RENDERER, "Bone", registry);
    
    // Add a scene runtime layer to run the scene
    auto runtimeLayer = pRenderContext->PushLayer<SceneRuntime>(registry);

    
    // 3. create a dummy scene, add an agent, attach a renderable module to it.
    Handle<Scene> hScene{};
    hScene.SetObject(registry, "ui_adventure.scene", false, false);
    Ptr<Scene> pScene = hScene.GetObject(registry, true);
    
    
    // 5. push scene to renderer
    runtimeLayer.lock()->PushScene(std::move(*pScene)); // push scene to render
    
    // 6. Run renderer and show the mesh!
    pRenderContext->CapFrameRate(40); // 40 FPS cap
    Bool running = true;
    while((running = pRenderContext->FrameUpdate(!running)))
        ;
    TTE_DEL(pRenderContext);
}

// Run full application
I32 CommandLine::Executor_Editor(const std::vector<TaskArgument>& args)
{
    _TestScene(CreateEditorContext({"BN100","PC","v2.0_Late"}));
    FreeEditorContext();
    return 0;
    //ApplicationUI App{};
    //return App.Run(args);
}

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv);
    Memory::DumpTrackedMemory();
    return exit;
}
