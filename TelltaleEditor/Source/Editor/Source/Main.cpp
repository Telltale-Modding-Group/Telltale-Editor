#include <Core/Context.hpp>
#include <Meta/Meta.hpp>

int main()
{

    {
        
        ToolContext& Context = *CreateToolContext();
        
        Context.Switch({"BN100","MacOS",""}); // use bone for now, as it has some more functionality
        
        DataStreamRef stream = Context.LoadLibraryResource("menuitem.prop");
        Meta::ClassInstance instance = Meta::ReadMetaStream(stream); // read the file
        
        DestroyToolContext();
    }

    return 0;
}
