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
-- Set disableMemberBlocking to true (else can ignore it) if you want all members which may be blocked to be non-blocked by force.
function NewProxyClass(name, memberName, underlyingClassTable, disableMemberBlocking)
    local clazz = {}
    clazz.VersionIndex = 0
    clazz.Name = name
    clazz.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic-- ensure no extra block size and not in headers
    if disableMemberBlocking then
        clazz.Flags = clazz.Flags + kMetaClassProxy
    end
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
    elseif game_id == "CSI3" then
        require("ToolLibrary/Game/CSI/3DOM/CSI3.lua")
        return RegisterCSI3(vendor, platform)
    else
        print("ERROR: This game is not currently supported")
    end

    return false
end