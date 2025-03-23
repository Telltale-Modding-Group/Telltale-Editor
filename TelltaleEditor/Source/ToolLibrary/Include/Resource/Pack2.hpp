#pragma once

#include <Resource/DataStream.hpp>
#include <Meta/Meta.hpp>

#include <vector>
#include <set>

// .PK2 (GAMEDATA.PK2 etc) FILES (OLD CONSOLE PACKS) ie 'TTArchive' legacy class only used in PS2 i think?
class GamePack2 {
public:
    
    static constexpr CString Extension = ".pk2";
    
    Bool SerialiseIn(DataStreamRef& in);
    
    Bool SerialiseOut(DataStreamRef& in);
    
    // Returns the binary stream of the given file name symbol in this data archive.
    DataStreamRef Find(const Symbol& fn, String* outName) const;
    
    // Adds a file to this archive, replacing the old one.
    void AddFile(const String& name, DataStreamRef stream);
    
    // Puts all file names inside this archive into the output result array
    void GetFiles(std::set<String>& result);
    
    void GetSubDirectories(Bool bRecurseAll, std::set<String>& result); // gets all resource names.
    
    void DeleteFile(const Symbol& resource); // delete a resource
    
    void RenameResource(const Symbol& resource, const String& name); // change name
    
    DataStreamRef CreateResource(const String& name); // create new
    
    void CopyResource(const Symbol& srcName, const String& destName); // copy duplicate
    
    // Clears everything and releases.
    inline void Reset()
    {
        
    }
    
private:
    
    void _GatherFiles(String folderName, DataStreamRef& cached, Meta::ClassInstance folder);
    
    friend class RegistryDirectory_GamePack2;
    
    struct File
    {
        
        String Name;
        U32 FileNameCRC;
        DataStreamRef Stream;
        
        inline Bool operator<(const File& rhs) const
        {
            return FileNameCRC < rhs.FileNameCRC;
        }
        
        inline Bool operator==(const File& rhs) const
        {
            return FileNameCRC == rhs.FileNameCRC;
        }
        
    };
    
    std::set<File> _Files;
    std::map<Symbol, U32> _FileNamesMap; // normal symbol to crc32
    
    inline typename std::set<File>::iterator _Find(Symbol fn, String* outName) const
    {
        auto it = _FileNamesMap.find(fn);
        if(it != _FileNamesMap.end())
        {
            File p{"",it->second, {}};
            auto f = _Files.find(p);
            if(f != _Files.end())
                return f;
        }
        U32 crc = (U32)(fn.GetCRC64() & 0xFFFF'FFFF);
        File p{"",crc, {}};
        return _Files.find(p);
    }
    
};
