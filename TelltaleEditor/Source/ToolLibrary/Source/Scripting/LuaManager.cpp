#include <Scripting/LuaManager.hpp>


LuaManager::~LuaManager() {
    // If the version is set, set version to empty and free adapter.
    if(_Version != LuaVersion::LUA_NONE){
        _Adapter->Shutdown();
        _Version = LuaVersion::LUA_NONE;
        TTE_DEL(_Adapter);
        _Adapter = nullptr;
    }
}

void LuaManager::Initialise(LuaVersion Vers) {
    TTE_ASSERT(_Version == LuaVersion::LUA_NONE && Vers != LuaVersion::LUA_NONE, "Cannot re-initialise lua manager / version is invalid");
    _Version = Vers;
    if(Vers == LuaVersion::LUA_5_2_3)
        _Adapter = TTE_NEW(LuaAdapter_523, *this);
    else if(Vers == LuaVersion::LUA_5_1_4)
        _Adapter = TTE_NEW(LuaAdapter_514, *this);
    else if(Vers == LuaVersion::LUA_5_0_2)
        _Adapter = TTE_NEW(LuaAdapter_502, *this);
    else
        TTE_ASSERT(false, "Invalid or unsupported lua version %d", (U32)Vers);
    _Adapter->Initialise();
}

void LuaManager::RunText(CString Code, U32 Len, CString ChunkName) { 
    TTE_ASSERT(_Version != LuaVersion::LUA_NONE, "Lua not initialised");
    _Adapter->RunChunk((U8*)Code, Len, false, ChunkName);
}

