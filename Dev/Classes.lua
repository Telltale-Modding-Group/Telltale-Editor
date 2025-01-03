-- This is the bulk of the library. This registers all the meta classes.

function RegisterAll(game_id, platform, vendor)
    
    if game_id == "BN100" then
        require("ToolLibrary/Game/BN100/BN100.lua")
        return RegisterBone100(vendor, platform)
    else
        print("ERROR: during testing only boneville is supported")
    end
    return false
    
end