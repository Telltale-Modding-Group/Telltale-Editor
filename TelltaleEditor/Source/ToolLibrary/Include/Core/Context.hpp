#pragma once

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>
#include <Resource/DataStream.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Scripting/ScriptManager.hpp>

// Used to identify a specific game release snapshot of associated game data for grouping formats. Please check the wiki.
struct GameSnapshot
{
    String ID;       // Game ID.
    String Platform; // Game platform.
    String Vendor;   // Vendor. Some games have mulitple differing releases, leave blank unless instructed.
};

// The tool context is used as a global context for the library when modding. Creating this and then calling initialise runs all the lua
// code to setup the library for modding the given game snapshot. ONLY ONE INSTANCE can be alive for one executable using this library.
class ToolContext
{
  public:
    // Releases the library. Switch can be re-called after this.
    inline void Release()
    {
        if (_Setup)
        {

            JobScheduler::Shutdown();
            Meta::RelGame();

            _Setup = false;
        }
    }

    // Switches the library configuration to a new game snapshot. All async jobs will have to complete so this can be blocking.
    inline void Switch(GameSnapshot snapshot)
    {
        if (_Setup)
            Release();

        _Snapshot = snapshot;
        TTE_ASSERT(_Snapshot.ID.length() && _Snapshot.Platform.length(), "Game or platform not specified");

        Meta::InitGame();
        JobScheduler::Initialise();

        _Setup = true;
    }

  private:
    // Call Switch with the game to setup the context to the given game. This constructor initialised low level APIs, constant for each game.
    inline ToolContext()
    {
        DataStreamManager::Initialise();
        _L[0].Initialise(LuaVersion::LUA_5_2_3);
        _L[1].Initialise(LuaVersion::LUA_5_1_4);
        _L[2].Initialise(LuaVersion::LUA_5_0_2);
        Meta::Initialise();
    }

  public:
    inline ~ToolContext()
    {
        Release();
        Meta::Shutdown();
        DataStreamManager::Shutdown();
        // Lua shuts down automatically in dtor
    }

    // Returns the lua manager version for running library scripts, not game scripts.
    inline LuaManager &GetLibraryLVM()
    {
        return _L[0]; // Use latest lua version for modding scripts
    }

    // Get the current game snapshot
    inline GameSnapshot GetSnapshot() { return _Snapshot; }

    // Loads a library resource, ie a resource which is required by the library. Eventually will all be stored in a ZIP next to executable,
    // for now it only needs to load from dev directory. This can be called even if switched has not be called yet.
    inline DataStreamRef LoadLibraryResource(String name)
    {
        // Fow now just load from the Dev/ directory, relative to cmake build dir.
        return DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, "../../Dev/" + name));
    }

    // Reads the library resource, see LoadlibraryResource, as a raw text file and returns the text. Returns empty string if errors.
    inline String LoadLibraryStringResource(String name)
    {
        DataStreamRef stream = LoadLibraryResource(name);
        if (!stream)
            return ""; // Stream could not be opened

        String result{};
        U8 *tmp = TTE_ALLOC(stream->GetSize() + 1, MEMORY_TAG_TEMPORARY); // temp buffer to read string from

        if (!stream->Read(tmp, stream->GetSize()))
        {
            TTE_FREE(tmp); // free buffer
            return "";     // could not read all bytes
        }

        tmp[stream->GetSize()] = 0; // add null terminator
        result = (CString)tmp;      // cast raw U8 bytes to c char string

        TTE_FREE(tmp); // Free buffer

        return result;
    }

    // Returns if a game is setup such that we can use the library
    inline Bool IsSetup() { return _Setup; }

  private:
    Bool _Setup = false;
    GameSnapshot _Snapshot = {};
    LuaManager _L[3];

    friend ToolContext *CreateToolContext();
};

// Creates the global tool context. Only one call per process.
ToolContext *CreateToolContext();

// Call at the end of the process
void DestroyToolContext();

// Call CreateToolContext before ANY library call.
ToolContext *GetToolContext();
