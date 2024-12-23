#include <Core/Context.hpp>

int main()
{

    {
        
        ToolContext Context{};
        
        Context.Switch({"TX100","PC",""});
        
    }

    return 0;
}
