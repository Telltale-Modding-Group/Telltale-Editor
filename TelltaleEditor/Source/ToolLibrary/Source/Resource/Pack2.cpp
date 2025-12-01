#include <Resource/Pack2.hpp>
#include <Meta/Meta.hpp>
#include <Core/Context.hpp>

#include <filesystem>

// .PK2 TTARCHIVE

void GamePack2::_GatherFiles(String folderN, DataStreamRef& cached, Meta::ClassInstance folder)
{
    String name = Meta::GetMember<String>(folder, "mName");
    Meta::ClassInstance fileInfo = Meta::GetMember(folder, "_mFileInfoBuf", true);
    Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)fileInfo._GetInternal());
    U32 numFiles = Meta::GetMember<U32>(folder, "_mNumFiles");
    
    for(U32 i = 0; i < numFiles; i++)
    {
        U32* pFileInfo = (U32*)(buf.BufferData.get() + (i * 12));
        String fileName = SymbolTable::Find(Symbol((U64)pFileInfo[0])); // NAME HASH CRC32 (store in db as crc32 proxy)
        if(fileName.length())
        {
            _FileNamesMap[Symbol(fileName)] = pFileInfo[0];
        }
        else
        {
            fileName = SymbolToHexString((Symbol)pFileInfo[0]);
        }
        U32 offset = pFileInfo[1];
        U32 size = pFileInfo[2];
        File f{};
        f.Stream = DataStreamManager::GetInstance()->CreateSubStream(cached, offset, size);
        TTE_ATTACH_DBG_STR(f.Stream.get(), "GamePack2 SubStream: " + fileName);
        f.Name = folderN + fileName;
        f.FileNameCRC = pFileInfo[0];
        std::replace(f.Name.begin(), f.Name.end(), '\\', '/');
        _FileNamesMap[Symbol(f.Name)] = f.FileNameCRC; // full path as well
        _FileNamesMap[Symbol(fileName)] = f.FileNameCRC; // crc of symbol just in case
        _Files.insert(std::move(f));
    }
    
    // add subfolders
    Meta::ClassInstance folderMember = Meta::GetMember(folder, "mFolders", true);
    Meta::ClassInstanceCollection& folderMap = Meta::CastToCollection(folderMember);
    for(U32 i = 0; i < folderMap.GetSize(); i++)
    {
        Meta::ClassInstance keyName = folderMap.GetKey(i);
        Meta::ClassInstance subFolder = folderMap.GetValue(i);
        String folderName = folderN + *((String*)keyName._GetInternal());
        _GatherFiles(std::move(folderName), cached, subFolder);
    }
}

Bool GamePack2::SerialiseIn(DataStreamRef& in)
{
    String p = in->GetURL().GetPath();
    // This we actually delegate the work to the meta system, as for some reason this is a meta stream file.
    
    U32 classID = Meta::FindClass("TTArchive", 0);
    
    if(classID == 0)
    {
        TTE_LOG("Cannot load in PK2 game data archive %s as the game class could not be found. This file can only be used in specific game versions which use it", p.c_str());
        return false;
    }
    
    Meta::ClassInstance instance = Meta::ReadMetaStream(p, in);
    
    if(instance)
    {
        Meta::ClassInstance cachedStream = Meta::GetMember(instance, "_mCachedPayload", true);
        DataStreamRef cached = ((Meta::DataStreamCache*)cachedStream._GetInternal())->Stream;
        Meta::ClassInstance archiveBase = Meta::GetMember(instance, "Baseclass_TTArchiveFolder", true);
        // mDataSize is not set properly in the game and is left 0. pointless lol
        _GatherFiles("", cached, archiveBase);
    }
    else
    {
        TTE_LOG("Could not load PK2 from meta system");
        return false;
    }
    
    return true;
}

Bool GamePack2::SerialiseOut(DataStreamRef& in)
{
    TTE_ASSERT(false, "Implementation required");
    // update the cached stream to the final to write one (Ideally a temp on disc, depends on size), as files streams may have changed
    // go though files and create folders accordingly. mDataSize = 0 always idk.
    return false;
}

DataStreamRef GamePack2::Find(const Symbol& fn, String* outName) const
{
    auto it = _Find(fn, outName);
    if(it != _Files.end())
    {
        return it->Stream;
    }
    return {};
}

void GamePack2::AddFile(const String& name, DataStreamRef stream)
{
    File f{};
    f.Name = name;
    f.FileNameCRC = CRC32((const U8*)name.c_str(), (U32)name.length());
    f.Stream = std::move(stream);
    _Files.insert(std::move(f));
}

void GamePack2::GetFiles(std::set<String>& result)
{
    for(auto& f: _Files)
        result.insert(f.Name);
}

void GamePack2::GetSubDirectories(Bool bRecurseAll, std::set<String> &result)
{
    // treat as flat for resource system
}

void GamePack2::DeleteFile(const Symbol& resource)
{
    auto it = _Find(resource, 0);
    if(it != _Files.end())
    {
        _Files.erase(it);
    }
}

void GamePack2::RenameResource(const Symbol& resource, const String& _name)
{
    auto it = _Find(resource, 0);
    if(it != _Files.end())
    {
        File f = std::move(*it);
        _Files.erase(it);
        f.Name = _name;
        String name = std::filesystem::path(name).filename().string();
        f.FileNameCRC = CRC32((const U8*)name.c_str(), (U32)name.length());
        _FileNamesMap[Symbol(name)] = f.FileNameCRC;
        _Files.insert(std::move(f));
    }
}

DataStreamRef GamePack2::CreateResource(const String& _name)
{
    File f{};
    f.Name = _name;
    String name = std::filesystem::path(name).filename().string();
    f.FileNameCRC = CRC32((const U8*)name.c_str(), (U32)name.length());
    f.Stream = DataStreamManager::GetInstance()->CreatePrivateCache(name);
    _FileNamesMap[Symbol(name)] = f.FileNameCRC;
    _Files.insert(f);
    return f.Stream;
}

void GamePack2::CopyResource(const Symbol& srcName, const String& _name)
{
    auto it = _Find(srcName, 0);
    if(it != _Files.end())
    {
        File f{};
        f.Stream = it->Stream;
        f.Name = _name;
        String name = std::filesystem::path(name).filename().string();
        f.FileNameCRC = CRC32((const U8*)name.c_str(), (U32)name.length());
        _FileNamesMap[Symbol(name)] = f.FileNameCRC;
        _Files.insert(std::move(f));
    }
}
