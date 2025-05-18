TTE_MountSystem("<Data>/", "/Users/lucassaragosa/Desktop/extract/Bone/", true) -- set input files
 
local navCam = Load("logical://<Data>/Properties/Preferences/prefs_tracker.prop") -- load a prop

function OnSomeValueUpdated(key, newValue)
    print("Hey im updated! " .. tostring(key) .. tostring(ContainerGetNumElements(newValue)))
end

PropertyAddKeyCallback(navCam, "Prop - Some Value", OnSomeValueUpdated)
PropertyRemoveKeyCallback(navCam, "Prop - Some Value", "OnSomeValueUpdated")

PropertyCreate(navCam, "Prop - Some Value", "DCArray<String>") -- add prop
ContainerInsertElement(PropertyGet(navCam, "Prop - Some Value"), "aabi is LAPTOP") -- set value

local keys = PropertyKeys(navCam)

for k,v in ipairs(keys) do
    local value = PropertyGet(navCam, v)
    local ptype = PropertyGetKeyType(navCam, v)
    if type(value) == "table" then
        print(v .. " [" .. ptype .. "] => ", TableToString(value))
    elseif PropertyIsContainer(navCam, v) then
        print(v .. " [" .. ptype .. "] => ")
        local index = ContainerGetNumElements(value) - 1
        while(index >= 0) do
            print(v .. " " .. tostring(index) .. " " .. ContainerGetElement(value, index))
            index = index - 1
        end
    else
        print(v .. " [" .. ptype .. "] => " .. tostring(value))
    end
end

Save("logical://<Data>/prefs_tracker.prop") -- modified