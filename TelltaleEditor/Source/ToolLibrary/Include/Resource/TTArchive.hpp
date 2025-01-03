#pragma once

#include <Resource/DataStream.hpp>
#include <vector>

//.TTARCH FILES
class TTArchive {
public:
    
    inline TTArchive(U32 version)
    {
        _Version = version; // version is determined by current game in tool context. pass it in here.
    }
    
    Bool SerialiseIn(DataStreamRef& in);
    
    Bool SerialiseOut(DataStreamRef& in);
    
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
    inline void GetFiles(std::vector<String>& result)
    {
        result.resize(_Files.size());
        for(auto& file : _Files)
            result.push_back(file.Name);
    }
    
private:
    
    struct FileInfo // file
    {
        String Name;
        DataStreamRef Stream;
    };
    
    U32 _Version = 0; // version. game sets this version in the lua scripts. each version differs in format by a lot!
    std::vector<FileInfo> _Files;
    std::vector<String> _Folders; // folder names
    
};
