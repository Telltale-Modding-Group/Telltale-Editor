#include <Core/Context.hpp>
#include <Resource/TTArchive2.hpp>

void RunMod(ToolContext& Context)
{
    // for now assume user has called 'editorui.exe "../../Dev/mod.lua"
    String src = Context.LoadLibraryStringResource("mod.lua");
    Context.GetLibraryLVM().RunText(src.c_str(), (U32)src.length(), false); // dont lock the context, allow any modding.
    Context.GetLibraryLVM().GC(); // gc after
}

int main()
{
    
    {
        
        ToolContext& Context = *CreateToolContext();
        RunMod(Context);
        
        DestroyToolContext();
        
    }
    
    DumpTrackedMemory(); // dump all memort leaks. after destroy context, nothing should exist

    return 0;
}
