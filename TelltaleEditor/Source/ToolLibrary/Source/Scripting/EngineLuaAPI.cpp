#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Resource/PropertySet.hpp>

#include <regex>
#include <unordered_map>
#include <filesystem>

using namespace Meta; // include all meta from this TU

// LOWER LEVEL SCRIPTING API FOR LUA FUNCTIONS DEFINED IN THE GAME ENGINE (eg ContainerGetNumElements)
// ONLY INCLUDES TOOL LIBRARY SPECIFIC STUFF. MOST OF THE STUFF IS FOUND IN EDITOR LUA GAME ENGINE .CPP

// LIBRARY SCRIPTING API IS IN NORMAL LIBRARYLUAAPI.CPP. THESE FUNCTIONS ARE DEFINED PER GAME IN THEIR GAME LUA SCRIPTS.

// ==================================================== CONTAINER API ====================================================

static U32 luaContainerGetNumElements(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 1, "Requires one argument");
    
    U32 num = 0; // return value
    
    ClassInstance inst = AcquireScriptInstance(man, -1);
    
    if(inst && IsCollection(inst))
        num = CastToCollection(inst).GetSize();
    else
        TTE_LOG("At ContainerGetNumElements: container was null or invalid");
    
    man.PushUnsignedInteger(num); // push num elements
    
    return 1;
}

static U32 luaContainerInsertElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        collection.PushValue(AcquireScriptInstance(man, -1), true); // copy value to container
    }
    else
    {
        TTE_LOG("At ContainerInsertElement: container was null or invalid");
    }
    
    return 0;
}

static U32 luaContainerRemoveElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        I32 index = man.ToInteger(-1);
        if(index >= 0 && index < (I32)collection.GetSize())
            collection.PopValue((U32)index, inst); // put into inst, as we dont need it anymore
    }
    else
        TTE_LOG("At ContainerRemoveElement: container was null or invalid");
    
    return 0;
}

static U32 luaContainerGetElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        I32 index = man.ToInteger(-1);
        if(index >= collection.GetSize())
        {
            TTE_ASSERT(false, "Cannot access element index %d in container, out of bounds. Is the loop using 0 based indices?", index);
            man.PushNil();
            return 1;
        }
        collection.PushTransientScriptRef(man, (U32)index, false, inst.ObtainParentRef());
    }
    else
    {
        TTE_LOG("At ContainerGetElement: container was null or invalid");
        man.PushNil();
    }
    
    return 1;
}

static U32 luaContainerClear(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 1, "At ContainerClear: one argument required");
    ClassInstance inst = AcquireScriptInstance(man, -1);
    if(inst && IsCollection(inst))
        CastToCollection(inst).Clear();
    return 0;
}

static U32 luaContainerReserve(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    ClassInstance inst = AcquireScriptInstance(man, 1);
    if(inst && IsCollection(inst))
    {
        U32 num = (U32)man.ToInteger(2);
        TTE_ASSERT(num < 0x10000, "Container too large!");
        CastToCollection(inst).Reserve(num);
    }
    return 0;
}

// obj emplace(container)
static U32 luaContainerEmplaceElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 1, "Requires one arg");
    
    ClassInstance inst = AcquireScriptInstance(man, -1);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        collection.PushValue({}, false); // move it
        collection.PushTransientScriptRef(man, collection.GetSize() - 1, false, inst.ObtainParentRef());
    }
    else
    {
        TTE_LOG("At ContainerEmplaceElement: container was null or invalid");
    }
    
    return 1;
}

// ==================================================== RESOURCE API ====================================================

#define GET_REGISTRY() Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man); TTE_ASSERT(reg,\
"At LuaResource function: no attached resource registry. This likely happens because no registry was created after a switch or you are calling" \
" this in a context not supported for registry access (eg asychronous jobs or non-game-script code)");

static std::regex GetPattern(const String& ending, std::unordered_map<String, std::regex>& pattern_cache)
{
    auto it = pattern_cache.find(ending);
    if (it != pattern_cache.end())
    {
        return it->second;
    }
    
    std::string pattern = "/";
    for (char c : ending)
    {
        if (std::isalpha(c))
        {
            pattern += "[";
            pattern += static_cast<char>(std::tolower(c));
            pattern += static_cast<char>(std::toupper(c));
            pattern += "]";
        } else
        {
            pattern += c;
        }
    }
    pattern += "$";
    
    std::regex compiled_pattern(pattern);
    pattern_cache[ending] = compiled_pattern;
    return compiled_pattern;
}

static Bool MatchDirectory(const String& path, const String& directory, std::unordered_map<String, std::regex>& pattern_cache)
{
    std::regex pattern = GetPattern(directory, pattern_cache);
    return std::regex_search(path, pattern);
}

U32 luaResourceSetRegister(LuaManager& man) // through this exists in lua, we will implement in C++ for ease
{
    GET_REGISTRY();
    std::unordered_map<String, std::regex> pattern_cache{};

    ResourceSet set{};
    
    // see Assembly.lua , this gets called (deferred) in the _resourceRegistry loop. do it upfront.
    
    TTE_ASSERT(man.Type(-1) == LuaType::TABLE, "At RegistrySetDescription(set): set was not a table");
    
    // --- SET NAME
    
    man.PushLString("setName");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Resource set name does not exist (set.setName)");
    String setName = set.Name = ScriptManager::PopString(man);
    
    // -- PRIORITY
    
    man.PushLString("priority");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "Resource set priority does not exist (set.priority)");
    set.Priority = ScriptManager::PopInteger(man);
    
    // -- CREATE LOGICAL LOCATION
    
    reg->CreateLogicalLocation("<" + set.Name + ">");
    
    // -- ENABLE MODE
    
    man.PushLString("enableMode");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Resource set enable mode not exist (set.enableMode)");
    String enableMode = ScriptManager::PopString(man);
    Bool bHasMainSet = true;
    
    if(CompareCaseInsensitive(enableMode, "constant"))
    {
        ;
    }
    else if(CompareCaseInsensitive(enableMode, "bootable"))
    {
        set.SetFlags.Add(ResourceSetFlags::DYNAMIC);
        set.SetFlags.Add(ResourceSetFlags::BOOTABLE);
    }
    else if(CompareCaseInsensitive(enableMode, "localization"))
    {
        set.SetFlags.Add(ResourceSetFlags::DYNAMIC);
        set.SetFlags.Add(ResourceSetFlags::STICKY);
    }
    else bHasMainSet = false;
    
    // -- LOGICAL DESTINATION MAPPING
    
    if(bHasMainSet)
    {
        man.PushLString("logicalDestination");
        man.GetTable(1);
        if(man.Type(-1) == LuaType::STRING)
        {
            String dest = man.ToString(-1);
            if(dest.length() > 0)
            {
                if(!StringStartsWith(dest, "<") || !StringEndsWith(dest, ">"))
                    TTE_LOG("WARNING: Logical destination for %s is not valid: %s", set.Name.c_str(), dest.c_str());
            }
            set.Mappings["<" + set.Name + ">"] = std::move(dest);
        }
        man.Pop(1);
    }
    
    // -- VERSION TYPE
    
    man.PushLString("version");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Resource set version does not exist (set.version)");
    String version = ScriptManager::PopString(man);
    
    if(CompareCaseInsensitive(version, "trunk"))
    {
        set.Version = ResourceSetVersion::RESOURCE_SET_TRUNK;
    }
    else if(CompareCaseInsensitive(version, "patch"))
    {
        set.Version = ResourceSetVersion::RESOURCE_SET_PATCH;
    }
    
    // -- CREATE MAIN SET
    
    reg->_ResourceSets.push_back(std::move(set));
    set = ResourceSet{};
    
    // --- GAME DATA SET NAME
    
    man.PushLString("gameDataName");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Resource set name does not exist (set.gameDataName)");
    set.Name = ScriptManager::PopString(man);
    
    // -- GAME DATA PRIORITY
    
    man.PushLString("gameDataPriority");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "Resource set %s priority does not exist (set.gameDataName)", set.Name.c_str());
    set.Priority = ScriptManager::PopInteger(man);
    
    // -- GAME DATA ENABLE MODE
    
    man.PushLString("gameDataEnableMode");
    man.GetTable(1);
    TTE_ASSERT(man.Type(-1) == LuaType::STRING, "Resource set %s game data enable mode not exist (set.gameDataEnableMode)", set.Name.c_str());
    String gameDataEnableMode = ScriptManager::PopString(man);
    
    if(CompareCaseInsensitive(gameDataEnableMode, "bootable"))
    {
        set.SetFlags.Add(ResourceSetFlags::DYNAMIC);
        set.SetFlags.Add(ResourceSetFlags::BOOTABLE);
    }
    else if(CompareCaseInsensitive(gameDataEnableMode, "localization"))
    {
        set.SetFlags.Add(ResourceSetFlags::DYNAMIC);
        set.SetFlags.Add(ResourceSetFlags::STICKY);
    }
    
    // -- CREATE GAME DATA SET
    
    reg->_ResourceSets.push_back(set);
    
    // -- LOCAL DIR INCLUDE/EXCLUDE/ETC
    
    man.PushLString("localDir");
    man.GetTable(1);
    String localDir{};
    if(man.Type(-1) == LuaType::STRING)
        localDir = ScriptManager::PopString(man);
    else
        man.Pop(1);
    
    Bool bExcludeFromSet = false;
    
    if(localDir.length())
    {
        man.PushLString("localDirExclude");
        if(man.Type(-1) == LuaType::TABLE)
        {
            man.PushNil();
            while(man.TableNext(2) != 0)
            {
                String exclude = ScriptManager::PopString(man);
                if(MatchDirectory(localDir, exclude, pattern_cache))
                {
                    bExcludeFromSet = true;
                    man.Pop(1);
                    break;
                }
            }
        }
        man.Pop(1);
    }
    
    if(!bExcludeFromSet)
    {
        man.PushLString("localDirIncludeBase");
        man.GetTable(1);
        TTE_ASSERT(man.Type(-1) == LuaType::BOOL, "Resource set %s include base does not exist (set.localDirIncludeBase)", setName.c_str());
        bExcludeFromSet = !ScriptManager::PopBool(man); // recursive not supported in non tool
    }
    
    if(!bExcludeFromSet)
    {
        String dir = "<" + setName + ">/";
        reg->CreateConcreteDirectoryLocation(dir, localDir);
        reg->ResourceSetMapLocation(set.Name, dir, "<" + setName + ">");
    }
    
    // -- GAME DATA ARCHIVES
    
    man.PushLString("gameDataArchives");
    man.GetTable(1);
    if(man.Type(-1) == LuaType::TABLE)
    {
        man.PushNil();
        while(man.TableNext(2) != 0)
        {
            String archivePath = ScriptManager::PopString(man);
            String location = reg->ResourceAddressResolveToConcreteLocationID(archivePath);
            String resource = reg->ResourceAddressGetResourceName(archivePath);
            reg->CreateConcreteArchiveLocation(location, resource);
            reg->ResourceSetMapLocation(set.Name, location + resource + "/", "<" + setName + ">");
        }
        man.Pop(1);
    }
    man.Pop(1);
    
    if(version.length() == 0 || CompareCaseInsensitive(version, "trunk"))
    {
        // autoapply resource set
        reg->_DeferredApplications.push_back(set.Name);
        
        if(bHasMainSet && CompareCaseInsensitive(enableMode, "constant"))
        {
            reg->_DeferredApplications.push_back(setName); // game data set
        }
        
    }
    
    return 0;
}

static U32 luaResourcePrintLocations(LuaManager& man)
{
    GET_REGISTRY();
    
    reg->PrintLocations();
    
    return 0;
}

static U32 luaResourceAddObject(LuaManager& man)
{
    return 0;
}

static U32 luaResourceCreate(LuaManager& man)
{
    GET_REGISTRY();
    ResourceAddress addr = reg->CreateResolvedAddress(man.ToString(1), true);
    man.PushBool(addr ? reg->CreateResource(addr) : false);
    return 1;
}

static U32 luaResourceRevert(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->RevertResource(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaResourceSave(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->SaveResource(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaFileStripExtension(LuaManager& man)
{
    String name = man.ToString(1);
    size_t pos = name.find_last_of('.');
    if(pos != String::npos)
        name = name.substr(0, pos);
    man.PushLString(name.c_str());
    return 1;
}

static U32 luaFileSetExtension(LuaManager& man)
{
    man.PushLString(FileSetExtension(man.ToString(1), man.ToString(2)));
    return 1;
}

template<U32 Ret>
static U32 luaNonCompat(LuaManager& man)
{
    for(U32 i = 0; i < Ret; i++)
        man.PushNil();
    return Ret;
}

static U32 luaFileGetName(LuaManager& man)
{
    man.PushLString(FileGetName(man.ToString(1)));
    return 1;
}

static U32 luaFileGetExtension(LuaManager& man)
{
    man.PushLString(FileGetExtension(man.ToString(1)));
    return 1;
}

// LEGACY API FOR COMPAT
struct LuaFindFirstFileState
{
    std::set<String> Files;
    typename std::set<String>::const_iterator FileIt;
    StringMask Mask = "";
};

thread_local LuaFindFirstFileState _FindFirstFileState{};

static U32 luaFileFindFirstFile(LuaManager& man)
{
    GET_REGISTRY();
    String mask = man.ToString(1);
    //String path = man.ToString(2); will ignore
    _FindFirstFileState.Mask = mask;
    reg->GetResourceNames(_FindFirstFileState.Files, &_FindFirstFileState.Mask);
    _FindFirstFileState.FileIt = _FindFirstFileState.Files.begin();
    if (_FindFirstFileState.FileIt != _FindFirstFileState.Files.end())
    {
        man.PushLString(*_FindFirstFileState.FileIt++);
    }
    else
    {
        man.PushNil();
    }
    return 1;
}

static U32 luaFileFindNextFile(LuaManager& man)
{
    String mask = man.ToString(1);
    if(mask != _FindFirstFileState.Mask || _FindFirstFileState.FileIt == _FindFirstFileState.Files.end())
        man.PushNil();
    else
    {
        man.PushLString(*_FindFirstFileState.FileIt++);
    }
    return 1;
}

static U32 luaFileExistsGlobal(LuaManager& man)
{
    man.PushBool(std::filesystem::exists(man.ToString(1)));
    return 1;
}

static U32 luaFileExists(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceExists(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaFileDelete(LuaManager& man)
{
    GET_REGISTRY();
    reg->DeleteResource(ScriptManager::ToSymbol(man, 1));
    return 0;
}

static U32 luaFileCopy(LuaManager& man)
{
    GET_REGISTRY();
    reg->CopyResource(ScriptManager::ToSymbol(man, 1), man.ToString(2));
    return 0;
}

static U32 luaResourceSetGetLocationID(LuaManager& man)
{
    GET_REGISTRY();
    Symbol set = ScriptManager::ToSymbol(man, 1);
    Symbol resource = ScriptManager::ToSymbol(man, 2);
    String loc = reg->ResourceSetGetLocationID(set, resource);
    if(loc.length())
        man.PushLString(loc);
    else
        man.PushNil();
    return 1;
}

static U32 luaResourceSetResourceExists(LuaManager& man)
{
    GET_REGISTRY();
    Symbol set = ScriptManager::ToSymbol(man, 1);
    Symbol resource = ScriptManager::ToSymbol(man, 2);
    String loc = reg->ResourceSetGetLocationID(set, resource);
    man.PushBool(loc.length() > 0);
    return 1;
}

static U32 luaResourceSetReconfigure(LuaManager& man)
{
    GET_REGISTRY();
    ScriptManager::GetGlobal(man, "_SceneRuntime", true);
    Bool bHasRuntime = man.Type(-1) != LuaType::NIL;
    man.Pop(1);
    std::set<Symbol> off{}, on{};
    std::map<Symbol, I32> priorityRemap{};
    ITERATE_TABLE(it, 1)
    {
        Bool bState = man.Type(it.ValueIndex()) == LuaType::BOOL;
        I32 priorityOverride = man.ToInteger(it.ValueIndex());
        Symbol setName = ScriptManager::ToSymbol(man, it.KeyIndex());
        if(bState || priorityOverride == 0)
        {
            if(priorityOverride != 0)
                on.insert(setName);
            else
                off.insert(setName);
        }
        else
        {
            on.insert(setName);
            priorityRemap[setName] = priorityOverride;
        }
    }
    reg->ReconfigureResourceSets(off, on, bHasRuntime);
    for(const auto& remap: priorityRemap)
        reg->ResourceSetChangePriority(remap.first, remap.second);
    return 0;
}

static U32 luaResourceSetNonPurgable(LuaManager& man)
{
    GET_REGISTRY();
    Symbol resourceName = ScriptManager::ToSymbol(man, 1);
    Bool bOnOff = man.ToBool(2);
    reg->ResourceSetNonPurgable(resourceName, bOnOff);
    return 0;
}

static U32 luaResourceSetMapLocation(LuaManager& man)
{
    GET_REGISTRY();
    reg->ResourceSetMapLocation(ScriptManager::ToSymbol(man, 1), man.ToString(3), man.ToString(2));
    return 0;
}

static U32 luaResourceIsSticky(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceSetIsSticky(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaResourceIsBootable(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceSetIsBootable(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaResourceIsDynamic(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceSetIsDynamic(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

U32 luaResourceSetGetAll(LuaManager& man)
{
    GET_REGISTRY();
    Bool onlyDynamic = man.GetTop() >= 2 ? man.ToBool(2) : false;
    Bool onlyBootable = man.GetTop() >= 3 ? man.ToBool(3) : false;
    Bool onlySticky = man.GetTop() >= 4 ? man.ToBool(4) : false;
    man.PushTable();
    reg->_Guard.lock();
    U32 index = 0;
    for(const auto& set: reg->_ResourceSets)
    {
        if(   !onlyDynamic  || set.SetFlags.Test(ResourceSetFlags::DYNAMIC)
           || !onlyBootable || set.SetFlags.Test(ResourceSetFlags::BOOTABLE)
           || !onlySticky   || set.SetFlags.Test(ResourceSetFlags::STICKY))
        {
            man.PushInteger(++index);
            man.PushLString(set.Name);
            man.SetTable(-3);
        }
    }
    reg->_Guard.unlock();
    return 1;
}

static U32 luaResourceSetExists(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceSetExists(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaResourceSetEnabled(LuaManager& man)
{
    GET_REGISTRY();
    man.PushBool(reg->ResourceSetIsEnabled(ScriptManager::ToSymbol(man, 1)));
    return 1;
}

static U32 luaResourceSetEnable(LuaManager& man)
{
    GET_REGISTRY();
    Symbol set = ScriptManager::ToSymbol(man, 1);
    I32 priorityOverride = man.GetTop() == 2 ? man.ToInteger(2) : 0;
    Bool hasPriorityOverride = man.GetTop() == 2;
    if(hasPriorityOverride)
        reg->ResourceSetEnable(set, priorityOverride);
    else
        reg->ResourceSetEnable(set);
    return 0;
}

static U32 luaResourceSetDisable(LuaManager& man)
{
    GET_REGISTRY();
    ScriptManager::GetGlobal(man, "_SceneRuntime", true);
    Bool bHasRuntime = man.Type(-1) != LuaType::NIL;
    man.Pop(1);
    Symbol set = ScriptManager::ToSymbol(man, 1);
    reg->ResourceSetDisable(set, bHasRuntime);
    man.PushBool(true); // always say ok.
    return 1;
}

static U32 luaResourceSetDestroy(LuaManager& man)
{
    GET_REGISTRY();
    ScriptManager::GetGlobal(man, "_SceneRuntime", true);
    Bool bHasRuntime = man.Type(-1) != LuaType::NIL;
    man.Pop(1);
    Symbol set = ScriptManager::ToSymbol(man, 1);
    reg->ResourceSetDestroy(set, bHasRuntime);
    man.PushBool(true); // always say ok.
    return 1;
}

static U32 luaResourceSetDefaultLocation(LuaManager& man)
{
    GET_REGISTRY();
    reg->ResourceSetDefaultLocation(man.ToString(1));
    return 0;
}

static U32 luaResourceSetCreate(LuaManager& man)
{
    GET_REGISTRY();
    String name = man.ToString(1);
    I32 priority = man.GetTop() >= 2 ? man.ToInteger(2) : 0;
    Bool dynamic = man.GetTop() >= 3 ? man.ToBool(3) : false;
    Bool boot = man.GetTop() >= 4 ? man.ToBool(4) : false;
    Bool stick = man.GetTop() >= 5 ? man.ToBool(5) : false;
    if(name.length() && !reg->ResourceSetExists(name))
    {
        reg->ResourceSetCreate(name, priority, ResourceSetVersion::RESOURCE_SET_TRUNK, dynamic, boot, stick);
        // descriptor path is arg 6 but we won't use that
    }
    return 0;
}

static U32 luaResourceSetChangePriority(LuaManager& man)
{
    GET_REGISTRY();
    reg->ResourceSetChangePriority(ScriptManager::ToSymbol(man, 1), man.ToInteger(2));
    return 0;
}

static U32 luaResourceResolveURLToConcrete(LuaManager& man)
{
    GET_REGISTRY();
    ResourceAddress addr = reg->CreateResolvedAddress(man.ToString(1), false);
    if(addr)
    {
        String loc = addr.GetLocationName();
        if(loc.length() && loc[loc.length()-1] != '/')
            loc += "/";
        loc += addr.GetName();
        String concrete = reg->ResourceAddressResolveToConcreteLocationID(loc);
        man.PushLString(concrete + addr.GetName());
    }
    else
    {
        man.PushNil();
    }
    return 1;
}

static U32 luaResourceResolveAddressToConcreteLocationID(LuaManager& man)
{
    GET_REGISTRY();
    man.PushLString(reg->ResourceAddressResolveToConcreteLocationID(man.ToString(1)));
    return 1;
}

U32 luaResourceReportReferencedAssets(LuaManager& man)
{
    TTE_LOG("************* Referenced Assets *************");
    GET_REGISTRY();
    reg->_Guard.lock();
    for(const auto& h: reg->_AliveHandles)
    {
        TTE_LOG("* %s",SymbolTable::FindOrHashString(h._ResourceName).c_str());
    }
    reg->_Guard.unlock();
    TTE_LOG("************* **************** *************");
    return 0;
}

static U32 luaResourceLocationInjectIntoResourceSystem(LuaManager& man)
{
    GET_REGISTRY();
    String path = man.ToString(1);
    if(!StringEndsWith(path, "/"))
        path+="/";
    std::replace(path.begin(), path.end(), '\\', '/');
    Bool exists = reg->ResourceLocationExists(path);
    if(!exists)
        reg->CreateConcreteDirectoryLocation(path, path);
    man.PushBool(!exists);
    return 1;
}

static U32 luaResourceLocationGetSymbolsNames(LuaManager& man)
{
    GET_REGISTRY();
    String location = man.ToString(1);
    StringMask mask = man.ToString(2);
    std::set<String> n{};
    reg->GetLocationResourceNames(location, n, &mask);
    man.PushTable();
    I32 index = 0;
    for(auto& s: n)
    {
        man.PushInteger(++index);
        man.PushLString(s);
        man.SetTable(-3);
    }
    return 1;
}

static U32 luaResourceIsLoaded(LuaManager& man)
{
    GET_REGISTRY();
    HandleBase handle{};
    handle.SetObject(ScriptManager::ToSymbol(man, 1));
    man.PushBool(handle.IsLoaded(reg));
    return 1;
}

static U32 luaResourceGetURLAsLocal(LuaManager& man)
{
    GET_REGISTRY();
    ResourceAddress addr = reg->CreateResolvedAddress(man.ToString(1), false);
    if(addr)
    {
        String loc = addr.GetLocationName();
        if(loc.length() && loc[loc.length()-1] != '/')
            loc += "/";
        loc += addr.GetName();
        String concrete = reg->ResourceAddressResolveToConcreteLocationID(loc);
        man.PushLString(concrete + addr.GetName());
    }
    else
    {
        man.PushNil();
    }
    return 1;
}

static U32 luaResourceGetURL(LuaManager& man)
{
    GET_REGISTRY();
    String resourceName = man.ToString(1);
    String loc = reg->LocateConcreteResourceLocation(resourceName);
    if(!StringEndsWith(loc, "/"))
        loc += "/";
    man.PushLString(loc + resourceName);
    return 1;
}

static U32 luaResourceGetSymbolsNames(LuaManager& man)
{
    GET_REGISTRY();
    StringMask mask = man.ToString(1);
    std::set<String> n{};
    reg->GetResourceNames(n, &mask);
    man.PushTable();
    I32 index = 0;
    for(auto& s: n)
    {
        man.PushInteger(++index);
        man.PushLString(s);
        man.SetTable(-3);
    }
    return 1;
}

static U32 luaResourceGetName(LuaManager& man)
{
    GET_REGISTRY();
    if(ScriptManager::GetScriptObjectTag(man, 1) == ObjectTag::HANDLEABLE)
    {
        Handleable* pResource = ScriptManager::GetScriptObject<Handleable>(man, 1);
        String name = pResource ? reg->LocateResource(pResource) : "";
        if(name.length())
            man.PushLString(name);
        else
            man.PushNil();
    }
    else if(man.Type(1) == LuaType::STRING)
    {
        man.PushCopy(1); // return input string
    }
    else
    {
        ClassInstance prop = Meta::AcquireScriptInstance(man, 1);
        String name = prop ? reg->LocateResource(prop._GetInternal()) : "";
        if(name.length())
            man.PushLString(name);
        else
            man.PushNil();
    }
    return 1;
}

U32 luaResourceExistsLogicalLocation(LuaManager& man)
{
    GET_REGISTRY();
    String locStr = man.ToString(1);
    reg->_Guard.lock();
    for(const auto& loc: reg->_Locations)
    {
        if(CompareCaseInsensitive(locStr, loc->Name) && dynamic_cast<ResourceLogicalLocation*>(loc.get()))
            return true;
    }
    reg->_Guard.unlock();
    return 1;
}

U32 luaResourceExists(LuaManager& man)
{
    GET_REGISTRY();
    reg->_Guard.lock();
    if(ScriptManager::GetScriptObjectTag(man, 1) == ObjectTag::HANDLEABLE)
    {
        Handleable* pResource = ScriptManager::GetScriptObject<Handleable>(man, 1);
        man.PushBool(pResource != nullptr);
    }
    else if(man.Type(1) == LuaType::STRING)
    {
        ResourceAddress addr = reg->CreateResolvedAddress(man.ToString(1), false);
        Ptr<ResourceLocation> loc = reg->_Locate(addr.GetLocationName());
        man.PushBool(loc && loc->HasResource(addr.GetName()));
    }
    else
    {
        ClassInstance prop = Meta::AcquireScriptInstance(man, 1);
        man.PushBool(prop._GetInternal() != nullptr);
    }
    reg->_Guard.unlock();
    return 1;
}

static U32 luaResourceDelete(LuaManager& man)
{
    GET_REGISTRY();
    Symbol name{};
    if(ScriptManager::GetScriptObjectTag(man, 1) == ObjectTag::HANDLEABLE)
    {
        Handleable* pResource = ScriptManager::GetScriptObject<Handleable>(man, 1);
        name = reg->LocateResource(pResource);
    }
    else
    {
        ClassInstance prop = Meta::AcquireScriptInstance(man, 1);
        name = reg->LocateResource(prop._GetInternal());
    }
    if(name.GetCRC64())
    {
        man.PushBool(reg->UnloadResource(name));
    }
    else
    {
        man.PushBool(false);
    }
    return 1;
}

U32 luaResourceCreateLogicalLocation(LuaManager& man)
{
    GET_REGISTRY();
    String locStr = man.ToString(1);
    reg->_Guard.lock();
    Bool pushed = false;
    for(const auto& loc: reg->_Locations)
    {
        if(CompareCaseInsensitive(locStr, loc->Name) && dynamic_cast<ResourceLogicalLocation*>(loc.get()))
        {
            pushed = true;
            man.PushBool(false);
            break;
        }
    }
    if(!pushed)
    {
        reg->CreateLogicalLocation(locStr);
        man.PushBool(true);
    }
    reg->_Guard.unlock();
    return 1;
}

U32 luaResourceArchiveFind(LuaManager& man)
{
    GET_REGISTRY();
    StringMask mask = man.ToString(1);
    man.PushTable();
    I32 index = 0;
    for(const auto& arc: reg->_Locations)
    {
        RegistryDirectory* dir = arc->GetConcreteDirectory();
        if(dir && (dynamic_cast<RegistryDirectory_TTArchive*>(dir) || dynamic_cast<RegistryDirectory_TTArchive2*>(dir)))
        {
            man.PushInteger(++index);
            man.PushLString(arc->Name);
            man.SetTable(-3);
        }
    }
    return 1;
}

static U32 luaResourceResourceName(LuaManager& man)
{
    GET_REGISTRY();
    man.PushLString(reg->ResourceAddressGetResourceName(man.ToString(1)));
    return 1;
}

U32 luaResourceArchiveIsActive(LuaManager& man)
{
    GET_REGISTRY();
    String name = man.ToString(1);
    for(const auto& arc: reg->_Locations)
    {
        RegistryDirectory* dir = arc->GetConcreteDirectory();
        if(!dir)
            continue;
        if(!CompareCaseInsensitive(name, arc->Name))
            continue;
        if(dynamic_cast<RegistryDirectory_TTArchive*>(dir) && dynamic_cast<RegistryDirectory_TTArchive*>(dir)->_Archive.IsActive())
            man.PushBool(true);
        else if(dynamic_cast<RegistryDirectory_TTArchive2*>(dir) && dynamic_cast<RegistryDirectory_TTArchive2*>(dir)->_Archive.IsActive())
            man.PushBool(true);
        else
        {
            man.PushBool(false);
            break;
        }
        return 1;
    }
    return 1;
}

U32 luaResourceCopy(LuaManager& man)
{
    GET_REGISTRY();
    Bool bResult = false;
    ResourceAddress src = reg->CreateResolvedAddress(man.ToString(1), true);
    ResourceAddress dst = reg->CreateResolvedAddress(man.ToString(2), true);
    U32 cls = Meta::FindClassByExtension(FileGetExtension(src.GetName()), 0);
    if(cls != 0)
    {
        // copy resource to new
        reg->_Guard.lock();
        Symbol rn = src.GetName();
        Symbol drn = dst.GetName();
        HandleObjectInfo hoiCopy{};
        for(const auto& h: reg->_AliveHandles)
        {
            if(h._ResourceName == rn)
            {
                hoiCopy = h;
                hoiCopy._Flags.Remove(HandleFlags::NEEDS_DESTROY);
                hoiCopy._Flags.Remove(HandleFlags::NON_PURGABLE);
                hoiCopy._OpenStream = {};
                hoiCopy._Locator = dst.GetLocationName();
                bResult = true;
                break;
            }
            else if(h._ResourceName == drn)
            {
                TTE_LOG("Cannot copy resource %s, destination already exists (%s)", src.GetName().c_str(), dst.GetName().c_str());
                break;
            }
        }
        if(bResult)
        {
            hoiCopy._Handle = hoiCopy._Handle->Clone();
            reg->_AliveHandles.insert(std::move(hoiCopy));
        }
        reg->_Guard.unlock();
    }
    else
    {
        TTE_LOG("Resource copy needs implement"); // will do this in the future if needed. (Resource address checks)
    }
    man.PushBool(bResult);
    return 1;
}

U32 luaLoad(LuaManager& man)
{
    GET_REGISTRY();
    ResourceAddress addr = reg->CreateResolvedAddress(man.ToString(1), true);
    HandleObjectInfo p{};
    p._ResourceName = addr.GetName();
    auto it = reg->_AliveHandles.find(p);
    if(it != reg->_AliveHandles.end())
    {
       if(it->_Handle)
           ScriptManager::PushWeakScriptReference(man, it->_Handle);
        else if(it->_Instance.GetClassID() && Meta::GetClass(it->_Instance.GetClassID()).Flags & Meta::_CLASS_PROP)
        {
            Meta::ClassInstance& erased = *const_cast<Meta::ClassInstance*>(&it->_Instance);
            erased.PushWeakScriptRef(man, erased.ObtainParentRef());
        }
        else
        {
            man.PushNil();
            TTE_LOG("WARNING: At Load(): no common instance was found. Internal error");
        }
    }
    else
    {
        std::unique_lock<std::recursive_mutex> lck{reg->_Guard};
        Ptr<ResourceLocation> fileLoc = reg->_Locate(addr.GetLocationName());
        if(!fileLoc)
        {
            TTE_LOG("At Load(): resource location %s from address not found or invalid", addr.GetLocationName().c_str());
            man.PushNil();
        }
        else
        {
            HandleObjectInfo hoi{};
            hoi._ResourceName = addr.GetName();
            hoi._Locator = addr.GetLocationName();
            hoi._Flags.Add(HandleFlags::NEEDS_NORMALISATION);
            hoi._Flags.Add(HandleFlags::NEEDS_SERIALISE_IN);
            hoi._OpenStream = fileLoc->LocateResource(addr.GetName(), nullptr);
            if(!hoi._OpenStream)
            {
                TTE_LOG("WARNING: At Load(): could not open file stream from %s for %s", addr.GetLocationName().c_str(), addr.GetName().c_str());
            }
            reg->_ProcessDirtyHandle(std::move(hoi), lck);
        }
    }
    return 1;
}

static U32 luaResourceCreateConcreteDir(LuaManager& man)
{
    GET_REGISTRY();
    String name = man.ToString(1), phys = man.ToString(2);
    reg->CreateConcreteDirectoryLocation(name, phys);
    return 0;
}

static U32 luaResourceCreateConcreteArc(LuaManager& man)
{
    GET_REGISTRY();
    String name = man.ToString(2), loc = man.ToString(3);
    reg->CreateConcreteArchiveLocation(name, loc);
    return 0;
}

// ==================================================== REGISTRY ====================================================

LuaFunctionCollection luaGameEngine(Bool bWorker)
{ // always define all
    LuaFunctionCollection col{};
    
    // 'LuaResource'
    PUSH_FUNC(col, "ResourceCreateConcreteDirectoryLocation", &luaResourceCreateConcreteDir, "nil ResourceCreateConcreteDirectoryLocation(name, path)",
              "Creates a concrete directory location in the resource system, essentially mounting it to a physical folder on the users computer device,"
              " Physical path is relative to the working directory or absolute if you prefer.");
    PUSH_FUNC(col, "ResourceCreateConcreteArchiveLocation", &luaResourceCreateConcreteArc, "nil ResourceCreateConcreteArchiveLocation(--[[ignored]] archiveID, name, location)",
              "Creates a concrete archive location to mount into the resource system. The first argument is here for compatibilty! "
              "You should not use this function unless using it for debugging and personal tools inside TTE (ie don't use in a telltale game)."
              " ArchiveID should be the value of location + name + a '/' at the end. It is default done this way overriding what you pass in. The name is the archive file name"
              " so should end in .ttarch2 or .ttarch. The location is the existing (or this does nothing) resource location the archive exists inside of."
              " Techically you can also pass in the cache mode as a fourth argument but prefer to use the set cache mode function.");
    PUSH_FUNC(col, "Load", &luaLoad, "resource Load(address)", "Loads a resource. Must be a telltale file. If loaded, returns the previous loaded instance."
              " This returns a *weak* reference, so when the object is unloaded then it is invalidated.");
    PUSH_FUNC(col, "LoadAsync", &luaLoad, "resource LoadAsync(address)", "Loads a resource. Must be a telltale file. If loaded, returns the previous loaded instance."
              " This returns a *weak* reference, so when the object is unloaded then it is invalidated.");// MAYBE THESE TWO ASYNC COULD BE ACTUALLY MADE ASYNC IN FUTURE.
    PUSH_FUNC(col, "LoadAsyncAndWait", &luaLoad, "resource LoadAsyncAndWait(address)", "Loads a resource."
              " Must be a telltale file. If loaded, returns the previous loaded instance."
              " This returns a *weak* reference, so when the object is unloaded then it is invalidated.");
    PUSH_FUNC(col, "ResourceCopy", &luaResourceCopy, "bool ResourceCopy(sourceName, destName)", "Copies a resource. If the file is meta described, then"
              " the source must be loaded and the new resource will be loaded into the cache with destName. Else copies resource files.");
    PUSH_FUNC(col, "ResourceArchiveIsActive", &luaResourceArchiveIsActive, "bool ResourceArchiveIsActive(archiveName)",
              "Returns if the given archive is currently active, ie is loaded. Pass in the archive name.");
    PUSH_FUNC(col, "ResourceAddressGetResourceName", &luaResourceResourceName, "string ResourceAddressGetResourceName(address)",
              "Gets the resource name string from the given resource address / URL.");
    PUSH_FUNC(col, "ResourceAchiveFind", &luaResourceArchiveFind, "nil ResourceArchiveFind(mask)",
              "Yes its spelt wrong by Telltale. Finds all archive locations. Mask must be non-null, eg *.ttarch2.");
    PUSH_FUNC(col, "ResourceAdvancePreloadBatch", &luaNonCompat<0>, "nil ResourceAdvancePreloadBatch(count)",
              "TTE Uses a different preloading system but this would normally advance to the next preloading batch of files. Don't use.");
    PUSH_FUNC(col, "ResourceArchiveSetCacheMode", &luaNonCompat<0>, "nil ResourceArchiveSetCacheMode(archiveID, mode)", // this might be included eventualy if archives are slow
              "Does nothing, here for compatibility reasons. This function was used externally only for episode download management."); // different cache modes.
    PUSH_FUNC(col, "ResourceArchiveWaitForCaching", &luaNonCompat<0>, "nil ResourceWaitForCaching(archiveID)",
              "Does nothing, here for compatibility reasons. This function was used externally only for episode download management.");
    PUSH_FUNC(col, "ResourceCreateLogicalLocation", &luaResourceCreateLogicalLocation, "nil ResourceCreateLogicalLocation(name)",
              "Creates a logical location with the given name. Returns false if it already existed. It must start and end with <> angle brackets.");
    PUSH_FUNC(col, "ResourceDelete", &luaResourceDelete, "bool ResourceDelete(resource)", "Deletes the resource from the loaded resources, essentially unloading it."
              " If not saved, changes could be lost. Returns if it was deleted. May return false if the resource is set as non purgable.");
    PUSH_FUNC(col, "ResourceExists", &luaResourceExists, "bool ResourceExists(resource)",
              "Returns if the given resource exists. If you pass in an actual loaded resource (eg propertySet) then this will return true if its valid"
              ". If you pass in a URL or resource name then it returns if it exists somewhere in the resource system.");
    PUSH_FUNC(col, "ResourceExistsLogicalLocation", &luaResourceExistsLogicalLocation, "bool ResourceExistsLogicalLocation(locationStr)",
              "Returns if the given logical location exists.");
    PUSH_FUNC(col, "ResourceGetLoadingCall", &luaNonCompat<1>, "nil ResourceGetLoadingCall()", "Does nothing, here for compatibility reasons");
    PUSH_FUNC(col, "ResourceGetName", &luaResourceGetName, "string ResourceGetName(resource)",
              "Gets the string name of the given resource. The resource is a currently loaded resource such as a propertySet.");
    PUSH_FUNC(col, "ResourceGetSymbols", &luaResourceGetSymbolsNames, "table ResourceGetSymbols(mask)",
              "Gets all of the resource names that match the given mask in the whole resource system. Returns tabled indexed 1 to N."
              " Prefer to use the get names version which ensures no symbol hash strings are returned");
    PUSH_FUNC(col, "ResourceGetNames", &luaResourceGetSymbolsNames, "table ResourceGetNames(mask)",
              "Gets all of the resource names that match the given mask in the whole resource system. Returns tabled indexed 1 to N.");
    PUSH_FUNC(col, "ResourceGetURL", &luaResourceGetURL, "string ResourceGetURL(resourceName)",
              "Gets the resource address of the given resource name with the current resource sets applied. Will include its resource location.");
    PUSH_FUNC(col, "ResourceGetURLAsLocal", &luaResourceGetURLAsLocal, "string ResourceGetURLAsLocal(urlString)",
              "Returns the URL as a full physical concrete path. You can pass in a folder or a filename address.");
    PUSH_FUNC(col, "ResourceInitializeWritebackCache", &luaNonCompat<0>, "nil ResourceInitializeWritebackCache()",
              "Does not do anything, only exists for compatibility reasons.");
    PUSH_FUNC(col, "ResourceInitializeLegacySubproject", &luaNonCompat<0>, "nil ResourceInitializeLegacySubproject()",
              "Does not do anything, only exists for compatibility reasons.");
    PUSH_FUNC(col, "ResourceInitializeSubprojects", &luaNonCompat<0>, "nil ResourceInitializeSubprojects()",
              "Does not do anything, only exists for compatibility reasons.");
    PUSH_FUNC(col, "ResourceIsLoaded", &luaResourceIsLoaded, "bool ResourceIsLoaded(resourceName)", "Returns true if the given resource is currently loaded"
              " in the resource system.");
    PUSH_FUNC(col, "ResourceLocationGetSymbols", &luaResourceLocationGetSymbolsNames, "table ResourceLocationGetSymbols(locationName, mask)",
              "Gets all resource names inside the given resource location. You also need to specify a mask for file names. Use * for all files."
              " This versions gets them as symbols which are not gaurunteed to be strings. Prefer to use the get names version.");
    PUSH_FUNC(col, "ResourceLocationGetNames", &luaResourceLocationGetSymbolsNames, "table ResourceLocationGetNames(locationName, mask)",
              "Gets all resource names inside the given resource location. You also need to specify a mask for file names. Use * for all files.");
    PUSH_FUNC(col, "ResourceLocationInjectIntoResourceSystem", &luaResourceLocationInjectIntoResourceSystem,
              "bool ResourceLocationInjectIntoResourceSystem(physicalPath)", "Injects a concrete resource location into the resource system."
              " The name will be equal to the physical path. Returns if succeeds, can fail if it already exists.");
    PUSH_FUNC(col, "ResourceOpenUser", &luaNonCompat<0>, "nil ResourceOpenUser()", "Does not do anything, only exists for compatibility reasons.");
    PUSH_FUNC(col, "ResourceReportReferencedAssets", &luaResourceReportReferencedAssets, "nil ResourceReportReferencedAssets()",
              "Prints to the console all assets that are currently loaded (ie referenced).");
    PUSH_FUNC(col, "ResourceResolveAddressToConcreteLocationID", &luaResourceResolveURLToConcrete, "string ResourceResolveAddressToConcreteLocationID(urlStr)",
              "Returns the concrete location ID for the given resource address.");
    PUSH_FUNC(col, "ResourceResolveURLToConcrete", &luaResourceResolveURLToConcrete, "string ResourceResolveURLToConcrete(urlStr)",
              "Resolves the URL to a concrete URL with the currently active resource sets. Returns nil if invalid url is passed in.");
    PUSH_FUNC(col, "ResourceRetryFailedResDescs", &luaNonCompat<0>, "nil ResourceRetryFailedResDescs()", "Does not do anything, here for compatibility.");
    PUSH_FUNC(col, "ResourceSaveManifest", &luaNonCompat<0>, "nil ResourceSaveManifest(file)", "Does not do anything, here for compatibility. Used in old games.");
    PUSH_FUNC(col, "ResourceLoadManifest", &luaNonCompat<0>, "nil ResourceLoadManifest(file)", "Does not do anything, here for compatibility. Used in old games.");
    PUSH_FUNC(col, "ResourceSetChangePriority", &luaResourceSetChangePriority, "nil ResourceSetChangePriority(setName, priority)",
              "Changes the priority of the given resource set.");
    PUSH_FUNC(col, "ResourceSetCreate", &luaResourceSetCreate, "nil ResourceSetCreate(name, priority, isDynamic, isBootable, isSticky)",
              "Creates a resource set at runtime. All arguments apart from the name are optional and default to false / 0.");
    PUSH_FUNC(col, "ResourceSetDefaultLocation", &luaResourceSetDefaultLocation, "nil ResourceSetDefaultLocation(locationName)",
              "Sets the default resource location to create resources in.");
    PUSH_FUNC(col, "ResourceSetDestroy", &luaResourceSetDisable, "bool ResourceSetDestroy(setName, force)",
              "Destroys the given resource set, removing it and disabling it. See the disable function. Force is optional and default false.");
    PUSH_FUNC(col, "ResourceSetDisable", &luaResourceSetDisable, "bool ResourceSetDisable(setName, force)",
              "Disables the resource set, optionally specifying to force that it gets disabled. Force is default true and optional. Returns"
              " if it was disabled.");
    PUSH_FUNC(col, "ResourceSetEnable", &luaResourceSetEnable, "nil ResourceSetEnable(setName, priorityOverride)",
              "Enables the given resource set, meaning all resource names that try to get loaded will be looked into the given set."
              " Optionally specify an override priority (default uses priority in the resource set description). Higher priority"
              " resource sets get searched first, meaning they override other files. Priorities can be negative or positive, any integer.");
    PUSH_FUNC(col, "ResourceSetEnabled", &luaResourceSetExists, "bool ResourceSetEnabled(setName)", "Returns if the given resource set is currently applied"
              " in the resource system, such that it maps resource names to its concrete resource locations.");
    PUSH_FUNC(col, "ResourceSetExists", &luaResourceSetExists, "bool ResourceSetExists(setName)", "Returns if the resource set exists in the resource system.");
    PUSH_FUNC(col, "ResourceSetGetAll", &luaResourceSetGetAll, "table ResourceSetGetAll(unused, onlyDynamic, onlyBootable, onlySticky)",
              "Gets a table of all resource set names, indexed 1 to N. The first argument must be nil or a string but is ignored."
              " The last three arguments are all optional and default to false. They specify how to filter the resource sets.");
    PUSH_FUNC(col, "ResourceSetIsSticky", &luaResourceIsSticky, "bool ResourceSetIsSticky(setName)", "Returns if the given resource set is sticky."
              " These tends to be localisation resource sets.");
    PUSH_FUNC(col, "ResourceSetIsBootable", &luaResourceIsBootable, "bool ResourceSetIsBootable(setName)", "Returns if the given resource set is bootable."
              " These can be booted from.");
    PUSH_FUNC(col, "ResourceSetIsDynamic", &luaResourceIsDynamic, "bool ResourceSetIsDynamic(setName)", "Returns if the given resource set is dynamic. "
              "These are dynamic.");
    PUSH_FUNC(col, "ResourceSetLoadingCall", &luaNonCompat<0>, "nil ResourceSetLoadingCall(string)", "Does nothing and provided for compatibility.");
    PUSH_FUNC(col, "ResourceSetMapLocation", &luaResourceSetMapLocation, "nil ResourceSetMapLocation(setName, destLocation, srcLocation)",
              "Maps a resource location to another concrete one for the given resource set. Turn it off and back on again to have effect if already enabled.");
    PUSH_FUNC(col, "ResourceSetNonPurgable", &luaResourceSetNonPurgable, "nil ResourceSetNonPurgable(resourceName, bOnOff)",
              "Sets if the resource is non purgable or default (default not). This means it won't be unloaded until set off again.");
    PUSH_FUNC(col, "ResourceSetReconfigure", &luaResourceSetReconfigure, "nil ResourceSetReconfigure(sets)",
              "Reconfigures resource set priorities. The argument should be a table of resource set name to priority override integer. If the"
              " priority is a boolean then it is treated as 'should this resource set be turned on or off', and the priority is left unchanged. "
              "By setting it to a number if will override the priority of it and enable it. Using a priority value of 0 acts as disabling it too.");
    PUSH_FUNC(col, "ResourceSetResourceExists", &luaResourceSetResourceExists, "bool ResourceSetResourceExists(setName, resourceName)",
              "Returns if the given resource name exists inside the given resource set in any one of its mapped locations.");
    PUSH_FUNC(col, "ResourceSetGetLocationID", &luaResourceSetGetLocationID, "string ResourceSetGetLocationID(setName, resourceName)",
              "Gets the concrete location ID which the resource name in the given resource set is inside. For example, could return the "
              "name of the texture mesh archive if its in that archive for the given resource set. Returns nil if not found.");
    PUSH_FUNC(col, "ResourceSetResourceGetURL", &luaNonCompat<1>, "nil ResourceSetResourceGetURL()", "Unused and here for compatibility");
    PUSH_FUNC(col, "Save", &luaResourceSave, "bool Save(resName)", "Save the resource. It must be loaded and have an associated location to know where to save it.");
    PUSH_FUNC(col, "Revert", &luaResourceRevert, "bool Revert(resName)", "Revert the object to reflect what is loaded on disk. "
              "Pass in the resource name and not path. It must be loaded else returns false.");
    PUSH_FUNC(col, "Create", &luaResourceCreate, "bool Create(resAddr)", "Create a brand new resource. Path is a resource address, so include the "
              "optional (default master loc mount point) logical identifier, eg logical://<User>/SaveGame.file");
    PUSH_FUNC(col, "RegisterSetDescription", &luaResourceSetRegister, "nil RegisterSetDescription(set)",
              "Registers a resource set description to the attached resource registry");
    PUSH_FUNC(col, "ResourcePrintLocations", &luaResourcePrintLocations, "nil ResourcePrintLocations()",
              "Prints all resource locations. Must have an attached resource registry.");
    PUSH_FUNC(col, "GameEngine_AddBuildVersionInfo", &luaResourceAddObject, "nil GameEngine_AddBuildVersionInfo(info)",
              "Adds build information dates. Ignored.");
    // dummy func. _resCdesc ('c' sometimes). some resdescs just call this. idk why
    
    // 'LuaFile'
    PUSH_FUNC(col, "FileStripExtension", &luaFileStripExtension, "string FileStripExtension(filename)", "Removes the file extension from the string");
    PUSH_FUNC(col, "FileStripExtention", &luaFileStripExtension, "string FileStripExtention(filename)", "Because Telltale employees can't spell this exists.");
    PUSH_FUNC(col, "FileSetExtension", &luaFileSetExtension, "string FileSetExtension(filename, extension)", "Sets the file extension of the given filename");
    PUSH_FUNC(col, "FileSetExtention", &luaFileSetExtension, "string FileSetExtention(filename, extension)", "Because Telltale employees are dyslexic this exists! <3");
    PUSH_FUNC(col, "FileMakeWriteable", &luaNonCompat<0>, "nil FileMakeWriteable(filename)", "Does nothing, not used in TTE or the runtime engine.");
    PUSH_FUNC(col, "FileMakeReadOnly", &luaNonCompat<0>, "nil FileMakeReadOnly(filename)", "Does nothing, not used in TTE or the runtime engine.");
    PUSH_FUNC(col, "FileClearLastErrorCorruptSaveFile", &luaNonCompat<0>, "nil FileClearLastErrorCorruptSaveFile()", "Does nothing, not used in TTE or the runtime engine.");
    PUSH_FUNC(col, "FileIsLastErrorCorruptSaveFile", &luaNonCompat<0>, "nil FileIsLastErrorCorruptSaveFile()", "Does nothing, not used in TTE or the runtime engine.");
    PUSH_FUNC(col, "FileGetFileName", &luaFileGetName, "string FileGetFileName(filename)", "Get the file name, removing any path and extensions.");
    PUSH_FUNC(col, "FileGetExtension", &luaFileGetExtension, "string FileGetExtension(filename)", "Removes the file extension without the dot of the given filename.");
    PUSH_FUNC(col, "FileFindFirst", &luaFileFindFirstFile, "string FileFindFirst(strMask,strPath)", "Find first file. Returns nil when done."
              " This is a legacy API function and is only here for compatibility!");
    PUSH_FUNC(col, "FileFindNext", &luaFileFindFirstFile, "string FileFindNext(strMask)", "Find next file. Returns nil when done."
              " This is a legacy API function and is only here for compatibility! The mask should be the same as the previous calls.");
    PUSH_FUNC(col, "FileExistsGlobal", &luaFileExistsGlobal, "bool FileExistsGlobal(filename)", "Checks if the global pathname (from C:/ or / in unix) exists.");
    PUSH_FUNC(col, "FileExists", &luaFileExists, "bool FileExists(filename)", "Checks whether the file exists in the resource system. Tests all "
              "resource locations.");
    PUSH_FUNC(col, "FileDelete", &luaFileExistsGlobal, "nil FileDelete(filename)", "Deletes the file in the mounted resource system");
    PUSH_FUNC(col, "FileCopy", &luaFileCopy, "nil FileCopy(srcName,dstName)", "Copies file source to file destination. Files should just be resource names without paths."
              " The destination resource is created in the same resource location.");
    
    // 'LuaContainer'
    col.Functions.push_back({"ContainerGetNumElements", &luaContainerGetNumElements,
        "int ContainerGetNumElements(container)", "Get the number of elements in a container."
        
    });
    col.Functions.push_back({"ContainerRemoveElement", &luaContainerRemoveElement,
        "nil ContainerRemoveElement(container, index)", "Remove the item at index from the container"
    });
    col.Functions.push_back({"ContainerInsertElement", &luaContainerInsertElement,
        "nil ContainerInsertElement(container, element)", "Insert at an item at the end of the container"
    });
    col.Functions.push_back({"ContainerEmplaceElement", &luaContainerEmplaceElement,
        "obj ContainerEmplaceElement(container)", "Emplace and return an element at the end of the container"
    });
    col.Functions.push_back({"ContainerGetElement", &luaContainerGetElement,
        "obj ContainerGetElement(container,_index)", "Get the item in the container"
    });
    col.Functions.push_back({"ContainerClear", &luaContainerClear,
        "nil ContainerClear(container)", "Clear the container"
    });
    col.Functions.push_back({"ContainerReserve", &luaContainerReserve,
        "nil ContainerReserve(container, capacity)", "Allocate backend memory for the container to reduce memory allocations until size is bigger than capacity"
    });
    
    PropertySet::InjectScriptAPI(col);
    
    return col;
}
