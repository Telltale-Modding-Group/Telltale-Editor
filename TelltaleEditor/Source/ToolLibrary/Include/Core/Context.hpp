#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Resource/DataStream.hpp>
#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Resource/Blowfish.hpp>
#include <Resource/Compression.hpp>

#include <mutex>

class ResourceRegistry;
class WeakSlotSignal;
class WeakSlotMaster;
class WeakSlotRef;

// The tool context is used as a global context for the library when modding. Creating this and then calling initialise runs all the lua
// code to setup the library for modding the given game snapshot. ONLY ONE INSTANCE can be alive for one executable using this library.
class ToolContext
{
public:
    // Releases the library. Switch can be re-called after this.
    void Release();
    
    // Switches the library configuration to a new game snapshot. All async jobs will have to complete so this can be blocking.
    void Switch(GameSnapshot snapshot);
    
private:
    
    // Call Switch with the game to setup the context to the given game. This constructor initialised low level APIs, constant for each game.
    ToolContext(LuaFunctionCollection PerState = {});
    
public:
    
    ~ToolContext();
    
    // Meta.cpp. This gets the active game currently Switched to. Returns nullptr if no game is currently set.
    const Meta::RegGame* GetActiveGame();
    
    // Returns the lua manager version for running library scripts, not game scripts.
    inline LuaManager& GetLibraryLVM()
    {
        return _L[0]; // Use latest lua version for modding scripts
    }
    
    // Returns the lua manager which is tied to the currently active game.
    LuaManager& GetGameLVM();
    
    // Get the current game snapshot
    inline GameSnapshot GetSnapshot()
    {
        return _Snapshot;
    }
    
    // Loads a library resource, ie a resource which is required by the library. Eventually will all be stored in a ZIP next to executable,
    // for now it only needs to load from dev directory. This can be called even if switched has not be called yet.
    // THIS CAN BE CALLED ASYNC
    inline DataStreamRef LoadLibraryResource(String name)
    {
        // Fow now just load from the Dev/ directory, relative to cmake build dir.
        return DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, "../../Dev/" + name));
    }
    
    // Reads the library resource, see LoadlibraryResource, as a raw text file and returns the text. Returns empty string if errors.
    String LoadLibraryStringResource(String name);
    
    // Returns if a game is setup such that we can use the library
    inline Bool IsSetup() 
    {
        return _Setup;
    }
    
    // Returns the number of script calls we are currently in. If zero, this function is called directly by the C++ source. If > 0, then
    // this call is from a lua script (indirectly).
    inline U32 GetLockDepth() 
    {
        return _LockedCallDepth;
    }
    
    // Creates a resource registry for the given game. Only use in the current game! Must be destroyed before a switch.
    // This binds itself to the library LVM!
    Ptr<ResourceRegistry> CreateResourceRegistry(Bool bAttachToAllThreads);
    
    // see below. creates a weak lot with an optional signal to check master is alive
    void CreateMasterWeakSlot(WeakSlotMaster& master, NULLABLE WeakSlotSignal* signal);
    
    // creates a weak refernce to a non expired master slot
    WeakSlotRef CreateWeakSlotReference(const WeakSlotMaster& master);
    
private:
    
    Bool _Setup = false;
    U32 _LockedCallDepth = 0; // tracks number of library calls to lua scripts, ensuring we cant change context stuff from the scripts during.
    GameSnapshot _Snapshot = {};
    LuaManager _L[3];
    LuaFunctionCollection _PerStateCollection; // functions to register for each worker thread lua state.
    
    std::mutex _DependentsLock;
    std::vector<WeakPtr<SnapshotDependentObject>> _SwitchDependents{}; // see game dependent object
    
    // WEAK POINTER
    std::mutex _WkLock;
    U64 _WkPtrSlots[128 * 1024]; // must be a multiple of 4 u16's (so u64 cast and all ones check)
    void _ReleaseRequiredSlotUnlocked(U32 grp, U32 sgrp, U32 nwref);
    void _DetachWeakSlotMaster(U32 slot, WeakSlotSignal* sig);
    void _DetachWeakSlotRef(U32 slot);
    void _IncrWeakRef(U32 slot);
    static void _AssertWeakSlot(U64 state);
    
    friend ToolContext* CreateToolContext(LuaFunctionCollection);

    friend void Meta::InitGame();
    
    friend class LuaManager;
    friend class WeakSlotSignal;
    friend class WeakSlotMaster;
    friend class WeakSlotRef;
    
};

// Creates the global tool context. Only one call per process. Optionally pass in lua API to register to each state, including
// the library LVM and also the worker thread local states.
ToolContext* CreateToolContext(LuaFunctionCollection PerStateAPI = {});

// Call at the end of the process
void DestroyToolContext();

// Call CreateToolContext before ANY library call.
ToolContext* GetToolContext();

// Returns if we are calling from the main thread, ie the thread that called CreateToolContext().
Bool IsCallingFromMain();

// For prop keys. Only used by context to register to Lua VM
std::vector<std::pair<CString, CString>>& GetPropKeyConstants();
