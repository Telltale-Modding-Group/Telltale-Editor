#include <Scripting/LuaManager.hpp>

// Include 5.0.2 headers

extern "C" {
    
#include <lua502/lua.h>
#include <lua502/lualib.h>
#include <lua502/lauxlib.h>
    
}

void LuaAdapter_502::Shutdown() {
    lua_close(_State);
}


void LuaAdapter_502::Initialise() {
    _State = lua_open();
    luaopen_base(_State);    // For basic Lua functions (like print, error handling)
    luaopen_table(_State);   // For tables (most important data structure in Lua)
    luaopen_io(_State);      // For I/O functions (e.g., print, file handling)
    luaopen_string(_State);  // For string manipulation functions
    luaopen_math(_State);    // For math functions (e.g., math.sin, math.random)
    luaopen_debug(_State);   // For debugging functions (e.g., debug.traceback)
}

void LuaAdapter_502::RunChunk(U8* Chunk, U32 Len, Bool IsCompiled, CString Name){
    int error = luaL_loadbuffer(_State, (const char*)Chunk, (size_t)Len, Name); // Load detected buffer
    if(error == 0) // OK. Function now at top of stack, run.
        lua_pcall(_State, 0, 0, 0);
    else{
        if(error == LUA_ERRSYNTAX)
            TTE_LOG("Running %s: syntax error(s) => %s", Name, lua_tostring(_State, 0));
        else if(error == LUA_ERRMEM)
            TTE_LOG("Running %s: memory allocation error");
        lua_pop(_State, 1); // Pop the error message string
        return;
    }
    if(error == 0)
        return; // OK
    if(error == LUA_ERRRUN)
        TTE_LOG("Running %s: runtime error(s) => %s", Name, lua_tostring(_State, 0));
    else if(error == LUA_ERRMEM)
        TTE_LOG("Running %s: memory allocation error");
    else if(error == LUA_ERRERR)
        TTE_LOG("Running %s: error handle error");
    lua_pop(_State, 1); // Pop the error message string
}
