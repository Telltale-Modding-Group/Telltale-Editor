#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <vector>

// Meta system is all inside this namespace. This is a reflection system initialised by the lua scripts.
namespace Meta {
    
    enum MetaMemberFlag {
        MEMBER_BLOCKED = 0, // this member has a block size around it
    };
    
    enum MetaClassFlag {
        CLASS_INTRINSIC = 0, // this class is an intrinsic type so is not added to meta class header
    };
    
    // A type member
    struct Member {
        
        String Name;
        U32 Offset;
        U32 Flags;
        
    };
    
    // A type class
    struct Class {
        
        String Name;
        U32 Flags;
        U32 Size;
        std::vector<Member> Members;
        U64 TypeHash;
        U32 VersionCRC;
        
    };
    
    void Initialise(ToolContext& context);
    
    void Shutdown(ToolContext& context);
    
}
