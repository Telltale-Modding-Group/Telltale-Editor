#include <Resource/TTArchive2.hpp>
#include <Meta/Meta.hpp>

Bool TTArchive2::SerialiseIn(DataStreamRef& in)
{
    if(!in)
        return false; // no input
    
    // wrap it
    DataStreamRef wrapper = DataStreamManager::GetInstance()->CreateContainerStream(in);
    in = std::move(wrapper); // replace in (wrapper keeps ref)
    
    if(in->GetSize() == 0)
        return false; // errored reading container header
    
    U32 Magic = {};
    SerialiseDataU32(in, 0, &Magic, false); // read magic
    _Version = Magic & 7; // bottom 3 bits are the version from 'TTAX' where X is ASCII version (number + 0x30)
    // check top bits again TTA0 (ignore bottom bits of version) and if version is out of range
    if((Magic & 0xFFFFFFF8) != 0x54544130)
    {
        TTE_LOG("Error opening archive 2: invalid format"); // not a ttarch2
        return false;
    }
    if(_Version < 2 || _Version > 4)
    {
        TTE_LOG("Error opening archive 2: unsupported version. Please contact us with this file"); // TTA0,1,5?,... not seen these!
        _Version = 0;
        return false;
    }
    
    U32 filenameBufferSize = {};
    SerialiseDataU32(in, 0, &filenameBufferSize, false);
    if(filenameBufferSize > 0x10000000) // this check is in the executables too
    {
        TTE_LOG("Error opening archive 2: file name buffer is too large");
        _Version = 0;
        return false;
    }
    
    U32 fileCount = {};
    SerialiseDataU32(in, 0, &fileCount, false);
    if(fileCount > 0xFFFFF) // this check is in the executables too
    {
        TTE_LOG("Error opening archive 2: contains too many files");
        _Version = 0;
        return false;
    }
    
    // setup
    _Files.clear();
    _Files.reserve((size_t)fileCount);
    
    std::vector<InternalFileInfo> _inf{}; // for reading
    _inf.reserve(_Files.capacity());
    
    for(U32 i = 0; i < fileCount; i++) // read files
    {
        
        InternalFileInfo& info = _inf.emplace_back();
        SerialiseDataU64(in, 0, &info.Offset, false); // name crc. skip this, we will get these from the filename table.
        SerialiseDataU64(in, 0, &info.Offset, false); // file offset
        
        if(_Version == 2) // TTA2
        {
            U32 unused{}; // maybe some metadata. no clue, maybe was going to be the version crc of the main type?
            SerialiseDataU32(in, 0, &unused, false);
        }
        
        SerialiseDataU32(in, 0, &info.Size, false); // file size
        
        U32 unused{}; // i think these used to be folder indices. they obviously don't use these here.
        SerialiseDataU32(in, 0, &unused, false);
        
        U32 filenameBufferOffset = {};
        SerialiseDataU32(in, 0, &filenameBufferOffset, false); // they read two ushorts, ill read the uint and flip the top&bottom ushorts.
        filenameBufferOffset = ((filenameBufferOffset & 0xFFFF) << 16) | (filenameBufferOffset >> 16);

        info.NameOffset = filenameBufferOffset;
        
        TTE_ASSERT(filenameBufferOffset < filenameBufferSize, "Error opening archive 2: file name buffer offset is invalid");
        
    }
    
    // all read. now create sub streams
    
    U64 FilenamesOffset = in->GetPosition(); // position for filename table
    U64 FileDataOffset = FilenamesOffset + filenameBufferSize;
    
    // create temp buffer
    U8* TempFileNames = TTE_ALLOC(filenameBufferSize, MEMORY_TAG_TEMPORARY);
    if(!in->Read(TempFileNames, filenameBufferSize))
    {
        TTE_FREE(TempFileNames);
        TTE_LOG("Cannot open archive 2: could not read file name buffer");
        _Version = 0;
        _Files.clear();
        return false;
    }
    
    for(U32 i = 0; i < fileCount; i++)
    {
        // read file names
        FileInfo& inf = _Files.emplace_back();
        inf.Name = (CString)(TempFileNames + _inf[i].NameOffset); // these are null terminated, create string memory w/ std string
        
        // create sub stream
        inf.Stream = DataStreamManager::GetInstance()->CreateSubStream(in, _inf[i].Offset, (U64)_inf[i].Size);
    }
    
    // free temp buffer
    TTE_FREE(TempFileNames);
    TempFileNames = nullptr;
    
    return true;
}

// we can write the archive n the main thread, it doesn't take too long. the compression and writing to file can be done async if needed.
Bool TTArchive2::SerialiseOut(DataStreamRef& o, ContainerParams params, JobHandle& handle)
{
    
    if(_Files.size() > 0xFFFFF)
    {
        TTE_ASSERT(false, "TTArchive::SerialiseOut => cannot write more than %d files in one archive", 0xFFFFF);
        return false;
    }
    
    // ========================== 1: Write header stream and name table
    
    DataStreamRef headerStream = DataStreamManager::GetInstance()->CreatePrivateCache("TTArchive2::Header");
    DataStreamRef nameStream = DataStreamManager::GetInstance()->CreatePrivateCache("TTArchive2::NameTable");

    // Write magic
    U8 Magic[4] {'0', 'A', 'T', 'T'};
    Magic[0] = 0x30 + _Version;
    headerStream->Write(Magic, 4);
    
    // Write magic again, it will be replaced the name table after the calculation
    headerStream->Write(Magic, 4);
    
    // Write file count
    U32 files = (U32)_Files.size();
    SerialiseDataU32(headerStream, nullptr, &files, true);
    
    // Write file information for each file
    std::sort(_Files.begin(), _Files.end(), FileInfoSort{}); // archive hashes need to be sorted for internal binary sorts
    U32 nameTableOffs = 0;
    U64 runningOffset = 0;
    for(auto& file : _Files)
    {
        U64 data = Symbol(file.Name).GetCRC64();
        SerialiseDataU64(headerStream, nullptr, &data, true); // write name hash
        
        data = runningOffset;
        SerialiseDataU64(headerStream, nullptr, &data, true); // write offset
        runningOffset += data;
        
        if(_Version == 2) // TTA2
        {
            U32 unused{};
            SerialiseDataU32(headerStream, 0, &unused, true);
        }
        
        U32 size = (U32)file.Stream->GetSize();
        SerialiseDataU32(headerStream, nullptr, &size, true); // write file size
        
        U32 unused{};
        SerialiseDataU32(headerStream, 0, &unused, true); // unused
        
        size = ((nameTableOffs & 0xFFFF) << 16) | (nameTableOffs >> 16);
        SerialiseDataU32(headerStream, nullptr, &size, true); // write name table offset (flipped, block and offset)
        nameTableOffs += (U32)file.Name.length() + 1; // + 1 null terminator
        
        // write name to name table
        nameStream->Write((const U8*)file.Name.c_str(), (U64)file.Name.length() + 1); // + null terminator
    }
    
    // pad name table
    DataStreamManager::GetInstance()->WriteZeros(nameStream, TTE_PADDING(nameTableOffs, 0x10000));
    
    headerStream->SetPosition(4);
    U32 NTSize = (U32)nameStream->GetSize();
    if(NTSize > 0x10000000)
    {
        TTE_LOG("Filenames are too large when writing TTArchive2");
        return false;
    }
    SerialiseDataU32(headerStream, nullptr, &NTSize, true); // write name table size

    headerStream->SetPosition(0);
    nameStream->SetPosition(0);
    
    // ========================== 2: Combine all file streams into sequential stream and write container
    
    std::shared_ptr<DataStreamSequentialStream> Container = DataStreamManager::GetInstance()->CreateSequentialStream("TTArchive2::Container");
    
    Container->PushStream(headerStream); // header stream
    Container->PushStream(nameStream); // name table stream
    
    for(auto& file : _Files)
        Container->PushStream(file.Stream); // push each file stream
    
    DataStreamRef casted = Container;
    Bool bResult = DataStreamManager::GetInstance()->FlushContainer(casted, Container->GetSize(), o, params, handle); // flush everything
    
    // ==========================
    
    return bResult;
}
