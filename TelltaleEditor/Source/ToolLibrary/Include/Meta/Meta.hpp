#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Resource/DataStream.hpp>
#include <vector>

// Meta system is all inside this namespace. This is a reflection system initialised by the lua scripts.
namespace Meta {
    
    enum MetaMemberFlag {
        MEMBER_BLOCKED = 0, // this member has a block size around it
        MEMBER_ENUM = 1, // this member is an enum
        MEMBER_FLAG = 2, // this member is a flags bitfield
    };
    
    enum MetaClassFlag {
        CLASS_INTRINSIC = 0, // this class is an intrinsic type so is not added to meta class header
    };
    
    // A type member
    struct Member {
        
        String Name;
        U32 Offset;
        U32 Flags;
        I32 ClassIndex; // member type
        
    };
    
    // A type class
    struct Class {
        
        String Name;
        U32 Flags;
        U32 Size;
        U64 TypeHash;
        U32 VersionCRC;
        
        U32 RTSize; // runtime internal size of the class
        
        std::vector<Member> Members;
        
    };
    
    struct ClassInstanceMemory {
        I32 ClassIndex;
        // Rest of the class is the instance members
    };
    
    using ClassInstance = std::shared_ptr<ClassInstanceMemory>;
    
    // Sections of binary stream
    enum StreamSection {
        STREAM_SECTION_MAIN = 0,
        STREAM_SECTION_ASYNC = 1,
        STREAM_SECTION_DEBUG = 2,
        STREAM_SECTION_COUNT = 3,
    };
    
    // Binary stream versions
    enum StreamVersion {
        MBIN = 0,
        MBES = 1,
        MTRE = 2,
        MCOM = 3,
        MSV4 = 4,
        MSV5 = 5,
        MSV6 = 6,
    };
    
    struct Stream {
        
        struct Section {
            DataStreamRef Data; // Section binary data stream
            Bool Compressed; // If section is compressed
        };
        
        struct VersionInfo {
            I32 ClassIndex;
            U32 VersionCRC;
        };
        
        StreamVersion Version;
        Section Sect[STREAM_SECTION_COUNT];
        std::vector<VersionInfo> VersionInf;
        
    };
    
    // INTERNAL USE FOR TOOL CONTEXT ONLY
    
    void Initialise();
    void Shutdown();
    
    void InitGame();
    void RelGame();
    
    // ==================================
    
    // Writes a meta stream file to the output stream, from the instance.
    Bool WriteMetaStream(ClassInstance instance, DataStreamRef stream);
    
    // Reads a meta stream file from the input stream, to the instance.
    Bool ReadWriteStream(ClassInstance instance, DataStreamRef stream);
    
    // Creates an instance of the given class
    ClassInstance CreateInstance(I32 ClassIndex);
    
    // Creates an exact copy of the given instance
    ClassInstance CopyInstance(ClassInstance instance);
    
}
