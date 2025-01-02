require("ToolLibrary/Game/BN100/LuaContainer.lua")

-- C++ code temp below for calculating version crc in later games.
--[=====[           // endian?
            U32 buf = (clz.Flags & CLASS_NON_BLOCKED) == 0 ? 0xFFFF'FFFFu : 0u;
            U32 crc = CRC32((U8*)&buf, 4, 0);
            for(auto& member: clz.Members)
            {
                if((member.Flags & MEMBER_SKIP) != 0)
                    continue; // skip
                crc = CRC32((U8*)member.Name.c_str(), (U32)member.Name.size(), crc);
                crc = CRC32((U8*)&Classes[member.ClassID].TypeHash, 8, crc);
                U8 blocked = (Classes[member.ClassID].Flags & CLASS_NON_BLOCKED) == 0 ? 1 : 0;
                crc = CRC32(&blocked, 1, crc);
            }
            return crc;
--]=====]

-- Calculate the version CRC of a given class in Bone: Out from Boneville. In the macOS executable for Application1, see function at 0xDE28E
function VersionCRCBone100(classTable)
	temp = 0
	if not MetaFlagQuery(classTable.Flags, kMetaClassNonBlocked) then
		temp = tonumber("0xFFFF")
	end
	hash = MetaHashInt(0, temp)

	if type(classTable.Members) == "table" then -- if we have members iterate through them
		for _,member in pairs(classTable.Members) do
			if not MetaFlagQuery(member.Flags, kMetaMemberSerialiseDisable) then
				hash = MetaHashString(hash, member.Name)
				hash = MetaHashString(hash, member.Class.Name)
			end
		end
	end

	return hash
end

function RegisterBone100(vendor, platform)

	MetaSetVersionFn("VersionCRCBone100") -- Set version CRC calculation function, before anything else.
	MetaRegisterIntrinsics() -- First, we must register all of the intrinsic types.

	-- TEMPORARY TESTING CODE. WILL BE REPLACED.
	-- I will also note here that type names that have 'class ' 'struct ''enum ' and 'std::' etc in them are removed from the class name
	-- this was realised by telltale when they started using hashed versions, because compilers name things differently
	-- so in this version they actually still use those, right? (... need to check this)

	local col = {}
	col.Name = "SArray<Symbol,3>"
	MetaRegisterCollection(col, 3, kMetaSymbol) -- todo we need to add mSize and mCapacity and Baseclass_ContainerInterface along with ContainerInterface type.

	local MetaVec2 = {}
	MetaVec2.Name = "Vector2"
	MetaVec2.Flags = kMetaClassNonBlocked
	MetaVec2.Members = {}
	MetaVec2.Members[1] = { Name = "x", Class = kMetaFloat }
	MetaVec2.Members[2] = { Name = "y", Class = kMetaFloat }
	MetaRegisterClass(MetaVec2)

	local MetaFlags = {} -- CHECKED. version CRCs match
	MetaFlags.Name = "Flags"
	MetaFlags.Flags = kMetaClassNonBlocked
	MetaFlags.Members = {}
	MetaFlags.Members[1] = { Name = "mFlags", Class = kMetaInt }
	MetaRegisterClass(MetaFlags)

	local MetaCITest = {} -- baseclass for all containers in most games (when writing these scripts, we wil havea funtion which sets this for each)
	MetaCITest.Name = "class ContainerInterface"
	MetaCITest.Flags = kMetaClassAbstract
	MetaRegisterClass(MetaCITest)

	local MetaComplexTest = {} -- DCArray_String_(1j6j2xe).vers. part in brackets is base64 of version crc. needs to be 0xC6E09792. not getting that...
	MetaComplexTest.Name = "DCArray<String>"
	MetaComplexTest.Members = {}
	MetaComplexTest.Members[1] = { Name = "Baseclass_ContainerInterface", Class = MetaCITest, Flags = kMetaMemberGhost + kMetaMemberBaseClass }
	MetaComplexTest.Members[2] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberGhost }
	MetaComplexTest.Members[3] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberGhost }
	MetaRegisterCollection(MetaComplexTest, nil, kMetaString)

	-- after above is fixed, do we need to rename or give option to add class before String so class String? because version crc depends on that type name.
	-- of its members, not itself though...
	

	print(string.format("version crc is %x", MetaGetVersionCRC(MetaComplexTest)))

	return true
end