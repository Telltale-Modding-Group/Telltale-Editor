#include <Scripting/LuaManager.hpp>

// Include 5.0.2 headers

extern "C" {
    
#include <lua502/lua.h>
    
}


// Helper macro to get the lua_State type variable inside the adapter class to pass into the lua c api.
#define LSTATE (lua_State*)_State

void LuaAdapter_502::Shutdown(LuaManager &manager) {
    lua_close(LSTATE);
}


void LuaAdapter_502::Initialise(LuaManager &manager) {
    _State = lua_open();
}
