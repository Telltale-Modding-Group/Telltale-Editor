#pragma once

#include <Resource/DataStream.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <vector>
#include <set>
#include <algorithm>

//.TTARCH2 FILES. CACHES, UNTIL DESTROYED, ANY STREAM(S) PASSED INTO SERIALISE IN.
class TTArchive2 {
public:
    
    static constexpr CString Extension = ".ttarch2";
    
    inline TTArchive2(U32 version) : _Version(version) {}
    
    ~TTArchive2();
    
    Bool SerialiseIn(DataStreamRef& in);
    
    // May be async. Do not reference out between. If return false it failed and no async. Else check handle validity.
    Bool SerialiseOut(DataStreamRef& out, ContainerParams params, JobHandle& handle);
    
    // Returns the binary stream of the given file name symbol in this data archive.
    inline DataStreamRef Find(const Symbol& fn, String* outName) const
    {
        FileInfo proxy{"", fn, DataStreamRef{}};
        auto it = std::lower_bound(_Files.begin(), _Files.end(), proxy);
        if(it != _Files.end() && it->NameSymbol == fn)
        {
            if(outName)
                *outName = it->Name;
            it->Stream->SetPosition(0);
            return it->Stream;
        }
        return {};
    }
    
    // Adds a file to this archive, replacing the old one.
    inline void AddFile(const String& name, DataStreamRef stream)
    {
        TTE_ASSERT(stream, "Stream not valid");
        
        // Check to replace any
        for(auto& file : _Files)
        {
            if(Symbol(name) == Symbol(file.Name))
            {
                file.Stream = stream; // update to the new stream
                return;
            }
        }
        
        // Doesn't exist, add new entry
        FileInfo inf{name, Symbol(name), std::move(stream)};
        VectorInsertSorted(_Files, std::move(inf));
    }
    
    // Puts all file names inside this archive into the output result array
    inline void GetFiles(std::set<String>& result)
    {
        for(auto& file : _Files)
            result.insert(file.Name);
    }
    
    inline void Reset()
    {
        _Files.clear();
    }
    
    inline Bool IsActive()
    {
        return _Files.size() > 0;
    }
    
private:
    
    struct FileInfo // file
    {
        
        String Name;
        Symbol NameSymbol;
        DataStreamRef Stream;
        
        inline Bool operator<(const FileInfo& rhs) const
        {
            return NameSymbol < rhs.NameSymbol;
        }
        
    };
    
    struct InternalFileInfo // internal while reading
    {
        
        U64 Offset; // offset in archive
        U64 CRC; // fn crc
        U32 Size; // size of file
        U32 NameOffset; // offset in filename buffer
        
    };
    
    U32 _Version; // 2 = TTA2, 3 = TTA3, 4 = TTA4.
    std::vector<FileInfo> _Files;
    
    friend class RegistryDirectory_TTArchive2;
    
};
