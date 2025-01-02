#include <Core/Context.hpp>

int main()
{

    {

        ToolContext *Context = CreateToolContext();

        Context->Switch({"TX100", "PC", ""});

        DestroyToolContext();
    }

    return 0;
}
