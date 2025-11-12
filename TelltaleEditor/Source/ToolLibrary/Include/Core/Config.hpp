#pragma once

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

#ifndef TTE_DISABLE_LOGGING

inline void LogConsole() {}

void LogConsole(CString Msg, ...); // Defined in Context.cpp

void ToggleLoggerCache(Bool bOnOff); // Switch on logging caching. You can call dump log after this and everthing between these calls will be put into the text file.

void DumpLoggerCache(CString file); // Dumps all cached log. Pass in the absolute path

// In DEBUG, simply log any messages simply to the console. Adds a newline character. This is a workaround so empty VA_ARGS works ok. If changing
// printf, change assert to.
#define TTE_LOG(...) LogConsole(__VA_ARGS__)

#else

// No logging.
#define TTE_LOG(_, ...)

#endif

// ===================================================================        ASSERTS
// ===================================================================

#ifdef DEBUG

// First argument is expression to test, second is format string (optional) then optional format string arguments
#define TTE_ASSERT(EXPR, ...)       \
if (!(EXPR))                        \
{                                   \
TTE_LOG(__VA_ARGS__);               \
DebugBreakpoint();                  \
}

// First argument is expression to test, second is format string (optional) then optional format string arguments
#define TTE_ASSERT_AND_RETURN(EXPR, returnValue, ...)       \
if (!(EXPR))                        \
{                                   \
TTE_LOG(__VA_ARGS__);               \
DebugBreakpoint();                  \
return returnValue;                 \
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

#define PLATFORM_WINDOWS

#elif defined(MACOS)

// MacOS Platform Specifics
#define PLATFORM_NAME "MacOS"
#define LUA_MACOSX

#define POP_COUNT(x) __builtin_popcount(x)
#define CLZ_BITS(x) __builtin_ctz(x)


#define PLATFORM_MAC

#elif defined(LINUX)

// Linux Platform Specifics
#define PLATFORM_NAME "Linux"
#define LUA_LINUX

#define POP_COUNT(x) __builtin_popcount(x)
#define CLZ_BITS(x) __builtin_ctz(x)


#define PLATFORM_LINUX

#else

#error "Unknown platform!"

#endif

/**
 * @brief Sets a breakpoint
 */
void DebugBreakpoint();

/**
 * @brief Shows a message box and waits.
 */
void PlatformMessageBoxAndWait(const String& title, const String& message);

// ===================================================================         HANDLEABLE (DEFINED IN RESOURCE REGISTRY)
// ===================================================================         Defined here as needed everywhere!

class ResourceRegistry;

// If you inherit from this you can lock handles to files.
class HandleLockOwner
{
    
    friend class Handleable;
    
    U32 _LockOwnerID;
    
public:
    
    HandleLockOwner(HandleLockOwner&&) = default; // allow moving
    HandleLockOwner& operator=(HandleLockOwner&&) = default;
    
    inline HandleLockOwner(const HandleLockOwner&) : _LockOwnerID(0) {}
    
    inline HandleLockOwner& operator=(const HandleLockOwner&) { _LockOwnerID = 0; return *this; }
    
    HandleLockOwner();
    
#ifdef DEBUG
    virtual ~HandleLockOwner() = default; // to check no derives for specific
#else
    ~HandleLockOwner() = default;
#endif
    
};

enum class CommonClass;

// All common classes which are normalised into from telltale types (eg meshes, textures, dialogs) inherit from this to be used in Handle.
class Handleable : public std::enable_shared_from_this<Handleable>, public ReferenceObjectInterface
{
    
    mutable std::atomic<U64> _LockTimeStamp;
    mutable std::atomic<U32> _LockKey;
    
protected:
    
    mutable WeakPtr<ResourceRegistry> _Registry;
    
public:
    
    // Returns false if locked already (ie fail)
    // locks the resource returning the lock key. this is done to ensure no other accesses
    virtual Bool Lock(const HandleLockOwner& lockOwner) final;
    
    virtual void Unlock(const HandleLockOwner& lockOwner) final;
    
    virtual U64 GetLockedTimeStamp(const HandleLockOwner& owner, Bool bRelaxed) final; // pass in if you know you already have the lock (eg cached etc)
    
    // Overrides the internal locked time stamp. Do it relaxed if you *know* you have locked this resource (Will be asserted)
    virtual void OverrideLockedTimeStamp(const HandleLockOwner& owner, Bool bRelaxed, U64 value) final;
    
    Bool IsLocked() const;
    
    Bool OwnedBy(const HandleLockOwner& lockOwner, Bool bLockIfAvail) const;
    
    Ptr<ResourceRegistry> GetRegistry() const; // get registry
    
    inline Bool ForceLock(const HandleLockOwner& lockOwner)
    {
        Bool lock = Lock(lockOwner);
        TTE_ASSERT(lock, "Lock acquisition failed but was required here");
        return lock;
    }
    
    virtual void FinaliseNormalisationAsync() = 0;

    virtual CommonClass GetCommonClassType() = 0;
    
    virtual Ptr<Handleable> Clone() const = 0;
    
    inline Handleable(const Handleable& rhs) : _LockKey(), _LockTimeStamp(), _Registry(rhs._Registry) { }
    
    inline Handleable& operator=(const Handleable& rhs)
    {
        _Registry = rhs._Registry;
        return *this;
    }
    
    inline Handleable(Handleable&& rhs)
    {
        this->operator=(std::move(rhs));
    }
    
    inline Handleable(Ptr<ResourceRegistry> reg)
    {
        _Registry = std::move(reg);
    }
    
    Handleable& operator=(Handleable&& rhs); // moveable though.
    
    virtual ~Handleable() = default;
    
};

template<typename T> // extend common classes from this!
class HandleableRegistered : public Handleable
{
public:
    
    inline virtual Ptr<Handleable> Clone() const override;
    
    inline HandleableRegistered(const HandleableRegistered& rhs) : Handleable(rhs) {}
    inline HandleableRegistered(HandleableRegistered&& rhs) : Handleable(std::move(rhs)) {}
    
    inline HandleableRegistered& operator=(HandleableRegistered&& rhs)
    {
        Handleable::operator=(std::move(rhs));
        return *this;
    }
    
    inline HandleableRegistered& operator=(const HandleableRegistered& rhs)
    {
        Handleable::operator=(rhs);
        return *this;
    }
    
    inline HandleableRegistered(Ptr<ResourceRegistry> reg); // used for force instantiate the coersion functions.
    
};

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

// ===================================================================         GLOBAL TELLTALE EDITOR API STRUCTS
// ===================================================================

// Lots of classes which can only exist between game switches use this. Such that if they exist outside, we catch the error.
struct SnapshotDependentObject
{
    
    const CString ObjName;
    
    inline SnapshotDependentObject(CString obj) : ObjName(obj) {}
    
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
    MEMORY_TAG_EDITOR_UI,
    MEMORY_TAG_REFERENCE_OBJECT,
};

// each scriptable object in the library (eg ttarchive, ttarchive2, etc) has its own ID. See scriptmanager, GetScriptObjectTag and PushScriptOwned.
enum ObjectTag : U32
{
    TTARCHIVE1 = 1, // TTArchive instance, see TTArchive.hpp
    TTARCHIVE2 = 2, // TTArchive2 instance, see TTArchive2.hpp
    HANDLEABLE = 3, // Handleable common instance. eg Animation, Chore or MeshInstance.
};

// Used to identify a specific game release snapshot of associated game data for grouping formats
struct GameSnapshot
{
    String ID; // game ID, see wiki.
    String Platform;// game platform, see wiki.
    String Vendor; // vendor. some games have mulitple differing releases. leave blank unless instructed.
};

// ===================================================================         COMMON CLASS INFO (ALL ARE IN EDITOR SUB-PROJECT BUT HERE FOR API)
// ===================================================================

template<typename T> inline Ptr<Handleable> AllocateCommon(Ptr<ResourceRegistry> registry)
{
    static_assert(std::is_base_of<Handleable, T>::value, "T must be handleable");
    Ptr<Handleable> handle =  TTE_NEW_PTR(T, MEMORY_TAG_COMMON_INSTANCE, registry);
    return handle;
}

template<> inline Ptr<Handleable> AllocateCommon<Placeholder>(Ptr<ResourceRegistry>)
{
    return {};
}

class ResourceRegistry;
class LuaFunctionCollection;

typedef Ptr<Handleable> CommonClassAllocator(Ptr<ResourceRegistry> registry);

struct CommonClassInfo
{

    CommonClass Class;
    CString ConstantName = "";
    CString ClassName = ""; // list of ; separated type names
    CString ClassHandleName = ""; // list of handle type names
    CString Extension = ""; // list of extensions
    CommonClassAllocator* ClassAllocator = nullptr; // C++ class handleable alloc

    // Create instance
    inline Ptr<Handleable> Make(Ptr<ResourceRegistry> registry) const
    {
        return ClassAllocator ? ClassAllocator(std::move(registry)) : nullptr;
    }

    // FUNCTIONALITY IS GENERALLY PRIVATE BUT ACCESSIBLE AS IN A SEPARATE PROJECT. INTERFACE WITH TOOL CONTEXT / TELLTALE EDITOR CLASSES!

    static CommonClassInfo GetCommonClassInfo(CommonClass cls);

    static CommonClassInfo GetCommonClassInfoByExtension(const String& extension);

    static void RegisterCommonClass(CommonClassInfo info);

    static void RegisterConstants(LuaFunctionCollection& Col);

    static void Shutdown();

    template<typename T>
    inline static CommonClassInfo Make(CommonClass clz, CString kName)
    {
        CommonClassInfo info = CommonClassInfo{ clz, kName, T::Class, T::ClassHandle, T::Extension, &AllocateCommon<T> };
        RegisterCommonClass(info);
        return info;
    }

};

// All common classes. Registered in editor project
enum class CommonClass
{
    NONE,
    MESH,
    TEXTURE,
    SCENE,
    INPUT_MAPPER,
    ANIMATION,
    SKELETON,
    PROPERTY_SET,
    CHORE,
    /* UPCOMING:
    TRANSITION_REMAPPER,
    DIALOG,
    PARTICLE,
    SPRITE,
    FONT,
    LANGUAGE_DATABASE,
    STYLE_GUIDE,
    RULES,
    WALK_BOXES,
    */
};
