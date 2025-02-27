#pragma once

#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <cstdarg>
#include <memory>

// =================================================================== LIBRARY CONFIGURATION
// ===================================================================

#define TTE_VERSION "1.0.0"

// DEBUG or RELEASE must be defined (not to 0, just either defined or not defined)

#if defined(DEBUG) == defined(RELEASE)
#error "Neither or both debug or release configuration macros defined!"
#endif

// ===================================================================        LOGGING
// ===================================================================        LOGGING
// ===================================================================

#ifdef DEBUG

// Helper logging functions, if VA_ARGS is empty overloading is used to not do anything. In the future make this more sophisticated, log to UI and files.
inline void LogConsole() {}

inline void LogConsole(const char* Msg, ...) {
    va_list va{};
    va_start(va, Msg);
    vprintf(Msg, va);
    va_end(va);
    
    // Check if we need a new line
    size_t len = strlen(Msg);
    if(len && Msg[len-1] != '\n')
        printf("\n");
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

// ===================================================================         TYPES
// ===================================================================

using U8 = uint8_t;
using I8 = int8_t;
using U16 = uint16_t;
using I16 = int16_t;
using U32 = uint32_t;
using I32 = int32_t;
using U64 = uint64_t;
using I64 = int64_t;

using Float = float;

using String = std::string;
using CString = const char *;

using Bool = bool;

// ===================================================================   PLATFORM SPECIFICS
// ===================================================================

#if defined(_WIN64)

// Windows Platform Specifics
#define PLATFORM_NAME "Windows"
// Lua library platform
#define LUA_WIN

#elif defined(MACOS)

// MacOS Platform Specifics
#define PLATFORM_NAME "MacOS"
#define LUA_MACOSX

#elif defined(LINUX)

// Linux Platform Specifics
#define PLATFORM_NAME "Linux"
#define LUA_LINUX

#else

#error "Unknown platform!"

#endif

/**
 * @brief Sets a breakpoint
 */
void DebugBreakpoint();

// Below are platform-defined file IO routines. U64 is casted to the relavent platform file handle.

// Opens the given file in read-write mode. Returns a platform specific handle.
U64 FileOpen(CString path);

Bool FileWrite(U64 Handle, const U8* Buffer, U64 Nbytes);

Bool FileRead(U64 Handle, U8* Buffer, U64 Nbytes);

U64 FileSize(U64 Handle); // Returns total size

void FileClose(U64 Handle);

U64 FileNull(); // Return the invalid file handle.

String FileNewTemp(); // Return path for available new temp file.

U64 FilePos(U64 Handle); // Return file position

void FileSeek(U64 Handle, U64 Offset); // SEEK_SET

void OodleOpen(void*& CompressorOut, void*& DecompressorOut); // opens oodle lib

void OodleClose(); // close oodle lib

U8* AllocateAnonymousMemory(U64 size); // allocates READ-ONLY memory which doesn't exist anywhere but always reads zeros.

void FreeAnonymousMemory(U8*, U64); // free memory associated with allocate anonymous memory. pass in the total size you allocated.

// ===================================================================         UTILS
// ===================================================================

#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

// padding bytes needed to align _Off to align _Algn
#define TTE_PADDING(_Off, _Algn) (((_Algn) - ((_Off) % (_Algn))) % (_Algn))

// rounds the U32 argument up to the nearest whole power of two. https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
#define TTE_ROUND_UPOW2_U32(dstVar, srcVar) { dstVar = srcVar - 1; dstVar |= dstVar >> 1; dstVar |= dstVar >> 2; \
dstVar |= dstVar >> 4; dstVar |= dstVar >> 8; dstVar |= dstVar >> 16; dstVar++; }

inline void NullDeleter(void*) {} // Useful in shared pointer, in which nothing happens on the final deletion.

// Helper class. std::priority_queue normally does not let us access by finding elements. Little hack to bypass and get internal vector container.
template <typename T> class hacked_priority_queue : public std::priority_queue<T>
{ // Not applying library convention, see this as an 'extension' to std::
  public:
    std::vector<T> &get_container() { return this->c; }

    const std::vector<T> &get_container() const { return this->c; }

    auto get_cmp() { return this->comp; }
};

// Checks if a string starts with the other string prefix
inline bool StringStartsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

template <typename T> // checks if the weak ptr has no reference at all, even to an std::shared_ptr that has been reset.
inline bool IsWeakPtrUnbound(const std::weak_ptr<T>& weak) {
    // Compare the weak pointer to a default-constructed weak pointer
    return !(weak.owner_before(std::weak_ptr<T>()) || std::weak_ptr<T>().owner_before(weak));
}

// helper to call object destructor
template<typename T>
inline void DestroyObject(T& val)
{
    val.~T();
}

class ToolContext; // forward declaration. used a lot. see context.hpp

class DataStream; // See DataStream.hpp

// Useful alias for data stream pointer, which deallocates automagically when finished with.
using DataStreamRef = std::shared_ptr<DataStream>;

// ===================================================================         MEMORY
// ===================================================================

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
};

// each object in the library (eg ttarchive, ttarchive2, etc) has its own ID. See scriptmanager, GetScriptObjectTag and PushScriptOwned.
enum ObjectTag : U32
{
    TTARCHIVE1 = 1, // TTArchive instance, see TTArchive.hpp
    TTARCHIVE2 = 2, // TTArchive2 instance, see TTArchive2.hpp
};

#ifdef DEBUG // use tracked memory

U8* _DebugAllocateTracked(U64 _Nbytes, MemoryTag tag, CString filename, U32 number, CString objName);
void _DebugDeallocateTracked(U8* Ptr);

#define TTE_ALLOC(_Nbytes, _MemoryTag) _DebugAllocateTracked(_Nbytes, _MemoryTag, (CString) __FILE__, (U32) __LINE__, nullptr)

#define TTE_FREE(ptr) _DebugDeallocateTracked((U8*)ptr)

#define TTE_NEW(_Type, _MemoryTag, ...) new (_DebugAllocateTracked(sizeof(_Type), _MemoryTag, (CString)  \
                                        __FILE__, (U32) __LINE__, #_Type)) _Type(__VA_ARGS__)

#define TTE_DEL(_Inst) { if(_Inst) { DestroyObject(*_Inst); TTE_FREE((U8*)_Inst); } }

#else

// Release. Dont need to track any allocations.

#define TTE_NEW(_Type, _MemoryTag, ...) new _Type(__VA_ARGS__)

#define TTE_DEL(_Inst) delete _Inst

// Should initialise to zero
#define TTE_ALLOC(_NBytes, _MemoryTag) new U8[_NBytes]()

#define TTE_FREE(_ByteArray) delete[] ((U8*)_ByteArray)

#endif

void DumpTrackedMemory(); // if in debug mode, prints all tracked memory allocations.
