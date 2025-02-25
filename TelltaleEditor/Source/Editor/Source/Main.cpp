#include <Core/Context.hpp>

#include <Scene.hpp>
#include <Renderer/RenderContext.hpp>

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
	CreateToolContext();
	GetToolContext()->Switch({"BN100","Macos",""});
	RenderContext::Initialise(); // multiple render contexts can be created. so a static initialisataion and shutdown is needed for SDL3
	
	{
		RenderContext context("Bone: Out from Boneville");
		
		// setup a scene
		Scene scene{};
		scene.SetName("adv_testing");
		
		context.PushScene(std::move(scene)); // push scene to render
		
		Bool running = true;
		while((running = context.FrameUpdate(!running)))
			;
	}
	
	RenderContext::Shutdown();
	DestroyToolContext();
	DumpTrackedMemory();
	abort(); // I KNOW. this is temp
}

int main()
{
	RunApp();
    return 0;
}
