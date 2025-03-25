#include <Core/Context.hpp>

#include <Common/Scene.hpp> 
#include <Renderer/RenderContext.hpp>
#include <Runtime/SceneRuntime.hpp>
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
static void RunRender()
{
    {
        TelltaleEditor editor{{"BN100","MacOS",""}, false}; // editor. dont run UI yet (doesn't exist)
        {
            // This simple examples loads a scene and runs it
            RenderContext context("Bone: Out from Boneville");
            
            // Create resource system and attach to render context for runtime
            Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
            registry->MountSystem("<Archives>/", "/Users/lucassaragosa/Desktop/Game/Bone");
            registry->PrintLocations();
            
            // Add a scene runtime layer to run the scene
            auto runtimeLayer = context.PushLayer<SceneRuntime>(registry);
            
            // 1. preload all textures
            /*std::set<String> tex{};
            StringMask m("*.d3dtx");
            registry->GetResourceNames(tex, &m);
            std::vector<HandleBase> handles{};
            for(auto& t: tex)
            {
                auto& handle = handles.emplace_back();
                handle.SetObject<RenderTexture>(registry, t, false, false); // no unload, no ensure load yet as preload will
            }
            U32 preload = registry->Preload(std::move(handles), false);
            registry->WaitPreload(preload); // preload and wait for it
             */
            
            // 2. load a mesh
            Handle<Mesh::MeshInstance> hMesh{};
            hMesh.SetObject(registry, "obj_woodsplita.d3dmesh", false, true);
            
            // 3. create a dummy scene, add an agent, attach a renderable module to it.
            Scene scene{};
            SceneModuleTypes types{};
            types.Set(SceneModuleType::RENDERABLE, true);
            scene.AddAgent("Mesh Test Agent", types);
            
            // 4. set renderable agent mesh
            scene.GetAgentModule<SceneModuleType::RENDERABLE>("Mesh Test Agent").Renderable.AddMesh(registry, hMesh);
            
            // 5. push scene to renderer
            runtimeLayer.lock()->PushScene(std::move(scene)); // push scene to render
            
            // 6. Run renderer and show the mesh!
            context.CapFrameRate(40); // 40 FPS cap
            Bool running = true;
            while((running = context.FrameUpdate(!running)))
                ;
        }
    }
    DumpTrackedMemory();
}

// TESTS: load all of given type mask in archive
static void LoadAll(CString mask)
{
    {
        TelltaleEditor editor{{"CSI3","PC",""}, false}; // editor. dont run UI yet (doesn't exist)
        
        Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
        registry->MountSystem("<Data>/", "/Users/lucassaragosa/Desktop/extract");
        
        //editor.EnqueueResourceLocationExtractTask(registry, "<ISO>/", "/users/lucassaragosa/desktop/extract", "", true);
        //editor.Wait();
        
        std::set<String> wExt{};
        StringMask sMask = mask;
        registry->GetResourceNames(wExt, &sMask);
        
        for(auto& fn: wExt)
        {
            DataStreamRef ref = registry->FindResource(fn);
            if(ref)
            {
                DataStreamRef oref = editor.LoadLibraryResource("TestResources/Dbg/Decrypt/" + fn);
                DataStreamRef dref = Meta::MapDecryptingStream(ref);
                DataStreamManager::GetInstance()->Transfer(dref, oref, dref->GetSize());
                ref->SetPosition(0);
                Meta::ReadMetaStream(fn, ref, editor.LoadLibraryResource("TestResources/Dbg/debug_" + fn + ".txt"));
            }
            else
            {
                TTE_LOG("No stream for %s", fn.c_str());
            }
        }
        
        registry->PrintLocations();
        
    }
}

static void GenerateFilesSymMap(CString mount, GameSnapshot snapshot)
{
    {
        TelltaleEditor editor{snapshot, false}; // editor. dont run UI yet (doesn't exist)
        
        Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
        registry->MountSystem("<Data>/", mount);
        
        std::set<String> all{};
        registry->GetResourceNames(all, nullptr);
        
        DataStreamRef out = editor.LoadLibraryResource("Dump.symmap");
        SymbolTable t(true);
        for(auto& f: all)
            t.Register(f);
        t.SerialiseOut(out);
        
        TTE_LOG("Done");
        
    }
}

int main()
{
    //LoadAll("!*.lenc");
    RunRender();
    //GenerateFilesSymMap("/users/lucassaragosa/desktop/game/bone", {"BN100","MacOS", ""});
    return 0;
}
