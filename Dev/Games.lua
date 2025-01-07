-- This script is used by the modding runtime to register all games and information about them.

local texasHoldem = {}
texasHoldem.Name = "Telltale Texax Hold'em"
texasHoldem.ID = "TX100" -- Texas S1 (100)
texasHoldem.DefaultMetaVersion = "MBIN" -- Version of meta streams which files created will be
texasHoldem.LuaVersion = "5.0.2" -- uses lua 5.0.2
texasHoldem.IsArchive2 = false
texasHoldem.ArchiveVersion = 0 -- ttarch version 0
MetaRegisterGame(texasHoldem) -- does not have any encryption, so no encryption keys

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
MetaRegisterGame(bone1)