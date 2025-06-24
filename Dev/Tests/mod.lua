if TTE_GetPlatform() == "Windows" then
    TTE_MountSystem("<Data>/", "D:/Games/Bone - Out from Boneville/26_06_2007/Pack/data/", true) -- set input files
    TTE_MountSystem("<Vers>/", "c:/Users/lucas/Desktop/extract/Vers/", true)
    TTE_MountSystem("<Extract>/", "c:/Users/lucas/Desktop/extract/", true)
else
    TTE_MountSystem("<Data>/", "/Users/lucassaragosa/Desktop/Game/Boneville/StreamedData_Mac/", true) -- set input files
    TTE_MountSystem("<Vers>/", "/users/lucassaragosa/desktop/versprobe/", true)
    TTE_MountSystem("<Extract>/", "/users/lucassaragosa/desktop/versprobe/extract/", true)
end

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
    TTE_Log("Probing version file info")
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

local function Extract()
    local files = ResourceGetNames("!*.anm;!*.vox")
    for _,file in ipairs(files) do
        local locator = ResourceResolveAddressToConcreteLocationID(ResourceGetURL(file))
        if string.match(locator, "ttarch") then
            TTE_Log("- extract " .. file)
            FileCopy(ResourceGetURL(file), "<Extract>/" .. file)
        else
            TTE_Log("- skip external " .. file)
        end
    end
end

local function Test()
    local files = ResourceGetNames("*.d3dmesh")
    for _,file in ipairs(files) do
        TTE_Log("- " .. file)
        -- FileCopy(ResourceGetURL(file), "logical://<Extract>/" .. file)
        local ms = TTE_OpenMetaStream(ResourceGetURL(file))
    end
end

Test()