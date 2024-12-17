#pragma once

#include <Core/Config.hpp>

// This is the low level LUA API. This deals with calls to the lua library for compilation and running of scripts. Decompilation is also managed here in terms of luadec.

// IMPORTANT: *NO* lua C headers are to be included anywhere APART from the adapter translation units (Scripting/Adapter/Lua5**.cpp)!

// Supported LUA versions which telltale use in release games.
enum class LuaVersion {
    LUA_NONE, // No version set
    LUA_5_2_3,// Lua 5.2.3
    LUA_5_1_4,// Lua 5.1.4
    LUA_5_0_2 // Lua 5.0.2
};

// For use below
class LuaAdapterBase;

// Used interally for lua states for defined in each Lua version.
struct lua_State;

// Lua Manager class for managing lua calling and versioning. Like most parts of this library, it is not thread safe unless explicity said.
class LuaManager {
public:
    
    // Initialises, must be called after construction, the lua manager with the given version. Only one instance can run per version!
    void Initialise(LuaVersion Vers);
    
    // Runs a chunk of uncompiled lua source. Pass in the C string and its length. Pass in the Name of the lua file as the optional last argument.
    void RunText(CString Code, U32 Len, CString Name = "defaultrun.lua");

    // Default constructor.
    LuaManager() = default;
    
    // Releases lua.
    ~LuaManager();
    
    // Gets the version of this lua manager.
    inline LuaVersion GetVersion() {
        return _Version;
    }
    
private:

    LuaVersion _Version = LuaVersion::LUA_NONE; // This lua version
    
    LuaAdapterBase* _Adapter = nullptr; // Adapter for the version, see below
    
};

// Base class for lua version adapters (see below for each). Each includes the lua headers from the given version in its own translation unit for defining these functions.
class LuaAdapterBase {
protected:
    
    LuaManager& _Manager;
    lua_State* _State = nullptr; // lua_State specific to the verison.
    
public:
    
    virtual void Initialise() = 0;
    
    virtual void Shutdown() = 0;
    
    virtual void RunChunk(U8* Chunk, U32 Len, Bool IsCompiled, CString Name) = 0;
    
    inline LuaAdapterBase(LuaManager& manager) : _Manager(manager) {}
    
    inline virtual ~LuaAdapterBase() {}
    
};

// For Lua 5.2.3
class LuaAdapter_523 : public LuaAdapterBase {
public:
    
    inline LuaAdapter_523(LuaManager& _Man) : LuaAdapterBase(_Man) {}
    
    void Initialise() override;
    
    void Shutdown() override;
    
    void RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;
    
    
};

// For Lua 5.1.4
class LuaAdapter_514 : public LuaAdapterBase {
public:
    
    inline LuaAdapter_514(LuaManager& _Man) : LuaAdapterBase(_Man) {}
    
    void Initialise() override;
    
    void Shutdown() override;
    
    void RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;
    
    
};

// For Lua 5.0.2
class LuaAdapter_502 : public LuaAdapterBase {
public:
    
    inline LuaAdapter_502(LuaManager& _Man) : LuaAdapterBase(_Man) {}
    
    void Initialise() override;
    
    void Shutdown() override;
    
    void RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;
    
    
};
