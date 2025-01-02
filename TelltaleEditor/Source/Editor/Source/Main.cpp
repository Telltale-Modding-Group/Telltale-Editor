#include <Core/Context.hpp>
#include <Resource/TTArchive.hpp>

int main()
{

    {
        ToolContext *Context = CreateToolContext();
        
        Context->Switch({"BN100","MacOS",""}); // use bone for now, as it has some more functionality
        // Context->Switch({"TX100", "PC", ""});
        // If you have the file in the Dev dir uncomment this. its too large for git! but it works on my machine...
        /* open testing archive
        TTArchive test{0}; // eventually passed in verison to be decided by lua script.
        DataStreamRef stream = Context.LoadLibraryResource("BONEVILLE.DATA.ttarch");
        test.SerialiseIn(stream);
        
        // testing log all files
        std::vector<String> filenames{};
        test.GetFiles(filenames);
        for(auto& file: filenames)
            TTE_LOG(file.c_str()); */

    
        DestroyToolContext();
    }

    return 0;
}
