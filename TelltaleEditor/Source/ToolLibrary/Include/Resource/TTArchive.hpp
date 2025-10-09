#pragma once

#include <Resource/DataStream.hpp>
#include <vector>
#include <set>

//.TTARCH FILES
class TTArchive {
public:
    
    static constexpr CString Extension = ".ttarch"; // also .TTA as well
    
    /**
     * VERSIONS:
     * 0: First used in Boneville, no encryption.
     * 1: Used in Boneville PC: encrypted header block with simple blowfish
     */
    inline TTArchive(U32 version)
    {
        _Version = version; // version is determined by current game in tool context. pass it in here.
    }
    
    Bool SerialiseIn(DataStreamRef& in);
    
    Bool SerialiseOut(DataStreamRef& in); // not intrinsically async. however, a job wrapping this can be used as it stays in this function
    
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
    
    // Clears everything and releases.
    inline void Reset()
    {
        _Files.clear();
        _Folders.clear();
    }
    
    inline Bool IsActive()
    {
        return _Files.size() > 0;
    }
    
private:
    
    friend class RegistryDirectory_TTArchive;
    
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
    
    struct FileInfoSorter
    {
        
        inline Bool operator() (const FileInfo& lhs, const FileInfo& rhs) const
        {
            return lhs.NameSymbol < rhs.NameSymbol;
        }
        
    };
    
    U32 _Version = 0; // version. game sets this version in the lua scripts. each version differs in format by a lot!
    std::vector<FileInfo> _Files;
    std::vector<String> _Folders; // folder names
    
};
