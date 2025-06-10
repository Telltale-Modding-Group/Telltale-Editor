#include <Resource/TTArchive.hpp>
#include <Meta/Meta.hpp>
#include <Resource/Blowfish.hpp>

Bool TTArchive::SerialiseIn(DataStreamRef& in)
{
    if(_Version == 0 || _Version == 1) // earliest versions (abstract numbers, not serialised)
    {

        DataStreamRef cachedIn = in;
        if(_Version == 1)
        {
            // Encrypted header
            U32 headerSize{};
            SerialiseDataU32(in, 0, &headerSize, false);
            TTE_ASSERT_AND_RETURN(headerSize < 0x100000, false, "Corrupt archive: header size too big");
            U8* Temp = TTE_ALLOC(headerSize, MEMORY_TAG_TEMPORARY);
            if(!in->Read(Temp, headerSize))
            {
                TTE_ASSERT_AND_RETURN(false, false, "Corrupt archive: header size too big");
            }
            Blowfish::GetInstance()->Decrypt(Temp, headerSize);
            in = DataStreamManager::GetInstance()->CreateBufferStream("__Temp__ArchiveRead", headerSize, Temp, true);
        }

        U32 read{}; // temp read value
        SerialiseDataU32(in, 0, &read, false); // read number of folders
        U32 numFolders = read;
        
        TTE_ASSERT_AND_RETURN(numFolders < 10000, false, "Corrupt archive: too many folders");
        
        for(U32 i = 0; i < numFolders; i++)
        {
            _Folders.push_back(DataStreamManager::GetInstance()->ReadString(in)); // read folder name string
        }
        
        SerialiseDataU32(in, 0, &read, false); // read number of files
        U32 numFiles = read;
        
        TTE_ASSERT_AND_RETURN(numFiles < 1000000, false, "Corrupt archive: too many files");
        
        struct _FileInf{U32 sz; U32 off;};
        std::vector<_FileInf> offsets{};
        
        for(U32 i = 0; i < numFiles; i++) // FILE INFO
        {
            
            FileInfo inf{}; // filled later
            _FileInf bytes{}; // internal
            inf.Name = DataStreamManager::GetInstance()->ReadString(in);
            inf.NameSymbol = Symbol(inf.Name);
            
            SerialiseDataU32(in, 0, &read, false);
            TTE_ASSERT_AND_RETURN(read == 0, false, "Archive format needs checking"); // folder index ??
            // always zero otherwise. seen until even newest games, this zero in the file info.
            
            SerialiseDataU32(in, 0, &bytes.off, false); // read file offset
            SerialiseDataU32(in, 0, &bytes.sz, false); // read file size
            
            _Files.push_back(std::move(inf));
            offsets.push_back(bytes);
            
        }
        
        U32 fileDataOff{};
        SerialiseDataU32(in, 0, &read, false); // file data start offset.
        fileDataOff = read;
        
        SerialiseDataU32(in, 0, &read, false); // no idea what this is. looks like they dont use it though. its size of files plus some weird extra

        in = cachedIn;
        
        // create sub streams for each file.
        for(U32 i = 0; i < numFiles; i++)
        {
            _Files[i].Stream = DataStreamManager::GetInstance()->CreateSubStream(in, (U64)fileDataOff +
                                                                                 (U64)offsets[i].off, (U64)offsets[i].sz);
        }
        offsets.clear();
        
        // sort files
        std::sort(_Files.begin(), _Files.end(), FileInfoSorter{});
        
        return true; // OK
    }
    
    TTE_ASSERT(false, "Bad archive version: %d", _Version);
    return false;
}

Bool TTArchive::SerialiseOut(DataStreamRef& in)
{
    TTE_ASSERT("IMPLEMENT ME");
    return false; // TODO
}

