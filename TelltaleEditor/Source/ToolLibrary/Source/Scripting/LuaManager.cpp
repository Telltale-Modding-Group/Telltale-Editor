#include <Core/Context.hpp>
#include <Scripting/ScriptManager.hpp>

LuaManager::~LuaManager()
{
    // If the version is set, set version to empty and free adapter.
    if (_Version != LuaVersion::LUA_NONE)
    {
        _Adapter->Shutdown();
        _Version = LuaVersion::LUA_NONE;
        TTE_DEL(_Adapter);
        _Adapter = nullptr;
    }
}

// Overriden 'require' lua function to load ourselfs. 'ToolLibrary/XXX' loads from lib resources XXX.
U32 luaRequireOverride(LuaManager &man)
{
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Invalid argument passed to require");
    String value = ScriptManager::PopString(man);

    // Check not already required
    ScriptManager::GetGlobal(man, value, true);
    if (man.Type(-1) != LuaType::NIL)
    {
        man.Pop(1);
        TTE_LOG("Trying to re-require '%s', it has already been included in the VM", value.c_str());
        return 0;
    }
    man.Pop(1);

    if (StringStartsWith(value, "ToolLibrary/"))
    {

        value = value.substr(12);
        String script = GetToolContext()->LoadLibraryStringResource(value);

        if (script.length() == 0)
            TTE_LOG("When loading script '%s': file empty or could not be read", value.c_str());

        if (man.RunText(script.c_str(), (U32)script.length(), value.c_str()))
        {
            man.PushNil();                                               // value of it doesn't matter, only that it exists
            ScriptManager::SetGlobal(man, "ToolLibrary/" + value, true); // set global
        }
    }
    else
    {
        TTE_ASSERT(false, "Non tool library requires not supported yet."); // TODO find resource.
    }
    return 0;
}

void LuaManager::Initialise(LuaVersion Vers)
{
    TTE_ASSERT(_Version == LuaVersion::LUA_NONE && Vers != LuaVersion::LUA_NONE, "Cannot re-initialise lua manager / version is invalid");
    _Version = Vers;
    if (Vers == LuaVersion::LUA_5_2_3)
        _Adapter = TTE_NEW(LuaAdapter_523, MEMORY_TAG_SCRIPTING, *this);
    else if (Vers == LuaVersion::LUA_5_1_4)
        _Adapter = TTE_NEW(LuaAdapter_514, MEMORY_TAG_SCRIPTING, *this);
    else if (Vers == LuaVersion::LUA_5_0_2)
        _Adapter = TTE_NEW(LuaAdapter_502, MEMORY_TAG_SCRIPTING, *this);
    else
        TTE_ASSERT(false, "Invalid or unsupported lua version %d", (U32)Vers);

    _Adapter->Initialise();

    PushFn(&luaRequireOverride);
    ScriptManager::SetGlobal(*this, "require", false);
}

Bool LuaManager::RunText(CString Code, U32 Len, CString ChunkName)
{
    TTE_ASSERT(_Version != LuaVersion::LUA_NONE, "Lua not initialised");
    return _Adapter->RunChunk((U8 *)Code, Len, false, ChunkName);
}

I32 LuaManager::ToInteger(I32 index) { return _Adapter->ToInteger(index); }

void LuaManager::CallFunction(U32 Nargs, U32 Nresults) { return _Adapter->CallFunction(Nargs, Nresults); }

Bool LuaManager::CheckStack(U32 extra) { return _Adapter->CheckStack(extra); }

void LuaManager::PushBool(Bool value) { _Adapter->Push(LuaType::BOOL, &value); }

void LuaManager::PushEnv()
{
    _Adapter->Push((LuaType)999, 0); // proxy
}

void LuaManager::PushInteger(I32 value)
{
    _Adapter->Push((LuaType)888, &value); // proxy
}

void LuaManager::PushOpaque(void *value) { _Adapter->Push(LuaType::LIGHT_OPAQUE, value); }

void LuaManager::PushLString(String value) { _Adapter->Push(LuaType::STRING, (void *)value.c_str()); }

void LuaManager::PushNil() { _Adapter->Push(LuaType::NIL, 0); }

void LuaManager::PushFloat(Float value) { _Adapter->Push(LuaType::NUMBER, &value); }

void LuaManager::PushTable() { _Adapter->Push(LuaType::TABLE, 0); }

void LuaManager::Pop(U32 N) { _Adapter->Pop(N); }

I32 LuaManager::GetTop() { return _Adapter->GetTop(); }

void LuaManager::GetTable(I32 index, Bool bRaw) { _Adapter->GetTable(index, bRaw); }

void LuaManager::SetTable(I32 index, Bool bRaw) { _Adapter->SetTable(index, bRaw); }

void LuaManager::SetTableRaw(I32 index, I32 arrayIndex) { _Adapter->SetTableRaw(index, arrayIndex); }

I32 LuaManager::UpvalueIndex(I32 index) { return _Adapter->UpvalueIndex(index); }

void LuaManager::GetTableRaw(I32 index, I32 arrayIndex) { _Adapter->GetTableRaw(index, arrayIndex); }

Bool LuaManager::LoadChunk(const String &nm, const U8 *c, U32 s, Bool cm) { return _Adapter->LoadChunk(nm, c, s, cm); }

void LuaManager::PushFn(LuaCFunction f) { _Adapter->Push(LuaType::FUNCTION, (void *)f); }

I32 LuaManager::TableNext(I32 index) { return _Adapter->TableNext(index); }

LuaType LuaManager::Type(I32 index) { return _Adapter->Type(index); }

Bool LuaManager::Compare(I32 lhs, I32 rhs, LuaOp op) { return _Adapter->Compare(lhs, rhs, op); }

CString LuaManager::Typename(LuaType t) { return _Adapter->Typename(t); }

Bool LuaManager::GetMetatable(I32 index) { return _Adapter->GetMetatable(index); }

Bool LuaManager::SetMetatable(I32 index) { return _Adapter->SetMetatable(index); }

void LuaManager::SetTop(I32 index) { _Adapter->SetTop(index); }

void LuaManager::PushCopy(I32 index) { _Adapter->PushCopy(index); }

void LuaManager::Remove(I32 index) { _Adapter->Remove(index); }

void LuaManager::Insert(I32 index) { _Adapter->Insert(index); }

void LuaManager::Replace(I32 index) { _Adapter->Replace(index); }

Bool LuaManager::ToBool(I32 index) { return _Adapter->ToBool(index); }

Float LuaManager::ToFloat(I32 index) { return _Adapter->ToFloat(index); }

String LuaManager::ToString(I32 index) { return _Adapter->ToString(index); }

void *LuaManager::ToPointer(I32 index) { return _Adapter->ToPointer(index); }

void LuaManager::Error() { _Adapter->Error(); }
