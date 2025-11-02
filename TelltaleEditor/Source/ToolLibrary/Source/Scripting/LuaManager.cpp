#include <Scripting/ScriptManager.hpp>
#include <Core/Context.hpp>
#include <Resource/DataStream.hpp>

LuaManager::~LuaManager()
{
    // If the version is set, set version to empty and free adapter.
    if (_Version != LuaVersion::LUA_NONE)
    {
        
        for(U32 i = 0; i < (U32)LuaConstantMetaTable::NUM; i++)
            if(_ConstantMetaTables[i])
                UnrefReg(_ConstantMetaTables[i]);
        
        _Adapter->Shutdown();
        _Version = LuaVersion::LUA_NONE;
        TTE_DEL(_Adapter);
        _Adapter = nullptr;
    }
    if (_WeakRefSlot)
    {
        _WeakRefSlot->_St.store(false);
        if (_WeakRefSlot->_Wk.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            TTE_FREE(_WeakRefSlot);
        }
        _WeakRefSlot = nullptr;
    }
}

//overriden 'require' lua function to load ourselfs. 'ToolLibrary/XXX' loads from Dev/ToolLibrary/Scripts
U32 luaRequireOverride(LuaManager& man)
{
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Invalid argument passed to require");
    String value = ScriptManager::PopString(man);
    
    // check not already required
    ScriptManager::GetGlobal(man, value, true);
    if(man.Type(-1) != LuaType::NIL)
    {
        man.Pop(1);
        TTE_LOG("Trying to re-require '%s', it has already been included in the VM", value.c_str());
        return 0;
    }
    man.Pop(1);
    
    if(StringStartsWith(value, "ToolLibrary/"))
    {
        
        value = value.substr(12);
        String script = GetToolContext()->LoadLibraryStringResource("Scripts/" + value);
        
        if(script.length() == 0)
            TTE_LOG("When loading script '%s': file empty or could not be read", value.c_str());
        
        if(man.RunText(script.c_str(), (U32)script.length(), false, value.c_str())){
            man.PushNil(); // value of it doesn't matter, only that it exists
            ScriptManager::SetGlobal(man, "ToolLibrary/Scripts/" + value, true); // set global
        }
        
    }else{
        
        TTE_ASSERT(false, "Non tool library requires not supported yet."); // TODO find resource. and dofile, loadfile. (move to editor, hook)
        
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
    
    for(U32 i = 0; i < (U32)LuaConstantMetaTable::NUM; i++)
        _ConstantMetaTables[i] = 0;
}

void LuaManager::SetConstantMetaTable(I32 index, LuaConstantMetaTable mt)
{
    PushEnv();
    PushInteger(_ConstantMetaTables[(I32)mt]);
    GetTable(-2, true);
    Remove(-2); // remove env
    SetMetatable(ToAbsolute(index)); // pops mt
}

I32 LuaManager::RefReg()
{
    return _Adapter->RefReg();
}

void LuaManager::UnrefReg(I32 ref)
{
    _Adapter->UnrefReg(ref);
}

void LuaManager::GetReg(I32 ref)
{
    _Adapter->GetReg(ref);
}

void LuaManager::RegisterConstantMetaTable(LuaConstantMetaTable mt)
{
    I32 ref = RefReg();
    if(_ConstantMetaTables[(I32)mt] != 0)
    {
        UnrefReg(_ConstantMetaTables[(I32)mt]); // overriding
    }
    _ConstantMetaTables[(I32)mt] = ref;
}

static int _CompileWriter(lua_State*, const void* p, size_t sz, void* strm)
{
    if(sz > 0)
    {
        DataStream* stream = (DataStream*)strm;
        return stream->Write((const U8*)p, sz) ? 0 : 1; // write bytes
    }
    return 0;
}

Bool LuaManager::Compile(DataStream* stream)
{
    if(!stream)
    {
        TTE_ASSERT("Cannot compile script: output data stream invalid or not specified");
        return false;
    }
    return _Adapter->DoDump(&_CompileWriter, stream);
}

void LuaManager::GC()
{
    _Adapter->GC();
}

Bool LuaManager::RunText(CString Code, U32 Len, Bool bBlock, CString ChunkName)
{
    TTE_ASSERT(_Version != LuaVersion::LUA_NONE, "Lua not initialised");
    if(bBlock)
    {
        GetToolContext()->_LockedCallDepth++;
    }
    Bool bResult = _Adapter->RunChunk((U8 *)Code, Len, false, ChunkName);
    if(bBlock)
    {
        GetToolContext()->_LockedCallDepth--;
    }
    return bResult;
}

I32 LuaManager::ToInteger(I32 index){
    return _Adapter->ToInteger(index);
}

void LuaManager::CallFunction(U32 Nargs, U32 Nresults, Bool bBlock)
{
    if(bBlock && !JobScheduler::IsRunningFromWorker())
    {
        GetToolContext()->_LockedCallDepth++;
    }
    _Adapter->CallFunction(Nargs, Nresults);
    if(bBlock && !JobScheduler::IsRunningFromWorker())
    {
        GetToolContext()->_LockedCallDepth--;
    }
}

Bool LuaManager::CheckStack(U32 extra)
{
    return _Adapter->CheckStack(extra);
}

void LuaManager::PushBool(Bool value)
{
    _Adapter->Push(LuaType::BOOL, &value);
}

void LuaManager::PushEnv()
{
    _Adapter->Push((LuaType)999, 0); // proxy
}

void LuaManager::PushInteger(I32 value)
{
    _Adapter->Push((LuaType)777, &value); // proxy
}

void LuaManager::PushUnsignedInteger(U32 value)
{
    _Adapter->Push((LuaType)888, &value); // proxy
}

void LuaManager::PushOpaque(void* value)
{
    _Adapter->Push(LuaType::LIGHT_OPAQUE, value);
}

void LuaManager::PushLString(String value)
{
    _Adapter->Push(LuaType::STRING, (void*)&value);
}

void LuaManager::PushNil()
{
    _Adapter->Push(LuaType::NIL, 0);
}

void LuaManager::PushFloat(Float value)
{
    _Adapter->Push(LuaType::NUMBER, &value);
}

void LuaManager::PushTable()
{
    _Adapter->Push(LuaType::TABLE, 0);
}

void LuaManager::Pop(U32 N)
{
    _Adapter->Pop(N);
}

I32 LuaManager::GetTop()
{
    return _Adapter->GetTop();
}

void LuaManager::GetTable(I32 index, Bool bRaw)
{
    _Adapter->GetTable(index, bRaw);
}

void LuaManager::SetTable(I32 index, Bool bRaw)
{
    _Adapter->SetTable(index, bRaw);
}

void LuaManager::SetTableRaw(I32 index, I32 arrayIndex)
{
    _Adapter->SetTableRaw(index, arrayIndex);
}

I32 LuaManager::UpvalueIndex(I32 index)
{
    return _Adapter->UpvalueIndex(index);
}

void LuaManager::GetTableRaw(I32 index, I32 arrayIndex)
{
    _Adapter->GetTableRaw(index, arrayIndex);
}

Bool LuaManager::LoadChunk(const String& nm, const U8* c, U32 s, LoadChunkMode cm)
{
    return _Adapter->LoadChunk(nm, c, s, cm);
}

void LuaManager::PushFn(LuaCFunction f)
{
    _Adapter->Push(LuaType::FUNCTION, (void*)f);
}

I32 LuaManager::TableNext(I32 index)
{
    return _Adapter->TableNext(index);
}

void* LuaManager::CreateUserData(U32 z)
{
    return _Adapter->CreateUserData(z);
}

Bool LuaManager::GetMetaTable(I32 i)
{
    return _Adapter->GetMetatable(i);
}

I32 LuaManager::ToAbsolute(I32 i)
{
    return _Adapter->Abs(i);
}

Bool LuaManager::SetMetaTable(I32 i)
{
    return _Adapter->SetMetatable(i);
}

LuaType LuaManager::Type(I32 index)
{
    return _Adapter->Type(index);
}

Bool LuaManager::Compare(I32 lhs, I32 rhs, LuaOp op)
{
    return _Adapter->Compare(lhs, rhs, op);
}

CString LuaManager::Typename(LuaType t)
{
    return _Adapter->Typename(t);
}

Bool LuaManager::GetMetatable(I32 index)
{
    return _Adapter->GetMetatable(index);
}

Bool LuaManager::SetMetatable(I32 index)
{
    return _Adapter->SetMetatable(index);
}

void LuaManager::SetTop(I32 index)
{
    _Adapter->SetTop(index);
}

void LuaManager::PushCopy(I32 index)
{
    _Adapter->PushCopy(index);
}

void LuaManager::Remove(I32 index)
{
    _Adapter->Remove(index);
}

void LuaManager::Insert(I32 index)
{
    _Adapter->Insert(index);
}

void LuaManager::Replace(I32 index)
{
    _Adapter->Replace(index);
}

Bool LuaManager::ToBool(I32 index)
{
    return _Adapter->ToBool(index);
}

Float LuaManager::ToFloat(I32 index)
{
    return _Adapter->ToFloat(index);
}

String LuaManager::ToString(I32 index)
{
    return _Adapter->ToString(index);
}

void* LuaManager::ToPointer(I32 index)
{
    return _Adapter->ToPointer(index);
}

void LuaManager::Error()
{
    _Adapter->Error();
}
