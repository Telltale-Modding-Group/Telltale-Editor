#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <mutex>

#include <Core/Util.hpp>

// =================================================================== LIBRARY CONFIGURATION
// ===================================================================

#define TTE_VERSION "1.0.0"

// DEBUG or RELEASE must be defined (not to 0, just either defined or not defined)

#if defined(DEBUG) == defined(RELEASE)
#error "Neither or both debug or release configuration macros defined!"
#endif

// ===================================================================        LOGGING
// ===================================================================

#ifdef DEBUG

// Helper logging functions, if VA_ARGS is empty overloading is used to not do anything. In the future make this more sophisticated, log to UI and files.
inline void LogConsole() {}

inline void LogConsole(CString Msg, ...) {
    static std::mutex _Guard{}; // used for \n not coming up because of interleaved calls multithreaded
    _Guard.lock();
    va_list va{};
    va_start(va, Msg);
    vprintf(Msg, va);
    va_end(va);
    
    // Check if we need a new line
    size_t len = strlen(Msg);
    if(len && Msg[len-1] != '\n')
        printf("\n");
    _Guard.unlock();
}

// In DEBUG, simply log any messages simply to the console. Adds a newline character. This is a workaround so empty VA_ARGS works ok. If changing
// printf, change assert to.
#define TTE_LOG(...) LogConsole(__VA_ARGS__)

#else

// In RELEASE, no logging.
#define TTE_LOG(_, ...)

#endif

// ===================================================================        ASSERTS
// ===================================================================

#ifdef DEBUG

// First argument is expression to test, second is format string (optional) then optional format string arguments
#define TTE_ASSERT(EXPR, ...)                                                                                                                        \
if (!(EXPR))                                                                                                                                     \
{                                                                                                                                                \
TTE_LOG(__VA_ARGS__);                                                                                                                    \
DebugBreakpoint();                                                                                                                           \
}

#else

// In RELEASE, don't ignore but do LOG them.
#define TTE_ASSERT(EXPR, ...) \
if (!(EXPR))  \
{ \
TTE_LOG(__VA_ARGS__); \
}

#endif

// ===================================================================   PLATFORM SPECIFICS
// ===================================================================

#if defined(_WIN64)

// Windows Platform Specifics
#define PLATFORM_NAME "Windows"
// Lua library platform
#define LUA_WIN

#define POP_COUNT(x) __popcnt(x)
#define CLZ_BITS(x)  _tzcnt_u32(x)

#elif defined(MACOS)

// MacOS Platform Specifics
#define PLATFORM_NAME "MacOS"
#define LUA_MACOSX

#define POP_COUNT(x) __builtin_popcount(x)
#define CLZ_BITS(x) __builtin_ctz(x)

#elif defined(LINUX)

// Linux Platform Specifics
#define PLATFORM_NAME "Linux"
#define LUA_LINUX

#define POP_COUNT(x) __builtin_popcount(x)
#define CLZ_BITS(x) __builtin_ctz(x)

#else

#error "Unknown platform!"

#endif

/**
 * @brief Sets a breakpoint
 */
void DebugBreakpoint();

// ===================================================================         FILE API
// ===================================================================

// Below are platform-defined file IO routines. U64 is casted to the relavent platform file handle.

// Opens the given file in read-write mode. Returns a platform specific handle.
U64 FileOpen(CString path);

Bool FileWrite(U64 Handle, const U8* Buffer, U64 Nbytes);

Bool FileRead(U64 Handle, U8* Buffer, U64 Nbytes);

U64 FileSize(U64 Handle); // Returns total size

void FileClose(U64 Handle, U64 maxWrittenOffset);

U64 FileNull(); // Return the invalid file handle.

String FileNewTemp(); // Return path for available new temp file.

U64 FilePos(U64 Handle); // Return file position

void FileSeek(U64 Handle, U64 Offset); // SEEK_SET

void OodleOpen(void*& CompressorOut, void*& DecompressorOut); // opens oodle lib

void OodleClose(); // close oodle lib

U8* AllocateAnonymousMemory(U64 size); // allocates READ-ONLY memory which doesn't exist anywhere but always reads zeros.

void FreeAnonymousMemory(U8*, U64); // free memory associated with allocate anonymous memory. pass in the total size you allocated.

inline void NullDeleter(void*) {} // Useful in shared pointer, in which nothing happens on the final deletion.

// Lots of classes which can only exist between game switches use this. Such that if they exist outside, we catch the error.
struct GameDependentObject
{
    
    const CString ObjName;
    
    inline GameDependentObject(CString obj) : ObjName(obj) {}
    
};

// Not an enum class for ease of use. Used in TTE_NEW.
enum MemoryTag
{
    MEMORY_TAG_SCHEDULER, // Scheduler related allocation.
    MEMORY_TAG_SCRIPTING, // Lua and Script Manager allocation
    MEMORY_TAG_DATASTREAM, // DataStream allocation
    MEMORY_TAG_TEMPORARY, // small timescale temp allocation
    MEMORY_TAG_TOOL_CONTEXT, // tool context allocation
    MEMORY_TAG_META_TYPE, // meta type instance
    MEMORY_TAG_META_COLLECTION, // meta dynamic array
    MEMORY_TAG_RUNTIME_BUFFER, // runtime buffer
    MEMORY_TAG_BLOWFISH, // blowfish encryption data
    MEMORY_TAG_SCRIPT_OBJECT, // similar to SCRIPTING, however it is a object managed by the lua GC
    MEMORY_TAG_TEMPORARY_ASYNC, // temporary async stuff
    MEMORY_TAG_RENDERER, // renderer linear heap etc
    MEMORY_TAG_LINEAR_HEAP, // linear heap pages
    MEMORY_TAG_TRANSIENT_FENCE, // small U32 allocation. could be optimised in the future
    MEMORY_TAG_RESOURCE_REGISTRY, // resource registry
    MEMORY_TAG_COMMON_INSTANCE, // common editor type instance, eg Mesh,Texture,Dialog,Chore,etc
    MEMORY_TAG_RENDER_LAYER, // render layers, editor layer, scene runtime etc
    MEMORY_TAG_ANIMATION_DATA, // Animation value interfaces
    MEMORY_TAG_OBJECT_DATA, // ObjOwner::ObjData<T> see Scene header in Editor
    MEMORY_TAG_CALLBACK, // Method impl
    MEMORY_TAG_SCENE_DATA,
};

// each scriptable object in the library (eg ttarchive, ttarchive2, etc) has its own ID. See scriptmanager, GetScriptObjectTag and PushScriptOwned.
enum ObjectTag : U32
{
    TTARCHIVE1 = 1, // TTArchive instance, see TTArchive.hpp
    TTARCHIVE2 = 2, // TTArchive2 instance, see TTArchive2.hpp
};
