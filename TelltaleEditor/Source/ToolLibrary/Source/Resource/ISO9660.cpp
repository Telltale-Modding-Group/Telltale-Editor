#include <Resource/ISO9660.hpp>

// ---- HELPERS

#define ADVANCE(X) in->SetPosition(in->GetPosition() + X)
#define ADVANCE_DESC(X) in->SetPosition(0x8000 + (0x800 * X))
#define ADVANCE_LOCATOR(X) in->SetPosition((0x800 * X))

#define FLIP_ENDIAN(val) \
(((val) >> 24) & 0x000000FF) | \
(((val) >> 8) & 0x0000FF00) | \
(((val) << 8) & 0x00FF0000) | \
(((val) << 24) & 0xFF000000)

template<I32 L> // L= max length
inline String _ReadString(DataStreamRef& stream, U32 lenOverride = 0)
{
    U8 Str[L]{0};
    if(lenOverride == 0)
        lenOverride = L;
    else
        TTE_ASSERT(lenOverride < L, "ISO9660: Invalid string length");
    TTE_ASSERT(stream->Read(Str, (U64)lenOverride), "String read fail in ISO-9660");
    
    I32 end = lenOverride;
    while (end > 0 && Str[end - 1] == ' ')
    {
        --end;
    }
    
    return String(reinterpret_cast<char*>(Str), end);
}

// ------ IMPL

Bool ISO9660::VolumeTs::SerialiseIn(DataStreamRef &ref)
{
    U8 Temp[8];
    TTE_ASSERT(ref->Read(Temp, 4), "ISO9660-Timestamp: Could not read bytes");
    Temp[4] = 0;
    Year = (CString)Temp;
    Temp[2] = 0;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Month = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Day = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Hour = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Minute = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Second = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 2), "ISO9660-Timestamp: Could not read bytes");
    Centisecond = (CString)Temp;
    TTE_ASSERT(ref->Read(Temp, 1), "ISO9660-Timestamp: Could not read bytes");
    GMTOffset = (I8)Temp[0];
    
    return true;
}

Bool ISO9660::_ReadDir(DataStreamRef &in, ISO9660::DirectoryRecord& r)
{
    U8 Temp[8];

    U64 finalPosition = in->GetPosition();
    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read directory record bytes");
    finalPosition += Temp[0]; // read size
    
    TTE_ASSERT(in->Read(&r.ExtendedAttribRecordLength, 1), "ISO9660: Could not read directory record bytes");
    
    TTE_ASSERT(in->Read((U8*)&r.LogicalLocator, 4), "ISO9660: Could not read directory record bytes");
    ADVANCE(4); // big end
    
    TTE_ASSERT(in->Read((U8*)&r.DataLength, 4), "ISO9660: Could not read directory record bytes");
    ADVANCE(4); // big end
    
    TTE_ASSERT(in->Read(&r.Ts.YearsSince1900, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.Ts.Month, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.Ts.Day, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.Ts.Hour, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.Ts.Minute, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.Ts.Second, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read((U8*)&r.Ts.GMTOffset, 1), "ISO9660: Could not read directory record bytes");
    
    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read directory record bytes");
    r.Fl = Flags{(U32)Temp[0]};
    
    TTE_ASSERT(in->Read(&r.InterleavedUnitSize, 1), "ISO9660: Could not read directory record bytes");
    TTE_ASSERT(in->Read(&r.InterleavedUnitGap, 1), "ISO9660: Could not read directory record bytes");
    
    if(r.InterleavedUnitGap || r.InterleavedUnitSize)
    {
        TTE_ASSERT(false, "Interleaved ISOs not currently supported");
        return false;
    }
    
    TTE_ASSERT(in->Read((U8*)&r.ISOIndex, 2), "ISO9660: Could not read directory record bytes");
    ADVANCE(2); // big endian
    
    U8 nameLength = 0;
    TTE_ASSERT(in->Read(&nameLength, 1), "ISO9660: Could not read directory record bytes");
    
    Bool bReadChildren = false;
    if(r.Fl.Test(DirectoryRecord::DIRECTORY) && nameLength == 1)
    {
        TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read directory record bytes");
        if(Temp[0] == 0x00)
        {
            r.Name = ".";
        }
        else if(Temp[0] == 0x01)
        {
            r.Name = "..";
        }
        else
        {
            TTE_ASSERT(false, "ISO-9660 Directroy recorc has invalid name specifier byte");
            return false;
        }
    }
    else
    {
        bReadChildren = r.Fl.Test(DirectoryRecord::DIRECTORY);
        r.Name = _ReadString<207>(in, nameLength);
        auto pos = r.Name.find_last_of(';');
        String fn = r.Name;
        if(pos != String::npos)
            r.Name = fn.substr(0, pos);
    }
    
    if ((nameLength & 1) == 0) // padding
    {
        TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read directory record padding byte");
    }
    
    r.SULength = (U32)(finalPosition - in->GetPosition());
    
    if (r.SULength > 0)
    {
        TTE_ASSERT(r.SULength <= 220, "ISO9660: Invalid System use area size");
        TTE_ASSERT(in->Read(r.SU, r.SULength), "ISO9660: Could not read System Use Area bytes");
    }
    
    if (finalPosition != in->GetPosition())
    {
        TTE_LOG("ISO-9660: Directory record size invalid");
        return false;
    }
    
    // READ CHILDREN
    if(bReadChildren)
    {
        U64 cachePos = in->GetPosition();
        ADVANCE_LOCATOR(r.LogicalLocator);
        U64 currentPos = in->GetPosition();
        for(;;)
        {
            U64 myPos = in->GetPosition();
            if(myPos >= currentPos + r.DataLength)
                break;
            DirectoryRecord rec{};
            if(!_ReadDir(in, rec))
                return false;
            r.Children.push_back(std::move(rec));
        }
        in->SetPosition(cachePos);
    }
    
    return true;
}

Bool ISO9660::PrimaryVolumeDesc::SerialisePrimaryIn(DataStreamRef& in)
{
    U8 Temp[256];

    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read bytes"); // expect version 1
    if(Temp[0] != 0x01)
    {
        TTE_LOG("Could not open ISO image: unexpected version for use (version != 1)");
        return false;
    }
    
    ADVANCE(1); // skip unused byte
    
    _SystemID = _ReadString<32>(in);
    _VolumeID = _ReadString<32>(in);
    
    ADVANCE(8); // skip unused bytes
    
    // VOLUME SPACE SET (NUMBER OF LOGICAL BLOCKS) - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_NumLogicalBlocks, 4), "ISO9660: Could not read bytes");
    ADVANCE(4); // skip big endian form
    
    ADVANCE(32); // skip unused bytes
    
    // VOLUME SET SIZE - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_NumISOs, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    
    // VOLUME INDEX - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_ISOIndex, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    TTE_ASSERT(_ISOIndex <= _NumISOs, "ISO9660: corrupt when reading [0]");
    
    // LOGICAL BLOCK SIZE - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_LogicalBlockSize, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    
    if(_LogicalBlockSize != 0x800)
    {
        TTE_LOG("Logical block size is not 2048. Not supported");
        return false;
    }
    
    TTE_ASSERT(in->Read((U8*)&_PathTableSize, 4), "ISO9660: Could not read bytes");
    ADVANCE(4); // skip big endian form

    TTE_ASSERT(in->Read((U8*)&_LPathTableLocator, 4), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read((U8*)&_LPathTableAdditionalLocator, 4), "ISO9660: Could not read bytes"); // little endian, no big one stored
    
    TTE_ASSERT(in->Read((U8*)&_MPathTableLocator, 4), "ISO9660: Could not read bytes");
    _MPathTableLocator = FLIP_ENDIAN(_MPathTableLocator);
    
    TTE_ASSERT(in->Read((U8*)&_MPathTableAdditionalLocator, 4), "ISO9660: Could not read bytes"); // big endian for M
    _MPathTableAdditionalLocator = FLIP_ENDIAN(_MPathTableAdditionalLocator);
    
    if(!_ReadDir(in, _Root))
        return false;
    
    _ParentName = _ReadString<128>(in);
    _Publisher = _ReadString<128>(in);
    _DataPreparer = _ReadString<128>(in);
    _ApplicationID = _ReadString<128>(in);
    _CopyrightFileName = _ReadString<37>(in);
    _AbstractFileName = _ReadString<37>(in);
    _BibliographicFileName = _ReadString<37>(in);
    
    TTE_ASSERT(_Creation.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Modification.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Expiration.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Effective.SerialiseIn(in), "ISO9660: Timestamp read fail");
    
    TTE_ASSERT(in->Read((U8*)&_FileStructureVersion, 1), "ISO9660: Could not read bytes");
    
    return true;
}

Bool ISO9660::BootVolumeDesc::SerialiseIn(DataStreamRef &in)
{
    
    U8 Ver{};
    TTE_ASSERT(in->Read(&Ver, 1), "ISO9660: Invalid file data stream or could not read bytes");
    
    if(Ver != 1)
    {
        TTE_LOG("ISO-9660: Boot volume descriptor version was not 1");
        return false;
    }
    
    _SystemID = _ReadString<32>(in);
    _ID = _ReadString<32>(in);
    
    return true;
}

Bool ISO9660::SupplementaryVolumeDesc::SerialiseIn(DataStreamRef &in)
{
    U8 Temp[256]{};
    
    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read bytes in supplementary volume descriptor");
    if(Temp[0] == 0x01)
        _Type = Type::SUPPLEMENTARY;
    else if(Temp[0] == 0x02)
        _Type = Type::ENHANCED;
    else
    {
        TTE_LOG("Unknown supplementary volume type");
        return false;
    }
    
    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read bytes in supplementary volume descriptor");
    if(Temp[0] & 1)
        _Flags.Add(SFlags::ESCAPE_EXT);
    
    TTE_ASSERT(in->Read(_SystemID, 32), "ISO9660: Could not read bytes in supplementary volume descriptor");
    TTE_ASSERT(in->Read(_VolumeID, 32), "ISO9660: Could not read bytes in supplementary volume descriptor");
    
    ADVANCE(8); // unused
    
    TTE_ASSERT(in->Read((U8*)&_NumLogicalBlocks, 4), "ISO9660: Could not read bytes in supplementary volume descriptor");
    ADVANCE(4); // skip big endian
    
    for(U32 i = 0; i < 3; i++)
    {
        Temp[3] = 0;
        TTE_ASSERT(in->Read(Temp, 3), "ISO9660: Could not read bytes in supplementary volume descriptor");
        if(!memcmp(Temp, "%/@", 3))
            _Encodings.Add(Encoding::JOLIET);
        else if(!memcmp(Temp, "%/E", 3))
            _Encodings.Add(Encoding::ISO_10646);
        else if(!memcmp(Temp, "%/C", 3))
            _Encodings.Add(Encoding::ISO_8859_1);
        else if(memcmp(Temp, "\x00\x00\x00", 3))
        {
            TTE_LOG("Unknown character set escape sequence encoding: %s", Temp);
        }
    }
    
    ADVANCE(24); // empty
    
    // VOLUME SET SIZE - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_NumISOs, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    
    // VOLUME INDEX - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_ISOIndex, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    TTE_ASSERT(_ISOIndex <= _NumISOs, "ISO9660: corrupt when reading [0]");
    
    // LOGICAL BLOCK SIZE - SEE 8.2.4 OF ECMA STANDARD
    TTE_ASSERT(in->Read((U8*)&_LogicalBlockSize, 2), "ISO9660: Could not read bytes");
    ADVANCE(2); // skip big endian form
    
    TTE_ASSERT(in->Read((U8*)&_PathTableSize, 4), "ISO9660: Could not read bytes");
    ADVANCE(4); // skip big endian form
    
    TTE_ASSERT(in->Read((U8*)&_LPathTableLocator, 4), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read((U8*)&_LPathTableAdditionalLocator, 4), "ISO9660: Could not read bytes"); // little endian, no big one stored
    
    TTE_ASSERT(in->Read((U8*)&_MPathTableLocator, 4), "ISO9660: Could not read bytes");
    _MPathTableLocator = FLIP_ENDIAN(_MPathTableLocator);
    
    TTE_ASSERT(in->Read((U8*)&_MPathTableAdditionalLocator, 4), "ISO9660: Could not read bytes"); // big endian for M
    _MPathTableAdditionalLocator = FLIP_ENDIAN(_MPathTableAdditionalLocator);
    
    if(!_ReadDir(in, _Root))
        return false;
    
    TTE_ASSERT(in->Read(_ParentName, 128), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read(_Publisher, 128), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read(_DataPreparer, 128), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read(_ApplicationID, 128), "ISO9660: Could not read bytes");
    
    TTE_ASSERT(in->Read(_CopyrightFileName, 37), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read(_AbstractFileName, 37), "ISO9660: Could not read bytes");
    TTE_ASSERT(in->Read(_BibliographicFileName, 37), "ISO9660: Could not read bytes");
    
    TTE_ASSERT(_Creation.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Modification.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Expiration.SerialiseIn(in), "ISO9660: Timestamp read fail");
    TTE_ASSERT(_Effective.SerialiseIn(in), "ISO9660: Timestamp read fail");
   
    TTE_ASSERT(in->Read((U8*)&_FileStructureVersion, 1), "ISO9660: Could not read bytes");
    
    return true;
}

Bool ISO9660::PartitionVolumeDesc::SerialiseIn(DataStreamRef &in)
{
    U8 Temp[32]{};
    
    TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read bytes"); // expect version 1
    if(Temp[0] != 0x01)
    {
        TTE_LOG("Could not open ISO image: unexpected version for use (version != 1)");
        return false;
    }
    
    ADVANCE(8); // unused
    
    _SystemID = _ReadString<32>(in);
    _PartitionID = _ReadString<32>(in);
    
    TTE_ASSERT(in->Read((U8*)&_LogicalLocator, 4), "ISO9660: Could not read partition descriptor bytes");
    ADVANCE(4); // big endian
    
    TTE_ASSERT(in->Read((U8*)&_PartitionSize, 4), "ISO9660: Could not read partition descriptor bytes");
    ADVANCE(4); // big endian
    
    return true;
}

Bool ISO9660::SerialiseIn(DataStreamRef &in)
{
    U8 Temp[8]{};
    
    // READ DESCRIPTORS
    U32 descriptorIndex = 0;
    for(;;descriptorIndex++)
    {
        
        ADVANCE_DESC(descriptorIndex);
        
        TTE_ASSERT(in->Read(Temp, 6), "ISO9660: Could not read bytes from ISO file stream");
        if(memcmp(Temp + 1, "CD001", 5))
        {
            TTE_LOG("Could not open ISO image: not ISO-9660 CD-ROM compliant (CD001 ID missing)");
            return false;
        }
        
        if(Temp[0] == 0x00)
        {
            // BOOT
            if(!_BootDescs.emplace_back().SerialiseIn(in))
                return false;
        }
        else if(Temp[0] == 0x01)
        {
            // PRIMARY
            if(!_PrimaryVolume.SerialisePrimaryIn(in))
                return false;
        }
        else if(Temp[0] == 0x02)
        {
            // SUPPL. DESC.
            if(!_SupplementaryDescs.emplace_back().SerialiseIn(in))
                return false;
        }
        else if(Temp[0] == 0x03)
        {
            // PARITION INFO
            if(!_PartitionDescs.emplace_back().SerialiseIn(in))
                return false;
        }
        else if(Temp[0] == 0xFF)
        {
            // LAST DESCRTIPTOR
            TTE_ASSERT(in->Read(Temp, 1), "ISO9660: Could not read end descriptor bytes");
            TTE_ASSERT(Temp[0] == 1, "ISO9660: End description is invalid");
            break;
        }
        else
        {
            TTE_LOG("Invalid ISO-9660 volume descriptor: %d", (U32)Temp[0]);
            return false;
        }
    }
    
    ADVANCE_LOCATOR(_PrimaryVolume._Root.LogicalLocator);
    
    U64 finalPosition = _PrimaryVolume._Root.DataLength + in->GetPosition();
    
    while(in->GetPosition() < finalPosition)
    {
        DirectoryRecord record{};
        if(!_ReadDir(in, record))
            return false;
        _Records.push_back(std::move(record));
    }
    
    _CachedInput = in;
    
    return true;
}

void ISO9660::_GetFilesInternal(std::set<String>& result, const String& currentPath, std::vector<DirectoryRecord>& recs)
{
    for(auto& record: recs)
    {
        if(record.Fl.Test(DirectoryRecord::DIRECTORY))
        {
            String path = currentPath + record.Name + "/";
            _GetFilesInternal(result, path, record.Children);
            continue;;
        }
        result.insert(currentPath + record.Name);
    }
}

void ISO9660::GetFiles(std::set<String> &fileList)
{
    String rootPath = "";
    _GetFilesInternal(fileList, rootPath, _Records);
}

DataStreamRef ISO9660::_FindInternal(std::vector<DirectoryRecord>& recs, const Symbol& name, const String& cpath, String& o)
{
    for(auto& rec: recs)
    {
        Symbol recName = Symbol(rec.Name);
        Symbol recPath = Symbol(cpath + rec.Name);
        if(!rec.Fl.Test(DirectoryRecord::DIRECTORY) && (recName == name || recPath == name))
        {
            o = rec.Name;
            return DataStreamManager::GetInstance()->CreateSubStream(_CachedInput, 0x800 * rec.LogicalLocator, rec.DataLength);
        }
        DataStreamRef ref = _FindInternal(rec.Children, name, cpath + rec.Name + "/", o);
        if(ref)
            return ref;
    }
    return {};
}

DataStreamRef ISO9660::Find(const Symbol &resourceName, String &outResource)
{
    if(!_CachedInput)
        return {};
    auto& records = _Records;
    return _FindInternal(_Records, resourceName, "", outResource);
}
