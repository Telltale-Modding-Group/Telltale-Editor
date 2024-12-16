#include <Scripting/LuaManager.hpp>

// Include 5.2.3 headers
#include <lua523/lua.hpp>

void LuaAdapter_523::Shutdown(LuaManager &manager) {
    lua_close(_State);
}


void LuaAdapter_523::Initialise(LuaManager &manager) {
    _State = luaL_newstate();
}

void LuaAdapter_502::RunChunk(U8* Chunk, U32 Len, Bool IsCompiled){
    luaL_dostring(<#L#>, <#s#>)
}
