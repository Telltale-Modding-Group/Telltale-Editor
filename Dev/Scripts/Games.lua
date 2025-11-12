-- This script is used by the modding runtime to register all games and information about them.

require("ToolLibrary/Game/TX100/TX100.lua")
require("ToolLibrary/Game/BN100/BN100.lua")
require("ToolLibrary/Game/CSI/3DOM/CSI3.lua")

MetaRegisterGame(TX100_GetGameDescriptor())
MetaRegisterGame(Bone1_GetGameDescriptor())
MetaRegisterGame(CSI3_GetGameDescriptor())

-- test for ttarch2. DONT USE yet just for testing and compat.
--[[
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
]]--