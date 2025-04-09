-- Register platform input mappings

function RegisterPlatformMappings(platform)
    -- For now lets just keep it PC
    local mappings = {}

    mappings["Left Click"] = "Mouse Left Button;Abstract Confirm"
    mappings["Right Click"] = "Mouse Right Button;Abstract Cancel"

    return mappings
end