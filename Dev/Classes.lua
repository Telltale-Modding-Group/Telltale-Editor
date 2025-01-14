-- This is the bulk of the library. This registers all the meta classes.

-- Helper function to create a new class table
function NewClass(name, index)
    local clazz = {} -- initialise defaults
    clazz.VersionIndex = index
    clazz.Name = name
    clazz.Members = {}
    clazz.Flags = 0
    return clazz 
end

-- Helper function to create a new member table
function NewMember(name, classTable, flags)
    local member = {}
    member.Name = name
    member.Class = classTable
    member.Flags = flags
    return member
end

-- Returns new class (registers) table for which the underlying argument is the class in memory/file but it now can go under this name
function NewProxyClass(name, memberName, underlyingClassTable)
    local clazz = {}
    clazz.VersionIndex = 0
    clazz.Name = name
    clazz.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- ensure no extra block size and not in headers
    clazz.Members = {}
    clazz.Members[1] = NewMember(memberName, underlyingClassTable, kMetaMemberVersionDisable) -- the actual member
    MetaRegisterClass(clazz)
    return clazz
end

-- Adds an enum descriptor for a given member in a class
function AddEnum(classTable, memberIndex, name, value)
    if classTable.Members[memberIndex].EnumInfo == nil then
        classTable.Members[memberIndex].EnumInfo = {}
    end
    table.insert(classTable.Members[memberIndex].EnumInfo, { Name = name, Value = value })
end

-- Adds a flag descriptor for a given member in a class
function AddFlag(classTable, memberIndex, name, value)
    if classTable.Members[memberIndex].FlagInfo == nil then
        classTable.Members[memberIndex].FlagInfo = {}
    end
    table.insert(classTable.Members[memberIndex].FlagInfo, { Name = name, Value = value })
end

function RegisterAll(game_id, platform, vendor)
    
    if game_id == "BN100" then
        require("ToolLibrary/Game/BN100/BN100.lua")
        return RegisterBone100(vendor, platform)
    else
        print("ERROR: during testing only boneville is supported")
    end

    return false
end