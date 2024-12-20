#include <Scripting/LuaManager.hpp>

// Include 5.2.3 headers
#include <lua523/lua.hpp>

void LuaAdapter_523::Shutdown() { lua_close(_State); }

void LuaAdapter_523::Initialise()
{
    _State = luaL_newstate();
    luaL_openlibs(_State);
}

void LuaAdapter_523::RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name)
{
    int error = luaL_loadbufferx(_State, (const char *)Chunk, (size_t)Len, Name, IsCompiled ? "b" : "t"); // Load text buffer
    if (error == 0)
    { // OK. Function now at top of stack, run.
        error = lua_pcall(_State, 0, 0, 0);
    }
    else
    {
        if (error == LUA_ERRSYNTAX)
        {
            TTE_LOG("Running %s: syntax error(s) => %s", Name, lua_tostring(_State, 0));
        }
        else if (error == LUA_ERRMEM)
        {
            TTE_LOG("Running %s: memory allocation error", Name);
        }
        else if (error == LUA_ERRGCMM)
        {
            TTE_LOG("Running %s: gc error", Name);
        }
        lua_pop(_State, 1); // Pop the error message string
        return;
    }
    if (error == LUA_OK)
        return; // OK
    if (error == LUA_ERRRUN)
    {
        TTE_LOG("Running %s: runtime error(s) => %s", Name, lua_tostring(_State, 0));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Running %s: memory allocation error", Name);
    }
    else if (error == LUA_ERRGCMM)
    {
        TTE_LOG("Running %s: gc error", Name);
    }
    else if (error == LUA_ERRERR)
    {
        TTE_LOG("Running %s: error handle error", Name);
    }
    lua_pop(_State, 1); // Pop the error message string
}
