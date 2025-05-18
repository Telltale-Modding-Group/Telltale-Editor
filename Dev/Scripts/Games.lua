-- This script is used by the modding runtime to register all games and information about them.

--[[
local texasHoldem = {}
texasHoldem.Name = "Telltale Texax Hold'em"
texasHoldem.ID = "TX100" -- Texas S1 (100)
texasHoldem.DefaultMetaVersion = "MBIN" -- Version of meta streams which files created will be
texasHoldem.LuaVersion = "5.0.2" -- uses lua 5.0.2
texasHoldem.IsArchive2 = false
texasHoldem.ArchiveVersion = 0 -- ttarch version 0
MetaRegisterGame(texasHoldem) -- does not have any encryption, so no encryption keys
]]

local bone1 = { Key = {} }
bone1.Name = "Bone: Out from Boneville"
bone1.ID = "BN100" -- Bone Book 1 ('Out from Boneville') (100)
bone1.DefaultMetaVersion = "MBIN"
bone1.LuaVersion = "5.0.2"
bone1.IsArchive2 = false
bone1.ArchiveVersion = 0 -- ttarch version 0
-- Bone1 is released on mac and windows only
bone1.Key["MacOS"] = "34246C3343726C7564326553576945324F6163396C7574786C3732522D2A384931714F346F616A6C5F24652369616370342A75466C6530"
bone1.Key["PC"]    = "81D89B9955E26573B4DBE3C963DB8587AB999BDC6EEB689FA790DDBA6AE29364A1B4A0B492D96B9CB7E3E6D168A8849F87D29498A1E871"
bone1.Platforms = "PC;MacOS"
bone1.Vendors = ""
MetaPushGameCapability(bone1, kGameCapSeparateAnimationTransform)
MetaPushGameCapability(bone1, kGameCapUsesLenc)
MetaPushGameCapability(bone1, kGameCapRawClassNames)
MetaRegisterGame(bone1)

local CSI3 = { } -- No encryption
CSI3.Name = "CSI: 3 Dimensions of Murder"
CSI3.ID = "CSI3"
CSI3.DefaultMetaVersion = "MBIN" -- Although, the PS2 uses a special one of version BMS3 (and encrypted EMS3, not used), MBIN is supported.
CSI3.LuaVersion = "5.0.2"
CSI3.IsArchive2 = false
CSI3.ArchiveVersion = 0 -- still archive version 0
CSI3.Platforms = "PC;PS2"
CSI3.Vendors = ""
MetaPushGameCapability(CSI3, kGameCapRawClassNames) -- CHECK OTHER CAPS
MetaRegisterGame(CSI3)

-- test for ttarch2. DONT USE yet just for testing and compat.
local mcsm = { }
mcsm.Name = "Minecraft: Story Mode"
mcsm.ID = "MC100" -- Minecraft Story Mode S1 (100)
mcsm.ResourceSetMask = "_resdesc_*.lua"
mcsm.DefaultMetaVersion = "MSV5"
mcsm.LuaVersion = "5.2.3"
mcsm.IsArchive2 = true
mcsm.ArchiveVersion = 4 -- ttarch version 4
mcsm.ModifiedEncryption = true -- uses modified blowfish
mcsm.Key = "8CD29B9987DE94A9E69DA5947FCEC1BCCC9C8EDDB1A6669FB28ADDBA9CDEC29AD376627FAECCA7D1D8E6D9D2AB63829F92CC9498D3E4A0"
mcsm.Platforms = "PC" -- todo more
mcsm.Vendors = ""
MetaRegisterGame(mcsm)