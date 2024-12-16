#include <Scripting/LuaManager.hpp>

// Include 5.0.2 headers

extern "C" {
    
#include <lua502/lua.h>
    
}

void LuaAdapter_502::Shutdown(LuaManager &manager) {
    lua_close(_State);
}


void LuaAdapter_502::Initialise(LuaManager &manager) {
    _State = lua_open();
}
