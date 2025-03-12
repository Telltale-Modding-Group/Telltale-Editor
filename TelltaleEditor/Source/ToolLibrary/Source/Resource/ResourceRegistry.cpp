#include <Resource/ResourceRegistry.hpp>
#include <Core/Context.hpp>

#include <regex>
#include <unordered_map>

// STRING MASK

Bool StringMask::MaskCompare(CString pattern, CString str, CString end, MaskMode mode)
{
    Bool allowPartialMatch = (mode == MASKMODE_ANY_SUBSTRING || mode == MASKMODE_ANY_ENDING);
    
    while (*str && str != end)
    {
        CString patternPtr = pattern;
        CString strPtr = str;
        
        while (*patternPtr && strPtr != end)
        {
            char patternChar = *patternPtr;
            char strChar = *strPtr;
            
            if (patternChar == '*')
            {
                // Skip consecutive '*' characters
                while (*patternPtr == '*') ++patternPtr;
                if (!*patternPtr) return true;  // Trailing '*' matches everything
                
                // Try matching the remaining pattern at different positions in `str`
                while (*str && str != end) {
                    if (MaskCompare(patternPtr, str, end, mode)) return true;
                    ++str;
                }
                return false;
            }
            else if (patternChar == '?')
            {
                if (strChar == '.') return false;  // '?' does not match '.'
            }
            else
            {
                // Case-insensitive match
                if (std::toupper(strChar) != std::toupper(patternChar))
                {
                    if (allowPartialMatch)
                    {
                        break;  // Move `str` forward and try again
                    }
                    return false;
                }
            }
            
            ++patternPtr;
            ++strPtr;
        }
        
        // If we matched the whole pattern, return success
        while (*patternPtr == '*') ++patternPtr;
        if (*patternPtr == '\0')
            return true;
        
        // If partial matching is allowed, try shifting `str` and reattempt matching
        if (allowPartialMatch)
        {
            ++str;
            continue;
        }
        return false;
    }
    
    return false;
}

Bool StringMask::MatchSearchMask(CString testString,CString searchMask,StringMask::MaskMode mode, Bool* excluded)
{
    if (!*searchMask) return true;  // Empty mask matches everything
    
    Bool foundMatch = false;
    
    while (*searchMask)
    {
        Bool isExclusion = false;
        CString patternStart = searchMask;
        
        // Find next ';' separator
        CString nextPattern = strchr(searchMask, ';');
        
        // Handle exclusion (`-pattern`)
        if (*patternStart == '-')
        {
            isExclusion = true;
            ++patternStart;  // Skip '-'
        }
        
        // If mode is MASKMODE_ANY_ENDING_NO_DIRECTORY, remove directory part
        if (mode == StringMask::MASKMODE_ANY_ENDING_NO_DIRECTORY)
        {
            const char* lastSlash = strrchr(patternStart, '/');
            if (lastSlash) patternStart = lastSlash + 1;
        }
        
        // Check if testString matches the current pattern
        if (!foundMatch || isExclusion)
        {
            bool matches = MaskCompare(patternStart, testString, nextPattern, mode);
            if (matches != isExclusion)
            {
                // If `isExclusion`, we invert the match result
                foundMatch = true;
            }
            else if (isExclusion)
            {
                // If excluded, update the flag and return 0
                if (excluded) *excluded = true;
                return false;
            }
        }
        
        // Move to the next pattern if available
        searchMask = nextPattern ? nextPattern + 1 : nullptr;
    }
    
    return foundMatch;
}

// FILESYSTEM DIRECTORY

namespace fs = std::filesystem;

Ptr<RegistryDirectory> RegistryDirectory_System::OpenDirectory(const String& name)
{
    fs::path dirPath = fs::path(_Path) / name;
    if(fs::is_directory(dirPath))
    {
        // sub directory
        return TTE_NEW_PTR(RegistryDirectory_System, MEMORY_TAG_RESOURCE_REGISTRY, dirPath.string());
    }
    return {};
}

Bool RegistryDirectory_System::GetResourceNames(std::set<String>& resources, const StringMask* optionalMask)
{
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_regular_file())
        {
            String fileName = entry.path().filename().string();
            
            // Apply optional mask filtering
            if (!optionalMask || *optionalMask == fileName) {
                resources.insert(fileName);
            }
        }
    }
    return true;
}

Bool RegistryDirectory_System::GetSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_directory())
        {
            String dirName = entry.path().filename().string();
            
            // Apply optional mask filtering
            if (!optionalMask || *optionalMask == dirName)
            {
                directories.insert(dirName);
            }
        }
    }
    return true;
}

Bool RegistryDirectory_System::GetAllSubDirectories(std::set<String>& directories, const StringMask* optionalMask) {
    for (const auto& entry : fs::recursive_directory_iterator(_Path))
    {
        if (entry.is_directory())
        {
            String dirName = entry.path().filename().string();
            
            // Apply optional mask filtering
            if (!optionalMask || *optionalMask == dirName)
            {
                directories.insert(dirName);
            }
        }
    }
    return true;
}

Bool RegistryDirectory_System::HasResource(const Symbol& resourceName, const String*)
{
    _LastLocatedResourceStatus = false;
    _LastLocatedResource.clear();
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_regular_file())
        {
            if (resourceName == entry.path().filename().string())
            {
                _LastLocatedResource = entry.path().filename().string();
                _LastLocatedResourceStatus = true;
                break;
            }
        }
    }
    return _LastLocatedResourceStatus;
}


String RegistryDirectory_System::GetResourceName(const Symbol& resource)
{
    Bool has = HasResource(resource, nullptr);
    return has ? _LastLocatedResource : "";
}

Bool RegistryDirectory_System::DeleteResource(const Symbol& resource)
{
    if(!HasResource(resource, nullptr))
        return false;
    
    fs::path resourcePath = fs::path(_Path) / _LastLocatedResource;
    
    return fs::remove(resourcePath);
}

Bool RegistryDirectory_System::RenameResource(const Symbol& resource, const String& newName)
{
    TTE_ASSERT(resource != newName, "Resource name already the same");
    if(!HasResource(resource, nullptr))
        return false;
    fs::path resourcePath = fs::path(_Path) / _LastLocatedResource;
    fs::path newPath = fs::path(_Path) / newName;
    
    try
    {
        fs::rename(resourcePath, newPath);
        _LastLocatedResource = newName;
        _LastLocatedResourceStatus = true;
        return true;
    } catch (...)
    {
        TTE_ASSERT(false, "Filesystem error!");
        return false;
    }
}

DataStreamRef RegistryDirectory_System::CreateResource(const String& name)
{
    fs::path filePath = fs::path(_Path) / name;
    
    _LastLocatedResource = name;
    _LastLocatedResourceStatus = true;
    ResourceURL url{ResourceScheme::FILE, filePath.string()};
    return DataStreamManager::GetInstance()->CreateFileStream(url);
}

Bool RegistryDirectory_System::CopyResource(const Symbol& resource, const String& dstResourceNameStr)
{
    TTE_ASSERT(resource != dstResourceNameStr, "Resource name already the same");
    if(!HasResource(resource, nullptr))
        return false;
    fs::path resourcePath = fs::path(_Path) / _LastLocatedResource;
    fs::path newPath = fs::path(_Path) / dstResourceNameStr;
    
    try
    {
        fs::copy(resourcePath, newPath, fs::copy_options::overwrite_existing);
        _LastLocatedResource = dstResourceNameStr;
        return true;
    } catch (...)
    {
        TTE_ASSERT(false, "Filesystem error!");
        return false;
    }
}

DataStreamRef RegistryDirectory_System::OpenResource(const Symbol& resourceName, String* on)
{
    if(!HasResource(resourceName, nullptr))
        return {};
    
    fs::path resourcePath = fs::path(_Path) / _LastLocatedResource;
    
    if (fs::exists(resourcePath))
    {
        if(on)
            *on = _LastLocatedResource;
        ResourceURL url{ResourceScheme::FILE, resourcePath.string()};
        return DataStreamManager::GetInstance()->CreateFileStream(url);
    }
    
    return {};
}

Bool RegistryDirectory_System::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> &resources, Ptr<ResourceLocation> &self, const StringMask *optionalMask)
{
    
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_regular_file())
        {
            String fileName = entry.path().filename().string();
            
            if (optionalMask && *optionalMask != fileName)
                continue;
            
            resources.push_back(std::make_pair(Symbol(fileName), self));
        }
    }
    
    return true;
}

void RegistryDirectory_System::RefreshResources()
{
    // Could do more eventually?
    _LastLocatedResource.clear();
}

// TTARCH1 DIRECTORY

Bool RegistryDirectory_TTArchive::GetResourceNames(std::set<String>& resources, const StringMask* optionalMask)
{
    _Archive.GetFiles(resources);
    if (optionalMask)
    {
        for (auto it = resources.begin(); it != resources.end(); )
        {
            if (*optionalMask != *it)
            {
                it = resources.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    return true;
}

Bool RegistryDirectory_TTArchive::GetSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    return true;
}

Bool RegistryDirectory_TTArchive::GetAllSubDirectories(std::set<String>& directories, const StringMask* optionalMask) {
    return true;
}

Bool RegistryDirectory_TTArchive::HasResource(const Symbol& resourceName, const String* o)
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;

    _Archive.Find(resourceName, &_LastLocatedResource);
    _LastLocatedResourceStatus = _LastLocatedResource.length() != 0;

    return _LastLocatedResourceStatus;
}

String RegistryDirectory_TTArchive::GetResourceName(const Symbol& resource)
{
    return HasResource(resource, nullptr) ? _LastLocatedResource : "";
}

Bool RegistryDirectory_TTArchive::DeleteResource(const Symbol& resource)
{
    for(auto it = _Archive._Files.begin(); it != _Archive._Files.end(); )
    {
        if(resource == it->Name)
        {
            _Archive._Files.erase(it);
            return true;
        }
        else ++it;
    }
    
    return false;
}

Bool RegistryDirectory_TTArchive::RenameResource(const Symbol& resource, const String& newName)
{
    if(HasResource(newName, nullptr))
    {
        TTE_LOG("When renaming to %s: a resource already exists with thie name", newName.c_str());
        return false;
    }
    for(auto it = _Archive._Files.begin(); it != _Archive._Files.end(); )
    {
        if(resource == it->Name)
        {
            it->Name = newName;
            break;
        }
        else ++it;
    }
    
    return true;
}

DataStreamRef RegistryDirectory_TTArchive::CreateResource(const String& name)
{
    if(HasResource(name, nullptr))
    {
        TTE_LOG("Cannot create %s: resource already exists. Delete it first or rename it.",name.c_str());
        return {};
    }
    DataStreamRef stream = DataStreamManager::GetInstance()->CreatePrivateCache(name);
    _Archive.AddFile(name, stream);
    _LastLocatedResource = name;
    _LastLocatedResourceStatus = true;
    return stream;
}

Bool RegistryDirectory_TTArchive::CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr)
{
    if(HasResource(dstResourceNameStr, nullptr) || !HasResource(srcResourceName, nullptr))
    {
        TTE_LOG("Cannot copy resource to %s: resource already exists or source does not.",dstResourceNameStr.c_str());
        return {};
    }
    DataStreamRef srcStream = _Archive.Find(srcResourceName, nullptr);
    if (!srcStream) return false;
    srcStream = DataStreamManager::GetInstance()->Copy(srcStream, 0, srcStream->GetSize());
    
    _Archive.AddFile(dstResourceNameStr, srcStream);
    
    _LastLocatedResource = dstResourceNameStr;
    _LastLocatedResourceStatus = true;
    return true;
}

DataStreamRef RegistryDirectory_TTArchive::OpenResource(const Symbol& resourceName, String* outName)
{
    return _Archive.Find(resourceName, outName);
}

void RegistryDirectory_TTArchive::RefreshResources()
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
}

Ptr<RegistryDirectory> RegistryDirectory_TTArchive::OpenDirectory(const String& name)
{
    return {};
}

Bool RegistryDirectory_TTArchive::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                                                Ptr<ResourceLocation>& self, const StringMask* optionalMask)
{
    resources.reserve(resources.size() + _Archive._Files.size());
    
    for(auto& file: _Archive._Files)
    {
        if(optionalMask && *optionalMask != file.Name)
            continue;
        resources.push_back(std::make_pair(Symbol(file.Name), self));
    }
    
    return true;
}

// TTARCH2 DIRECTORY

Ptr<RegistryDirectory> RegistryDirectory_TTArchive2::OpenDirectory(const String& name)
{
    return {};
}

Bool RegistryDirectory_TTArchive2::GetResourceNames(std::set<String>& resources, const StringMask* optionalMask)
{
    _Archive.GetFiles(resources);
    if (optionalMask)
    {
        for (auto it = resources.begin(); it != resources.end(); )
        {
            if (*optionalMask != *it)
            {
                it = resources.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    return true;
}

Bool RegistryDirectory_TTArchive2::GetSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    return true;
}

Bool RegistryDirectory_TTArchive2::GetAllSubDirectories(std::set<String>& directories, const StringMask* optionalMask) {
    return true;
}

Bool RegistryDirectory_TTArchive2::HasResource(const Symbol& resourceName, const String*)
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
    
    _Archive.Find(resourceName, &_LastLocatedResource);
    _LastLocatedResourceStatus = _LastLocatedResource.length() != 0;
    
    return _LastLocatedResourceStatus;
}

String RegistryDirectory_TTArchive2::GetResourceName(const Symbol& resource)
{
    return HasResource(resource, nullptr) ? _LastLocatedResource : "";
}

Bool RegistryDirectory_TTArchive2::DeleteResource(const Symbol& resource)
{
    for(auto it = _Archive._Files.begin(); it != _Archive._Files.end(); )
    {
        if(resource == it->Name)
        {
            _Archive._Files.erase(it);
            return true;
        }
        else ++it;
    }
    
    return false;
}

Bool RegistryDirectory_TTArchive2::RenameResource(const Symbol& resource, const String& newName)
{
    if(HasResource(newName, nullptr))
    {
        TTE_LOG("When renaming to %s: a resource already exists with thie name", newName.c_str());
        return false;
    }
    for(auto it = _Archive._Files.begin(); it != _Archive._Files.end(); )
    {
        if(resource == it->Name)
        {
            it->Name = newName;
            break;
        }
        else ++it;
    }
    
    return true;
}

DataStreamRef RegistryDirectory_TTArchive2::CreateResource(const String& name)
{
    if(HasResource(name, nullptr))
    {
        TTE_LOG("Cannot create %s: resource already exists. Delete it first or rename it.",name.c_str());
        return {};
    }
    DataStreamRef stream = DataStreamManager::GetInstance()->CreatePrivateCache(name);
    _Archive.AddFile(name, stream);
    _LastLocatedResource = name;
    _LastLocatedResourceStatus = true;
    return stream;
}

Bool RegistryDirectory_TTArchive2::CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr)
{
    if(HasResource(dstResourceNameStr, nullptr) || !HasResource(srcResourceName, nullptr))
    {
        TTE_LOG("Cannot copy resource to %s: resource already exists or source does not.",dstResourceNameStr.c_str());
        return {};
    }
    DataStreamRef srcStream = _Archive.Find(srcResourceName, nullptr);
    if (!srcStream) return false;
    srcStream = DataStreamManager::GetInstance()->Copy(srcStream, 0, srcStream->GetSize());
    
    _Archive.AddFile(dstResourceNameStr, srcStream);
    
    _LastLocatedResource = dstResourceNameStr;
    _LastLocatedResourceStatus = true;
    return true;
}

DataStreamRef RegistryDirectory_TTArchive2::OpenResource(const Symbol& resourceName, String* on)
{
    return _Archive.Find(resourceName, on);
}

void RegistryDirectory_TTArchive2::RefreshResources()
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
}

Bool RegistryDirectory_TTArchive2::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                                                Ptr<ResourceLocation>& self, const StringMask* optionalMask)
{
    resources.reserve(resources.size() + _Archive._Files.size());
    
    for(auto& file: _Archive._Files)
    {
        
        if(optionalMask && *optionalMask != file.Name)
            continue;
        
        resources.push_back(std::make_pair(Symbol(file.Name), self));
    }
    
    return true;
}

// LOGICAL RESOURCE LOCATION

Bool ResourceLogicalLocation::GetResourceNames(std::set<String>& names, const StringMask* optionalMask)
{
    for(auto& set : SetStack)
    {
        if(set.Resolved && !set.Resolved->GetResourceNames(names, optionalMask))
            return false;
    }
    return true;
}

Bool ResourceLogicalLocation::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                                           Ptr<ResourceLocation>& self, const StringMask* optionalMask)
{
    for(auto& set : SetStack)
    {
        Ptr<ResourceLocation> self = set.Resolved;
        if(set.Resolved && !set.Resolved->GetResources(resources, self, optionalMask))
            return false;
    }
    return true;
}

DataStreamRef ResourceLogicalLocation::LocateResource(const Symbol& name, String* outName)
{
    for(auto& set : SetStack) // most important part: this priority checks highest first.
    {
        if(set.Resolved)
        {
            DataStreamRef resolved = set.Resolved->LocateResource(name, outName);
            if(resolved)
                return resolved;
        }
    }
    return {};
}

Bool ResourceLogicalLocation::HasResource(const Symbol &name)
{
    for(auto& set : SetStack)
    {
        if(set.Resolved && set.Resolved->HasResource(name))
            return true;
    }
    return false;
}

// LUA API

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
    std::unordered_map<String, std::regex> pattern_cache{};
    
    TTE_ASSERT(man.GetTop() == 1, "RegisterSetDescription(set) only accepts one argument.");
    ScriptManager::GetGlobal(man, "_resourceRegistryInternal", true);
    
    TTE_ASSERT(man.Type(-1) == LuaType::LIGHT_OPAQUE, "No resource registry is bound the current Lua VM!"); // resource registry needed!
    ResourceRegistry* reg = (ResourceRegistry*)man.ToPointer(-1);
    man.Pop(1);
    
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
        reg->_DeferredApplications.push_back(setName);
        
        if(bHasMainSet && CompareCaseInsensitive(enableMode, "constant"))
        {
            reg->_DeferredApplications.push_back(set.Name); // game data set
        }
        
    }
    
    return 0;
}

static U32 luaResourcePrintLocations(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 1, "ResourcePrintLocations does not accept arguments!");
    ScriptManager::GetGlobal(man, "_resourceRegistryInternal", true);
    
    TTE_ASSERT(man.Type(-1) == LuaType::LIGHT_OPAQUE, "No resource registry is bound the current Lua VM!"); // resource registry needed!
    ResourceRegistry* reg = (ResourceRegistry*)man.ToPointer(-1);
    
    reg->PrintLocations();
    
    return 0;
}

void InjectResourceAPI(LuaFunctionCollection& Col, Bool bWorker)
{
    
    PUSH_FUNC(Col, "RegisterSetDescription", &luaResourceSetRegister);
    PUSH_FUNC(Col, "ResourcePrintLocations", &luaResourcePrintLocations);
    
}

// RESOURCE REGISTRY

#define SCOPE_LOCK() std::unique_lock<std::mutex> lck{_Guard}

void ResourceRegistry::_BindVM()
{
    TTE_ASSERT(IsCallingFromMain(), "This can only be called from the main thread. Resource sets can only be mounted on the main thread."
               " This is because the Lua VM version is dependent on the game and there is only one Lua VM with a version specific to the current game.");
    _LVM.PushOpaque(this);
    ScriptManager::SetGlobal(_LVM, "_resourceRegistryInternal", true);
}

void ResourceRegistry::_UnbindVM()
{
    _LVM.PushNil();
    ScriptManager::SetGlobal(_LVM, "_resourceRegistryInternal", true);
}

void ResourceRegistry::_ApplyMountDirectory(RegistryDirectory* pMountPoint, std::unique_lock<std::mutex>& lck)
{
    if(pMountPoint)
    {
        StringMask mask{Meta::GetInternalState().Games[Meta::GetInternalState().GameIndex].ResourceSetDescMask};
        if(mask.length()) // older games have no resource sets
        {
            U64 start = GetTimeStamp();
            std::vector<std::pair<String, DataStreamRef>> toExecute{};
            std::set<String> resDescs{};
            pMountPoint->GetResourceNames(resDescs, &mask);
            // open all resource set lua scripts, decrypt and run.
            for(auto& set: resDescs)
            {
                DataStreamRef setStream = pMountPoint->OpenResource(set, nullptr);
                if(!setStream)
                {
                    TTE_LOG("Ignoring resource set %s: could not open stream", set.c_str());
                }
                else
                {
                    toExecute.push_back(std::make_pair(set, ScriptManager::DecryptScript(setStream)));
                }
            }
            if(toExecute.size())
            {
                lck.unlock(); // ensure we are not locked!
                _BindVM();
                
                String path = pMountPoint->GetPath();
                std::replace(path.begin(), path.end(), '\\', '/');
                if (!path.empty() && path.back() != '/')
                    path += '/';
                
                _LVM.PushLString(path.c_str());
                ScriptManager::SetGlobal(_LVM, "_currentDirectory", true); // set required for resource scripts.
                
                for(auto& exec: toExecute)
                {
                    U64 sz = exec.second->GetSize();
                    U8* temp = TTE_ALLOC(sz, MEMORY_TAG_TEMPORARY);
                    TTE_ASSERT(exec.second->Read(temp, sz), "Failed to read resource set script");
                    if(!_LVM.LoadChunk(exec.first, temp, (U32)sz, LoadChunkMode::ANY))
                    {
                        TTE_LOG("Failed to parse and load resource set description %s!", exec.first.c_str());
                    }
                    else
                    {
                        _LVM.CallFunction(0, 0, true);
                    }
                    TTE_FREE(temp);
                }
                
                _LVM.PushNil();
                ScriptManager::SetGlobal(_LVM, "_currentDirectory", true); // reset it
                
                _UnbindVM();
            }
            
            // autoapply any needed
            std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>> patches{};
            
            for(auto& deferred: _DeferredApplications)
            {
                ResourceSet* pSet = _FindSet(deferred);
                if(pSet)
                {
                    _PrepareResourceSet(pSet, patches);
                    _DoApplyResourceSet(pSet, patches);
                    patches.clear();
                }
                else
                {
                    TTE_LOG("WARN: Deferred resource set auto apply could not locate %s", deferred.c_str());
                }
            }
            _DeferredApplications.clear();
            
            Float secs = GetTimeStampDifference(start, GetTimeStamp());
            String fmt = GetFormatedTime(secs);
            TTE_LOG("Mounted %s successfully in %s", pMountPoint->GetPath().c_str(), fmt.c_str());
        }
    }
}

void ResourceRegistry::_CheckConcrete(const String &name)
{
    TTE_ASSERT(StringStartsWith(name, "<") && StringEndsWith(name, ">/"), "Conrete location must be in format <XXX>/: %s", name.c_str());
}

void ResourceRegistry::_CheckLogical(const String &name)
{
    TTE_ASSERT(StringStartsWith(name, "<") && StringEndsWith(name, ">"), "Logical location must be in format <XXX>: %s", name.c_str());
}

String ResourceRegistry::ResourceAddressResolveToConcreteLocationID(const String& path)
{
    SCOPE_LOCK();
    size_t pos = path.find_last_of("/\\");
    
    while (pos != String::npos)
    {
        String topLevelPath = path.substr(0, pos + 1); // Keep the trailing slash
        
        for(auto& dir: _Locations)
        {
            if(CompareCaseInsensitive(dir->GetPhysicalPath(), topLevelPath) || CompareCaseInsensitive(dir->Name, topLevelPath))
            {
                return dir->Name;
            }
        }
        
        pos = path.find_last_of("/\\", pos - 1);
        
        if (pos == 0) break;
    }
    
    return "";
}

void ResourceRegistry::PrintSets()
{
    SCOPE_LOCK();
    TTE_LOG("=============== Registry Sets ===============");
    for(auto& loc: _ResourceSets)
    {
        TTE_LOG("Resource Set: %s [Active: %s] [Priority %d]", loc.Name.c_str(), loc.SetFlags.Test(ResourceSetFlags::APPLIED) ? "yes":"no", loc.Priority);
    }
    TTE_LOG("=============== ============= ===============");
}

void ResourceRegistry::PrintLocations()
{
    SCOPE_LOCK();
    TTE_LOG("=============== Registry Locations ===============");
    for(auto& loc: _Locations)
    {
        TTE_LOG("Resource Location: %s", loc->Name.c_str());
    }
    TTE_LOG("=============== ================== ===============");
}

String ResourceRegistry::ResourceAddressGetResourceName(const String& address)
{
    fs::path p{address};
    return p.filename().string();
}

void ResourceRegistry::MountSystem(const String &id, const String &fspath)
{
    _CheckConcrete(id);
    SCOPE_LOCK();
    if(_Locate(id) != nullptr)
    {
        TTE_LOG("WARNING: Concrete resource location %s already exists! Ignoring.", id.c_str());
        return;
    }
    auto dir = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_System>, MEMORY_TAG_RESOURCE_REGISTRY, id, fspath);
    _Locations.push_back(dir);
    _ApplyMountDirectory(&dir->Directory, lck);
}

Ptr<ResourceLocation> ResourceRegistry::_Locate(const String &logicalName)
{
    // check locations
    for(auto& loc: _Locations)
    {
        if(CompareCaseInsensitive(logicalName, loc->Name))
            return loc;
    }
    return nullptr;
}

void ResourceRegistry::CreateLogicalLocation(const String &name)
{
    _CheckLogical(name.c_str());
    SCOPE_LOCK();
    if(_Locate(name) != nullptr)
    {
        ; // already exists, common, ignore
    }
    else
    {
        auto logicalLocation = TTE_NEW_PTR(ResourceLogicalLocation, MEMORY_TAG_RESOURCE_REGISTRY, name);
        _Locations.push_back(std::move(logicalLocation));
    }
}

void ResourceRegistry::CreateConcreteDirectoryLocation(const String &name, const String &physPath)
{
    // very similar to mount. doesnt search for resource sets.
    _CheckConcrete(name);
    SCOPE_LOCK();
    if(_Locate(name) != nullptr)
    {
        ; // exists, ignore
    }
    else
    {
        auto concreteLocation = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_System>, MEMORY_TAG_RESOURCE_REGISTRY, name, physPath);
        _Locations.push_back(std::move(concreteLocation));
    }
}

void ResourceRegistry::CreateConcreteArchiveLocation(const String& name, const String& resourceName)
{
    _CheckConcrete(name);
    String archiveID = name + resourceName + "/";
    SCOPE_LOCK();
    
    Ptr<ResourceLocation> parent = _Locate(name);
    
    if(parent == nullptr)
    {
        TTE_ASSERT(false, "Archive parent location ID does not exist: %s!",name.c_str());
        return;
    }
    
    String archivePhysicalPath = parent->GetPhysicalPath() + resourceName;
    
    // EDGE CASE: archive is already existing. override it with a new updated archive.
    Ptr<ResourceLocation> alreadyLoaded = _Locate(archiveID);
    if(alreadyLoaded)
    {
        // its already created, so reload it.
        ResourceLocation* pLoc = alreadyLoaded.get();
        ResourceConcreteLocation<RegistryDirectory_TTArchive>* asArchive1 = dynamic_cast<ResourceConcreteLocation<RegistryDirectory_TTArchive>*>(pLoc);
        ResourceConcreteLocation<RegistryDirectory_TTArchive2>* asArchive2 = dynamic_cast<ResourceConcreteLocation<RegistryDirectory_TTArchive2>*>(pLoc);
        if(asArchive1)
        {
            TTE_ASSERT(StringEndsWith(resourceName, ".ttarch"), "Resource is not a valid TTARCH filename: %s", resourceName.c_str());
            asArchive1->Directory._Archive.Reset();
            DataStreamRef newStream = parent->LocateResource(resourceName, nullptr);
            TTE_ASSERT(false, "Failed retrieve stream for archive %s!", resourceName.c_str());
            lck.unlock();
            asArchive1->Directory._Archive.SerialiseIn(newStream);
            lck.lock();
        }
        else if(asArchive2)
        {
            TTE_ASSERT(StringEndsWith(resourceName, ".ttarch2"), "Resource is not a valid TTARCH2 filename: %s", resourceName.c_str());
            asArchive2->Directory._Archive.Reset();
            DataStreamRef newStream = parent->LocateResource(resourceName, nullptr);
            TTE_ASSERT(false, "Failed retrieve stream for archive %s!", resourceName.c_str());
            lck.unlock();
            asArchive2->Directory._Archive.SerialiseIn(newStream);
            lck.lock();
        }
        else
        {
            TTE_ASSERT(false, "The resource location %s already exists but is not an archive!", pLoc->Name.c_str());
        }
    }
    else
    {
        // create archive location
        if(StringEndsWith(resourceName, ".ttarch"))
        {
            TTArchive arc{Meta::GetInternalState().GetActiveGame().ArchiveVersion};
            DataStreamRef archiveStream = parent->LocateResource(resourceName, nullptr);
            lck.unlock(); // may take time, dont keep everyone waiting!
            TTE_ASSERT(archiveStream && arc.SerialiseIn(archiveStream), "TTArchive serialise/read fail!");
            lck.lock();
            auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_TTArchive>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(arc));
            _Locations.push_back(std::move(pLoc));
        }
        else if(StringEndsWith(resourceName, ".ttarch2"))
        {
            TTArchive2 arc{Meta::GetInternalState().GetActiveGame().ArchiveVersion};
            DataStreamRef archiveStream = parent->LocateResource(resourceName, nullptr);
            lck.unlock(); // may take time, dont keep everyone waiting!
            TTE_ASSERT(archiveStream && arc.SerialiseIn(archiveStream), "TTArchive2 serialise/read fail!");
            lck.lock();
            auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_TTArchive2>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(arc));
            _Locations.push_back(std::move(pLoc));
        }
        else
        {
            TTE_ASSERT(false, "Resource must be a telltale archive to be a concrete archive location %s", resourceName.c_str());
        }
    }
}

ResourceSet* ResourceRegistry::_FindSet(const Symbol &name)
{
    for(auto& set: _ResourceSets)
    {
        if(Symbol(set.Name) == name)
            return &set;
    }
    return nullptr;
}

Bool ResourceRegistry::ResourceSetExists(const Symbol& name)
{
    SCOPE_LOCK();
    return _FindSet(name) != nullptr;
}

void ResourceRegistry::ResourceSetChangePriority(const Symbol& name, I32 priority)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    if(pSet)
        pSet->Priority = priority;
}

void ResourceRegistry::ResourceSetEnable(const Symbol& name, I32 priority)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    if(pSet && !pSet->SetFlags.Test(ResourceSetFlags::APPLIED))
    {
        if(priority != UINT_MAX)
            pSet->Priority = priority;
        
        std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>> patches{};
        _PrepareResourceSet(pSet, patches);
        _DoApplyResourceSet(pSet, patches);
    }
}

void ResourceRegistry::ResourceSetDestroy(const Symbol &name,Bool bDefer)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    if(pSet)
    {
        pSet->SetFlags.Add(ResourceSetFlags::PENDING_DESTRUCT);
        std::set<ResourceSet*> turnOn, turnOff{};
        _ReconfigureSets(turnOff, turnOn, lck, bDefer);
    }
}

void ResourceRegistry::ResourceSetDisable(const Symbol& name, Bool defer)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    if(pSet && pSet->SetFlags.Test(ResourceSetFlags::APPLIED))
    {
        std::set<ResourceSet*> turnOn, turnOff{};
        turnOff.insert(pSet);
        _ReconfigureSets(turnOff, turnOn, lck, defer);
    }
}

Bool ResourceRegistry::ResourceSetIsEnabled(const Symbol& name)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    return pSet && pSet->SetFlags.Test(ResourceSetFlags::APPLIED);
}

Bool ResourceRegistry::ResourceSetIsBootable(const Symbol& name)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    return pSet && pSet->SetFlags.Test(ResourceSetFlags::BOOTABLE);
}

Bool ResourceRegistry::ResourceSetIsDynamic(const Symbol& name)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    return pSet && pSet->SetFlags.Test(ResourceSetFlags::DYNAMIC);
}

Bool ResourceRegistry::ResourceSetIsSticky(const Symbol& name)
{
    SCOPE_LOCK();
    ResourceSet* pSet = _FindSet(name);
    return pSet && pSet->SetFlags.Test(ResourceSetFlags::STICKY);
}

void ResourceRegistry::ResourceSetMapLocation(const Symbol& name, const String& src, const String& dst)
{
    ResourceSet* pSet = _FindSet(name);
    if(pSet)
    {
        pSet->Mappings[src] = dst;
    }
}

void ResourceRegistry::ResourceSetCreate(const String& name, I32 priority, ResourceSetVersion ver, Bool isDynamic, Bool isBootable, Bool isSticky)
{
    ResourceSet set{};
    set.Name = name;
    set.Priority = priority;
    set.Version = ver;
    if(isDynamic)
        set.SetFlags.Add(ResourceSetFlags::DYNAMIC);
    if(isBootable)
        set.SetFlags.Add(ResourceSetFlags::BOOTABLE);
    if(isSticky)
        set.SetFlags.Add(ResourceSetFlags::STICKY);
    {
        SCOPE_LOCK();
        TTE_ASSERT(_FindSet(name) == nullptr, "Resource set %s already exists!", name.c_str());
        _ResourceSets.push_back(std::move(set));
    }
}

void ResourceRegistry::ReconfigureResourceSets(const std::set<Symbol>& turnOff, const std::set<Symbol>& turnOn, Bool bDeferUnloads)
{
    SCOPE_LOCK();
    std::set<ResourceSet*> ton, toff{};
    for(auto& set : _ResourceSets)
    {
        for(auto& off: turnOff)
        {
            if(off == set.Name)
                toff.insert(&set);
        }
        for(auto& on: turnOn)
        {
            if(on == set.Name)
                ton.insert(&set);
        }
    }
    _ReconfigureSets(toff, ton, lck, bDeferUnloads);
}

void ResourceRegistry::_UnloadResources(const std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> &resources,
                                            std::unique_lock<std::mutex>& lck, U32 mx)
{
    TTE_ASSERT(false, "Implement me");
    // resources is a map of file names to their directory, eg an archive or on disk.
    // ensure to unlock when not needed, and make sure to erase the section of resources being destroyed so no other thread (tiny chance) tries to
}

void ResourceRegistry::_DestroyResourceSet(ResourceSet *pSet)
{
    // remove the set logical locator (is this needed? pretty sure...)
    String logicalLocator = "<" + pSet->Name + ">";
    for(auto it = _Locations.begin(); it != _Locations.end(); it++)
    {
        if(CompareCaseInsensitive(it->get()->Name, logicalLocator))
        {
            _Locations.erase(it);
            break;
        }
    }
    pSet->SetFlags.Remove(ResourceSetFlags::PENDING_DESTRUCT);
}

void ResourceRegistry::_GetResourcesToUnload(ResourceSet* pSet,
                                             std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& outResources)
{
    for(auto& mapping: pSet->Mappings)
    {
        Ptr<ResourceLocation> source = _Locate(mapping.first);
        if(!source)
        {
            TTE_LOG("When unloading %s: source mapping %s does not exist!",
                    pSet->Name.c_str(), mapping.first.c_str());
        }
        else
        {
            source->GetResources(outResources, source, nullptr);
        }
    }
}

void ResourceRegistry::_UnapplyResourceSet(ResourceSet* pSet)
{
    // REMOVE FROM LOGICAL DESTINATION
    
    std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>> patches{};
    _PrepareResourceSet(pSet, patches); // gather patches to remove
    
    for(auto& patch: patches)
    {
        
        ResourceLogicalLocation* pLogicalLocation = dynamic_cast<ResourceLogicalLocation*>(patch.second.get()); // destination MUST be logical!
        TTE_ASSERT(pLogicalLocation, "Destination resource configuration set mapping was not a logical location!");
        
        Bool bErased = false;
        for(auto it = pLogicalLocation->SetStack.begin(); it != pLogicalLocation->SetStack.end(); it++)
        {
            if(CompareCaseInsensitive(it->Set, patch.first->Name))
            {
                bErased = true;
                pLogicalLocation->SetStack.erase(it);
                break;
            }
        }
        if(!bErased)
            TTE_LOG("WARNING: Resource set mapping was not found while unmapping %s from %s in %s",
                    patch.first->Name.c_str(), pLogicalLocation->Name.c_str(), pSet->Name.c_str());
        
    }
    
    // remove set and find unload resources
    pSet->SetFlags.Remove(ResourceSetFlags::APPLIED);
}

void ResourceRegistry::_PrepareResourceSet(ResourceSet* pSet, std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>>& patches)
{
    // find locations into patches.
    for(auto& mapping: pSet->Mappings)
    {
        Ptr<ResourceLocation> source = _Locate(mapping.first);
        Ptr<ResourceLocation> destination = _Locate(mapping.second);
        if(!(source && destination && source != destination))
        {
            TTE_LOG("When applying %s: mapping %s => %s could not be applied as one or more of them either don't exist or they are the same!",
                    pSet->Name.c_str(), mapping.first.c_str(), mapping.second.c_str());
            // TTE_ASSERT(false, "Resource set pre-apply failed!");
        }
        else
        {
            patches[std::move(source)] = std::move(destination);
        }
    }
}

void ResourceRegistry::_DoApplyResourceSet(ResourceSet* pSet, const std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>>& patches)
{
    for(auto& patch: patches)
    {
        
        ResourceLogicalLocation* pLogicalLocation = dynamic_cast<ResourceLogicalLocation*>(patch.second.get()); // destination MUST be logical!
        TTE_ASSERT(pLogicalLocation, "Destination resource configuration set mapping was not a logical location!");
        
        ResourceLogicalLocation::SetInfo setInfo{};
        setInfo.Set = pSet->Name;
        setInfo.Priority = pSet->Priority;
        setInfo.Resolved = patch.first;
        
        pLogicalLocation->SetStack.insert(std::move(setInfo));
        
    }
    pSet->SetFlags.Add(ResourceSetFlags::APPLIED);
}

void ResourceRegistry::_ReconfigureSets(const std::set<ResourceSet*>& turnOff, const std::set<ResourceSet*>& turnOn, std::unique_lock<std::mutex>& lck, Bool bDefer)
{
    // UNLOAD
    
    std::set<ResourceSet*> pendingDestruction = turnOff;
    for(auto& set: _ResourceSets)
    {
        if(set.SetFlags.Test(ResourceSetFlags::PENDING_DESTRUCT))
        {
            pendingDestruction.insert(&set);
        }
    }
    if(bDefer)
    {
        for(auto& set: pendingDestruction)
        {
            _UnapplyResourceSet(set);
            _GetResourcesToUnload(set, _DeferredUnloads);
        }
    }
    else
    {
        std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> toUnload{};
        for(auto& set: pendingDestruction)
        {
            _UnapplyResourceSet(set);
            _GetResourcesToUnload(set, toUnload);
        }
        _UnloadResources(toUnload, lck, UINT32_MAX);
        toUnload.clear();
    }
    
    // remove
    for(auto it = _ResourceSets.begin(); it != _ResourceSets.end();)
    {
        if(it->SetFlags.Test(ResourceSetFlags::PENDING_DESTRUCT))
        {
            _DestroyResourceSet(&(*it));
            _ResourceSets.erase(it);
        }
        else ++it;
    }
    
    // LOAD
    
    std::map<Ptr<ResourceLocation>,Ptr<ResourceLocation>> patches{};
    for(auto& set: turnOn)
    {
        _PrepareResourceSet(set, patches);
        if(patches.size())
            _DoApplyResourceSet(set, patches);
        patches.clear(); // keep memory but clear
    }
}

void ResourceRegistry::Update(Float budget)
{
    U64 start = GetTimeStamp();
    
    // DEFERRED UNLOADS
    {
        SCOPE_LOCK();
        for(auto& p: _DeferredUnloads)
        {
            _UnloadResources(_DeferredUnloads, lck, 16); // every 16 lets check time
            
            U64 cur = GetTimeStamp();
            if(GetTimeStampDifference(start, cur) >= budget)
                return;
        }
    }
    
    // DEFERRED LOADS?
    
}

void ResourceRegistry::_LocateResourceInternal(Symbol name, String* outName, DataStreamRef* outStream)
{
    Ptr<ResourceLocation> masterLocation = _Locate("<>");
    TTE_ASSERT(masterLocation, "No master location found!!");
    DataStreamRef resStream = {};
    resStream = masterLocation->LocateResource(name, outName);
    if(outStream)
        *outStream = std::move(resStream);
}

DataStreamRef ResourceRegistry::FindResource(const Symbol& name)
{
    SCOPE_LOCK();
    DataStreamRef ref{};
    _LocateResourceInternal(name, nullptr, &ref);
    return ref;
}

String ResourceRegistry::FindResourceName(const Symbol& name)
{
    SCOPE_LOCK();
    String n{};
    _LocateResourceInternal(name, &n, nullptr);
    return n;
}

void ResourceRegistry::GetResourceNames(std::set<String>& outNames, const StringMask* optionalMask)
{
    Ptr<ResourceLocation> masterLocation = _Locate("<>");
    TTE_ASSERT(masterLocation, "No master location found!!");
    masterLocation->GetResourceNames(outNames, optionalMask);
}
