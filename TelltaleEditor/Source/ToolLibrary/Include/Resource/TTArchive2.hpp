#pragma once

#include <Resource/DataStream.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <vector>
#include <set>

//.TTARCH2 FILES. CACHES, UNTIL DESTROYED, ANY STREAM(S) PASSED INTO SERIALISE IN.
class TTArchive2 {
public:
    
    inline TTArchive2(U32 version) : _Version(version) {}
    
    Bool SerialiseIn(DataStreamRef& in);
    
    // May be async. Do not reference out between. If return false it failed and no async. Else check handle validity.
    Bool SerialiseOut(DataStreamRef& out, ContainerParams params, JobHandle& handle);
    
    // Returns the binary stream of the given file name symbol in this data archive.
    inline DataStreamRef Find(const Symbol& fn) const
    {
        for(auto& file : _Files)
        {
            if(Symbol(file.Name) == fn)
                return file.Stream;
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
        _Files.push_back({name, stream});
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
	
private:
    
    struct FileInfo // file
    {
        
        String Name;
        DataStreamRef Stream;
        
    };
    
    // for internal sorting before writes
    struct FileInfoSort
    {
        
        inline Bool operator()(const FileInfo& lhs, const FileInfo& rhs)
        {
            return Symbol(lhs.Name) < Symbol(rhs.Name);
        }
        
    };
    
    struct InternalFileInfo // internal while reading
    {
        U64 Offset; // offset in archive
        U32 Size; // size of file
        U32 NameOffset; // offset in filename buffer
    };

    U32 _Version; // 2 = TTA2, 3 = TTA3, 4 = TTA4.
    std::vector<FileInfo> _Files;
    
	friend class RegistryDirectory_TTArchive2;
	
};
