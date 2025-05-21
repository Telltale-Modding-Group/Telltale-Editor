#include <Resource/ResourceRegistry.hpp>
#include <Core/Context.hpp>

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
    if(!fs::exists(_Path))
        return {};
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
    if(!fs::exists(_Path))
        return true;
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
    if(!fs::exists(_Path))
        return true;
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
    if(!fs::exists(_Path))
        return true;
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
    if(!fs::exists(_Path))
        return false;
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
    if(!fs::exists(_Path))
        return "";
    Bool has = HasResource(resource, nullptr);
    return has ? _LastLocatedResource : "";
}

Bool RegistryDirectory_System::DeleteResource(const Symbol& resource)
{
    if(!fs::exists(_Path))
        return false;
    if(!HasResource(resource, nullptr))
        return false;
    
    fs::path resourcePath = fs::path(_Path) / _LastLocatedResource;
    
    return fs::remove(resourcePath);
}

Bool RegistryDirectory_System::RenameResource(const Symbol& resource, const String& newName)
{
    if(!fs::exists(_Path))
        return false;
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
    if(!fs::exists(_Path))
        return {};
    fs::path filePath = fs::path(_Path) / name;
    
    _LastLocatedResource = name;
    _LastLocatedResourceStatus = true;
    ResourceURL url{ResourceScheme::FILE, filePath.string()};
    return DataStreamManager::GetInstance()->CreateFileStream(url);
}

Bool RegistryDirectory_System::CopyResource(const Symbol& resource, const String& dstResourceNameStr)
{
    if(!fs::exists(_Path))
        return false;
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
    if(!fs::exists(_Path))
        return {};
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
    if(!fs::exists(_Path))
        return true;
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

Bool RegistryDirectory_TTArchive::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck)
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

Bool RegistryDirectory_GamePack2::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck)
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

Bool RegistryDirectory_ISO9660::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck)
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

Bool RegistryDirectory_TTArchive2::UpdateArchiveInternal(const String& resourceName, Ptr<ResourceLocation>& location, std::unique_lock<std::recursive_mutex>& lck)
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

RegistryDirectory* ResourceLogicalLocation::LocateConcreteDirectory(const Symbol& resourceName)
{
    for(auto& set : SetStack)
    {
        Ptr<ResourceLocation> self = set.Resolved;
        RegistryDirectory* pDirectory = set.Resolved ? set.Resolved->LocateConcreteDirectory(resourceName) : nullptr;
        if(pDirectory)
            return pDirectory;
    }
    return nullptr;
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

// RESOURCE REGISTRY

#define SCOPE_LOCK() std::unique_lock<std::recursive_mutex> lck{_Guard}

void ResourceRegistry::ResourceSetNonPurgable(const Symbol &resourceName, Bool bOnOff)
{
    SCOPE_LOCK();
    for(auto it = _AliveHandles.begin(); it != _AliveHandles.end(); it++)
    {
        if(it->_ResourceName == resourceName)
        {
            it->_Flags.Set(HandleFlags::NON_PURGABLE, bOnOff);
            return;
        }
    }
    for(auto& loaded: _DirtyHandles)
    {
        if(loaded._ResourceName == resourceName)
        {
            loaded._Flags.Set(HandleFlags::NON_PURGABLE, bOnOff);
            return;
        }
    }
}

String ResourceRegistry::ResourceSetGetLocationID(const Symbol &setName, const Symbol &resourceName)
{
    SCOPE_LOCK();
    for(const auto& set : _ResourceSets)
    {
        if(setName == set.Name)
        {
            for(const auto& mapping: set.Mappings)
            {
                Ptr<ResourceLocation> loc = _Locate(mapping.first);
                if(loc->HasResource(resourceName))
                    return loc->Name;
            }
            break;
        }
    }
    return "";
}

Bool ResourceRegistry::ResourceExists(const Symbol &resourceName)
{
    SCOPE_LOCK();
    for(auto& alive: _AliveHandles)
    {
        if(alive._ResourceName == resourceName)
            return true;
    }
    for(auto& alive: _DirtyHandles)
    {
        if(alive._ResourceName == resourceName)
            return true;
    }
    return _Locate("<>")->HasResource(resourceName);
}

void ResourceRegistry::CopyResource(const Symbol &resourceName, const String &destName)
{
    if(resourceName != destName)
    {
        RegistryDirectory* pDirectory = _Locate("<>")->LocateConcreteDirectory(resourceName);
        if(pDirectory)
        {
            DataStreamRef stream = pDirectory->OpenResource(resourceName, nullptr);
            DataStreamRef out = pDirectory->CreateResource(destName);
            DataStreamManager::GetInstance()->Transfer(stream, out, stream->GetSize());
        }
    }
}

void ResourceRegistry::DeleteResource(const Symbol &resourceName)
{
    SCOPE_LOCK();
    RegistryDirectory* pDirectory = _Locate("<>")->LocateConcreteDirectory(resourceName);
    if(pDirectory)
        pDirectory->DeleteResource(resourceName);
}

String ResourceRegistry::LocateConcreteResourceLocation(const Symbol &resourceName)
{
    SCOPE_LOCK();
    for(const auto& loc: _Locations)
    {
        if(loc->GetConcreteDirectory())
        {
            if(loc->GetConcreteDirectory()->HasResource(resourceName, nullptr))
                return loc->Name;
        }
    }
    return "";
}

ResourceAddress ResourceRegistry::CreateResolvedAddress(const String& resourceName, Bool bDefaultToCache)
{
    ResourceAddress addr{};
    
    size_t schemeEnd = resourceName.find(':');
    String scheme;
    String path;
    
    if (schemeEnd != String::npos)
    {
        scheme = ToLower(String(resourceName, 0, schemeEnd));
        path = String(resourceName, schemeEnd + 1);
        
        // Skip optional "//" after scheme
        if (StringStartsWith(path, "//"))
            path = path.substr(2);
    }
    else
    {
        // No scheme provided
        scheme = "locator";
        path = resourceName;
    }
    
    if(scheme != "logical" && StringStartsWith(path, "<") && path.find('>') != String::npos)
        scheme = "logical";
    
    // Trim leading slashes
    while (!path.empty() && path[0] == '/')
        path = path.substr(1);
    
    const bool isLocationOnly = !path.empty() && path.back() == '/';
    
    if (scheme == "locator")
    {
        if (isLocationOnly)
        {
            TTE_LOG("Invalid locator address %s: file name is required", resourceName.c_str());
            return {};
        }
        
        SCOPE_LOCK();
        addr.Name = FileGetFilename(path);
        addr.LocationName = _DefaultLocation;
    }
    else if (scheme == "ttcache")
    {
        if (path.find('/') != String::npos)
        {
            TTE_LOG("Invalid resource address %s: sub-locators not permitted in cache", resourceName.c_str());
            return {};
        }
        addr.IsCache = true;
        addr.Name = path;
    }
    else if (scheme == "logical")
    {
        String locationStr = FileGetPath(path);
        
        if (locationStr.empty() && !isLocationOnly)
        {
            TTE_LOG("Invalid resource address %s: missing logical locator path", resourceName.c_str());
            return {};
        }
        
        // Ensure LocationName ends in '/' (locators always do)
        if (!locationStr.empty() && locationStr.back() != '/')
            locationStr += '/';
        
        addr.LocationName = isLocationOnly ? path : locationStr;
        addr.Name = isLocationOnly ? "" : FileGetFilename(path);
        
        {
            SCOPE_LOCK();
            
            if (!_Locate(addr.LocationName))
            {
                TTE_LOG("Logical locator %s could not be resolved for %s", addr.LocationName.c_str(), resourceName.c_str());
                return {};
            }
        }
    }
    else if (scheme.empty() && bDefaultToCache)
    {
        if (path.find('/') != String::npos)
        {
            TTE_LOG("Invalid resource address %s: cache resource must not have directories.", resourceName.c_str());
            return {};
        }
        addr.IsCache = true;
        addr.Name = path;
    }
    else
    {
        TTE_LOG("Unknown or unsupported scheme '%s' in resource address %s", scheme.c_str(), resourceName.c_str());
        return {};
    }
    
    return addr;
}


ResourceAddress ResourceRegistry::CreateResolvedAddress(const Symbol &resourceName)
{
    // Locator
    ResourceAddress addr{};
    addr.Name = SymbolTable::Find(resourceName);
    if(addr.Name.length() == 0)
    {
        TTE_LOG("Could not resolve resource name from symbol Symbol<%s>", SymbolToHexString(resourceName).c_str());
        return {};
    }
    {
        SCOPE_LOCK();
        addr.LocationName = "";
    }
    return addr;
}

Bool ResourceRegistry::RevertResource(const Symbol &resourceName)
{
    SCOPE_LOCK();
    // if dirty, won't worry.
    HandleObjectInfo hoi{};
    hoi._ResourceName = resourceName;
    auto it = _AliveHandles.find(hoi);
    if(it != _AliveHandles.end() && !it->_Flags.Test(HandleFlags::NON_PURGABLE))
    {
        hoi = std::move(*it);
        hoi._Flags.Add(HandleFlags::NEEDS_SERIALISE_IN);
        hoi._Flags.Add(HandleFlags::NEEDS_NORMALISATION);
        hoi._Flags.Remove(HandleFlags::NEEDS_DESTROY);
        _AliveHandles.erase(it);
        _ProcessDirtyHandle(std::move(hoi), lck);
        return true;
    }
    return false;
}

Bool ResourceRegistry::SaveResource(const Symbol &resourceName, const String& locator)
{
    SCOPE_LOCK();
    String name = SymbolTable::Find(resourceName);
    if(name.length() == 0)
    {
        TTE_LOG("Cannot save resource: unknown symbol %s", SymbolToHexString(resourceName).c_str());
        return false;
    }
    HandleObjectInfo hoi{};
    hoi._ResourceName = resourceName;
    auto it = _AliveHandles.find(hoi);
    if(it != _AliveHandles.end())
    {
        Ptr<ResourceLocation> pDestLocation{};
        if(locator.length() == 0 || locator == "<>")
        {
            pDestLocation = it->_Locator.length() ? _Locate(it->_Locator) : _DefaultLocation.length() ? _Locate(_DefaultLocation) : _Locate("<>");
        }
        else
        {
            pDestLocation = _Locate(locator);
        }
        if(!pDestLocation || !pDestLocation->GetConcreteDirectory())
        {
            TTE_LOG("Cannot save resource %s: don't know where to create the file. Please make sure you have it already loaded or call Create first."
                    " Double check the location path if specified which may be invalid.",
                    name.c_str());
            return false;
        }
        U32 clz = Meta::FindClassByExtension(FileGetExtension(name), 0);
        if(!clz)
        {
            TTE_LOG("Cannot save resource %s: the file extension is not supported or is not a meta class for saving / common instance.", name.c_str());
            return false;
        }
        RegistryDirectory* pDir = pDestLocation->GetConcreteDirectory();
        DataStreamRef stream = pDir->CreateResource(name);
        Bool bResult = false;
        if(it->_Instance)
        {
            // prop / mcd raw
            bResult = Meta::WriteMetaStream(name, it->_Instance, stream, {});
        }
        else
        {
            Meta::ClassInstance instance = Meta::CreateInstance(clz);
            const Meta::Class& clazz = Meta::GetClass(clz);
            bResult = InstanceTransformation::PerformSpecialiseAsync(it->_Handle, instance, GetThreadLVM());
            if(bResult)
                bResult = Meta::WriteMetaStream(name, instance, stream, {});
        }
        return bResult;
    }
    else
    {
        TTE_LOG("Cannot save resource %s: not loaded", name.c_str());
    }
    return false;
}

Bool ResourceRegistry::CreateResource(const ResourceAddress& address)
{
    SCOPE_LOCK();
    if(!address)
        return false;
    U32 clz = Meta::FindClassByExtension(FileGetExtension(address.Name), 0);
    if(!clz)
    {
        TTE_LOG("Cannot create resource %s: the file extension is invalid or does not map to a valid meta class. Only meta described classes "
                "can be created this way.", address.Name.c_str());
        return false;
    }
    DataStreamRef resource{};
    if(address.IsCache)
    {
        HandleObjectInfo hoi{};
        hoi._ResourceName = address.Name;
        if(_AliveHandles.find(hoi) != _AliveHandles.end())
        {
            TTE_LOG("Cannot create resource %s: already exists in cache!", address.Name.c_str());
            return false;
        }
        else
        {
            for(const auto& dirty : _DirtyHandles)
            {
                if(dirty._ResourceName == hoi._ResourceName)
                {
                    TTE_LOG("Cannot create resource %s: already exists in cache!", address.Name.c_str());
                    return false;
                }
            }
        }
        hoi._Locator = address.LocationName;
        if(Meta::GetClass(clz).Flags & Meta::_CLASS_PROP)
        {
            hoi._Instance = Meta::CreateInstance(clz); // just prop MCD instance
        }
        else
        {
            CommonInstanceAllocator* allocator = Meta::GetCommonAllocator(clz);
            if(!allocator)
            {
                TTE_LOG("Cannot create resource %s: the common class was not registered for coersion. This means that it is "
                        "not supported for cache or common instances. If you believe this is an error please contact us!", address.Name.c_str());
                return false;
            }
            hoi._Flags.Add(HandleFlags::LOADED);
            hoi._Handle = allocator(shared_from_this());
        }
        _AliveHandles.insert(std::move(hoi));
        return true;
    }
    Ptr<ResourceLocation> pResolved = _Locate(address.LocationName.length() == 0 ? _DefaultLocation : address.LocationName);
    if(pResolved)
    {
        RegistryDirectory* pDirectory = pResolved->GetConcreteDirectory();
        if(!pDirectory)
        {
            TTE_LOG("Cannot create resource %s: resource location requested (%s) is not concrete. Please provide the concrete location as you are creating"
                    " the resource and therefore must know where it needs to go!", address.Name.c_str(), address.LocationName.c_str());
            return false;
        }
        if(pDirectory->HasResource(address.Name, &address.Name))
        {
            TTE_LOG("Cannot create resource %s: already exists!", address.Name.c_str());
            return false;
        }
        resource = pDirectory->CreateResource(address.Name);
        if(!resource)
        {
            TTE_LOG("Could not create resource %s inside concrete directory %s!", address.Name.c_str(), address.LocationName.c_str());
            return false;
        }
    }
    Meta::ClassInstance quickSave = Meta::CreateInstance(clz);
    if(!quickSave)
    {
        TTE_LOG("Cannot create instance of %s when creating %s", Meta::GetClass(clz).Name.c_str(), address.Name.c_str());
        return false;
    }
    return Meta::WriteMetaStream(address.Name, std::move(quickSave), resource, {});
}

Ptr<ResourceRegistry> ResourceRegistry::GetBoundRegistry(LuaManager& man)
{
    ScriptManager::GetGlobal(man, "__ResourceRegistry", false);
    if(man.Type(-1) == LuaType::LIGHT_OPAQUE)
        return ((ResourceRegistry*)ScriptManager::PopOpaque(man))->shared_from_this();
    man.Pop(1);
    return {};
}

void ResourceRegistry::BindLuaManager(LuaManager& man)
{
    man.PushOpaque(this);
    ScriptManager::SetGlobal(man, "__ResourceRegistry", false);
}

ResourceRegistry::ResourceRegistry(LuaManager& man) : GameDependentObject("ResourceRegistry"), _LVM(man)
{
    TTE_ASSERT(Meta::GetInternalState().GameIndex != -1, "Resource registries can only be when a game is set!");
    TTE_ASSERT(man.GetVersion() == Meta::GetInternalState().Games[Meta::GetInternalState().GameIndex].LVersion,
               "The lua manager used for the current game must match the version being used for this resource registry");
    
    CreateLogicalLocation("<>");
}

void ResourceRegistry::_ApplyMountDirectory(RegistryDirectory* pMountPoint, std::unique_lock<std::recursive_mutex>& lck)
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
    if(!pos)
    {
        return LocateConcreteResourceLocation(path);
    }
    else if(pos == path.length() - 1)
    {
        auto loc = _Locate(path);
        return loc ? loc->Name : "";
    }
    
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
    return FileGetFilename(address);
}

void ResourceRegistry::_LegacyApplyMount(Ptr<ResourceConcreteLocation<RegistryDirectory_System>>& dir, ResourceLogicalLocation* pMaster,
                                         const String& folderID, const String& fspath, std::unique_lock<std::recursive_mutex>& lck)
{
    // Find all .TTARCH archives. Lets put their priority higher to prefer those resources over filesystem, such that telltale do.
    
    std::set<String> archives{};
    StringMask mask = _ArchivesMask(UsingLegacyCompat());
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

void ResourceRegistry::MountArchive(const String &_id, const String &fspath)
{
    StringMask mask = _ArchivesMask(UsingLegacyCompat());
    TTE_ASSERT(mask == fspath, "Archive files cannot be mounted this way. Prefer to use MountArchive(...)");
    String id = _id;
    if(!StringEndsWith(id, "/"))
        id += "/";
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

String ResourceRegistry::LocateResource(const void *pResourceMemory)
{
    SCOPE_LOCK();
    for(const auto& h: _AliveHandles)
    {
        if(h._Handle.get() == pResourceMemory || h._Instance._GetInternal() == pResourceMemory)
            return SymbolTable::Find(h._ResourceName);
    }
    for(const auto& h: _DirtyHandles)
    {
        if(h._Handle.get() == pResourceMemory || h._Instance._GetInternal() == pResourceMemory)
            return SymbolTable::Find(h._ResourceName);
    }
    return "";
}

void ResourceRegistry::MountSystem(const String &id, const String& _fspath, Bool force)
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
    
    if(bUsesResourceSys && !force)
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

void ResourceRegistry::GetLocationResourceNames(const Symbol &location, std::set<String> &outNames, const StringMask *optionalMask)
{
    SCOPE_LOCK();
    for(const auto& l: _Locations)
    {
        if(location == l->Name)
        {
            l->GetResourceNames(outNames, optionalMask);
            return;
        }
    }
}

Bool ResourceRegistry::ResourceLocationExists(const Symbol &name)
{
    SCOPE_LOCK();
    for(const auto& l: _Locations)
    {
        if(name == l->Name)
            return true;
    }
    return false;
}

void ResourceRegistry::CreateConcreteDirectoryLocation(const String &_name, const String &_physPath)
{
    String physPath = _physPath;
    String name = _name;
    // very similar to mount. doesnt search for resource sets.
    if(!StringEndsWith(physPath, "/"))
        physPath += "/";
    if(!StringEndsWith(name, "/"))
        name += "/";
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
                                                  const String& archivePhysicalPath, Ptr<ResourceLocation>& parent, std::unique_lock<std::recursive_mutex>& lck)
{
    DataStreamRef archiveStream = parent->LocateResource(resourceName, nullptr);
    return archiveStream ? _ImportArchivePack(resourceName, archiveID, archivePhysicalPath, archiveStream, lck) : false;
}

Bool ResourceRegistry::_ImportArchivePack(const String& resourceName, const String& archiveID,
                                          const String& archivePhysicalPath, DataStreamRef& archiveStream, std::unique_lock<std::recursive_mutex>& lck)
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

void ResourceRegistry::CreateConcreteArchiveLocation(const String& _name, const String& resourceName)
{
    String name = _name;
    if(!StringEndsWith(name, "/"))
        name += "/";
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
    if(pSet && pSet->SetFlags.Test(ResourceSetFlags::APPLIED) && pSet->SetFlags.Test(ResourceSetFlags::DYNAMIC))
    {
        std::set<ResourceSet*> turnOn, turnOff{};
        turnOff.insert(pSet);
        _ReconfigureSets(turnOff, turnOn, lck, defer);
    }
}

void ResourceRegistry::ResourceSetDefaultLocation(const String &id)
{
    SCOPE_LOCK();
    if(_Locate(id))
    {
        _DefaultLocation = id;
        return;
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

static Bool _PerformHandleNormalise(HandleObjectInfo& handle, Ptr<ResourceRegistry> pRegistry)
{
    Bool res = true;
    TTE_ASSERT(handle._Instance, "Cannot normalise handle as the instance is not present / did not load correctly");
    LuaManager& L = GetThreadLVM();
    
    const auto& clazz = Meta::GetInternalState().Classes.find(handle._Instance.GetClassID())->second;
    if((clazz.Flags & Meta::_CLASS_PROP) == 0)
    {
        if(!handle._Handle)
        {
            CommonInstanceAllocator* pAllocator = Meta::GetCommonAllocator(handle._Instance.GetClassID());
            if(pAllocator)
            {
                handle._Handle = pAllocator(pRegistry);
            }
            else
            {
                // should be registered in the registry! if not likely because the compiler did not register it from the instantiation of the template.
                TTE_LOG("Could not allocate common instance for class %s! The instance allocator was not found in the coersion registry. This likely means the compiler has failed"
                        " us or more likely this class cannot be used as a resource at the moment because it is not supported!", Meta::GetClass(handle._Instance.GetClassID()).Name.c_str());
                handle._Flags.Remove(HandleFlags::NEEDS_NORMALISATION);
                return false;
            }
        }
        InstanceTransformation::PerformNormaliseAsync(handle._Handle, handle._Instance, L);
        // remove instance, not needed
        handle._Instance = {};
    }

    handle._Flags.Add(HandleFlags::LOADED);
    handle._Flags.Remove(HandleFlags::NEEDS_NORMALISATION);
    return res;
}

void ResourceRegistry::_ProcessDirtyHandle(HandleObjectInfo&& handle, std::unique_lock<std::recursive_mutex>& lck)
{
    if(handle._Flags.Test(HandleFlags::NON_PURGABLE))
        handle._Flags.Remove(HandleFlags::NEEDS_DESTROY);
    if(handle._Flags.Test(HandleFlags::NEEDS_DESTROY))
    {
        handle._OnUnload(*this, lck);
        return;
    }
    Bool bInsert = true;
    String name = {};
    if(handle._Flags.Test(HandleFlags::NEEDS_SERIALISE_IN))
    {
        if(handle._OpenStream)
        {
            name = SymbolTable::Find(handle._ResourceName);
            if(name.length() == 0)
                name = SymbolToHexString(handle._ResourceName);
            handle._Instance = Meta::ReadMetaStream(name, handle._OpenStream); // dont want to lock, as resource is not findable at the moment (until pushed into alive)
            handle._OpenStream.reset(); // release stream
        }
        else
        {
            // Was not found, ignore
            handle._Flags.Remove(HandleFlags::NEEDS_NORMALISATION);
        }
        handle._Flags.Remove(HandleFlags::NEEDS_SERIALISE_IN);
    }
    if(handle._Flags.Test(HandleFlags::NEEDS_NORMALISATION))
    {
        bInsert = _PerformHandleNormalise(handle, shared_from_this());
    }
    if(bInsert)
        _AliveHandles.insert(std::move(handle)); // make normal again
}

void ResourceRegistry::_ProcessDirtyHandles(Float budget, U64 startStamp, std::unique_lock<std::recursive_mutex>& lck)
{
    while (_DirtyHandles.size() && GetTimeStampDifference(startStamp, GetTimeStamp()) < budget)
    {
        auto handle = std::move(_DirtyHandles.back());
        _DirtyHandles.pop_back();
        _ProcessDirtyHandle(std::move(handle), lck);
    }
}

Bool ResourceRegistry::UnloadResource(const Symbol &resourceName)
{
    SCOPE_LOCK();
    Bool bFound = false;
    HandleObjectInfo proxy{};
    proxy._ResourceName = resourceName;
    auto it = _AliveHandles.lower_bound(proxy);
    // test alive handles
    if(it != _AliveHandles.end() && it->_ResourceName == resourceName)
    {
        Bool bHasLoaded = it->_Flags.Test(HandleFlags::LOADED) && !it->_Flags.Test(HandleFlags::NON_PURGABLE);
        if(!bHasLoaded)
        {
            // unload it
            HandleObjectInfo&& handle = std::move(_AliveHandles.extract(it).value());
            handle._Flags.Add(HandleFlags::NEEDS_DESTROY); // everything else ignored
            _ProcessDirtyHandle(std::move(handle), lck);
        }
        return bHasLoaded;
    }
    // test currently dirty
    for(auto it = _DirtyHandles.begin(); it != _DirtyHandles.end(); it++)
    {
        if(it->_ResourceName == resourceName)
        {
            if(!it->_Flags.Test(HandleFlags::NON_PURGABLE))
            {
                it->_Flags.Add(HandleFlags::NEEDS_DESTROY);
                HandleObjectInfo&& handle = std::move(*it);
                _DirtyHandles.erase(it);
                _ProcessDirtyHandle(std::move(handle), lck);
            }
            else
            {
                return false;
            }
            return true;
        }
    }
    return false;
}

void HandleBase::_SetObject(Ptr<ResourceRegistry> &registry, Symbol name, Bool bUnloadOld, Bool bEnsureLoaded)
{
    // IF OLD NEEDS UNLOAD
    if(_ResourceName && bUnloadOld)
    {
        Bool bFound = false;
        std::unique_lock<std::recursive_mutex> lck{registry->_Guard}; // SCOPE LOCK
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

void HandleObjectInfo::_OnUnload(ResourceRegistry &registry, std::unique_lock<std::recursive_mutex> &lck)
{
    ;
}

Bool HandleBase::IsLoaded(Ptr<ResourceRegistry> &registry)
{
    return _ResourceName ? registry->_EnsureHandleLoadedLocked(*this, true) : false;
}

void HandleBase::EnsureIsLoaded(Ptr<ResourceRegistry> &registry)
{
    if(_ResourceName)
        registry->_EnsureHandleLoadedLocked(*this, false);
    else
        TTE_LOG("WARNING: Handle<T>::EnsureLoaded() called but resource name not assigned");
}

Ptr<Handleable> HandleBase::_GetObject(Ptr<ResourceRegistry>& registry, Meta::ClassInstance* pClazz)
{
    if(!_ResourceName.GetCRC64())
        return {};
    std::lock_guard<std::recursive_mutex> _Lk(registry->_Guard);
    HandleObjectInfo proxy{};
    proxy._ResourceName = _ResourceName;
    auto it = registry->_AliveHandles.lower_bound(proxy);
    if(it != registry->_AliveHandles.end() && it->_ResourceName == _ResourceName)
    {
        if(pClazz)
            *pClazz = it->_Instance;
        return it->_Handle;
    }
    return {};
}

Bool ResourceRegistry::_SetupHandleResourceLoad(HandleObjectInfo &hoi, std::unique_lock<std::recursive_mutex> &lck)
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
            TTE_LOG("WARNING: The resource %s was not found in the resource registry so cannot be loaded! Empty placeholder will be used.", name.c_str());
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
            _ProcessDirtyHandle(std::move(handle), lck);
            return bResult;
        }
        return true;
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
            _ProcessDirtyHandle(std::move(*it), lck);
            _DirtyHandles.erase(it);
            return bResult;
        }
    }
    if(bOnlyQuery)
        return false; // not loaded
    // CREATE NEW HOI, LOAD IT. Exception here is props, which there is not Handle memory its just the class.
    HandleObjectInfo hoi{};
    hoi._ResourceName = handle._ResourceName;
    Bool bResult = _SetupHandleResourceLoad(hoi, lck);
    if(bResult)
    {
        _ProcessDirtyHandle(std::move(hoi), lck);
    }
    return bResult;
}

void ResourceRegistry::_UnloadResources(std::vector<std::pair<Symbol, Ptr<ResourceLocation>>> &resources,
                                        std::unique_lock<std::recursive_mutex>& lck, U32 mx)
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

void ResourceRegistry::_ReconfigureSets(const std::set<ResourceSet*>& turnOff, const std::set<ResourceSet*>& turnOn, std::unique_lock<std::recursive_mutex>& lck, Bool bDefer)
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
            if(CompareCaseInsensitive(_DefaultLocation, it->Name))
                _DefaultLocation = "<>";
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
                job->HOI[i]._Handle = job->Allocators[i](job->Registry);
                bFail = !_PerformHandleNormalise(job->HOI[i], job->Registry);
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
        std::unique_lock<std::recursive_mutex> lck{job->Registry->_Guard};
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
                        // might be in dirty. check and if so then unload it (if non-purgable ignore)
                        for(auto it = job->Registry->_DirtyHandles.begin(); it != job->Registry->_DirtyHandles.end(); it++)
                        {
                            if(it->_ResourceName == job->HOI[i]._ResourceName)
                            {
                                if(it->_Flags.Test(HandleFlags::NON_PURGABLE))
                                    continue;
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

// HANDLEABLE BASE

Bool Handleable::Lock(const HandleLockOwner& owner)
{
    U32 expected = 0;
    Bool locked = false;
    if((locked = _LockKey.compare_exchange_strong(expected, owner._LockOwnerID)))
        _LockTimeStamp.store(GetTimeStamp());
    return locked;
}

void Handleable::Unlock(const HandleLockOwner& owner)
{
    TTE_ASSERT(owner._LockOwnerID, "Invalid handle lock, it was not constructed.");
    U32 expected = owner._LockOwnerID;
    _LockTimeStamp.store(0, std::memory_order_relaxed); // should defintely be locked anyway (Will assert). if not, terminate as should NOT happen
    TTE_ASSERT(_LockKey.compare_exchange_strong(expected, 0), "Cannot unlock handle as the key was wrong!");
}

HandleLockOwner::HandleLockOwner() : _LockOwnerID((U32)rand() | 0x100) {}

Handleable& Handleable::operator=(Handleable&& rhs)
{
    _LockKey.store(rhs._LockKey.exchange(0));
    _Registry = std::move(rhs._Registry);
    return *this;
}

Bool Handleable::IsLocked() const
{
    return _LockKey.load() != 0;
}

Bool Handleable::OwnedBy(const HandleLockOwner& lockOwner, Bool bLock) const
{
    U32 expected = 0;
    if (_LockKey.load() == lockOwner._LockOwnerID)
        return true;
    return bLock ? _LockKey.compare_exchange_strong(expected, lockOwner._LockOwnerID) : false;
}

U64 Handleable::GetLockedTimeStamp(const HandleLockOwner& owner, Bool bRelaxed)
{
    TTE_ASSERT(OwnedBy(owner, false), "Lock is not owned");
    return _LockTimeStamp.load(bRelaxed ? std::memory_order_relaxed : std::memory_order_acquire);
}

void Handleable::OverrideLockedTimeStamp(const HandleLockOwner &owner, Bool bRelaxed, U64 value)
{
    TTE_ASSERT(OwnedBy(owner, false), "Lock is not owned");
    _LockTimeStamp.store(value, bRelaxed ? std::memory_order_relaxed : std::memory_order_release);
}

Ptr<ResourceRegistry> Handleable::GetRegistry() const
{
    return _Registry.expired() ? nullptr : _Registry.lock();
}

void Meta::_Impl::_Coersion<Handle<Placeholder>>::Extract(Handle<Placeholder>& out, ClassInstance& inst)
{
    ClassInstance mem = GetMember(inst, "mHandle", true); // mHandle must exist (It should in all games).
    TTE_ASSERT(Is(mem, "Symbol") || Is(mem, "class Symbol"), "Handle<T>::mHandle is not a Symbol");
    Symbol val{};
    Meta::ExtractCoercableInstance(val, mem);
    out.SetObject(val);
}

void Meta::_Impl::_Coersion<Handle<Placeholder>>::Import(const Handle<Placeholder>& in, ClassInstance& inst)
{
    ClassInstance mem = GetMember(inst, "mHandle", true);
    TTE_ASSERT(Is(mem, "Symbol") || Is(mem, "class Symbol"), "Handle<T>::mHandle is not a Symbol");
    Symbol val = in.GetObject();
    Meta::ImportCoercableInstance(val, mem);
}

void Meta::_Impl::_Coersion<Handle<Placeholder>>::ImportLua(const Handle<Placeholder>& in, LuaManager& man)
{
    man.PushLString(SymbolTable::FindOrHashString(in.GetObject()));
}

void Meta::_Impl::_Coersion<Handle<Placeholder>>::ExtractLua(Handle<Placeholder>& out, LuaManager& man, I32 stackIndex)
{
    out.SetObject(ScriptManager::ToSymbol(man, stackIndex));
}
