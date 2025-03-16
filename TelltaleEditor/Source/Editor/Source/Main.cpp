#include <Core/Context.hpp>

#include <Common/Scene.hpp> 
#include <Renderer/RenderContext.hpp>
#include <Meta/Meta.hpp>

#include <TelltaleEditor.hpp>

// Simply run mod.lua
static void RunMod()
{
    
    ToolContext* Context = CreateToolContext();
    Context->Switch({"BN100", "MacOS", ""});
    
    {

        // for now assume user has called 'editorui.exe "../../Dev/mod.lua"
        String src = Context->LoadLibraryStringResource("mod.lua");
        Context->GetLibraryLVM().RunText(src.c_str(), (U32)src.length(), false, "mod.lua"); // dont lock the context, allow any modding.
        Context->GetLibraryLVM().GC(); // gc after
        
    }
    
    DestroyToolContext();
    
    DumpTrackedMemory(); // dump all memort leaks. after destroy context, nothing should exist
}

// Run full application, with optional GUI
static void RunApp()
{
    {
        TelltaleEditor editor{{"BN100","MacOS",""}, false}; // editor. dont run UI yet (doesn't exist)
        {
            // This simple examples loads a scene and runs it
            RenderContext context("Bone: Out from Boneville");
            
            // Create resource system and attach to render context for runtime
            Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
            registry->MountSystem("<Archives>/", "/Users/lucassaragosa/Desktop/Game/Bone-Out from Boneville.app/Contents/Resources");
            registry->PrintLocations();
            
            context.AttachResourceRegistry(registry);
            
            // Loading and previewing a mesh example
            
            // load all textures.
            std::set<String> tex{};
            StringMask m("*.d3dtx");
            registry->GetResourceNames(tex, &m);
            std::vector<Ptr<RenderTexture>> loadedTextures{};
            for(auto& t: tex)
            {
                auto s = registry->FindResource(t);
                Meta::ClassInstance textureInstance{};
                if( (textureInstance = Meta::ReadMetaStream(t, s)) )
                {
                    Ptr<RenderTexture> tex = TTE_NEW_PTR(RenderTexture, MEMORY_TAG_TEMPORARY);
                    loadedTextures.push_back(tex);
                    editor.EnqueueNormaliseTextureTask(textureInstance, tex);
                }
                else TTE_LOG("Failed %s", t.c_str());
            }
            
            // 1. load a mesh
            DataStreamRef stream = registry->FindResource("adv_forestWaterfall.d3dmesh");
            DataStreamRef debugStream = editor.LoadLibraryResource("TestResources/debug.txt");
            Meta::ClassInstance inst = Meta::ReadMetaStream("adv_forestWaterfall.d3dmesh", stream, std::move(debugStream));
            
            // 2. create a dummy scene, add an agent, attach a renderable module to it.
            Scene scene{};
            SceneModuleTypes types{};
            types.Set(SceneModuleType::RENDERABLE, true);
            scene.AddAgent("Mesh Test Agent", types);
            
            // 3. normalise the mesh above into the scene agent 'Icon' in the scene, from telltale specific to the common format
            editor.EnqueueNormaliseMeshTask(&scene, "Mesh Test Agent", std::move(inst));
            editor.Wait();
            
            // 4. push scene to renderer

            context.PushScene(std::move(scene)); // push scene to render
            
            // 5. Run renderer and show the mesh!
            context.CapFrameRate(40); // 40 FPS cap
            Bool running = true;
            while((running = context.FrameUpdate(!running)))
                ;
        }
    }
    DumpTrackedMemory();
}

static void TestNew()
{
    {
        TelltaleEditor editor{{"BN100","MacOS",""}, false}; // editor. dont run UI yet (doesn't exist)
        
        Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
        registry->MountArchive("<PS2_Data>/", "/Users/lucassaragosa/Desktop/Game/CSI3/CSI3_PS2.iso");
        
        std::set<String> fs{};
        registry->GetResourceNames(fs, 0);
        
        for(auto& f: fs)
            TTE_LOG(f.c_str());
        
    }
}

int main()
{
    TestNew();
    return 0;
}
