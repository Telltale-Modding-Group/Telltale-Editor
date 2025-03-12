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
			
			// Loading and previewing a mesh example
			
			// 1. load a mesh
			DataStreamRef stream = editor.LoadLibraryResource("TestResources/adv_forestWaterfall.d3dmesh");
			DataStreamRef debugStream = editor.LoadLibraryResource("TestResources/debug.txt");
			Meta::ClassInstance inst = Meta::ReadMetaStream(stream, std::move(debugStream));
			
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

static void TestResources()
{
	{
		
		TelltaleEditor editor{{"MC100","MacOS",""}, false}; // initialise library and editor with no UI
		
		// create a testing resource registry
		Ptr<ResourceRegistry> registry = editor.CreateResourceRegistry();
		
		registry->MountSystem("<Archives>/", "/Users/lucassaragosa/Desktop/Game/MC100.app/Contents/Resources");
		registry->PrintSets();
		
	}
	DumpTrackedMemory();
}

int main()
{
	TestResources();
	return 0;
}
