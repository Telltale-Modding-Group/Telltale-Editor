#include <Core/Context.hpp>

int main()
{

    {
        
        ToolContext Context{};
        
        Context.Switch({"TEXAS","PC",""});
        
        String src = Context.LoadLibraryStringResource("Meta.lua");
        if(src.length() == 0)
            TTE_LOG("Could not load meta.lua");
        else
            ScriptManager::RunText(Context.GetLibraryLVM(), src);
        
    }

    return 0;
}
