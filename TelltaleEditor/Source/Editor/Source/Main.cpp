#include <Core/Context.hpp>

#include <Common/Scene.hpp> 
#include <Renderer/RenderContext.hpp>
#include <Runtime/SceneRuntime.hpp>
#include <Meta/Meta.hpp>
#include <AnimationManager.hpp>
#include <TelltaleEditor.hpp>

// Run full application, with optional GUI
I32 CommandLine::Executor_Editor(const std::vector<TaskArgument>& args)
{
    {
        TelltaleEditor* editor = CreateEditorContext({"BN100","MacOS",""}, false); // editor. dont run UI yet (doesn't exist)
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
            
            //SceneModuleTypes types{};
            //types.Set(SceneModuleType::RENDERABLE, true);
            //types.Set(SceneModuleType::SKELETON, true);
            //scene.AddAgent("Agent", types, {});
            
            // 4. set agent module info
            //scene.GetAgentModule<SceneModuleType::RENDERABLE>("Agent").Renderable.AddMesh(registry, hMesh);
            //scene.GetAgentModule<SceneModuleType::SKELETON>("Agent").Skl.SetObject(registry, "sk03_thorn.skl", true, true);
            
            // 5. push scene to renderer
            runtimeLayer.lock()->PushScene(std::move(*pScene)); // push scene to render
            
            // 6. Run renderer and show the mesh!
            context.CapFrameRate(40); // 40 FPS cap
            Bool running = true;
            while((running = context.FrameUpdate(!running)))
                ;
        }
        FreeEditorContext();
    }
    return 0;
}

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv);
    Memory::DumpTrackedMemory();
    return exit;
}
