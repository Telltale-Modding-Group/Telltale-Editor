#pragma once

#include <Core/Config.hpp>
#include <Core/Symbol.hpp>
#include <Resource/Compression.hpp>
#include <Resource/DataStream.hpp>
#include <Core/LinearHeap.hpp>
#include <Core/BitSet.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp>
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

// HIGH LEVEL TELLTALE RESOURCE SYSTEM

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

/// A string mask to help find resources.
class StringMask : public String {
public:
    
    enum MaskMode {
        MASKMODE_SIMPLE_MATCH = 0,          // Exact match required (except for wildcards)
        MASKMODE_ANY_SUBSTRING = 1,         // Pattern can match anywhere within `str`
        MASKMODE_ANY_ENDING = 2,            // Pattern can match any suffix of `str`
        MASKMODE_ANY_ENDING_NO_DIRECTORY = 3 // Similar to MASKMODE_ANY_ENDING but ignores directory separators
    };
    
    /**
     * @brief Checks if a given string matches a search mask with wildcard patterns.
     *
     * @param testString The string to check against the search mask.
     * @param searchMask A semicolon-separated list of patterns (e.g., "*.dlog;*.chore;-private_*").
     *                   - Patterns prefixed with '-' indicate exclusion (negation).
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
        return MaskCompare(this->c_str(), rhs.c_str(), rhs.c_str() + rhs.length(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(const String& rhs) const
    {
        return !MaskCompare(this->c_str(), rhs.c_str(), rhs.c_str() + rhs.length(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator==(CString rhs) const
    {
        return MaskCompare(this->c_str(), rhs, nullptr, MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(CString rhs) const
    {
        return !MaskCompare(this->c_str(), rhs, nullptr, MASKMODE_SIMPLE_MATCH);
    }
    
};

struct ResourceLocation;

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
    
    virtual Ptr<RegistryDirectory> OpenDirectory(const String& name); // nothing in archive
    
    virtual ~RegistryDirectory_TTArchive2() = default;
    
};

// TODO other directory types: PS3 encrypted ISOs (AES 128), standard PlayStation ISOs (decrypted, detect), same for xbox and wii and nx

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
    
};

/**
 A higher level logical resource directory. These exist when resource sets have been applied.
 */
struct ResourceLogicalLocation : ResourceLocation
{
    
    struct SetInfo
    {
        
        String Set; // set name
        U32 Priority; // priority
        Ptr<ResourceLocation> Resolved; // actual location
        
        inline Bool operator<(const SetInfo& rhs) const
        {
            return Priority < rhs.Priority;
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
    
    inline bool HasResource(const Symbol& name) override
    {
        return Directory.HasResource(name, nullptr);
    }
    
    inline String GetPhysicalPath() override
    {
        return Directory.GetPath();
    }
    
};


/**
 A resource registry. There can be multiple instances of these. These manage finding resources, preloading batches of resources and resource sets.
 Mount points are where we physically base this resource system, they are the root directories. These can be user savedata, game data Archives, or high level ISOs/STFS packs for consoles.
 These are searched for resource set descriptions when the mount point is created (_resdesc files are searched for _).
 We have logical groups of files called resource sets.
 This resource registry should ONLY be used with one game at once! It assumes the lua version is constant.
 Ensure that these only exist BETWEEN game switches!
 Concrete locations have a trailing slash, while logical locators do not!
 */
class ResourceRegistry : public GameDependentObject
{
    
    LuaManager& _LVM; // local LVM used for this registry. Must be alive and acts as a parent!

    std::vector<ResourceSet> _ResourceSets; // available high level resource groups
    
    std::vector<Ptr<ResourceLocation>> _Locations; // applied resource sets
    
    std::mutex _Guard; // this is a thread safe class, all calls are safe with this guard.
    
    std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> _DeferredUnloads; // deferred resource unloads
    
    Ptr<ResourceLocation> _Locate(const String& logicalName); // locate internal no lock
    
    void _CheckLogical(const String& name); // checks asserts its OK.
    
    void _CheckConcrete(const String& name); // checks asserts its OK.
    
    // searches and loads any resource sets (_resdesc_).
    void _ApplyMountDirectory(RegistryDirectory* pMountPoint, std::unique_lock<std::mutex>& lck);
    
    void _BindVM(); // bind to current VM
    
    void _UnbindVM(); // unbind from current VM
     
    ResourceSet* _FindSet(const Symbol& name); // find a resource set
    
    // configure sets and unload resources if needed. can defer until
    void _ReconfigureSets(const std::set<ResourceSet*>& turnOff, const std::set<ResourceSet*>& turnOn, std::unique_lock<std::mutex>& lck, Bool bDefer);
    
    void _UnloadResources(const std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
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
    
    friend U32 luaResourceSetRegister(LuaManager& man); // access allowed
    
    friend struct ToolContext;
    
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
     Updates the resource registry. If any resource unloads are deferred they will happen here. Doesn't need to be called if you have not explicitly said to defer anything.
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
     Disables a resource set. Unloads any of its unlocked resources. Optionally specify to defer any unloads to a later Update call such that work is spread out.
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
    
};
