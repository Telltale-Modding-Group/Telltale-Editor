#include <Core/Context.hpp>
#include <Resource/TTArchive2.hpp>

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
static void RunApp(Bool runUI)
{
	{
		TelltaleEditor editor{{"BN100", "MacOS", ""}, runUI};
		Bool running;
		do
		{
			running = editor.Update(false);
		}
		while(running);
	}
	DumpTrackedMemory();
}

int main()
{
	RunApp(false);
    return 0;
}
