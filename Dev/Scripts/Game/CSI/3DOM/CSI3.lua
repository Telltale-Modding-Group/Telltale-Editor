-- CSI: 3 Dimensions of Murder. This game is special in the sense that the PS2 version is vastly different. It uses a one off stream type and
-- archives are actually meta streams and not usual TTArchives. PC version as normal. PS2 version was released a year later, probably why

require("ToolLibrary/Game/CSI/3DOM/TTArchive.lua")
require("ToolLibrary/Game/VersionCRC.lua")

function CSI3_GetGameDescriptor()
	local CSI3 = {} -- No encryption
	CSI3.Name = "CSI: 3 Dimensions of Murder"
	CSI3.ID = "CSI3"
	CSI3.DefaultMetaVersion =
	"MBIN"                       -- Although, the PS2 uses a special one of version BMS3 (and encrypted EMS3, not used), MBIN is supported.
	CSI3.LuaVersion = "5.0.2"
	CSI3.IsArchive2 = false
	CSI3.ArchiveVersion = 0 -- still archive version 0
	CSI3.Platforms = "PC;PS2"
	CSI3.Vendors = ""
	MetaPushGameCapability(CSI3, kGameCapRawClassNames) -- CHECK OTHER CAPS
	MetaRegisterGame(CSI3)
	return CSI3
end

function RegisterCSI3Collection(containerInterfaceTbl, name, fl, k, v)
	local withBaseclass = { VersionIndex = 0 }
	withBaseclass.Name = name
	withBaseclass.Flags = fl
	withBaseclass.Members = {}
	withBaseclass.Members[1] = { Name = "Baseclass_ContainerInterface", Class = containerInterfaceTbl, Flags =
	kMetaMemberMemoryDisable + kMetaMemberBaseClass }
	withBaseclass.Members[2] = { Name = "mSize", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	withBaseclass.Members[3] = { Name = "mCapacity", Class = kMetaInt, Flags = kMetaMemberMemoryDisable }
	MetaRegisterCollection(withBaseclass, k, v)
	return withBaseclass
end

function RegisterCSI3(venderUnused, platform)
	MetaSetVersionFn("VersionCRC_V0")
	MetaRegisterIntrinsics()

	local MetaCI = { VersionIndex = 0 } -- baseclass for containers
	MetaCI.Name = "ContainerInterface"
	MetaCI.Flags = kMetaClassAbstract
	MetaRegisterClass(MetaCI)

	RegisterCSI3TTArchive(MetaCI) -- register .PK2 archive. Normal .ttarch handled as usual

	return true
end
