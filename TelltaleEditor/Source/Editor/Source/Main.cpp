#include <Core/Config.hpp>

#include <Resource/DataStream.hpp>

// ============================ TEMPORARY STUFF ============================



// ========================================================================

int main()
{

    // check build/telltaleeditor/source/editor/debug
    DataStreamManager::Initialise();
    
    ResourceURL url{"file:test.txt"};
    
    auto stream = url.Open();
    stream->Write((const U8*)"hello people", 12);
    
    DataStreamManager::Shutdown();

    TTE_LOG("Running TTE Version %s", TTE_VERSION);
    
    return 0;
}
