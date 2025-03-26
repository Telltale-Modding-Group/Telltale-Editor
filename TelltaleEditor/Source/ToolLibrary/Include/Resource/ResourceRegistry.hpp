#pragma once

#include <Core/Config.hpp>
#include <Core/Symbol.hpp>
#include <Resource/Compression.hpp>
#include <Resource/DataStream.hpp>
#include <Core/LinearHeap.hpp>
#include <Core/BitSet.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/ISO9660.hpp>
#include <Resource/TTArchive2.hpp>
#include <Resource/Pack2.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Meta/Meta.hpp>

#include <cctype>
#include <cstring>
#include <set>
#include <mutex>
#include <type_traits>
#include <filesystem>
#include <algorithm>
#include <map>
#include <thread>
#include <atomic>
#include <random>

// folders excluded from disk unless explicitly mounted when recursively going through directories
#define EXCLUDE_SYSTEM_FILTER "!*.DS_Store;!*.app/*"

// HIGH LEVEL TELLTALE RESOURCE SYSTEM

// ================================================== RESOURCE LIFETIME MANAGEMENT ==================================================

class ResourceRegistry;

// If you inherit from this you can lock handles to files.
class HandleLockOwner
{
    
    friend class Handleable;
    
    U32 _LockOwnerID;
    
public:
    
    HandleLockOwner(HandleLockOwner&&) = default; // allow moving
    HandleLockOwner& operator=(HandleLockOwner&&) = default;
    
    HandleLockOwner(const HandleLockOwner&) = delete; // copying is NOT allowed. we have locked it, are not sharing it. like me with money.
    HandleLockOwner& operator=(const HandleLockOwner&) = delete;
 
    HandleLockOwner();
    
#ifdef DEBUG
    virtual ~HandleLockOwner() = default; // to check no derives for specific
#else
    ~HandleLockOwner() = default;
#endif
    
};

// All common classes which are normalised into from telltale types (eg meshes, textures, dialogs) inherit from this to be used in Handle.
class Handleable : public std::enable_shared_from_this<Handleable>
{
    
    mutable std::atomic<U32> _LockKey;
    
public:
    
    // Returns false if locked already (ie fail)
    virtual Bool Lock(const HandleLockOwner& lockOwner) final; // locks the resource returning the lock key. this is done to ensure no other accesses
    
    virtual void Unlock(const HandleLockOwner& lockOwner) final;
    
    Bool IsLocked() const;
    
    Bool OwnedBy(const HandleLockOwner& lockOwner, Bool bLockIfAvail) const;
    
    inline Bool ForceLock(const HandleLockOwner& lockOwner)
    {
        Bool lock = Lock(lockOwner);
        TTE_ASSERT(lock, "Lock acquisition failed but was required here");
        return lock;
    }
    
    virtual void FinaliseNormalisationAsync() = 0;
    
    Handleable() = default;
    
    Handleable(const Handleable&) = delete;
    Handleable& operator=(const Handleable&) = delete; // no copy allowed. must be done explicitly
    
    inline Handleable(Handleable&& rhs)
    {
        this->operator=(std::move(rhs));
    }
    
    Handleable& operator=(Handleable&& rhs); // moveable though.
    
    virtual ~Handleable() = default;
    
};

enum class HandleFlags
{
    NEEDS_NORMALISATION = 1, // needs to be normalised
    NEEDS_SERIALISE_IN = 2, // needs to be serialised in from the stream
    NEEDS_DESTROY = 4, // needs to be destroyed
    LOADED = 8, // has been normalised into, so is loaded.
    
    // other flags in future, serialise out needed, load dependent resources (eg textures in a mesh, non embeds in chore etc)
};

// Internal handle object info used by registry. Handles refer to these under the hood.
struct HandleObjectInfo
{
    
    Symbol _ResourceName; // name of this resource
    Meta::ClassInstance _Instance; // instance in the meta system
    Ptr<Handleable> _Handle; // pointer to actual alive common instance
    DataStreamRef _OpenStream; // if it needs to be serialised in or out this is the stream to/from
    Flags _Flags; // any flags
    
    void _OnUnload(ResourceRegistry& registry, std::unique_lock<std::mutex>& lck); // called when unloading, may be async
    
    inline Bool operator<(const HandleObjectInfo& rhs) const
    {
        return _ResourceName < rhs._ResourceName;
    }
    
    inline Bool operator==(const HandleObjectInfo& rhs) const
    {
        return _ResourceName == rhs._ResourceName;
    }
    
};

// Function proto for common instance allocator
using CommonInstanceAllocator = Ptr<Handleable> ();

template<typename T> Ptr<Handleable> AllocateCommon()
{
    static_assert(std::is_base_of<Handleable, T>::value, "T must be handleable");
    return TTE_NEW_PTR(T, MEMORY_TAG_COMMON_INSTANCE);
}

// Non-templated version for below
class HandleBase
{
    
    friend class ResourceRegistry;
    
protected:
    
    Ptr<Handleable> _GetObject(Ptr<ResourceRegistry>& registry);
    
    Symbol _ResourceName;
    CommonInstanceAllocator* _AllocatorFn; // for creating the common class
    
    inline HandleBase(Symbol rn, CommonInstanceAllocator* alloc) : _ResourceName(rn), _AllocatorFn(alloc) {}
    
    inline void _Validate()
    {
        TTE_ASSERT(_AllocatorFn, "HandleBase calls must be done via a Handle<T> class, as the type of common class must be known!");
    }
    
    void _SetObject(Ptr<ResourceRegistry>& registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded);
    
public:
    
    HandleBase() = default;
    
    Bool IsLoaded(Ptr<ResourceRegistry>& registry); // return if its currently loaded. Will return false if there is a future load in progress which hasn't been progressed
    
    void EnsureIsLoaded(Ptr<ResourceRegistry>& registry); // ensure this handle is currently loaded, the latest load
    
    // sets object file (eg a .d3dmesh). Specify to unload old resource, and if you want to ensure its loaded.
    template<typename T>
    inline void SetObject(Ptr<ResourceRegistry>& registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
    {
        if(!_AllocatorFn)
        {
            _AllocatorFn = &AllocateCommon<T>;
        }else TTE_ASSERT(_AllocatorFn == &AllocateCommon<T>, "Cannot switch between handle underlying type! It must stay the same");
        _SetObject(registry, name, bUnloadOld, bEnsureLoaded);
    }
    
    // Similar but does not load just sets the name, can be loaded later.
    inline void SetObject(Symbol name)
    {
        _ResourceName = name;
    }
    
};

// Handle to a common resource. This is basically a templated version of HandleBase, but caches the loaded handle to the common resource.
template<typename T>
class Handle : protected HandleBase
{
    
    Ptr<T> _Cached;
    
    static_assert(std::is_base_of<Handleable, T>::value, "T must be handleable");
    
    inline Handle(Symbol rn) : HandleBase(rn, &AllocateCommon<T>), _Cached{}
    {
    }
    
public:
    
    inline Handle() : HandleBase(Symbol(), &AllocateCommon<T>) {}
    
    // Gets the underlying resouce. The resource will always be valid but may not be loaded. You can use other functionality to ensure its loaded.
    inline Ptr<T> GetObject(Ptr<ResourceRegistry>& registry, Bool bEnsureLoaded)
    {
        if(_Cached)
            return _Cached;
        if(bEnsureLoaded)
            HandleBase::EnsureIsLoaded(registry);
        Ptr<Handleable> resource = _GetObject(registry);
        if(!resource)
            return {};
        T* casted = dynamic_cast<T*>(resource.get());
        TTE_ASSERT(casted, "Handle<T> has template parameter which does not match the underlying resource type!");
        _Cached = std::dynamic_pointer_cast<T>(resource);
        return _Cached;
    }
    
    inline void SetObject(Ptr<ResourceRegistry>& registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
    {
        return HandleBase::SetObject<T>(registry, name, bUnloadOld, bEnsureLoaded);
    }
    
    inline Bool IsLoaded(Ptr<ResourceRegistry>& registry)
    {
        return HandleBase::IsLoaded(registry);
    }
    
    // Flushes the cached value, meaning if its updated the cached resource here will get updated next GetObject
    inline void EnsureIsLoaded(Ptr<ResourceRegistry>& registry)
    {
        HandleBase::EnsureIsLoaded(registry);
        _Cached.reset();
    }
    
    inline void SetObject(Symbol name)
    {
        HandleBase::SetObject(name);
    }
    
};

// ================================================== RESOURCE PATCH SETS ==================================================

enum class ResourceSetVersion
{
    RESOURCE_SET_TRUNK = 0, // latest updated game data
    RESOURCE_SET_PATCH = 1, // bug fix
};

enum class ResourceSetFlags : U32
{
    DYNAMIC = 1, // if it can be unloaded or loaded
    BOOTABLE = 2, // can boot from this resource set
    STICKY = 4, // localisation
    APPLIED = 8, // is currently applied
    PENDING_DESTRUCT = 16,
};

struct ResourceSet
{
    
    String Name;
    ResourceSetVersion Version;
    Flags SetFlags;
    I32 Priority;
    
    std::map<String, String> Mappings; // logical mappings
    
};

// ================================================== STRING MASK HELPER ==================================================

/// A string mask to help find resources.
class StringMask : public String {
public:
    
    // MUST HAVE NO MEMBERS.
    
    enum MaskMode {
        MASKMODE_SIMPLE_MATCH = 0,          // Exact match required (except for wildcards)
        MASKMODE_ANY_SUBSTRING = 1,         // Pattern can match anywhere within `str`
        MASKMODE_ANY_ENDING = 2,            // Pattern can match any suffix of `str`
        MASKMODE_ANY_ENDING_NO_DIRECTORY = 3 // Similar to MASKMODE_ANY_ENDING but ignores directory separators
    };
    
    inline StringMask(const String& mask) : String(mask) {}
    
    inline StringMask(CString mask) : String(mask) {}
    
    /**
     * @brief Checks if a given string matches a search mask with wildcard patterns.
     *
     * @param testString The string to check against the search mask.
     * @param searchMask A semicolon-separated list of patterns (e.g., "*.dlog;*.chore;!private_*").
     *                   - Patterns prefixed with '!' indicate exclusion (negation).
     * @param mode The matching mode from StringMask::MaskMode.
     *             - MASKMODE_SIMPLE_MATCH: Exact match with wildcards.
     *             - MASKMODE_ANY_SUBSTRING: Pattern can appear anywhere.
     *             - MASKMODE_ANY_ENDING: Pattern can match the end.
     *             - MASKMODE_ANY_ENDING_NO_DIRECTORY: Ignores directories, only matches filenames.
     * @param excluded (Optional) If provided, set to `true` if `testString` is explicitly excluded.
     *
     * @return `true` if `testString` matches any pattern, `false` if it is excluded.
     */
    static Bool MatchSearchMask(
                                CString testString,
                                CString searchMask,
                                StringMask::MaskMode mode,
                                Bool* excluded = nullptr);
    
    static Bool MaskCompare(CString pattern, CString str, CString end, MaskMode mode);
    
    inline Bool operator==(const String& rhs) const
    {
        return MatchSearchMask(rhs.c_str(), this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(const String& rhs) const
    {
        return !MatchSearchMask(rhs.c_str(), this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator==(CString rhs) const
    {
        return MatchSearchMask(rhs, this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(CString rhs) const
    {
        return !MatchSearchMask(rhs, this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
};

inline Bool operator==(const String& lhs, const StringMask& mask) // otherway round just in case
{
    return mask == lhs;
}

static_assert(sizeof(String) == sizeof(StringMask), "String and StringMask must have same size and be castable.");

struct ResourceLocation;

// ================================================== RESOURCE DIRECTORIES ==================================================

/**
 A source for resources. This is a virtual class and specific children specify different locations where resources are searched for DataStreams.
 Differnet sub-classes of these represent different archive sources: such as default system file system, ttarchives, network or STFS/playstation bundles.
 */
class RegistryDirectory
{
    
    friend class ResourceRegistry;
    
protected:
    
    String _Path;
    
public:
    
    inline RegistryDirectory(const String& path) : _Path(path)
    {
        std::replace(_Path.begin(), _Path.end(), '\\', '/');
        if(!StringEndsWith(_Path, "/"))
            _Path += "/";
    }
    
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask) = 0;
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask) = 0;
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask) = 0;
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask) = 0;
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/) = 0;
    //virtual ResourceAccessType GetAccess(const Symbol& resource) = 0;
    //virtual Bool SetAccess(const Symbol& resource, ResourceAccessType) = 0;
    //virtual Bool GetResourceInfo(const Symbol& resource, ResourceInfo*) = 0;
    virtual String GetResourceName(const Symbol& resource) = 0;
    virtual Bool DeleteResource(const Symbol& resource) = 0;
    virtual Bool RenameResource(const Symbol& resource, const String& newName) = 0;
    virtual DataStreamRef CreateResource(const String& name) = 0;
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr) = 0;
    virtual DataStreamRef OpenResource(const Symbol& resourceName, String* outName) = 0; // optional outName to get resource name from symbol (we want to support that)
    virtual void RefreshResources() = 0;
    
    // Opens a sub-directory inside this directory
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name) = 0;
    
    virtual ~RegistryDirectory() = default;
    
    // The path for this directory.
    inline const String& GetPath() const
    {
        return _Path;
    }
    
};

/**
 Default system filesystem. We simply use the std::filesystem API
 */
class RegistryDirectory_System : public RegistryDirectory
{
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    
public:
    
    inline RegistryDirectory_System(const String& path) : RegistryDirectory(path), _LastLocatedResource() {}
    
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask); // with self
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names, recursively (set no dups)
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName, String* outName); // open resource
    virtual void RefreshResources();
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name);
    
    virtual ~RegistryDirectory_System() = default;
    
    inline Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck) // no update needed
    {
        return false;
    }
    
};

/**
 Flat telltale games file archive directory.
 */
class RegistryDirectory_TTArchive : public RegistryDirectory
{
    
    friend class ResourceRegistry;
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    
    TTArchive _Archive;
    
public:
    
    inline RegistryDirectory_TTArchive(const String& path, TTArchive&& arc) : RegistryDirectory(path), _LastLocatedResource(), _Archive(std::move(arc)) {}
    
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask); // put all resources into map with self
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names, recursively (set no dups)
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName, String* outName); // open resource
    virtual void RefreshResources(); // refresh
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck); // update from resource syss
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // nothing in archive
    
    virtual ~RegistryDirectory_TTArchive() = default;
    
};

/**
 Flat telltale games file archive directory.
 */
class RegistryDirectory_TTArchive2 : public RegistryDirectory
{
    
    friend class ResourceRegistry;
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    
    TTArchive2 _Archive;
    
public:
    
    inline RegistryDirectory_TTArchive2(const String& path, TTArchive2&& arc) : RegistryDirectory(path), _LastLocatedResource(), _Archive(std::move(arc)) {}
    
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask);
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names, recursively (set no dups)
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName,String* outName); // open resource
    virtual void RefreshResources(); // refresh
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck); // update from resource sys
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // nothing in archive
    
    virtual ~RegistryDirectory_TTArchive2() = default;
    
};

/**
 Heirarchial ISO 9660 standard ISO image used for CD-ROMS with console CDs
 */
class RegistryDirectory_ISO9660 : public RegistryDirectory
{
    
    friend class ResourceRegistry;
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    
    ISO9660 _ISO;
    
public:
    
    inline RegistryDirectory_ISO9660(const String& path, ISO9660&& arc) : RegistryDirectory(path), _LastLocatedResource(), _ISO(std::move(arc)) {}
    
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask);
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names, recursively (set no dups)
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName,String* outName); // open resource
    virtual void RefreshResources(); // refresh
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck); // update from resource sys
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // although its not flat, treat as it is. do nothing here.
    
    virtual ~RegistryDirectory_ISO9660() = default;
    
};

/**
 Legacy console telltale games pack file
 */
class RegistryDirectory_GamePack2 : public RegistryDirectory
{
    
    friend class ResourceRegistry;
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    
    GamePack2 _Pack;
    
public:
    
    inline RegistryDirectory_GamePack2(const String& path, GamePack2&& arc) : RegistryDirectory(path), _LastLocatedResource(), _Pack(std::move(arc)) {}
    
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask);
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names, recursively (set no dups)
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName,String* outName); // open resource
    virtual void RefreshResources(); // refresh
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck); // update from resource sys
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // although its not flat, treat as it is. do nothing here.
    
    virtual ~RegistryDirectory_GamePack2() = default;
    
};

// TODO other directory types: encrypted stuff, STFS saves..

// ================================================== RESOURCE HIGH LEVEL LOCATIONS ==================================================

/**
 Base class for resource locations. These are generated from resource sets.
 */
struct ResourceLocation
{
    
    inline ResourceLocation(const String& name) : Name(name) {}
    
    String Name;
    
    virtual Bool GetResourceNames(std::set<String>& names, const StringMask* optionalMask) = 0;
    
    // gets resources, in a map form, of resource symbol to its directory. eg: mesh.d3dmesh : MCSM_pc_txmesh.ttarch2 (RegistryDirectory_TTArchive2 instance) - ie concrete
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,Ptr<ResourceLocation>&, const StringMask* opmask) = 0;
    
    virtual DataStreamRef LocateResource(const Symbol& name, String* outName) = 0;
    
    virtual Bool HasResource(const Symbol& name) = 0;
    
    virtual String GetPhysicalPath() = 0; // get physical path if this is a system directory
    
    virtual Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck)  { return true; }  // internal update
    
};

/**
 A higher level logical resource directory. These exist when resource sets have been applied.
 */
struct ResourceLogicalLocation : ResourceLocation
{
    
    struct SetInfo
    {
        
        String Set; // set name
        I32 Priority; // priority
        Ptr<ResourceLocation> Resolved; // actual location
        
        inline Bool operator<(const SetInfo& rhs) const
        {
            return Priority > rhs.Priority;
        }
        
    };
    
    inline ResourceLogicalLocation(const String& name) : ResourceLocation(name) {}
    
    std::multiset<SetInfo> SetStack; // higher stack elements searched first when finding resources (eg localisation files instead of english default)
    
    virtual Bool GetResourceNames(std::set<String>& names, const StringMask* optionalMask);
    
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,Ptr<ResourceLocation>&, const StringMask* opmask);
    
    virtual DataStreamRef LocateResource(const Symbol& name, String* outName);
    
    virtual Bool HasResource(const Symbol& name);
    
    inline virtual String GetPhysicalPath() { return ""; } // no physical path
    
};

/**
 A concrete resource directory location. Points to a physical directory.
 */
template <typename T>
struct ResourceConcreteLocation : ResourceLocation
{
    
    static_assert(std::is_base_of<RegistryDirectory, T>::value, "Invalid template parameter");
    
    T Directory;
    
    template <typename... Args>
    ResourceConcreteLocation(const String& name, Args&&... args) : ResourceLocation(name), Directory(std::forward<Args>(args)...) {}
    
    inline Bool GetResourceNames(std::set<String>& names, const StringMask* optionalMask) override
    {
        return Directory.GetResourceNames(names, optionalMask);
    }
    
    inline Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                             Ptr<ResourceLocation>& self, const StringMask* opmask) override
    {
        return Directory.GetResources(resources, self, opmask);
    }
    
    inline DataStreamRef LocateResource(const Symbol& name, String* outName) override
    {
        return Directory.OpenResource(name, outName);
    }
    
    inline Bool HasResource(const Symbol& name) override
    {
        return Directory.HasResource(name, nullptr);
    }
    
    inline String GetPhysicalPath() override
    {
        return Directory.GetPath();
    }
    
    inline Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck) override
    {
        return Directory.UpdateArchiveInternal(resourceName, location, lck);
    }
    
};

// ======================== RESOURCE PRELOADING (INTERNAL) | USE API FROM RESOURCE REGISTRY ================================

// Number of preload batches
#define STATIC_PRELOAD_BATCH_SIZE 32

struct AsyncResourcePreloadBatchJob // async serialise and normalises
{
    
    enum Flag
    {
        BATCH_HIGH_PRIORITY = 1, // overwrite existing resources
    };
    
    Ptr<ResourceRegistry> Registry;
    U32 NumResources = 0; // 0 to 32 below
    Flags BatchFlags;
    HandleObjectInfo HOI[STATIC_PRELOAD_BATCH_SIZE];
    CommonInstanceAllocator* Allocators[STATIC_PRELOAD_BATCH_SIZE];
    
};

struct PreloadBatchJobRef // internal waitable batch job ref
{
    
    U32 PreloadOffset;
    JobHandle Handle;
    
};

Bool _AsyncPerformPreloadBatchJob(const JobThread& thread, void* job, void*);

// ================================================== RESOURCE REGISTRY MAIN CLASS ==================================================

/**
 A resource registry. There can be multiple instances of these. These manage finding resources, preloading batches of resources and resource sets.
 Mount points are where we physically base this resource system, they are the root directories. These can be user savedata, game data Archives, or high level ISOs/STFS packs for consoles.
 These are searched for resource set descriptions when the mount point is created (_resdesc files are searched for _).
 We have logical groups of files called resource sets.
 This resource registry should ONLY be used with one game at once! It assumes the lua version is constant.
 Ensure that these only exist BETWEEN game switches!
 Concrete locations have a trailing slash, while logical locators do not!
 */
class ResourceRegistry : public GameDependentObject, public std::enable_shared_from_this<ResourceRegistry>
{
    
    LuaManager& _LVM; // local LVM used for this registry. Must be alive and acts as a parent!
    
    std::vector<ResourceSet> _ResourceSets; // available high level resource groups
    
    std::vector<Ptr<ResourceLocation>> _Locations; // applied resource sets
    
    std::mutex _Guard; // this is a thread safe class, all calls are safe with this guard.
    
    std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> _DeferredUnloads; // deferred resource unloads
    
    std::vector<String> _DeferredApplications; // to be applied resource sets. will be done in mount when available, not in update.
    
    std::set<HandleObjectInfo> _AliveHandles; // alive handles
    
    std::vector<HandleObjectInfo> _DirtyHandles; // handles requiring any updates
    
    U32 _PreloadOffset; // current number of files preloaded. can wait until this number reaches a given number or force it to be one to ensure loads completed.
    
    U32 _PreloadSize; // total number of preload batches
    
    std::vector<PreloadBatchJobRef> _PreloadJobs;
    
    Ptr<ResourceLocation> _Locate(const String& logicalName); // locate internal no lock
    
    void _ProcessDirtyHandle(HandleObjectInfo&& handle, std::unique_lock<std::mutex>& lck);
    
    void _ProcessDirtyHandles(Float budget, U64 startStamp, std::unique_lock<std::mutex>& lck);
    
    void _CheckLogical(const String& name); // checks asserts its OK.
    
    void _CheckConcrete(const String& name); // checks asserts its OK.
    
    // searches and loads any resource sets (_resdesc_).
    void _ApplyMountDirectory(RegistryDirectory* pMountPoint, std::unique_lock<std::mutex>& lck);
    
    void _BindVM(); // bind to current VM
    
    void _UnbindVM(); // unbind from current VM
    
    ResourceSet* _FindSet(const Symbol& name); // find a resource set
    
    // configure sets and unload resources if needed. can defer until
    void _ReconfigureSets(const std::set<ResourceSet*>& turnOff, const std::set<ResourceSet*>& turnOn, std::unique_lock<std::mutex>& lck, Bool bDefer);
    
    void _UnloadResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                          std::unique_lock<std::mutex>& lck, U32 maxNumUnloads); // perform resource unload
    
    // gather resources to unload for this resource set
    void _GetResourcesToUnload(ResourceSet* pSet, std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& outResources);
    
    // gather mappings
    void _PrepareResourceSet(ResourceSet* pSet, std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>>& patches);
    
    // unapply it
    void _UnapplyResourceSet(ResourceSet* pSet);
    
    void _DestroyResourceSet(ResourceSet* pSet);
    
    void _DoApplyResourceSet(ResourceSet* pSet, const std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>>& patches); // apply resource set
    
    void _LocateResourceInternal(Symbol name, String* outName, DataStreamRef* outStream); // find resource
    
    void _LegacyApplyMount(Ptr<ResourceConcreteLocation<RegistryDirectory_System>>& dir, ResourceLogicalLocation* pMaster,
                           const String& folderID, const String& physicalPath, std::unique_lock<std::mutex>& lck); // open .ttarch, legacy resource system
    
    Bool _ImportArchivePack(const String& resourceName, const String& archiveID, const String& archivePhysicalPath,
                            DataStreamRef& archiveStream, std::unique_lock<std::mutex>& lck); // import ttarch/ttarch2/pk2 into parent as sub
    
    Bool _ImportAllocateArchivePack(const String& resourceName, const String& archiveID, const String& archivePhysicalPath,
                                    Ptr<ResourceLocation>& parent, std::unique_lock<std::mutex>& lck);
    
    Bool _EnsureHandleLoadedLocked(const HandleBase& handle, Bool bOnlyQuery); // locks
    
    Bool _SetupHandleResourceLoad(HandleObjectInfo& hoi, std::unique_lock<std::mutex>& lck); // performs a resource load. finds and opens stream and sets serialise and normalise flags
    
    static StringMask _ArchivesMask(Bool bLegacy);
    
    friend U32 luaResourceSetRegister(LuaManager& man); // access allowed
    
    friend struct ToolContext;
    friend struct ResourcesExtractionTask;
    friend class HandleBase;
    
    friend Bool _AsyncPerformPreloadBatchJob(const JobThread& thread, void* j, void*);
    
    /**
     Constructor. The lua manager version passed in MUST match any game scripts lua versions being run! This is because this will run any resource sets, which may use an older version!
     The lua manager must have the game engine API registered to it!
     By default, the master location is registered ("<>") here. This is a logical location so has no physical path.
     ONLY ACCESSIBLE via use of the ToolContext.
     */
    inline ResourceRegistry(LuaManager& man) : GameDependentObject("ResourceRegistry"), _LVM(man)
    {
        TTE_ASSERT(Meta::GetInternalState().GameIndex != -1, "Resource registries can only be when a game is set!");
        TTE_ASSERT(man.GetVersion() == Meta::GetInternalState().Games[Meta::GetInternalState().GameIndex].LVersion,
                   "The lua manager used for the current game must match the version being used for this resource registry");
        
        CreateLogicalLocation("<>");
    }
    
public:
    
    /**
     Returns true if the current game does not use the resource system and uses the old telltale resource system.
     */
    inline Bool UsingLegacyCompat()
    {
        return !Meta::GetInternalState().GetActiveGame().UsesArchive2();
    }
    
    /**
     Updates the resource registry. If any resource unloads are deferred they will happen here. Doesn't need to be called if you have not explicitly said to defer or preload anything.
     Pass in the time budget you want to maximum spend on this function such that anything not done will get done next call. In seconds.
     */
    void Update(Float timeBudget);
    
    /**
     THIS MUST BE CALLED FROM THE MAIN THREAD. LUA ENVIRONMENT USED IS THE GAMES' ONE!
     Mounts a system directory to this resource registry. The id should be in the format of <XXX>/, as all directories should have. XXX could be <User> for user savedata. Include trailing slash.
     or <Temp> for temp data. Known telltale tool ones are User,Tool,Temp,DiskCache,CloudUserSpace,Cache,SyncFs and <> (empty, meaning master, mapped already) (syncs with filesystem periodically - not implemented here).
     Pass in the path on the actual filesystem of this device, relative or absolute, std::filesystem will manage it.
     Typically you would mount the 'Packs' or 'Archives' folder which will find and load all resource set descriptions.
     This will search this directory for any resource descriptions and load them such that they can be enabled.
     In the actual engine, this function doesn't exist and it loads resdescs differently. The ID passed in from this function typically doesn't need to be used at all in finding resources,
     as the resource sets map to the master location anyway "<>".
     */
    void MountSystem(const String& id, const String& fspath);
    
    /**
     See MountSystem. This version however does not expect a file directory path, but a file such as an archive, ISO or any container pack file supported.
     Archives can be mounted like this if you only want to mount that single archive to the file system. However, prefer to use MountSystem as most games use a combination
     of archives as well as files in the users actual file system. Best use is for ISOs and other file system container files.
     */
    void MountArchive(const String& id, const String& fspath);
    
    /**
     Creates a logical location in the resource system. In URLs, if they start with this name (it must start and end with <>'s), it will look into this location for the rest of the URL.
     You can later map other locations into this one. This is done in resource set scripts automagically.
     All resource searches search from the master location, "<>".
     */
    void CreateLogicalLocation(const String& name);
    
    /**
     Creates a phyiscal (concrete) directory resource location. Logical dir name should be in the format same of MountSystem, eg <Project>/ or '<ProjectDataItalian>/', trailing forward slash!
     Also pass in the directory to map on disk. Can be relative or absolute to your computer.
     */
    void CreateConcreteDirectoryLocation(const String& logicalDirName, const String& physPath);
    
    /**
     Creates an archive location, opening the archive ready to be used. Location ID should be the location to look, eg '<Project>/'. Resouce name should be the filename of the archive,
     Location ID MUST have the trailing slash.
     for example GAME.ttarch.
     This new archive location ID is then <Project>/GAME.ttarch/ with the trailing slash.
     Typically you don't ever need to call this and it will be handles when resource sets are read, as these archive locations are mapped into logical destinations inside other resource sets.
     You should never need to use <Project>/GAME.ttarch/afile.txt to access a file, although it will work. You should want to access by the resource set its part of which will search this archive
     since it would be mapped to this new archive location.
     */
    void CreateConcreteArchiveLocation(const String& locationID, const String& resourceName);
    
    /**
     Checks if the given resource set exists. This is *not* a logical locator, and shouldn't have the angle brackets.
     */
    Bool ResourceSetExists(const Symbol& name);
    
    /**
     Changes the given resource set priority. Note if it is currently applied then this has no effect until it is reloaded.
     */
    void ResourceSetChangePriority(const Symbol& name, I32 priority);
    
    /**
     Enables a resource set. Specify optionally an override priority.
     */
    void ResourceSetEnable(const Symbol& name, I32 priorityOverride = UINT32_MAX);
    
    /**
     Disables a resource set. Unloads its resources, releasing the references. Optionally specify to defer any unloads to a later Update call such that work is spread out.
     */
    void ResourceSetDisable(const Symbol& name, Bool bDeferUnloads);
    
    /**
     Destroys and removes the given resource set. Finding it will return nothing after this call. Pass in if you want to defer any unloads to another Update() call if its being called periodically.
     */
    void ResourceSetDestroy(const Symbol& name, Bool bDefer);
    
    /**
     Returns if the given resource set is currently enabled (applied)
     */
    Bool ResourceSetIsEnabled(const Symbol& name);
    
    /**
     Returns if the given resource set is a bootable resource set.
     */
    Bool ResourceSetIsBootable(const Symbol& name);
    
    /**
     Returns if the given resource set is dynamic.
     */
    Bool ResourceSetIsDynamic(const Symbol& name);
    
    /**
     Returns if the given resource set is sticky.
     */
    Bool ResourceSetIsSticky(const Symbol& name);
    
    /**
     Maps locations. Pass in the resouce set which you want add this mapping to (first argument).
     When this resource set is enabled, any resources associated in the source resource set will be accessible via the destination resource set.
     Note that lots of locations can be mapped into one destination, so higher priority resources will be found and loaded first while a resource set is enabled.
     */
    void ResourceSetMapLocation(const Symbol& name, const String& src, const String& dst);
    
    /**
     Creates a resource set.
     */
    void ResourceSetCreate(const String& name, I32 priority, ResourceSetVersion version, Bool isDynamic, Bool isBootable, Bool isSticky = false);
    
    /**
     Configures resource sets. Turns off and on specific groups of resource sets. If any defer unload is true,, ensure Update() gets called regularly.
     */
    void ReconfigureResourceSets(const std::set<Symbol>& turnOff, const std::set<Symbol>& turnOn, Bool bDeferUnloads);
    
    /**
     Returns the concrete location ID for the given resource. Eg '/users/..../archives/file.text' will return the location name for /users/..../archives/.
     */
    String ResourceAddressResolveToConcreteLocationID(const String& address);
    
    // Returns the bottom level locator
    String ResourceAddressGetResourceName(const String& address);
    
    // Dumps all locations to console
    void PrintLocations();
    
    // Dumps all resource sets to console
    void PrintSets();
    
    // Most important in this class. Finds a resource in the currently enabled patch sets, highest loaded priority one will be returned.
    DataStreamRef FindResource(const Symbol& name);
    
    // Like FindResource but gets the name
    String FindResourceName(const Symbol& name);
    
    // Gets all resource names which match the optional mask, else all, in all currently resource sets.
    void GetResourceNames(std::set<String>& outNames, const StringMask* optionalMask);
    
    // Sets a set of resources to be loaded, which will be loaded async.
    // The handles should have been set with the resource names but not loaded, ie SetObject with the booleans all false.
    // Set overwrite to true to overwrite any existing resources which may be loaded when inserting.
    U32 Preload(std::vector<HandleBase>&& resourceHandles, Bool bOverwrite);
    
    // Preload offset. If bigger or equal to a return value of a previous Preload(), you can ensure all of those handles have loaded.
    U32 GetPreloadOffset();
    
    // Wait until preload offset finishes. Must be a return value from Preload
    void WaitPreload(U32 preloadOffset);
    
};
