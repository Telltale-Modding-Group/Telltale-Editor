#include <Core/Context.hpp>
#include <Meta/Meta.hpp>

int main()
{

    {
        
        ToolContext& Context = *CreateToolContext();
        
        // for now assume user has called 'editorui.exe "../../Dev/mod.lua"
        String src = Context.LoadLibraryStringResource("mod.lua");
        Context.GetLibraryLVM().RunText(src.c_str(), (U32)src.length(), false); // dont lock the context, allow any modding.
        Context.GetLibraryLVM().GC(); // gc after
        
        DestroyToolContext();
    }

    return 0;
}
