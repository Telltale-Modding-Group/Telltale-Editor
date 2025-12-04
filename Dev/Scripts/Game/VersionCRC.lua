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

-- Calculate the version CRC of a given class in Bone: Out from Boneville and more. 
-- In the macOS executable for Application1, see function at 0xDE28E.
-- This has a really annoying bug with *base classes* which are declared with pointers!
function VersionCRC_V0_PointerFix(classTable)
	local temp = 0
	if not MetaFlagQuery(classTable.Flags, kMetaClassNonBlocked) then
		temp = tonumber("0xFFFF")
	end
	local hash = MetaHashInt(0, temp)
	if type(classTable.Members) == "table" then 
		for _,member in pairs(classTable.Members) do
			if not MetaFlagQuery(member.Flags, kMetaMemberSerialiseDisable) and not MetaFlagQuery(member.Flags, kMetaMemberVersionDisable) then
				hash = MetaHashString(hash, member.Name)
				if member.Class.Name == "class DialogBase" then -- I know. Bone fix. Stupid pointers used in meta member types
					hash = MetaHashString(hash, "class DialogBase *")
				elseif member.Class.Name == "class AnimationValueInterfaceBase" then
					hash = MetaHashString(hash, "class AnimationValueInterfaceBase *")
				elseif member.Class.Name == "class AnimatedValueInterface<class Quaternion>" then
					hash = MetaHashString(hash, "class AnimatedValueInterface<class Quaternion> *")
				elseif member.Name == "Baseclass_PropertySet" then
					hash = MetaHashString(hash, "class PropertySet *")
				else
					hash = MetaHashString(hash, member.Class.Name)
				end
			end
		end
	end

	return hash
end

-- Like above but without funny bug fix
function VersionCRC_V0(classTable)
	local temp = 0
	if not MetaFlagQuery(classTable.Flags, kMetaClassNonBlocked) then
		temp = tonumber("0xFFFF")
	end
	local hash = MetaHashInt(0, temp)

	if type(classTable.Members) == "table" then 
		for _,member in pairs(classTable.Members) do
			if not MetaFlagQuery(member.Flags, kMetaMemberSerialiseDisable) and not MetaFlagQuery(member.Flags, kMetaMemberVersionDisable) then
				hash = MetaHashString(hash, member.Name)
				hash = MetaHashString(hash, member.Class.Name)
			end
		end
	end

	return hash
end