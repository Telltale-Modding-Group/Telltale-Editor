#pragma once

#include <Core/BitSet.hpp>
#include <Resource/DataStream.hpp>

#include <vector>
#include <set>

// .ISO FILES. INTERNATIONAL STANDARD FORMAT USED FOR READING CONSOLE ROMS OF GAME DATA AND EXECUTABLES. ECMA-119
class ISO9660 {
public:
    
    static constexpr CString Extension = ".iso";
    
    Bool SerialiseIn(DataStreamRef& in);
    
    void GetFiles(std::set<String>& result); // get file names
    
    DataStreamRef Find(const Symbol& resourceName, String& outResource);
    
    inline void Reset()
    {
        _SupplementaryDescs.clear();
        _BootDescs.clear();
        _PrimaryVolume = PrimaryVolumeDesc();
        _PartitionDescs.clear();
        _Records.clear();
        _CachedInput.reset();
    }
    
private:
    
    struct DirectoryRecord
    {
        
        enum Flag
        {
            HIDDEN = 1,
            DIRECTORY = 2,
            ASSOCIATED_FILE = 4, // split up file and this is one of it. symlinks etc.
            RECORD = 8, // structure of extent determined by extended attrib
            PROTECTED = 16, // permissions are specified
            FINAL = 128, // final record for a file
        };
        
        U8 ExtendedAttribRecordLength = 0;
        U32 LogicalLocator = 0;
        U32 DataLength = 0; // excl extended
        
        struct TimeStamp
        {
            U8 YearsSince1900;
            U8 Month; // 1 TO 12
            U8 Day; // 1 to 31
            U8 Hour; // 0 to 23
            U8 Minute; // 0 to 59
            U8 Second; // 0 to 59
            I8 GMTOffset; // offset from GMT which this time is, in units of 15 mins. +1 = 15 min offset. SIGNED!
        } Ts;
        
        Flags Fl; // see flags
        
        U8 InterleavedUnitSize = 0; // in units of logical block size. only if interleaved
        U8 InterleavedUnitGap = 0; // in units of logical block size. gap between each used block.
        
        U16 ISOIndex = 0; // iso index of this file
        
        String Name;
        
        U32 ParentIndex = 0; // parent directory index
        
        U8 SU[220]; // system use
        U32 SULength;
        
        std::vector<DirectoryRecord> Children;
        
    };
    
    struct VolumeTs
    {
        
        String Year, Month, Day, Hour, Minute, Second, Centisecond;
        
        I8 GMTOffset; // see dir record Timestamp
        
        Bool SerialiseIn(DataStreamRef& ref);
        
    };
    
    void _GetFilesInternal(std::set<String>& result, const String& currentPath, std::vector<DirectoryRecord>& recs);
    
    DataStreamRef _FindInternal(std::vector<DirectoryRecord>& recs, const Symbol& name, const String& currentPath, String& o);
    
    static Bool _ReadDir(DataStreamRef& in, DirectoryRecord&);
    
    struct PrimaryVolumeDesc
    {
        
        String _SystemID;
        String _VolumeID;
        
        U32 _NumLogicalBlocks;
        
        U16 _LogicalBlockSize;
        U32 _PathTableSize;
        
        U16 _NumISOs; // number of ISOs in this game disc
        U16 _ISOIndex; // ISO index in the game. 0 to num ISOs - 1
        
        U32 _LPathTableLocator;
        U32 _LPathTableAdditionalLocator;
        U32 _MPathTableLocator;
        U32 _MPathTableAdditionalLocator;
        
        DirectoryRecord _Root; // root directory
        
        String _ParentName; // parent volume set ID
        String _Publisher;
        String _DataPreparer; // person who preped the data (person name)
        String _ApplicationID;
        String _CopyrightFileName;
        String _AbstractFileName;
        String _BibliographicFileName;
        
        VolumeTs _Creation, _Modification, _Expiration, _Effective;
        
        U8 _FileStructureVersion; // should be 1 for ECMA
        
        Bool SerialisePrimaryIn(DataStreamRef& in);
        
    };
    
    struct SupplementaryVolumeDesc
    {
        
        enum Type
        {
            SUPPLEMENTARY = 1,
            ENHANCED = 2,
        };
        
        enum SFlags
        {
            ESCAPE_EXT = 1,
        };
        
        enum Encoding
        {
            STD_ASCII = 1, // standard normal CStr
            JOLIET = 2, // UTF16-LE
            ISO_8859_1 = 4, // LATEN1, 8ASCII
            ISO_10646 = 8, // UTF16 USC2
        };
        
        Type _Type;
        
        Flags _Flags;
        
        U8 _SystemID[32];
        U8 _VolumeID[32]; // both in character set defined
        
        U32 _NumLogicalBlocks;
        
        Flags _Encodings;
        
        U16 _NumISOs; // number of ISOs in this game disc
        U16 _ISOIndex; // ISO index in the game. 0 to num ISOs - 1
        
        U16 _LogicalBlockSize;
        U32 _PathTableSize;
        
        U32 _LPathTableLocator;
        U32 _LPathTableAdditionalLocator;
        U32 _MPathTableLocator;
        U32 _MPathTableAdditionalLocator;
        
        DirectoryRecord _Root; // root directory
        
        U8 _ParentName[128]; // parent volume set ID. these are all in the encoding specified above
        U8 _Publisher[128];
        U8 _DataPreparer[128]; // person who preped the data (person name)
        U8 _ApplicationID[128];
        U8 _CopyrightFileName[37];
        U8 _AbstractFileName[37];
        U8 _BibliographicFileName[37];
        
        VolumeTs _Creation, _Modification, _Expiration, _Effective;
        
        U8 _FileStructureVersion; // should be 1 for ECMA
        
        Bool SerialiseIn(DataStreamRef& in);
        
    };
    
    struct BootVolumeDesc
    {
        
        String _SystemID;
        String _ID;
        U32 _Spec; // specific. likely a logical locator
        
        Bool SerialiseIn(DataStreamRef& in);
        
    };
    
    struct PartitionVolumeDesc
    {
        
        String _SystemID;
        String _PartitionID;
        
        U32 _LogicalLocator;
        U32 _PartitionSize;
        
        Bool SerialiseIn(DataStreamRef& in);
        
    };
    
    PrimaryVolumeDesc _PrimaryVolume;
    
    std::vector<SupplementaryVolumeDesc> _SupplementaryDescs;
    std::vector<BootVolumeDesc> _BootDescs;
    
    std::vector<PartitionVolumeDesc> _PartitionDescs;
    
    std::vector<DirectoryRecord> _Records;
    
    DataStreamRef _CachedInput; // cached input stream from last serialise in for reading files
    
};
