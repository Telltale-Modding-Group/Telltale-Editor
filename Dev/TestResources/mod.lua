TTE_MountSystem("<Data>/", "/Users/lucassaragosa/Desktop/extract/texas/", true) -- set input files
TTE_MountSystem("<Vers>/", "/Users/lucassaragosa/Desktop/extract/Vers/", true)

-- open and copy vers into vers folder
local function SetupVersFiles()
    local resources = ResourceGetNames("*.vers")
    for _,res in pairs(resources) do
        if FileExists("<Data>/" .. res) then
            FileCopy("<Data>/" .. res, "<Vers>/" .. res)
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