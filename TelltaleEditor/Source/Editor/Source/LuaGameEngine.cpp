#include <Scripting/ScriptManager.hpp>

// This file contains pretty much all of the Telltale Tool LUA API, with documentation. Each namespace is for each lua system.

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><> SECTION <====> SCENE API <><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

namespace luaScene
{
    
    //static U32 luaScene
    
    static void Register(LuaFunctionCollection& Col)
    {
        
    }
    
}

void luaCompleteGameEngine(LuaFunctionCollection& Col)
{
    luaScene::Register(Col);
}
