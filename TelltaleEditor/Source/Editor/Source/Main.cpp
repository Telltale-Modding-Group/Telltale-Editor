#include <Core/Context.hpp>
#include <Resource/TTArchive2.hpp>

#include <Renderer/RenderContext.hpp>

void RunMod(ToolContext* Context)
{
    
    {

        // for now assume user has called 'editorui.exe "../../Dev/mod.lua"
        String src = Context->LoadLibraryStringResource("mod.lua");
        Context->GetLibraryLVM().RunText(src.c_str(), (U32)src.length(), false, "mod.lua"); // dont lock the context, allow any modding.
        Context->GetLibraryLVM().GC(); // gc after
        
    }
    
    DumpTrackedMemory(); // dump all memort leaks. after destroy context, nothing should exist
}

int main()
{
    ToolContext* Context = CreateToolContext();
    Context->Switch({"BN100", "MacOS", ""});
    RenderContext::Initialise();
    
    {
        RenderContext testContext{"Bone: Out from Boneville"};
        
        Bool bQuit = false;
        while(!bQuit)
            bQuit = testContext.FrameUpdate(bQuit);
        
    }
    
    RenderContext::Shutdown();
    DestroyToolContext();
    return 0;
}
