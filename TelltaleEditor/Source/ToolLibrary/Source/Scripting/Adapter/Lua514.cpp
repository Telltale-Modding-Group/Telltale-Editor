#include <Scripting/LuaManager.hpp>

// Include 5.1.4 headers

extern "C" {
    
#include <lua514/lua.h>
    
}

void LuaAdapter_514::Shutdown(LuaManager &manager) {
    lua_close(_State);
}


void LuaAdapter_514::Initialise(LuaManager &manager) {
    _State = lua_newstate(0,0);
}
