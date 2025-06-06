#include <Scripting/LuaManager.hpp>

// Include 5.1.4 headers

extern "C"
{
    
#include <lua514/lauxlib.h>
#include <lua514/lua.h>
#include <lua514/lualib.h>
#include <lua514/lgc.h>
    
}

void LuaAdapter_514::Shutdown() { lua_close(_State); }

void LuaAdapter_514::Initialise()
{
    _State = luaL_newstate();
    luaL_openlibs(_State);
}

Bool LuaAdapter_514::RunChunk(U8 *Chunk, U32 Len, Bool, CString Name)
{
    int error = luaL_loadbuffer(_State, (const char *)Chunk, (size_t)Len, Name); // Load detected text buffer
    if (error == 0)
    { // OK. Function now at top of stack, run.
        lua_pcall(_State, 0, 0, 0);
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
        lua_pop(_State, 1); // Pop the error message string
        return false;
    }
    if (error == 0)
        return true; // OK
    if (error == LUA_ERRRUN)
    {
        TTE_LOG("Running %s: runtime error(s) => %s", Name, lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Running %s: memory allocation error", Name);
    }
    else if (error == LUA_ERRERR)
    {
        TTE_LOG("Running %s: error handle error", Name);
    }
    lua_pop(_State, 1); // Pop the error message string
    return false;
}

Bool LuaAdapter_514::LoadChunk(const String& nm, const U8* buf, U32 sz, LoadChunkMode m)
{
    TTE_ASSERT(m != LoadChunkMode::BINARY, "This lua version does not accept binary scripts, they do not exist in this version!");
    int error = luaL_loadbuffer(_State, (const char*)buf, (size_t)sz, nm.c_str());
    if (error == LUA_ERRSYNTAX)
    {
        TTE_LOG("Loading %s: syntax error(s) => %s", nm.c_str(), lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Loading %s: memory allocation error", nm.c_str());
    }
    else if (error == 0)
    {
        // leave function ontop of stack
        return true; // OK
    }
    lua_settop(_State, -2); // remove top error
    return false;
}

I32 LuaAdapter_514::UpvalueIndex(I32 index){
    return lua_upvalueindex(index);
}

void LuaAdapter_514::CallFunction(U32 Nargs, U32 Nresults)
{
    int error = lua_pcall(_State, Nargs, Nresults, 0);
    if (error == 0)
        return; // OK
    if (error == LUA_ERRRUN)
    {
        TTE_LOG("Lua runtime error(s) => %s", lua_tostring(_State, -1));
    }
    else if (error == LUA_ERRMEM)
    {
        TTE_LOG("Lua memory allocation error");
    }
    else if (error == LUA_ERRERR)
    {
        TTE_LOG("Lua handle error");
    }
    lua_pop(_State, 1); // Pop the error message string
}

void LuaAdapter_514::GC()
{
    lua_gc(_State, LUA_GCCOLLECT, 0);
}

Bool LuaAdapter_514::CheckStack(U32 extra)
{
    return lua_checkstack(_State, (int)extra);
}

Bool LuaAdapter_514::DoDump(lua_Writer w, void* ud)
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
    else if (error == 0)
    {
        return true; // OK
    }
    return false;
}

//adapter for passing in lua manager instead
static int L514_CFunction(lua_State* L)
{
    LuaCFunction cfn = (LuaCFunction)lua_touserdata(L, lua_upvalueindex(1));
    LuaManager& man = *((LuaManager*)lua_touserdata(L,lua_upvalueindex(1)));
    return (int)cfn(man);
}

void LuaAdapter_514::Push(LuaType type, void* pValue)
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
        lua_pushvalue(_State,LUA_GLOBALSINDEX);
    else if(type == (LuaType)888)
        lua_pushinteger(_State, (LUA_INTEGER)(*(U32*)pValue));
    else if(type == (LuaType)777)
        lua_pushinteger(_State, (LUA_INTEGER)(*(I32*)pValue));
    else if(type == LuaType::FUNCTION){
        LuaCFunction fn = (LuaCFunction)pValue;
        lua_pushlightuserdata(_State, (void*)&_Manager); // push lua manager instance upvalue
        lua_pushlightuserdata(_State, (void*)fn); // push the function to be called's upvalue
        lua_pushcclosure(_State, &L514_CFunction, 2);
    }
}

void LuaAdapter_514::Pop(U32 N)
{
    lua_pop(_State, (int)N);
}

I32 LuaAdapter_514::GetTop()
{
    return (I32)lua_gettop(_State);
}

void LuaAdapter_514::GetTable(I32 index, Bool bRaw)
{
    if(bRaw)
        lua_rawget(_State, (int)index);
    else
        lua_gettable(_State, (int)index);
}

void LuaAdapter_514::SetTable(I32 index, Bool bRaw)
{
    if(bRaw)
        lua_rawset(_State, (int)index);
    else
        lua_settable(_State, (int)index);
}

void LuaAdapter_514::SetTableRaw(I32 index, I32 arrayIndex)
{
    lua_rawseti(_State, (int)index, (int)arrayIndex);
}

void LuaAdapter_514::GetTableRaw(I32 index, I32 arrayIndex)
{
    lua_rawgeti(_State, (int)index, (int)arrayIndex);
}

I32 LuaAdapter_514::TableNext(I32 index)
{
    return (int)lua_next(_State, (int)index);
}

LuaType LuaAdapter_514::Type(I32 index)
{
    int ltype = lua_type(_State, index);
    switch(ltype){
        case LUA_TNONE:
        case LUA_TTHREAD:
            return LuaType::NONE;
        case LUA_TNIL:
            return LuaType::NIL;
        case LUA_TTABLE:
            return LuaType::TABLE;
        case LUA_TUSERDATA:
            return LuaType::FULL_OPAQUE;
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

Bool LuaAdapter_514::Compare(I32 lhs, I32 rhs, LuaOp op)
{
    if(op == LuaOp::LT)
        return lua_lessthan(_State, lhs, rhs);
    else if(op == LuaOp::LE)
        return lua_lessthan(_State, lhs, rhs) || lua_equal(_State, lhs, rhs);
    return lua_equal(_State, lhs, rhs);
}

CString LuaAdapter_514::Typename(LuaType t)
{
    return lua_typename(_State, (int)t);
}

void LuaAdapter_514::SetTop(I32 index)
{
    lua_settop(_State,(int)index);
}

void LuaAdapter_514::PushCopy(I32 index)
{
    lua_pushvalue(_State,(int)index);
}

void* LuaAdapter_514::CreateUserData(U32 z)
{
    return lua_newuserdata(_State, (size_t)z);
}

Bool LuaAdapter_514::GetMetatable(I32 index)
{
    return lua_getmetatable(_State, index) != 0;
}

I32 LuaAdapter_514::Abs(I32 index)
{
    return (index < 0 && index > LUA_REGISTRYINDEX) ? lua_gettop(_State) + index + 1 : index;
}


Bool LuaAdapter_514::SetMetatable(I32 index)
{
    return lua_setmetatable(_State, index) != 0;
}

void LuaAdapter_514::Remove(I32 index)
{
    lua_remove(_State, (int)index);
}

void LuaAdapter_514::Insert(I32 index)
{
    lua_insert(_State, (int)index);
}

void LuaAdapter_514::Replace(I32 index)
{
    lua_replace(_State,(int)index);
}

Bool LuaAdapter_514::ToBool(I32 index)
{
    return (Bool)lua_toboolean(_State,(int)index);
}

Float LuaAdapter_514::ToFloat(I32 index)
{
    return (Float)lua_tonumber(_State, (int)index);
}

void LuaAdapter_514::GetReg(I32 ref)
{
    lua_rawgeti(_State, LUA_REGISTRYINDEX, ref);
}

I32 LuaAdapter_514::RefReg()
{
    return luaL_ref(_State, LUA_REGISTRYINDEX);
}

void LuaAdapter_514::UnrefReg(I32 ref)
{
    luaL_unref(_State, LUA_REGISTRYINDEX, ref);
}

String LuaAdapter_514::ToString(I32 index)
{
    return String(lua_tolstring(_State, (int)index, 0));
}

void* LuaAdapter_514::ToPointer(I32 index)
{
    return const_cast<void*>(lua_topointer(_State, (int)index));
}

void LuaAdapter_514::Error()
{
    lua_error(_State);
}

I32 LuaAdapter_514::ToInteger(I32 index)
{
    return (I32)lua_tointeger(_State, (int)index);
}
