#include <Scripting/LuaManager.hpp>

// Include 5.2.3 headers
#include <lua523/lua.hpp>

void LuaAdapter_523::Shutdown() { lua_close(_State); }

void LuaAdapter_523::Initialise()
{
    _State = luaL_newstate();
    luaL_openlibs(_State);
}

Bool LuaAdapter_523::RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name)
{
    int error = luaL_loadbufferx(_State, (const char *)Chunk, (size_t)Len, Name, IsCompiled ? "b" : "t"); // Load text buffer
    if (error == LUA_OK)
    { // OK. Function now at top of stack, run.
        error = lua_pcall(_State, 0, 0, 0);
    }
    else
    {
        if (error == LUA_ERRSYNTAX)
        {
            TTE_LOG("Loading %s: syntax error(s) => %s", Name, lua_tostring(_State, -1));
        }
        else if (error == LUA_ERRMEM)
        {
            TTE_LOG("Loading %s: memory allocation error", Name);
        }
        else if (error == LUA_ERRGCMM)
        {
            TTE_LOG("Loading %s: gc error", Name);
        }
        lua_pop(_State, 1); // Pop the error message string
        return false;
    }
    if (error == LUA_OK)
        return true; // OK
    if (error == LUA_ERRRUN)
    {
        TTE_LOG("Running %s: runtime error(s) => %s", Name, lua_tostring(_State, -1));
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
    return false;
}

I32 LuaAdapter_523::UpvalueIndex(I32 index){
    return lua_upvalueindex(index);
}

void LuaAdapter_523::CallFunction(U32 Nargs, U32 Nresults)
{
    int error = lua_pcall(_State, Nargs, Nresults, 0);
    if (error == LUA_OK)
        return; // OK
    luaL_traceback(_State, _State, lua_tostring(_State, -1), 1);
    if (error == LUA_ERRRUN)
    {
        TTE_LOG("Lua runtime error(s) => %s", lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Lua memory allocation error");
    }
    else if (error == LUA_ERRGCMM)
    {
        TTE_LOG("Lua GC error");
    }
    else if (error == LUA_ERRERR)
    {
        TTE_LOG("Lua handle error");
    }
    lua_pop(_State, 1); // Pop the error message string
}

Bool LuaAdapter_523::DoDump(lua_Writer w, void* ud)
{
    int error = lua_dump(_State, w, ud);
    if (error == LUA_ERRSYNTAX)
    {
        TTE_LOG("Compiling script: syntax error(s) => %s", lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Compiling script: memory allocation error");
    }
    else if (error == LUA_ERRGCMM)
    {
        TTE_LOG("Compiling script: gc error");
    }
    else if (error == 0)
    {
        return true; // OK
    }
    return false;
}

Bool LuaAdapter_523::CheckStack(U32 extra)
{
    return lua_checkstack(_State, (int)extra);
}

//adapter for passing in lua manager instead
static int L523_CFunction(lua_State* L)
{
    LuaCFunction cfn = (LuaCFunction)lua_touserdata(L, lua_upvalueindex(2));
    LuaManager& man = *((LuaManager*)lua_touserdata(L,lua_upvalueindex(1)));
    return (int)cfn(man);
}

void LuaAdapter_523::GC()
{
    lua_gc(_State, LUA_GCCOLLECT, 0);
}

Bool LuaAdapter_523::LoadChunk(const String& nm, const U8* buf, U32 sz, LoadChunkMode cm)
{
    int error = luaL_loadbufferx(_State, (const char*)buf, (size_t)sz, nm.c_str(), cm == LoadChunkMode::ANY ? "bt"
                                 : LoadChunkMode::BINARY == cm ? "b" : "t");
    if (error == LUA_ERRSYNTAX)
    {
        TTE_LOG("Loading %s: syntax error(s) => %s", nm.c_str(), lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Loading %s: memory allocation error", nm.c_str());
    }
    else if (error == LUA_ERRGCMM)
    {
        TTE_LOG("Loading %s: gc error", nm.c_str());
    }
    else if (error == LUA_OK)
    {
        // leave function ontop of stack
        return true; // OK
    }
    lua_settop(_State, -2); // remove top error
    return false;
}

void LuaAdapter_523::Push(LuaType type, void* pValue)
{
    if(type == LuaType::NIL)
        lua_pushnil(_State);
    else if(type == LuaType::BOOL)
        lua_pushboolean(_State, *(Bool*)pValue);
    else if(type == LuaType::LIGHT_OPAQUE)
        lua_pushlightuserdata(_State, pValue);
    else if(type == LuaType::NUMBER)
        lua_pushnumber(_State, (LUA_NUMBER)(*(Float*)pValue));
    else if(type == LuaType::STRING)
        lua_pushlstring(_State, ((String*)pValue)->c_str(), (size_t)((String*)pValue)->length());
    else if(type == LuaType::TABLE)
        lua_newtable(_State);
    else if(type == (LuaType)999)
        lua_pushglobaltable(_State);
    else if(type == (LuaType)888)
        lua_pushinteger(_State, (LUA_INTEGER)(*(U32*)pValue));
    else if(type == (LuaType)777)
        lua_pushinteger(_State, (LUA_INTEGER)(*(I32*)pValue));
    else if(type == LuaType::FUNCTION){
        LuaCFunction fn = (LuaCFunction)pValue;
        lua_pushlightuserdata(_State, (void*)&_Manager); // push lua manager instance upvalue
        lua_pushlightuserdata(_State, (void*)fn); // push the function to be called's upvalue
        lua_pushcclosure(_State, &L523_CFunction, 2);
    }
}

void LuaAdapter_523::Pop(U32 N)
{
    lua_pop(_State, (int)N);
}

I32 LuaAdapter_523::GetTop()
{
    return (I32)lua_gettop(_State);
}

void LuaAdapter_523::GetTable(I32 index, Bool bRaw)
{
    if(bRaw)
        lua_rawget(_State, (int)index);
    else
        lua_gettable(_State, (int)index);
}

void LuaAdapter_523::SetTable(I32 index, Bool bRaw)
{
    if(bRaw)
        lua_rawset(_State, (int)index);
    else
        lua_settable(_State, (int)index);
}

void LuaAdapter_523::SetTableRaw(I32 index, I32 arrayIndex)
{
    lua_rawseti(_State, (int)index, (int)arrayIndex);
}

void LuaAdapter_523::GetTableRaw(I32 index, I32 arrayIndex)
{
    lua_rawgeti(_State, (int)index, (int)arrayIndex);
}

I32 LuaAdapter_523::TableNext(I32 index)
{
    return (int)lua_next(_State, (int)index);
}

LuaType LuaAdapter_523::Type(I32 index)
{
    int ltype = lua_type(_State, index);
    switch(ltype){
        case LUA_TNONE:
        case LUA_TTHREAD:
            return LuaType::NONE;
        case LUA_TNIL:
            return LuaType::NIL;
        case LUA_TUSERDATA:
            return LuaType::FULL_OPAQUE;
        case LUA_TTABLE:
            return LuaType::TABLE;
        case LUA_TNUMBER:
            return LuaType::NUMBER;
        case LUA_TSTRING:
            return LuaType::STRING;
        case LUA_TBOOLEAN:
            return LuaType::BOOL;
        case LUA_TFUNCTION:
            return LuaType::FUNCTION;
        case LUA_TLIGHTUSERDATA:
            return LuaType::LIGHT_OPAQUE;
        default:
            return LuaType::NONE;
    }
}

void* LuaAdapter_523::CreateUserData(U32 z)
{
    return lua_newuserdata(_State, (size_t)z);
}

Bool LuaAdapter_523::GetMetatable(I32 index)
{
    return lua_getmetatable(_State, index) != 0;
}

void LuaAdapter_523::GetReg(I32 ref)
{
    lua_rawgeti(_State, LUA_REGISTRYINDEX, ref);
}

I32 LuaAdapter_523::RefReg()
{
    return luaL_ref(_State, LUA_REGISTRYINDEX);
}

void LuaAdapter_523::UnrefReg(I32 ref)
{
    luaL_unref(_State, LUA_REGISTRYINDEX, ref);
}

Bool LuaAdapter_523::SetMetatable(I32 index)
{
    return lua_setmetatable(_State, index) != 0;
}

Bool LuaAdapter_523::Compare(I32 lhs, I32 rhs, LuaOp op)
{
    return (Bool)lua_compare(_State, (int)lhs, (int)rhs, (int)op);
}

CString LuaAdapter_523::Typename(LuaType t)
{
    return lua_typename(_State, (int)t);
}

void LuaAdapter_523::SetTop(I32 index)
{
    lua_settop(_State,(int)index);
}

I32 LuaAdapter_523::Abs(I32 index)
{
    return (index < 0 && index > LUA_REGISTRYINDEX) ? GetTop() + index + 1 : index;
}

void LuaAdapter_523::PushCopy(I32 index)
{
    lua_pushvalue(_State,(int)index);
}

void LuaAdapter_523::Remove(I32 index)
{
    lua_remove(_State, (int)index);
}

void LuaAdapter_523::Insert(I32 index)
{
    lua_insert(_State, (int)index);
}

void LuaAdapter_523::Replace(I32 index)
{
    lua_replace(_State,(int)index);
}

Bool LuaAdapter_523::ToBool(I32 index)
{
    TTE_ASSERT(lua_type(_State, index) == LUA_TBOOLEAN, "Cannot convert to bool: %s", lua_typename(_State, lua_type(_State, index)));
    return (Bool)lua_toboolean(_State,(int)index);
}

Float LuaAdapter_523::ToFloat(I32 index)
{
    TTE_ASSERT(lua_type(_State, index) == LUA_TNUMBER, "Cannot convert to float: %s", lua_typename(_State, lua_type(_State, index)));
    return (Float)lua_tonumber(_State, (int)index);
}

I32 LuaAdapter_523::ToInteger(I32 index)
{
    TTE_ASSERT(lua_type(_State, index) == LUA_TNUMBER, "Cannot convert to integer: %s", lua_typename(_State, lua_type(_State, index)));
    return (I32)lua_tointeger(_State, (int)index);
}

String LuaAdapter_523::ToString(I32 index)
{
    TTE_ASSERT(lua_type(_State, index) == LUA_TSTRING, "Cannot convert to string: %s", lua_typename(_State, lua_type(_State, index)));
    return String(lua_tolstring(_State, (int)index, 0));
}

void* LuaAdapter_523::ToPointer(I32 index)
{
    return const_cast<void*>(lua_topointer(_State, (int)index));
}

void LuaAdapter_523::Error()
{
    lua_error(_State);
}
