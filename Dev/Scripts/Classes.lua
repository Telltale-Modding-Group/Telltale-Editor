-- This is the bulk of the library. This registers all the meta classes.

require("ToolLibrary/PlatformInputMappings.lua")

require("ToolLibrary/Game/TX100/TX100.lua")
require("ToolLibrary/Game/BN100/BN100.lua")
require("ToolLibrary/Game/CSI/3DOM/CSI3.lua")

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
    clazz.Flags = kMetaClassNonBlocked + kMetaClassIntrinsic -- ensure no extra block size and not in headers
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
        return RegisterBone100(vendor, platform)
    elseif game_id == "CSI3" then
        return RegisterCSI3(vendor, platform)
    elseif game_id == "TX100" then
        return RegisterTX100(vendor)
    else
        print("ERROR: This game is not currently supported or unknown: " .. game_id)
    end

    return false
end

-- Registers intrinsics common to *All* telltale games
function RegisterIntrinsics2()
    local MetaVec3 = { VersionIndex = 0 }
    MetaVec3.Name = "class Vector3"
    MetaVec3.Flags = kMetaClassNonBlocked
    MetaVec3.Members = {}
    MetaVec3.Members[1] = { Name = "x", Class = kMetaFloat }
    MetaVec3.Members[2] = { Name = "y", Class = kMetaFloat }
    MetaVec3.Members[3] = { Name = "z", Class = kMetaFloat }
    MetaRegisterClass(MetaVec3)

    local MetaPolar = { VersionIndex = 0 }
    MetaPolar.Name = "class Polar"
    MetaPolar.Members = {}
    MetaPolar.Members[1] = { Name = "mR", Class = kMetaFloat }
    MetaPolar.Members[2] = { Name = "mTheta", Class = kMetaFloat }
    MetaPolar.Members[3] = { Name = "mPhi", Class = kMetaFloat }
    MetaRegisterClass(MetaPolar)

    local MetaCol = { VersionIndex = 0 }
    MetaCol.Name = "class Color"
    MetaCol.Flags = kMetaClassNonBlocked
    MetaCol.Members = {}
    MetaCol.Members[1] = { Name = "r", Class = kMetaFloat }
    MetaCol.Members[2] = { Name = "g", Class = kMetaFloat }
    MetaCol.Members[3] = { Name = "b", Class = kMetaFloat }
    MetaCol.Members[4] = { Name = "a", Class = kMetaFloat }
    MetaRegisterClass(MetaCol)

    local MetaRect = { VersionIndex = 0 }
    MetaRect.Name = "class Rect"
    MetaRect.Members = {}
    MetaRect.Flags = kMetaClassNonBlocked
    MetaRect.Members[1] = { Name = "left", Class = kMetaInt }
    MetaRect.Members[2] = { Name = "right", Class = kMetaInt }
    MetaRect.Members[3] = { Name = "top", Class = kMetaInt }
    MetaRect.Members[4] = { Name = "bottom", Class = kMetaInt }
    MetaRegisterClass(MetaRect)

    local MetaTRectF = { VersionIndex = 0 }
    MetaTRectF.Name = "class TRect<float>"
    MetaTRectF.Members = {}
    MetaTRectF.Flags = kMetaClassNonBlocked
    MetaTRectF.Members[1] = { Name = "left", Class = kMetaFloat }
    MetaTRectF.Members[2] = { Name = "right", Class = kMetaFloat }
    MetaTRectF.Members[3] = { Name = "top", Class = kMetaFloat }
    MetaTRectF.Members[4] = { Name = "bottom", Class = kMetaFloat }
    MetaRegisterClass(MetaTRectF)

    local MetaQuat = { VersionIndex = 0 }
    MetaQuat.Name = "class Quaternion"
    MetaQuat.Flags = kMetaClassNonBlocked
    MetaQuat.Members = {}
    MetaQuat.Members[1] = { Name = "x", Class = kMetaFloat }
    MetaQuat.Members[2] = { Name = "y", Class = kMetaFloat }
    MetaQuat.Members[3] = { Name = "z", Class = kMetaFloat }
    MetaQuat.Members[4] = { Name = "w", Class = kMetaFloat }
    MetaRegisterClass(MetaQuat)

    local MetaFlags = { VersionIndex = 0 }
    MetaFlags.Name = "class Flags"
    MetaFlags.Flags = kMetaClassNonBlocked
    MetaFlags.Members = {}
    MetaFlags.Members[1] = { Name = "mFlags", Class = kMetaInt }
    MetaRegisterClass(MetaFlags)

    local transform = NewClass("class Transform", 0)
	transform.Members[1] = NewMember("mRot", MetaQuat)
	transform.Members[2] = NewMember("mTrans", MetaVec3)
	MetaRegisterClass(transform)

    return MetaVec3, MetaCol, MetaPolar, MetaRect, MetaTRectF, MetaQuat, MetaFlags, transform
end
