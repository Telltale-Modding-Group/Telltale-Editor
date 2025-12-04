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
#include <Resource/PSPKG.hpp>
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
#include <thread>

// folders excluded from disk unless explicitly mounted when recursively going through directories
#define EXCLUDE_SYSTEM_FILTER "!*.DS_Store;!*.app/*"

// HIGH LEVEL TELLTALE RESOURCE SYSTEM

// ================================================== RESOURCE LIFETIME MANAGEMENT ==================================================

template<typename T>
inline HandleableRegistered<T>::HandleableRegistered(Ptr<ResourceRegistry> reg) : Handleable(std::move(reg))
{
    // Register coersion for common T
    REGISTER_MY_COERSION(T);
}

template<typename T>
inline Ptr<Handleable> HandleableRegistered<T>::Clone() const
{
    Ptr<Handleable> pCloned = TTE_NEW_PTR(T, MEMORY_TAG_COMMON_INSTANCE, *dynamic_cast<const T*>(this));
    return pCloned;
}

enum class HandleFlags
{
    NEEDS_NORMALISATION = 1, // needs to be normalised
    NEEDS_SERIALISE_IN = 2, // needs to be serialised in from the stream
    NEEDS_DESTROY = 4, // needs to be destroyed
    LOADED = 8, // has been normalised into, so is loaded.
    NON_PURGABLE = 16, // flag 1 in engine
    CACHE_ONLY = 32, // cache only, cannot be saved, is a runtime resource
    
    // other flags in future, serialise out needed, load dependent resources (eg textures in a mesh, non embeds in chore etc)
};

// Internal handle object info used by registry. Handles refer to these under the hood.
struct HandleObjectInfo
{
    
    Symbol _ResourceName; // name of this resource
    String _Locator; // location of the resource for re-saving.
    Meta::ClassInstance _Instance; // instance in the meta system
    Ptr<Handleable> _Handle; // pointer to actual alive common instance
    DataStreamRef _OpenStream; // if it needs to be serialised in or out this is the stream to/from
    mutable Flags _Flags; // any flags
    
    void _OnUnload(ResourceRegistry& registry, std::unique_lock<std::recursive_mutex>& lck); // called when unloading, may be async
    
    inline Bool operator<(const HandleObjectInfo& rhs) const
    {
        return _ResourceName < rhs._ResourceName;
    }
    
    inline Bool operator==(const HandleObjectInfo& rhs) const
    {
        return _ResourceName == rhs._ResourceName;
    }

    ~HandleObjectInfo();
    
};

// Non-templated version for below
class HandleBase
{
    
    friend class ResourceRegistry;
    
protected:
    
    Ptr<Handleable> _GetObject(Ptr<ResourceRegistry>& registry, Meta::ClassInstance* pClassOut = nullptr);
    
    Symbol _ResourceName;
    
    void _SetObject(Ptr<ResourceRegistry>& registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded);
    
public:
    
    inline HandleBase(Symbol rn) : _ResourceName(rn) {}

    HandleBase() = default;
    
    Bool IsLoaded(Ptr<ResourceRegistry>& registry); // return if its currently loaded. Will return false if there is a future load in progress which hasn't been progressed
    
    void EnsureIsLoaded(Ptr<ResourceRegistry>& registry); // ensure this handle is currently loaded, the latest load
    
    // sets object file (eg a .d3dmesh). Specify to unload old resource, and if you want to ensure its loaded.
    template<typename T>
    inline void SetObject(Ptr<ResourceRegistry>& registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
    {
        _SetObject(registry, name, bUnloadOld, bEnsureLoaded);
    }
    
    inline void SetObject(Symbol name)
    {
        _ResourceName = name;
    }
    
    // Get resource name
    inline Symbol GetObject() const
    {
        return _ResourceName;
    }
    
    // Get resource name using symbol map lookup
    inline String GetObjectResolved() const
    {
        return SymbolTable::Find(_ResourceName);
    }
    
    inline Ptr<Handleable> GetBlindObject(Ptr<ResourceRegistry>& registry, Bool bEnsureLoaded)
    {
        if (bEnsureLoaded)
            EnsureIsLoaded(registry);
        return _GetObject(registry, nullptr);
    }
    
};

// Handle to a common resource. This is basically a templated version of HandleBase, but caches the loaded handle to the common resource.
template<typename T>
class Handle : public HandleBase
{
    
    Ptr<T> _Cached;
    
    static_assert(std::is_base_of<Handleable, T>::value, "T must be handleable");
    
    inline Handle(Symbol rn) : HandleBase(rn), _Cached{}
    {
        REGISTER_MY_COERSION(Handle<T>);
    }
    
public:
    
    inline Handle() : Handle(Symbol())
    {
    }
    
    // Gets the underlying resouce. The resource will always be valid but may not be loaded. You can use other functionality to ensure its loaded.
    inline Ptr<T> GetObject(Ptr<ResourceRegistry> registry, Bool bEnsureLoaded)
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
    
    inline void SetObject(Ptr<ResourceRegistry> registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
    {
        if(name != _ResourceName)
            _Cached = {};
        return HandleBase::SetObject<T>(registry, name, bUnloadOld, bEnsureLoaded);
    }
    
    // Similar but does not load just sets the name, can be loaded later.
    inline void SetObject(Symbol name)
    {
        if(name != _ResourceName)
            _Cached = {};
        HandleBase::SetObject(name);
    }
    
    // Flushes the cached value, meaning if its updated the cached resource here will get updated next GetObject
    inline void EnsureIsLoaded(Ptr<ResourceRegistry>& registry)
    {
        HandleBase::EnsureIsLoaded(registry);
        _Cached.reset();
    }
    
    inline Bool IsLoaded(Ptr<ResourceRegistry>& registry)
    {
        return HandleBase::IsLoaded(registry);
    }
    
    inline Symbol GetObject() const
    {
        return HandleBase::GetObject();
    }
    
    inline Bool operator<(const Handle& rhs)
    {
        return _ResourceName < rhs._ResourceName;
    }
    
    inline operator Bool() const
    {
        return _ResourceName.GetCRC64() != 0;
    }
    
};

// Special type for prop sets. Use HandlePropertySet alias.
template<>
class Handle<Placeholder> : public HandleBase
{

    Meta::ClassInstance _Cached;
    
    inline Handle(Symbol rn) : HandleBase(rn), _Cached{}
    {
    }
    
public:
    
    inline Handle() : HandleBase(Symbol()) {}
    
    // Gets the underlying resouce. The resource will always be valid but may not be loaded. You can use other functionality to ensure its loaded.
    inline Meta::ClassInstance GetObject(Ptr<ResourceRegistry> registry, Bool bEnsureLoaded)
    {
        if(_Cached)
            return _Cached;
        if(bEnsureLoaded)
            HandleBase::EnsureIsLoaded(registry);
        _GetObject(registry, &_Cached);
        return _Cached;
    }
    
    inline void SetObject(Ptr<ResourceRegistry> registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
    {
        if(name != _ResourceName)
            _Cached = {};
        _SetObject(registry, name, bUnloadOld, bEnsureLoaded);
    }
    
    // Flushes the cached value, meaning if its updated the cached resource here will get updated next GetObject
    inline void EnsureIsLoaded(Ptr<ResourceRegistry>& registry)
    {
        HandleBase::EnsureIsLoaded(registry);
        _Cached = {};
    }
    
    inline Bool IsLoaded(Ptr<ResourceRegistry>& registry)
    {
        return HandleBase::IsLoaded(registry);
    }
    
    inline Symbol GetObject() const
    {
        return HandleBase::GetObject();
    }
    
    // Similar but does not load just sets the name, can be loaded later.
    inline void SetObject(Symbol name)
    {
        if(name != _ResourceName)
            _Cached = {};
        HandleBase::SetObject(name);
    }
    
    inline Bool operator<(const Handle& rhs) const
    {
        return _ResourceName < rhs._ResourceName;
    }
    
    inline operator Bool() const
    {
        return _ResourceName.GetCRC64() != 0;
    }
    
};

// Special handle to a property set (ie meta instance)
using HandlePropertySet = Handle<Placeholder>;

namespace Meta::_Impl
{
    
    DECL_COERSION(Handle<Placeholder>, "Handle<PropertySet>")
    {
        static void Extract(Handle<Placeholder>& out, ClassInstance& inst);
        static void Import(const Handle<Placeholder>& in, ClassInstance& inst);
        static void ImportLua(const Handle<Placeholder>& in, LuaManager& man);
        static void ExtractLua(Handle<Placeholder>& out, LuaManager& man, I32 stackIndex);
    };
    
    DECL_COERSION_TP(Handle, T::ClassHandle)
    {
        static inline void Extract(Handle<T>& out, ClassInstance& inst)
        {
            ClassInstance mem = GetMember(inst, "mHandle", true); // mHandle must exist (It should in all games).
            TTE_ASSERT(Is(mem, "Symbol") || Is(mem, "class Symbol"), "Handle<T>::mHandle is not a Symbol");
            Symbol val{};
            Meta::ExtractCoercableInstance(val, mem);
            out.SetObject(val);
        }
        
        static inline void Import(const Handle<T>& in, ClassInstance& inst)
        {
            ClassInstance mem = GetMember(inst, "mHandle", true);
            TTE_ASSERT(Is(mem, "Symbol") || Is(mem, "class Symbol"), "Handle<T>::mHandle is not a Symbol");
            Symbol val = in.GetObject();
            Meta::ImportCoercableInstance(val, mem);
        }
        
        static inline void ImportLua(const Handle<T>& in, LuaManager& man)
        {
            man.PushLString(SymbolTable::FindOrHashString(in.GetObject()));
        }
        
        static inline void ExtractLua(Handle<T>& out, LuaManager& man, I32 stackIndex)
        {
            out.SetObject(ScriptManager::ToSymbol(man, stackIndex));
        }

    };

    
}

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

struct ResourceLocation;

// Resolved resource address. Location is the folder / location (eg master locator, directory on disk or a logical location). Name is the name of the resource.
class ResourceAddress
{
    
    friend class ResourceRegistry;
    
    String LocationName; // name of location. Empty for default
    String Name;
    Bool IsCache = false;
    
public:
    
    inline const String& GetLocationName() const
    {
        return LocationName;
    }
    
    inline const String& GetName() const
    {
        return Name;
    }
    
    inline operator Bool() const
    {
        return Name.length() > 0;
    }
    
};

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
    
    inline RegistryDirectory_System(const String& path) : RegistryDirectory(path), _LastLocatedResource()
    {
        if(!std::filesystem::is_directory(path))
        {
            Bool bOk;
            try
            {
                std::filesystem::create_directories(path);
                bOk = !std::filesystem::is_directory(path);
            }
            catch (...)
            {
                bOk = false;
            }
            if (!bOk)
            {
                RegistryDirectory::_Path = ".";
                TTE_LOG("WARNING: Registry system directory '%s' does not exist!", path.c_str());
            }
        }
    }
    
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
    
    inline Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck) // no update needed
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
    
public:
    
    TTArchive _Archive;
    
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
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck); // update from resource syss
    
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
    
public:
    
    TTArchive2 _Archive;
    
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
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck); // update from resource sys
    
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
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck); // update from resource sys
    
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
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck); // update from resource sys
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // although its not flat, treat as it is. do nothing here.
    
    virtual ~RegistryDirectory_GamePack2() = default;
    
};

/**
 Legacy console telltale games pack file
 */
class RegistryDirectory_PlaystationPKG : public RegistryDirectory
{
    
    friend class ResourceRegistry;
    
    String _LastLocatedResource;
    Bool _LastLocatedResourceStatus = false; // true if it existed
    String _PackageKey;
    PlaystationPKG _PKG;
    
public:
    
    inline RegistryDirectory_PlaystationPKG(const String& path, const String& pkKey, PlaystationPKG&& arc)
            : RegistryDirectory(path), _LastLocatedResource(), _PKG(std::move(arc)), _PackageKey(pkKey) {}
    
    virtual Bool GetResourceNames(std::set<String>& resources, const StringMask* optionalMask); // get file names
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                              Ptr<ResourceLocation>& self, const StringMask* optionalMask);
    virtual Bool GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // get sub directory names
    virtual Bool GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask); // empty return here
    virtual Bool HasResource(const Symbol& resourceName, const String* actualName /*optional*/); // pass in actual name if you know it to speed up.
    virtual String GetResourceName(const Symbol& resource); // to string resource name (quicker than symbol table
    virtual Bool DeleteResource(const Symbol& resource); // delete it if we can
    virtual Bool RenameResource(const Symbol& resource, const String& newName); // rename it
    virtual DataStreamRef CreateResource(const String& name); // create resource and open writing stream
    virtual Bool CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr); // copy resource to dest
    virtual DataStreamRef OpenResource(const Symbol& resourceName,String* outName); // open resource
    virtual void RefreshResources(); // refresh
    
    Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck); // update from resource sys
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // although its not flat, treat as it is. do nothing here.
    
    virtual ~RegistryDirectory_PlaystationPKG() = default;
    
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
    
    inline virtual RegistryDirectory* GetConcreteDirectory() { return nullptr; }
    
    virtual RegistryDirectory* LocateConcreteDirectory(const Symbol& resourceName) = 0;
    
    virtual Bool GetResourceNames(std::set<String>& names, const StringMask* optionalMask) = 0;
    
    // gets resources, in a map form, of resource symbol to its directory. eg: mesh.d3dmesh : MCSM_pc_txmesh.ttarch2 (RegistryDirectory_TTArchive2 instance) - ie concrete
    virtual Bool GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,Ptr<ResourceLocation>&, const StringMask* opmask) = 0;
    
    virtual DataStreamRef LocateResource(const Symbol& name, String* outName) = 0;
    
    virtual Bool HasResource(const Symbol& name) = 0;
    
    virtual String GetPhysicalPath() = 0; // get physical path if this is a system directory
    
    virtual Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck)  { return true; }  // internal update
    
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
    
    virtual RegistryDirectory* LocateConcreteDirectory(const Symbol& resourceName);
    
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
    
    inline virtual RegistryDirectory* LocateConcreteDirectory(const Symbol& resourceName) override
    {
        return HasResource(resourceName) ? &Directory : nullptr;
    }
    
    inline virtual RegistryDirectory* GetConcreteDirectory() override
    {
        return &Directory;
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
    
    inline Bool UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck) override
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
    
};

struct PreloadBatchJobRef // internal waitable batch job ref
{
    
    U32 PreloadOffset;
    JobHandle Handle;
    
};

struct FunctionBase;

struct PreloadCallback
{
    U32 PreloadFence;
    U32 CallbackLockToCalleeThread = 0;
    Ptr<FunctionBase> Callback;
    std::vector<Symbol> Resources;
    std::thread::id CalleeThread;
    String UpdateMask;
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
class ResourceRegistry : public SnapshotDependentObject, public std::enable_shared_from_this<ResourceRegistry>
{
public:
    
    void BindLuaManager(LuaManager& man);
    
    static Ptr<ResourceRegistry> GetBoundRegistry(LuaManager& man);
    
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
     Optionally pass in the update ID as to which non lock-to-thread callbacks to execute, empty does all but you can provide a different one such that uMask in a PreloadWithCallback
     call has to match updateId for it to be processed.
     */
    void Update(Float timeBudget, String updateId = "");
    
    /**
     THIS MUST BE CALLED FROM THE MAIN THREAD. LUA ENVIRONMENT USED IS THE GAMES' ONE!
     Mounts a system directory to this resource registry. The id should be in the format of <XXX>/, as all directories should have. XXX could be <User> for user savedata. Include trailing slash.
     or <Temp> for temp data. Known telltale tool ones are User,Tool,Temp,DiskCache,CloudUserSpace,Cache,SyncFs and <> (empty, meaning master, mapped already) (syncs with filesystem periodically - not implemented here).
     Pass in the path on the actual filesystem of this device, relative or absolute, std::filesystem will manage it.
     Typically you would mount the 'Packs' or 'Archives' folder which will find and load all resource set descriptions.
     This will search this directory for any resource descriptions and load them such that they can be enabled.
     In the actual engine, this function doesn't exist and it loads resdescs differently. The ID passed in from this function typically doesn't need to be used at all in finding resources,
     as the resource sets map to the master location anyway "<>".
     Set force legacy to true to recursive and get all resources like the old resource system telltale used (no resource sets). Useful for debugging and just getting all resources.
     */
    void MountSystem(const String& id, const String& fspath, Bool bForceLegacy);
    
    /**
     See MountSystem. This version however does not expect a file directory path, but a file such as an archive, ISO or any container pack file supported.
     Archives can be mounted like this if you only want to mount that single archive to the file system. However, prefer to use MountSystem as most games use a combination
     of archives as well as files in the users actual file system. Best use is for ISOs and other file system container files.
     */
    void MountArchive(const String& id, const String& fspath);
    
    /**
     See MountArchive and MountSystem. This version of mount archive does the same but for a PSP/PS3/etc PKG file. Pass in the package key name (see keys lua script).
     */
    void MountPlaystationPackage(const String& id, const String& fsPath, const String& packageKey);
    
    /**
     Creates a logical location in the resource system. In URLs, if they start with this name (it must start and end with <>'s), it will look into this location for the rest of the URL.
     You can later map other locations into this one. This is done in resource set scripts automagically.
     All resource searches search from the master location, "<>".
     */
    void CreateLogicalLocation(const String& name);
    
    /**
     Creates a phyiscal (concrete) directory resource location. Logical dir name must end with a, trailing forward slash! Name typically tends to be '<User>/' for example, but ''c:/path/" is ok too
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
     Tests if the resource location with the give name exists
     */
    Bool ResourceLocationExists(const Symbol& name);
    
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
     Return the name of the resource concrete location ID containing the given resource name in the resource set.
     */
    String ResourceSetGetLocationID(const Symbol& setName, const Symbol& resourceName);
    
    /**
     For a loaded resource, sets the purgability of the resource name. When on, the resource won't be unloaded at any time unless this is called again with false.
     */
    void ResourceSetNonPurgable(const Symbol& resourceName, Bool bOnOff);
    
    /**
     Sets the default resource location. Locator symbols will look here first.
     */
    void ResourceSetDefaultLocation(const String& id);
    
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
     Attempts to locate the resousrce name for a memory address. For property sets, this can be the meta instance internal memory, for common classes such as Scene this is the raw scene pointer
     you get from the Handle<T>::GetObject.
     */
    String LocateResource(const void* pResourceMemory);
    
    // Unloads a resource, If not purgable or does not exist (loaded) returns false.
    Bool UnloadResource(const Symbol& resourceName);
    
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
    
    // Open resource data stream from logical location. Not used for loaded
    DataStreamRef FindResourceFrom(const String& loc, const Symbol& filename);

    // Similar to find resource functions but will create the stream if needed on disk. Returns the FILE stream (not a memory cache stream)
    DataStreamRef OpenDataStream(const String& loc, const String& filename);
    
    // Like FindResource but gets the name
    String FindResourceName(const Symbol& name);
    
    // Gets all resource names which match the optional mask, else all, in all currently resource sets.
    void GetResourceNames(std::set<String>& outNames, const StringMask* optionalMask);
    
    // Gets all resource names which match the optional mask, else all. Specify the resource location to search in.
    void GetLocationResourceNames(const Symbol& location, std::set<String>& outNames, const StringMask* optionalMask);
    
    // Returns true if the given resource exists on file or in cache
    Bool ResourceExists(const Symbol& resourceName);
    
    // Creates a resource on file and saves it empty.
    Bool CreateResource(const ResourceAddress& address);
    
    // Revert resource to the one on disk, or first found file in the resource system.
    Bool RevertResource(const Symbol& resourceName);
    
    // Saves the given resource name. Must be loaded in the cache else returns false and does nothing. Saves to the first resource location matching its name (use sets!)
    // Optionally if you don't want it to save to its default output location using resource sets, specify a non-empty location being the resource location name.
    Bool SaveResource(const Symbol& resourceName, const String& location);
    
    // Deletes the resource
    void DeleteResource(const Symbol& resourceName);
    
    // Copies the resource. Copies into the same resource location.
    void CopyResource(const Symbol& resourceName, const String& destName);
    
    // Locates the concreteresource location of the given file name. Returns the name of the location. Will return empty string if not found or only in the cache.
    String LocateConcreteResourceLocation(const Symbol& resourceName);
    
    // See version with string. Finds it, must exist.
    ResourceAddress CreateResolvedAddressFromSymbol(const Symbol& resourceName);
    
    // Create a resolved resouce address. Include folder path, file name, prefix scheme (optional default file)
    // Valid path may be '<User>/file.txt' or 'ttcache:module_prop.prop'. set default to cache to default to the cache if no scheme (normal) (else locator)
    ResourceAddress CreateResolvedAddress(const String& resourceName, Bool bDefaultToCache);
    
    // Sets a set of resources to be loaded, which will be loaded async.
    // The handles should have been set with the resource names but not loaded, ie SetObject with the booleans all false.
    // Set overwrite to true to overwrite any existing resources which may be loaded when inserting.
    U32 Preload(std::vector<HandleBase>&& resourceHandles, Bool bOverwrite);
    
    // See Preload(). Preload but with a callback, in which a vector of symbols is passed (the resource names), as a const std::vector<Symbol>*!! POINTER!
    // Specify to lock the callback to only be called on the thread which calls this. In this case, you need to periodically call Update on this thread.
    // Also specify a mask (default *) in which that must be passed into ResourceRegistry::Update to process your callback, for finer control.
    U32 PreloadWithCallback(std::vector<HandleBase>&& resourceHandles, Bool bOverwrite, Ptr<FunctionBase> pCallback, Bool bLockCallbackToCalleeThread, String uMask = "*");
    
    // Preload offset. If bigger or equal to a return value of a previous Preload(), you can ensure all of those handles have loaded.
    U32 GetPreloadOffset();
    
    // Wait until preload offset finishes. Must be a return value from Preload
    void WaitPreload(U32 preloadOffset);
    
    // Helper to create a handle to a file. Example usage: auto myHandle = Registry->MakeHandle<Animation>("AnimationFile.anm", true)
    template<typename T>
    inline Handle<T> MakeHandle(const Symbol& FileName, Bool bEnsureLoaded)
    {
        Handle<T> hHandle{};
        hHandle.SetObject(shared_from_this(), FileName, false, bEnsureLoaded);
        return hHandle;
    }

    void GetResourceLocationNames(std::vector<String>& names);

    // Creates unsavable cached resource in memory
    inline Bool CreateCachedResource(String name, Ptr<Handleable> pObject)
    {
        TTE_ASSERT(pObject, "The handleable object cannot be null!");
        return _CreateCachedResourceUnlocked(name, pObject, {});
    }

    inline Bool CreateCachedPropertySet(String name, Meta::ClassInstance propInstance)
    {
        TTE_ASSERT(propInstance, "The property set cannot be null!");
        return _CreateCachedResourceUnlocked(name, {}, propInstance);
    }
    
    ~ResourceRegistry();
    
private:
    
    // ========== INTERNAL STATE
    
    LuaManager& _LVM; // local LVM used for this registry. Must be alive and acts as a parent!
    
    std::vector<ResourceSet> _ResourceSets; // available high level resource groups
    
    std::vector<Ptr<ResourceLocation>> _Locations; // applied resource sets
    
    std::recursive_mutex _Guard; // this is a thread safe class, all calls are safe with this guard.
    
    std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> _DeferredUnloads; // deferred resource unloads
    
    std::vector<String> _DeferredApplications; // to be applied resource sets. will be done in mount when available, not in update.
    
    std::set<HandleObjectInfo> _AliveHandles; // alive handles
    
    std::vector<HandleObjectInfo> _DirtyHandles; // handles requiring any updates
    
    U32 _PreloadOffset; // current number of files preloaded. can wait until this number reaches a given number or force it to be one to ensure loads completed.
    
    U32 _PreloadSize; // total number of preload batches
    
    std::vector<PreloadBatchJobRef> _PreloadJobs;
    
    String _DefaultLocation = "<>";
    
    std::set<String> _ErrorFiles; // reduce multi-log
    
    std::vector<PreloadCallback> _PreloadCallbacks; // post load callbacks
    
    // ========== INTERNAL FUNCTIONALITY
    
    Ptr<ResourceLocation> _Locate(const String& logicalName); // locate internal no lock
    
    void _AsyncProcessCallbacksUnlocked(CString optionalMask);

    Bool _CreateCachedResourceUnlocked(const String& name, Ptr<Handleable> asHandleable, Meta::ClassInstance asProp);
    
    void _ProcessDirtyHandle(HandleObjectInfo&& handle, std::unique_lock<std::recursive_mutex>& lck);
    
    void _ProcessDirtyHandles(Float budget, U64 startStamp, std::unique_lock<std::recursive_mutex>& lck);
    
    void _CheckLogical(const String& name); // checks asserts its OK.
    
    void _CheckConcrete(const String& name); // checks asserts its OK.
    
    void _ApplyMountArchive(const String& id, const String& fspath, const String& packageKey);
    
    // searches and loads any resource sets (_resdesc_).
    void _ApplyMountDirectory(RegistryDirectory* pMountPoint, std::unique_lock<std::recursive_mutex>& lck);

    ResourceSet* _FindSet(const Symbol& name); // find a resource set
    
    // configure sets and unload resources if needed. can defer until
    void _ReconfigureSets(const std::set<ResourceSet*>& turnOff, const std::set<ResourceSet*>& turnOn, std::unique_lock<std::recursive_mutex>& lck, Bool bDefer);
    
    void _UnloadResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                          std::unique_lock<std::recursive_mutex>& lck, U32 maxNumUnloads); // perform resource unload
    
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
                           const String& folderID, const String& physicalPath, std::unique_lock<std::recursive_mutex>& lck); // open .ttarch, legacy resource system
    
    Bool _ImportArchivePack(const String& resourceName, const String& archiveID, 
                            const String& archivePhysicalPath, const String& packageKey,
                            DataStreamRef& archiveStream, std::unique_lock<std::recursive_mutex>& lck); // import ttarch/ttarch2/pk2 into parent as sub
    
    Bool _ImportAllocateArchivePack(const String& resourceName, const String& archiveID, const String& archivePhysicalPath,
                                    Ptr<ResourceLocation>& parent, std::unique_lock<std::recursive_mutex>& lck);
    
    Bool _EnsureHandleLoadedLocked(const HandleBase& handle, Bool bOnlyQuery); // locks
    
    Bool _SetupHandleResourceLoad(HandleObjectInfo& hoi, std::unique_lock<std::recursive_mutex>& lck); // performs a resource load. finds and opens stream and sets serialise and normalise flags
    
    void _InsertSymbolTable(const String& fileName);
    
    static StringMask _ArchivesMask(Bool bLegacy);
    
    friend U32 luaResourceSetRegister(LuaManager& man); // access allowed
    
    friend struct ToolContext;
    friend struct ResourcesExtractionTask;
    friend class HandleBase;
    
    friend Bool _AsyncPerformPreloadBatchJob(const JobThread& thread, void* j, void*);
    friend U32 luaResourceSetGetAll(LuaManager& man);
    friend U32 luaResourceReportReferencedAssets(LuaManager& man);
    friend U32 luaResourceExistsLogicalLocation(LuaManager& man);
    friend U32 luaResourceExists(LuaManager& man);
    friend U32 luaResourceCreateLogicalLocation(LuaManager& man);
    friend U32 luaResourceArchiveFind(LuaManager& man);
    friend U32 luaResourceArchiveIsActive(LuaManager& man);
    friend U32 luaResourceCopy(LuaManager& man);
    friend U32 luaLoad(LuaManager& man);
    friend U32 luaFileCopy(LuaManager& man);
    friend U32 luaFileDelete(LuaManager& man);
    friend U32 luaFileExists(LuaManager& man);
    friend U32 luaResourceGetSymbolsNames(LuaManager& man);
    
    /**
     Constructor. The lua manager version passed in MUST match any game scripts lua versions being run! This is because this will run any resource sets, which may use an older version!
     The lua manager must have the game engine API registered to it!
     By default, the master location is registered ("<>") here. This is a logical location so has no physical path.
     ONLY ACCESSIBLE via use of the ToolContext.
     */
    ResourceRegistry(LuaManager& man);
    
};

