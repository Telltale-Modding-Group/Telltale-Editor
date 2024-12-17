#include <Scripting/LuaManager.hpp>

// Include 5.1.4 headers

extern "C" {
    
#include <lua514/lua.h>
#include <lua514/lualib.h>
#include <lua514/lauxlib.h>
    
}

void LuaAdapter_514::Shutdown() {
    lua_close(_State);
}


void LuaAdapter_514::Initialise() {
    _State = luaL_newstate();
    luaL_openlibs(_State);
}

void LuaAdapter_514::RunChunk(U8* Chunk, U32 Len, Bool IsCompiled, CString Name){
    int error = luaL_loadbuffer(_State, (const char*)Chunk, (size_t)Len, Name); // Load detected text buffer
    if(error == 0){ // OK. Function now at top of stack, run.
        lua_pcall(_State, 0, 0, 0);
    }else{
        if(error == LUA_ERRSYNTAX){
            TTE_LOG("Running %s: syntax error(s) => %s", Name, lua_tostring(_State, 0));
        }else if(error == LUA_ERRMEM){
            TTE_LOG("Running %s: memory allocation error", Name);
        }
        lua_pop(_State, 1); // Pop the error message string
        return;
    }
    if(error == 0)
        return; // OK
    if(error == LUA_ERRRUN){
        TTE_LOG("Running %s: runtime error(s) => %s", Name, lua_tostring(_State, 0));
    }else if(error == LUA_ERRMEM){
        TTE_LOG("Running %s: memory allocation error", Name);
    }else if(error == LUA_ERRERR){
        TTE_LOG("Running %s: error handle error", Name);
    }
    lua_pop(_State, 1); // Pop the error message string
}
