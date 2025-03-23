#include <Resource/ResourceRegistry.hpp>
#include <Core/Context.hpp>

#include <regex>
#include <unordered_map>

#include <filesystem>

// STRING MASK

Bool StringMask::MaskCompare(CString pattern, CString str, CString end, MaskMode mode)
{
    // In strict mode (e.g. MASKMODE_EXACT), extra characters after a match are not allowed.
    // allowPartialMatch is true for substring or ending modes.
    Bool allowPartialMatch = (mode == MASKMODE_ANY_SUBSTRING || mode == MASKMODE_ANY_ENDING);
    
    // Try matching at every position in the test string if partial matching is allowed.
    while (*str)
    {
        CString patternPtr = pattern;
        CString strPtr = str;
        
        while (patternPtr != end && *patternPtr && *strPtr)
        {
            char patternChar = *patternPtr;
            char strChar = *strPtr;
            
            if (patternChar == '*')
            {
                // Skip consecutive '*' characters.
                while (patternPtr != end && *patternPtr == '*')
                    ++patternPtr;
                // If the pattern ends with '*', then it matches the rest of the string.
                if (patternPtr == end || !*patternPtr)
                    return true;
                
                // Recursively try to match the remaining pattern.
                CString remainingPattern = patternPtr;
                for (CString t = strPtr; *t; ++t)
                {
                    if (MaskCompare(remainingPattern, t, end, mode))
                        return true;
                }
                return false;
            }
            else if (patternChar == '?')
            {
                // The '?' wildcard should not match a literal '.'
                if (strChar == '.')
                    return false;
            }
            else
            {
                // Case-insensitive comparison.
                if (std::toupper(strChar) != std::toupper(patternChar))
                {
                    if (allowPartialMatch)
                        break;  // Try the next starting position in the test string.
                    return false;
                }
            }
            
            ++patternPtr;
            ++strPtr;
        }
        
        // Skip any trailing '*' in the pattern.
        while (patternPtr != end && *patternPtr == '*')
            ++patternPtr;
        
        if (patternPtr == end)
        {
            // Here, if we're not allowing partial matches (i.e. strict mode),
            // then ensure the entire test string was consumed.
            if (!allowPartialMatch && *strPtr)
                return false;
            return true;
        }
        
        // In partial matching mode, try matching from the next character.
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
    
    while (searchMask && *searchMask)
    {
        Bool isExclusion = false;
        CString patternStart = searchMask;
        
        // Find next ';' separator
        CString nextPattern = strchr(searchMask, ';');
        
        // Handle exclusion (`!pattern`)
        if (*patternStart == '!')
        {
            isExclusion = true;
            ++patternStart;  // Skip '!'
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
            CString patternEnd = nextPattern ? nextPattern : patternStart + strlen(patternStart);
            Bool matches = MaskCompare(patternStart, testString, patternEnd, mode);
            
            //Bool matches = MaskCompare(patternStart, testString, nextPattern, mode);
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
    StringMask baseMask = EXCLUDE_SYSTEM_FILTER;
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_regular_file())
        {
            String fileName = entry.path().filename().string();
            
            // Apply optional mask filtering
            if (baseMask == fileName && (!optionalMask || *optionalMask == fileName)) {
                resources.insert(fileName);
            }
        }
    }
    return true;
}

Bool RegistryDirectory_System::GetSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    StringMask baseMask = EXCLUDE_SYSTEM_FILTER;
    for (const auto& entry : fs::directory_iterator(_Path))
    {
        if (entry.is_directory())
        {
            String dirName = entry.path().filename().string();
            
            // Apply optional mask filtering
            if (baseMask == dirName && (!optionalMask || *optionalMask == dirName))
            {
                directories.insert(dirName);
            }
        }
    }
    return true;
}

Bool RegistryDirectory_System::GetAllSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    const auto basePath = fs::absolute(_Path); // Get absolute path for reference length
    StringMask baseMask = EXCLUDE_SYSTEM_FILTER;
    
    for (const auto& entry : fs::recursive_directory_iterator(basePath))
    {
        if (entry.is_directory())
        {
            std::string relativePath = fs::relative(entry.path(), basePath).string();
            
            std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
            if (!relativePath.empty() && relativePath.back() != '/')
                relativePath += '/';
            
            // Apply optional mask filtering
            if (baseMask == relativePath && (!optionalMask || *optionalMask == relativePath))
                directories.insert(relativePath);
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

Bool RegistryDirectory_TTArchive::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck)
{
    StringMask mask = "*.ttarch;*.tta";
    TTE_ASSERT(resourceName == mask, "Resource is not a valid TTARCH filename: %s", resourceName.c_str());
    _Archive.Reset();
    DataStreamRef newStream = location->LocateResource(resourceName, nullptr);
    TTE_ASSERT(false, "Failed retrieve stream for archive %s!", resourceName.c_str());
    lck.unlock();
    _Archive.SerialiseIn(newStream);
    lck.lock();
    return true;
}

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
        TTE_LOG("When renaming to %s: a resource already exists with this name", newName.c_str());
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

// PK2 DIRECTORY

Ptr<RegistryDirectory> RegistryDirectory_GamePack2::OpenDirectory(const String &name)
{
    return {};
}

String RegistryDirectory_GamePack2::GetResourceName(const Symbol &resource)
{
    String n{};
    _Pack.Find(resource, &n);
    return n;
}

Bool RegistryDirectory_GamePack2::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck)
{
    TTE_ASSERT(StringEndsWith(resourceName, ".pk2"), "Resource is not a valid PK2 filename: %s", resourceName.c_str());
    _Pack.Reset();
    DataStreamRef newStream = location->LocateResource(resourceName, nullptr);
    TTE_ASSERT(false, "Failed retrieve stream for ISO %s!", resourceName.c_str());
    lck.unlock();
    Bool result = _Pack.SerialiseIn(newStream);
    lck.lock();
    return result;
}

Bool RegistryDirectory_GamePack2::GetResourceNames(std::set<String>& resources, const StringMask* optionalMask)
{
    _Pack.GetFiles(resources);
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

Bool RegistryDirectory_GamePack2::GetSubDirectories(std::set<String>& resources, const StringMask* optionalMask)
{
    _Pack.GetSubDirectories(false, resources);
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

Bool RegistryDirectory_GamePack2::GetAllSubDirectories(std::set<String>& resources, const StringMask* optionalMask)
{
    _Pack.GetSubDirectories(true, resources);
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

Bool RegistryDirectory_GamePack2::HasResource(const Symbol& resourceName, const String* o)
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
    
    _Pack.Find(resourceName, &_LastLocatedResource);
    _LastLocatedResourceStatus = _LastLocatedResource.length() != 0;
    
    return _LastLocatedResourceStatus;
}

String RegistryDirectory_ISO9660::GetResourceName(const Symbol& resource)
{
    return HasResource(resource, nullptr) ? _LastLocatedResource : "";
}

Bool RegistryDirectory_GamePack2::DeleteResource(const Symbol& resource)
{
    _Pack.DeleteFile(resource);
    return true;
}

Bool RegistryDirectory_GamePack2::RenameResource(const Symbol& resource, const String& newName)
{
    _Pack.RenameResource(resource, newName);
    return true;
}

DataStreamRef RegistryDirectory_GamePack2::CreateResource(const String& name)
{
    return _Pack.CreateResource(name);
}

Bool RegistryDirectory_GamePack2::CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr)
{
    _Pack.CopyResource(srcResourceName, dstResourceNameStr);
    return true;
}

DataStreamRef RegistryDirectory_GamePack2::OpenResource(const Symbol& resourceName, String* outName)
{
    String out{};
    return _Pack.Find(resourceName, outName ? outName : &out);
}

void RegistryDirectory_GamePack2::RefreshResources()
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
}

Ptr<RegistryDirectory> RegistryDirectory_ISO9660::OpenDirectory(const String& name)
{
    return {};
}

Bool RegistryDirectory_GamePack2::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                                               Ptr<ResourceLocation>& self, const StringMask* optionalMask)
{
    std::set<String> files{};
    _Pack.GetFiles(files);
    
    for(auto& f: files)
        resources.push_back(std::make_pair(Symbol(f), self));
    
    return true;
}

// ISO DIRECTORY

Bool RegistryDirectory_ISO9660::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck)
{
    TTE_ASSERT(StringEndsWith(resourceName, ".iso"), "Resource is not a valid ISO filename: %s", resourceName.c_str());
    _ISO.Reset();
    DataStreamRef newStream = location->LocateResource(resourceName, nullptr);
    TTE_ASSERT(false, "Failed retrieve stream for ISO %s!", resourceName.c_str());
    lck.unlock();
    Bool result = _ISO.SerialiseIn(newStream);
    lck.lock();
    return result;
}

Bool RegistryDirectory_ISO9660::GetResourceNames(std::set<String>& resources, const StringMask* optionalMask)
{
    _ISO.GetFiles(resources);
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

Bool RegistryDirectory_ISO9660::GetSubDirectories(std::set<String>& directories, const StringMask* optionalMask)
{
    return true;
}

Bool RegistryDirectory_ISO9660::GetAllSubDirectories(std::set<String>& directories, const StringMask* optionalMask) {
    return true;
}

Bool RegistryDirectory_ISO9660::HasResource(const Symbol& resourceName, const String* o)
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
    
    _ISO.Find(resourceName, _LastLocatedResource);
    _LastLocatedResourceStatus = _LastLocatedResource.length() != 0;
    
    return _LastLocatedResourceStatus;
}

Bool RegistryDirectory_ISO9660::DeleteResource(const Symbol& resource)
{
    TTE_LOG("Cannot delete files from ISOs!");
    return false;
}

Bool RegistryDirectory_ISO9660::RenameResource(const Symbol& resource, const String& newName)
{
    TTE_LOG("Cannot rename files from inside ISOs!");
    return false;
}

DataStreamRef RegistryDirectory_ISO9660::CreateResource(const String& name)
{
    TTE_LOG("Cannot create %s as ISOs are not editable", name.c_str());
    return {};
}

Bool RegistryDirectory_ISO9660::CopyResource(const Symbol& srcResourceName, const String& dstResourceNameStr)
{
    TTE_LOG("Cannot copy files inside ISOs!");
    return false;
}

DataStreamRef RegistryDirectory_ISO9660::OpenResource(const Symbol& resourceName, String* outName)
{
    String out{};
    return _ISO.Find(resourceName, outName ? *outName : out);
}

void RegistryDirectory_ISO9660::RefreshResources()
{
    _LastLocatedResource.clear();
    _LastLocatedResourceStatus = false;
}

Bool RegistryDirectory_ISO9660::GetResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>>& resources,
                                             Ptr<ResourceLocation>& self, const StringMask* optionalMask)
{
    std::set<String> files{};
    _ISO.GetFiles(files);
    
    for(auto& f: files)
        resources.push_back(std::make_pair(Symbol(f), self));
    
    return true;
}

// TTARCH2 DIRECTORY

Bool RegistryDirectory_TTArchive2::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::mutex>& lck)
{
    TTE_ASSERT(StringEndsWith(resourceName, ".ttarch2"), "Resource is not a valid TTARCH2 filename: %s", resourceName.c_str());
    _Archive.Reset();
    DataStreamRef newStream = location->LocateResource(resourceName, nullptr);
    TTE_ASSERT(false, "Failed retrieve stream for archive %s!", resourceName.c_str());
    lck.unlock();
    _Archive.SerialiseIn(newStream);
    lck.lock();
    return true;
}

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
    TTE_ASSERT(man.GetTop() == 1, "ResourcePrintLocations does not accept arguments!");
    ScriptManager::GetGlobal(man, "_resourceRegistryInternal", true);
    
    TTE_ASSERT(man.Type(-1) == LuaType::LIGHT_OPAQUE, "No resource registry is bound the current Lua VM!"); // resource registry needed!
    ResourceRegistry* reg = (ResourceRegistry*)man.ToPointer(-1);
    
    reg->PrintLocations();
    
    return 0;
}

static U32 luaResourceAddObject(LuaManager& man)
{
    return 0;
}

void InjectResourceAPI(LuaFunctionCollection& Col, Bool bWorker)
{
    
    PUSH_FUNC(Col, "RegisterSetDescription", &luaResourceSetRegister);
    PUSH_FUNC(Col, "ResourcePrintLocations", &luaResourcePrintLocations);
    PUSH_FUNC(Col, "GameEngine_AddBuildVersionInfo", &luaResourceAddObject); // dummy func. _resCdesc ('c' sometimes). some resdescs just call this. idk why
    
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

void ResourceRegistry::_LegacyApplyMount(Ptr<ResourceConcreteLocation<RegistryDirectory_System>>& dir, ResourceLogicalLocation* pMaster,
                                         const String& folderID, const String& fspath, std::unique_lock<std::mutex>& lck)
{
    // Find all .TTARCH archives. Lets put their priority higher to prefer those resources over filesystem, such that telltale do.
    
    std::set<String> archives{};
    StringMask mask = _ArchivesMask(true);
    TTE_ASSERT(dir->GetResourceNames(archives, &mask), "Could not gather resource names from %s", fspath.c_str());
    Ptr<ResourceLocation> castedPtr = dir;
    
    for(auto& arc: archives)
    {
        String archiveID = folderID + arc + "/";
        String physicalPath = fspath + arc;
        if(_Locate(archiveID))
            continue; // already exists
        
        TTE_ASSERT(_ImportAllocateArchivePack(arc, archiveID, physicalPath, castedPtr, lck), "Packed archive import failed");
        
        // Map archive location to master
        {
            ResourceLogicalLocation::SetInfo mapping{};
            mapping.Set = archiveID;
            mapping.Priority = 99; // set a higher priority than folders. prefer archives like the engine
            mapping.Resolved = _Locations.back();
            pMaster->SetStack.insert(std::move(mapping));
        }
        
    }
}

void ResourceRegistry::MountArchive(const String &id, const String &fspath)
{
    StringMask mask = _ArchivesMask(UsingLegacyCompat());
    TTE_ASSERT(mask == fspath, "Archive files cannot be mounted this way. Prefer to use MountArchive(...)");
    _CheckConcrete(id);
    SCOPE_LOCK();
    if(_Locate(id) != nullptr)
    {
        TTE_LOG("WARNING: Concrete resource location %s already exists! Ignoring.", id.c_str());
        return;
    }
    DataStreamRef is = DataStreamManager::GetInstance()->CreateFileStream(fspath);
    if(!is)
    {
        TTE_LOG("WARNING: Could not open stream %s when mounting archive to resource registry!", fspath.c_str());
        return;
    }
    if(_ImportArchivePack(fs::path(fspath).filename().string(), id, fspath, is, lck))
    {
        Ptr<ResourceLocation> pLocation = _Locations.back(); // last newly processed
        ResourceLogicalLocation* pLogicalMaster = dynamic_cast<ResourceLogicalLocation*>(_Locate("<>").get());
        TTE_ASSERT(pLogicalMaster, "Master location not found!");
        
        // Map newly mounted to master
        {
            ResourceLogicalLocation::SetInfo mapping{};
            mapping.Set = id;
            mapping.Priority = 0;
            mapping.Resolved = pLocation;
            pLogicalMaster->SetStack.insert(std::move(mapping));
        }
        
        // Similar to apply legacy mount. Search for any other archives inside this mounted one (eg if this is an ISO look for PK2/TTARCH)
        
        std::set<String> archives{};
        StringMask mask = _ArchivesMask(UsingLegacyCompat());
        TTE_ASSERT(pLocation->GetResourceNames(archives, &mask), "Could not gather resource names from %s", fspath.c_str());
        
        for(auto& arc: archives)
        {
            String archiveID = id + arc + "/";
            if(_Locate(archiveID))
                continue; // already exists
            
            DataStreamRef is = pLocation->LocateResource(arc, nullptr);
            if(is && _ImportArchivePack(arc, archiveID, fspath + "/" + arc, is, lck))
            {
                // Map archive location to master
                {
                    ResourceLogicalLocation::SetInfo mapping{};
                    mapping.Set = archiveID;
                    mapping.Priority = 99; // set a higher priority than folders. prefer archives like the engine
                    mapping.Resolved = _Locations.back();
                    pLogicalMaster->SetStack.insert(std::move(mapping));
                }
                
            }
            else
            {
                TTE_LOG("WARNING: The archive %s could not be opened!", arc.c_str());
            }
        }
        
    }
}


void ResourceRegistry::MountSystem(const String &id, const String& _fspath)
{
    
    // ensure no archives
    StringMask mask = _ArchivesMask(UsingLegacyCompat());
    TTE_ASSERT(mask != _fspath, "Archive files cannot be mounted this way. Prefer to use MountArchive(...)");
    mask = _ArchivesMask(!UsingLegacyCompat());
    TTE_ASSERT(mask != _fspath, "Archive files cannot be mounted this way. Prefer to use MountArchive(...)");
    
    _CheckConcrete(id);
    Bool bUsesResourceSys = !UsingLegacyCompat();
    SCOPE_LOCK();
    if(_Locate(id) != nullptr)
    {
        TTE_LOG("WARNING: Concrete resource location %s already exists! Ignoring.", id.c_str());
        return;
    }
    
    // Ensure form
    String fspath = _fspath;
    std::replace(fspath.begin(), fspath.end(), '\\', '/');
    if(!StringEndsWith(fspath, "/"))
        fspath += "/";
    
    auto dir = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_System>, MEMORY_TAG_RESOURCE_REGISTRY, id, fspath);
    _Locations.push_back(dir);
    
    if(bUsesResourceSys)
        _ApplyMountDirectory(&dir->Directory, lck); // actual resource source system. find resdescs.
    else
    {
        // Legacy. Look for all ttarch and files and register them.
        ResourceLogicalLocation* pLogicalMaster = dynamic_cast<ResourceLogicalLocation*>(_Locate("<>").get());
        TTE_ASSERT(pLogicalMaster, "Master location not found!");
        
        // Map newly mounted to master
        {
            ResourceLogicalLocation::SetInfo mapping{};
            mapping.Set = id;
            mapping.Priority = 0;
            mapping.Resolved = dir;
            pLogicalMaster->SetStack.insert(std::move(mapping));
        }
        
        // Apply any ttarch in this mount
        _LegacyApplyMount(dir, pLogicalMaster, id, fspath, lck);
        
        // Gather all subdirectories and add them as well
        std::set<String> subdirs{};
        TTE_ASSERT(dir->Directory.GetAllSubDirectories(subdirs, nullptr), "Could not get all subdirectories for %s", fspath.c_str());
        
        for(auto& sub: subdirs)
        {
            String folderID = id + sub;
            String physicalPath = fspath + sub;
            if(_Locate(folderID))
                continue; // already exists
            
            // Create sub directory concrete location and map it from master. Treat like flat filesystem in legacy games.
            auto subDir = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_System>, MEMORY_TAG_RESOURCE_REGISTRY, folderID, physicalPath);
            _Locations.push_back(subDir);
            
            // Map it to main
            {
                ResourceLogicalLocation::SetInfo mapping{};
                mapping.Set = folderID;
                mapping.Priority = 0;
                mapping.Resolved = subDir;
                pLogicalMaster->SetStack.insert(std::move(mapping));
            }
            
            _LegacyApplyMount(subDir, pLogicalMaster, folderID, physicalPath, lck);
            
        }
        
    }
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

StringMask ResourceRegistry::_ArchivesMask(Bool bLegacy)
{
    return bLegacy ? StringMask("*.ttarch;*.iso;*.pk2;*.tta") : StringMask("*.ttarch2;*.iso");
}

Bool ResourceRegistry::_ImportAllocateArchivePack(const String& resourceName, const String& archiveID,
                                                  const String& archivePhysicalPath, Ptr<ResourceLocation>& parent, std::unique_lock<std::mutex>& lck)
{
    DataStreamRef archiveStream = parent->LocateResource(resourceName, nullptr);
    return archiveStream ? _ImportArchivePack(resourceName, archiveID, archivePhysicalPath, archiveStream, lck) : false;
}

Bool ResourceRegistry::_ImportArchivePack(const String& resourceName, const String& archiveID,
                                          const String& archivePhysicalPath, DataStreamRef& archiveStream, std::unique_lock<std::mutex>& lck)
{
    // create archive location
    StringMask maskTTArch1 = "*.ttarch;*.tta";
    StringMask maskTTArch2 = "*.ttarch2";
    if(resourceName == maskTTArch1)
    {
        TTArchive arc{Meta::GetInternalState().GetActiveGame().ArchiveVersion};
        lck.unlock(); // may take time, dont keep everyone waiting!
        TTE_ASSERT(arc.SerialiseIn(archiveStream), "TTArchive serialise/read fail!");
        lck.lock();
        auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_TTArchive>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(arc));
        _Locations.push_back(std::move(pLoc));
    }
    else if(resourceName == maskTTArch2)
    {
        TTArchive2 arc{Meta::GetInternalState().GetActiveGame().ArchiveVersion};
        lck.unlock(); // may take time, dont keep everyone waiting!
        TTE_ASSERT(arc.SerialiseIn(archiveStream), "TTArchive2 serialise/read fail!");
        lck.lock();
        auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_TTArchive2>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(arc));
        _Locations.push_back(std::move(pLoc));
    }
    else if(StringEndsWith(resourceName, ".iso", false))
    {
        ISO9660 iso{};
        lck.unlock(); // may take time, dont keep everyone waiting!
        if(!iso.SerialiseIn(archiveStream))
        {
            TTE_LOG("Failed to import archive packed file %s!", resourceName.c_str());
            return false;
        }
        lck.lock();
        auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_ISO9660>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(iso));
        _Locations.push_back(std::move(pLoc));
    }
    else if(StringEndsWith(resourceName, ".pk2", false))
    {
        GamePack2 pk2{};
        lck.unlock(); // may take time, dont keep everyone waiting!
        if(!pk2.SerialiseIn(archiveStream))
        {
            TTE_LOG("Failed to import archive packed file (PK2) %s!", resourceName.c_str());
            return false;
        }
        lck.lock();
        auto pLoc = TTE_NEW_PTR(ResourceConcreteLocation<RegistryDirectory_GamePack2>, MEMORY_TAG_RESOURCE_REGISTRY, archiveID, archivePhysicalPath, std::move(pk2));
        _Locations.push_back(std::move(pLoc));
    }
    else
        return false;
    return true;
}

void ResourceRegistry::CreateConcreteArchiveLocation(const String& name, const String& resourceName)
{
    _CheckConcrete(name);
    TTE_ASSERT(_ArchivesMask(UsingLegacyCompat()) == resourceName,
               "The archive %s cannot be loaded as it is not supported in the currently active snapshot %s", resourceName.c_str(), Meta::GetInternalState().GetActiveGame().Name.c_str());
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
        if(!alreadyLoaded->UpdateArchiveInternal(resourceName, alreadyLoaded, lck))
        {
            TTE_ASSERT(false, "The resource location %s already exists but is not an archive!", alreadyLoaded->Name.c_str());
        }
    }
    else
    {
        if(!_ImportAllocateArchivePack(resourceName, archiveID, archivePhysicalPath, parent, lck))
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

static Bool _PerformHandleNormalise(HandleObjectInfo& handle)
{
    Bool res = true;
    TTE_ASSERT(handle._Instance, "Cannot normalise handle as the instance is not present / did not load correctly");
    LuaManager& L = GetThreadLVM();
    
    String fn = Meta::GetInternalState().Classes.find(handle._Instance.GetClassID())->second.NormaliserStringFn;
    auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
    TTE_ASSERT(fn.length() && normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found for instance: '%s'. Is this snapshot's normalisation "
               "routine defined for %s", fn.c_str(), Meta::GetInternalState().Classes.find(handle._Instance.GetClassID())->second.Name.c_str());
    
    if(fn.length())
    {
        ScriptManager::GetGlobal(L, fn, true);
        if(L.Type(-1) != LuaType::FUNCTION)
        {
            L.Pop(1);
            TTE_ASSERT(L.LoadChunk(fn, normaliser->second.Binary, normaliser->second.Size, LoadChunkMode::BINARY), "Could not load normaliser chunk for %s", fn.c_str());
        }
        handle._Instance.PushWeakScriptRef(L, handle._Instance.ObtainParentRef());
        L.PushOpaque(handle._Handle.get());
        L.CallFunction(2, 1, false);
        Bool result;
        if(!(result=ScriptManager::PopBool(L)))
        {
            TTE_LOG("Handle normalise failed in %s", fn.c_str());
            res = false;
        }
        else
        {
            handle._Handle->FinaliseNormalisationAsync();
        }
    }
    
    // remove instance, not needed
    handle._Instance = {};
    
    handle._Flags.Add(HandleFlags::LOADED);
    handle._Flags.Remove(HandleFlags::NEEDS_NORMALISATION);
    return res;
}

void ResourceRegistry::_ProcessDirtyHandle(HandleObjectInfo&& handle, std::unique_lock<std::mutex>& lck)
{
    if(handle._Flags.Test(HandleFlags::NEEDS_DESTROY))
    {
        handle._OnUnload(*this, lck);
        return;
    }
    String name = {};
    if(handle._Flags.Test(HandleFlags::NEEDS_SERIALISE_IN))
    {
        TTE_ASSERT(handle._OpenStream, "Cannot serialise in handle as no data stream was assigned");
        name = SymbolTable::Find(handle._ResourceName);
        if(name.length() == 0)
            name = SymbolToHexString(handle._ResourceName);
        handle._Instance = Meta::ReadMetaStream(name, handle._OpenStream); // dont want to lock, as resource is not findable at the moment (until pushed into alive)
        handle._OpenStream.reset(); // release stream
        handle._Flags.Remove(HandleFlags::NEEDS_SERIALISE_IN);
    }
    if(handle._Flags.Test(HandleFlags::NEEDS_NORMALISATION))
    {
        _PerformHandleNormalise(handle);
    }
    _AliveHandles.insert(std::move(handle)); // make normal again
}

void ResourceRegistry::_ProcessDirtyHandles(Float budget, U64 startStamp, std::unique_lock<std::mutex>& lck)
{
    while (_DirtyHandles.size() && GetTimeStampDifference(startStamp, GetTimeStamp()) < budget)
    {
        auto handle = std::move(_DirtyHandles.back());
        _DirtyHandles.pop_back();
        _ProcessDirtyHandle(std::move(handle), lck);
    }
}

void HandleBase::_SetObject(Ptr<ResourceRegistry> &registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
{
    _Validate();
    // IF OLD NEEDS UNLOAD
    if(_ResourceName && bUnloadOld)
    {
        Bool bFound = false;
        std::unique_lock<std::mutex> lck{registry->_Guard}; // SCOPE LOCK
        HandleObjectInfo proxy{};
        proxy._ResourceName = _ResourceName;
        auto it = registry->_AliveHandles.lower_bound(proxy);
        // test alive handles
        if(it != registry->_AliveHandles.end() && it->_ResourceName == _ResourceName)
        {
            Bool bHasLoaded = it->_Flags.Test(HandleFlags::LOADED);
            if(!bHasLoaded)
            {
                // unload it
                HandleObjectInfo&& handle = std::move(registry->_AliveHandles.extract(it).value());
                handle._Flags.Add(HandleFlags::NEEDS_DESTROY); // everything else ignored
                registry->_ProcessDirtyHandle(std::move(handle), lck);
            }
            bFound = true;
        }
        if(!bFound)
        {
            // test currently dirty
            for(auto it = registry->_DirtyHandles.begin(); it != registry->_DirtyHandles.end(); it++)
            {
                if(it->_ResourceName == _ResourceName)
                {
                    it->_Flags.Add(HandleFlags::NEEDS_DESTROY);
                    HandleObjectInfo&& handle = std::move(*it);
                    registry->_DirtyHandles.erase(it);
                    registry->_ProcessDirtyHandle(std::move(handle), lck);
                    bFound = true;
                    break;
                }
            }
        }
    }
    
    _ResourceName = name;
    
    // LOAD NEW
    if(name && bEnsureLoaded)
    {
        EnsureIsLoaded(registry);
    }
}

void HandleObjectInfo::_OnUnload(ResourceRegistry &registry, std::unique_lock<std::mutex> &lck)
{
    ;
}

Bool HandleBase::IsLoaded(Ptr<ResourceRegistry> &registry)
{
    _Validate();
    return _ResourceName ? registry->_EnsureHandleLoadedLocked(*this, true) : false;
}

void HandleBase::EnsureIsLoaded(Ptr<ResourceRegistry> &registry)
{
    _Validate();
    if(_ResourceName)
        registry->_EnsureHandleLoadedLocked(*this, false);
    else
        TTE_LOG("Ensure loaded was called with an empty handle");
}

Ptr<Handleable> HandleBase::_GetObject(Ptr<ResourceRegistry>& registry)
{
    _Validate();
    TTE_ASSERT(_ResourceName, "Invalid handle!");
    registry->_Guard.lock();
    HandleObjectInfo proxy{};
    proxy._ResourceName = _ResourceName;
    auto it = registry->_AliveHandles.lower_bound(proxy);
    if(it != registry->_AliveHandles.end() && it->_ResourceName == _ResourceName)
    {
        registry->_Guard.unlock();
        return it->_Handle;
    }
    registry->_Guard.unlock();
    return {};
}

Bool ResourceRegistry::_SetupHandleResourceLoad(HandleObjectInfo &hoi, std::unique_lock<std::mutex> &lck)
{
    hoi._Flags.Add(HandleFlags::NEEDS_SERIALISE_IN);
    hoi._Flags.Add(HandleFlags::NEEDS_NORMALISATION);
    if(!hoi._OpenStream)
    {
        // find stream for resource
        Ptr<ResourceLocation> master = _Locate("<>");
        hoi._OpenStream = master->LocateResource(hoi._ResourceName, nullptr);
        if(!hoi._OpenStream)
        {
            String name = SymbolTable::Find(hoi._ResourceName);
            if(name.length() == 0)
                name = SymbolToHexString(hoi._ResourceName);
            TTE_LOG("WARNING: The resource %s was not found in the resource registry so cannot be loaded!", name.c_str());
            return false;
        }
    }
    return true;
}

Bool ResourceRegistry::_EnsureHandleLoadedLocked(const HandleBase& handle, Bool bOnlyQuery)
{
    SCOPE_LOCK();
    HandleObjectInfo proxy{};
    proxy._ResourceName = handle._ResourceName;
    auto it = _AliveHandles.lower_bound(proxy);
    // test alive handles
    if(it != _AliveHandles.end() && it->_ResourceName == handle._ResourceName)
    {
        Bool bHasLoaded = it->_Flags.Test(HandleFlags::LOADED);
        if(bOnlyQuery)
            return bHasLoaded;
        if(!bHasLoaded)
        {
            // load it now
            
            HandleObjectInfo&& handle = std::move(_AliveHandles.extract(it).value());
            Bool bResult = _SetupHandleResourceLoad(handle, lck);
            if(bResult)
                _ProcessDirtyHandle(std::move(handle), lck);
            else
                _AliveHandles.insert(std::move(handle));
            return bResult;
        }
    }
    
    // test currently dirty
    for(auto it = _DirtyHandles.begin(); it != _DirtyHandles.end(); it++)
    {
        if(it->_ResourceName == handle._ResourceName)
        {
            if(bOnlyQuery)
                return false; // not loaded
            
            // load now, ensuring we normalise and serialise
            Bool bResult = _SetupHandleResourceLoad(*it, lck);
            if(bResult)
                _ProcessDirtyHandle(std::move(*it), lck);
            else
                _AliveHandles.insert(std::move(*it));
            _DirtyHandles.erase(it);
            return bResult;
        }
    }
    if(bOnlyQuery)
        return false; // not loaded
    HandleObjectInfo hoi{};
    hoi._ResourceName = handle._ResourceName;
    TTE_ASSERT(handle._AllocatorFn, "Allocate function for common class was not set. Handle<T> should be used!");
    hoi._Handle = handle._AllocatorFn();
    TTE_ASSERT(hoi._Handle.get(), "Could not allocate underlying common class instance for Handle<T>!");
    Bool bResult = _SetupHandleResourceLoad(hoi, lck);
    if(bResult)
        _ProcessDirtyHandle(std::move(hoi), lck);
    else
        _AliveHandles.insert(std::move(hoi));
    return bResult;
}

void ResourceRegistry::_UnloadResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> &resources,
                                        std::unique_lock<std::mutex>& lck, U32 mx)
{
    U32 processed = 0;
    for(auto uit = resources.begin(); uit != resources.end();)
    {
        auto& unload = *uit;
        Bool bFound = false;
        for(auto it = _DirtyHandles.begin(); it != _DirtyHandles.end();)
        {
            if(it->_ResourceName == unload.first)
            {
                it->_OnUnload(*this, lck);
                _DirtyHandles.erase(it);;
                bFound = true;
                break;
            }
            ++it;
        }
        if(!bFound)
        {
            HandleObjectInfo proxy{};
            proxy._ResourceName = unload.first;
            auto it = _AliveHandles.lower_bound(proxy);
            if(it != _AliveHandles.end() && it->_ResourceName == unload.first)
            {
                proxy = std::move(_AliveHandles.extract(it).value());
                proxy._OnUnload(*this, lck);
                bFound = true;
            }
        }
        if(bFound)
        {
            resources.erase(uit);
            if(++processed >= mx)
                break;
        } else uit++;
    }
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

U32 ResourceRegistry::GetPreloadOffset()
{
    SCOPE_LOCK();
    return _PreloadOffset;
}

U32 ResourceRegistry::Preload(std::vector<HandleBase> &&resourceHandles, Bool bOverwrite)
{
    SCOPE_LOCK();
    U32 numBatches = (U32)resourceHandles.size() / STATIC_PRELOAD_BATCH_SIZE;
    U32 remBatchNumResources = (U32)resourceHandles.size() % STATIC_PRELOAD_BATCH_SIZE;
    _PreloadJobs.reserve(_PreloadJobs.size() + numBatches + 1);
    _PreloadSize += numBatches;
    if(remBatchNumResources)
        _PreloadSize++;
    for(U32 i = 0; i <= numBatches; i++)
    {
        U32 numResources = i == numBatches ? remBatchNumResources : STATIC_PRELOAD_BATCH_SIZE;
        if(remBatchNumResources == 0)
            break;
        AsyncResourcePreloadBatchJob* pJob = TTE_NEW(AsyncResourcePreloadBatchJob, MEMORY_TAG_TEMPORARY_ASYNC);
        pJob->Registry = shared_from_this();
        if(bOverwrite)
            pJob->BatchFlags.Add(AsyncResourcePreloadBatchJob::BATCH_HIGH_PRIORITY);
        pJob->NumResources = numResources;
        for(U32 j = 0; j < numResources; j++)
        {
            HandleBase handle = resourceHandles.back();
            resourceHandles.pop_back();
            pJob->HOI[j]._ResourceName = handle._ResourceName;
            pJob->Allocators[j] = handle._AllocatorFn;
        }
        
        JobDescriptor J{};
        J.AsyncFunction = &_AsyncPerformPreloadBatchJob;
        J.UserArgA = pJob;
        J.UserArgB = nullptr;
        J.Priority = JOB_PRIORITY_NORMAL;
        
        PreloadBatchJobRef ref{};
        ref.Handle = JobScheduler::Instance->Post(J);
        ref.PreloadOffset = _PreloadSize; // all the same
        _PreloadJobs.push_back(std::move(ref));
    }
    return _PreloadSize;
}

Bool _AsyncPerformPreloadBatchJob(const JobThread& thread, void* j, void*)
{
    AsyncResourcePreloadBatchJob* job = (AsyncResourcePreloadBatchJob*)j;
    
    U32 nFailed = 0;
    std::stringstream ss{};
    DataStreamRef Streams[STATIC_PRELOAD_BATCH_SIZE];
    
    // Locate streams
    {
        job->Registry->_Guard.lock();
        Ptr<ResourceLocation> master = job->Registry->_Locate("<>");
        for(U32 i = 0; i < job->NumResources; i++)
        {
            DataStreamRef unsafe = master->LocateResource(job->HOI[i]._ResourceName, nullptr); // need to read to buffer
            DataStreamRef buffer = DataStreamManager::GetInstance()->CreateBufferStream("", unsafe->GetSize(), 0, 0);
            DataStreamManager::GetInstance()->Transfer(unsafe, buffer, unsafe->GetSize());
            Streams[i] = std::move(buffer);
        }
        job->Registry->_Guard.unlock();
    }
    
    // Load the resources
    for(U32 i = 0; i < job->NumResources; i++)
    {
        Bool bFail = false;
        if(Streams[i].get() != nullptr)
        {
            job->HOI[i]._Instance = Meta::ReadMetaStream(SymbolTable::FindOrHashString(job->HOI[i]._ResourceName), Streams[i]);
            if(job->HOI[i]._Instance)
            {
                // Normalise
                job->HOI[i]._Handle = job->Allocators[i]();
                bFail = !_PerformHandleNormalise(job->HOI[i]);
                job->HOI[i]._Instance = {}; // ignore instance, not needed
            }
        }else bFail = true;
        
        if(bFail)
        {
            if(ss.tellp())
                ss << ", ";
            ss <<  SymbolTable::Find(job->HOI[i]._ResourceName);
            nFailed++;
        }
    }
    
    // Insert loaded into registry
    {
        std::unique_lock<std::mutex> lck{job->Registry->_Guard};
        for(U32 i = 0; i < job->NumResources; i++)
        {
            if(job->HOI[i]._Flags.Test(HandleFlags::LOADED))
            {
                
                HandleObjectInfo proxy{};
                proxy._ResourceName = job->HOI[i]._ResourceName;
                if(job->BatchFlags.Test(AsyncResourcePreloadBatchJob::BATCH_HIGH_PRIORITY))
                {
                    auto iter = job->Registry->_AliveHandles.lower_bound(proxy);
                    if(iter != job->Registry->_AliveHandles.end() && iter->_ResourceName == proxy._ResourceName)
                    {
                        // erase
                        job->Registry->_AliveHandles.erase(iter);
                    }
                    else
                    {
                        // might be in dirty. check and if so then unload it
                        for(auto it = job->Registry->_DirtyHandles.begin(); it != job->Registry->_DirtyHandles.end(); it++)
                        {
                            if(it->_ResourceName == job->HOI[i]._ResourceName)
                            {
                                it->_Flags.Add(HandleFlags::NEEDS_DESTROY);
                                job->Registry->_ProcessDirtyHandle(std::move(*it), lck);
                                job->Registry->_DirtyHandles.erase(it);
                                break;
                            }
                        }
                    }
                    job->Registry->_AliveHandles.insert(std::move(job->HOI[i]));
                }
                else
                {
                    // if it exists, dont bother.
                    auto iter = job->Registry->_AliveHandles.lower_bound(proxy);
                    if(iter == job->Registry->_AliveHandles.end() || iter->_ResourceName != proxy._ResourceName)
                    {
                        Bool bFound = false;
                        // might be in dirty. check and if so then unload it
                        for(auto it = job->Registry->_DirtyHandles.begin(); it != job->Registry->_DirtyHandles.end(); it++)
                        {
                            if(it->_ResourceName == job->HOI[i]._ResourceName)
                            {
                                bFound = true;
                                break;
                            }
                        }
                        if(!bFound)
                            job->Registry->_AliveHandles.insert(std::move(job->HOI[i]));
                    } // else it exists
                }
            }
        }
        job->Registry->_PreloadOffset++;
    }
    
    if(nFailed)
    {
        String s = ss.str();
        TTE_LOG("At async preload batch, %d files failed to load successfully: %s", nFailed, s.c_str());
    }
    
    TTE_DEL(job);
    return true;
}

void ResourceRegistry::Update(Float budget)
{
    U64 start = GetTimeStamp();
    
    // REMOVE UNUSED PRELOAD JOB REFS
    {
        SCOPE_LOCK();
        for(auto it = _PreloadJobs.cbegin(); it != _PreloadJobs.end();)
        {
            if(it->PreloadOffset <= _PreloadOffset)
            {
                _PreloadJobs.erase(it);
            }else ++it;
        }
    }
    
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
    
    // PROCESS DIRTY
    {
        SCOPE_LOCK();
        _ProcessDirtyHandles(budget, start, lck);
    }
    
}

void ResourceRegistry::WaitPreload(U32 preloadOffset)
{
    std::vector<JobHandle> handles{};
    {
        SCOPE_LOCK();
        for(auto it = _PreloadJobs.cbegin(); it != _PreloadJobs.end();)
        {
            if(it->PreloadOffset <= _PreloadOffset) // remove any others that are done now
            {
                _PreloadJobs.erase(it);
            }
            else if(it->PreloadOffset <= preloadOffset) // remove all less than or equal to the juncture we want it to be (preload offset argument), and wait on them.
            {
                handles.push_back(it->Handle);
                _PreloadJobs.erase(it);
            }
            else ++it;
        }
    }
    if(handles.size())
        JobScheduler::Instance->Wait((U32)handles.size(), handles.data());
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
    if(ref)
    {
        // The resource needs to be thread safe. Before unlocking, lets create a copy of this resource (the complete bytes) so that its not going to interfere.
        DataStreamRef buffer = DataStreamManager::GetInstance()->CreateBufferStream("", ref->GetSize(), 0, 0);
        DataStreamManager::GetInstance()->Transfer(ref, buffer, ref->GetSize());
        buffer->SetPosition(0);
        ref = buffer;
    }
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
