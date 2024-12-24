-- This script is the main bulk of this library. It registers all meta classes (MetaClassDescription), ie the formats.

-- Called by library. ID is the game ID (see wiki), platform is the platform which the game files are on (see wiki)
-- vendor is for differing releases.
function RegisterAll(id, platform, vendor)

	if id == "TX100" then -- oldest, texas holdem.
		require("ToolLibrary/Game/TX100.lua")
		return RegisterTX100(vendor) -- only released on windows
	end

	return false
end