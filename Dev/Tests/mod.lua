TTE_MountSystem("<Data>/", "D:/Games/Bone - Out from Boneville/26_06_2007/Pack/data/", true) -- set input files
TTE_MountSystem("<Vers>/", "c:/Users/lucas/Desktop/extract/Vers/", true)

-- open and copy vers into vers folder
local function SetupVersFiles()
    TTE_Log("Gathering version file info")
    local resources = ResourceGetNames("*.vers")
    for _,res in pairs(resources) do
        local url = ResourceGetURL(res)
        TTE_Log("- " .. url)
        if FileExists(url) then
            FileCopy(url, "<Vers>/" .. ResourceAddressGetResourceName(url))
        end
    end
end

-- open and copy vers into vers folder
local function ProbeVers()
    local classes = MetaGetClasses()
    for _,clazz in pairs(classes) do
        local versName = MetaGetSerialisedVersionInfoFileName(clazz, 0)
        versName = string.gsub(versName, "%(", "")
        versName = string.gsub(versName, "%)", "")
        if FileExists("<Vers>/" .. versName) then
            FileDelete("<Vers>/" .. versName)
            print(versName .. " successfully matched")
        end
    end
end

local function Test()
    local prop = Load("module_text.prop")
    local props = PropertyKeys(prop)
    for _,p in pairs(props) do
        TTE_Log(p .. " " .. PropertyGetKeyType(prop, p))
    end
    Save("logical://<Vers>/module_text.prop")
end

ProbeVers()
