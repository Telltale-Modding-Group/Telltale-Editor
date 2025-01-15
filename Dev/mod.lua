-- TESTING DEVELOPMENT MOD SCRIPT. USERS CAN PROVIDE THEIR OWN, THIS ONE IS FOR TESTING.

TTE_Switch("BN100", "MacOS", "")

function ends_with(str, suffix)
    return str:sub(-#suffix) == suffix
end

archive = TTE_OpenTTArchive("/Users/lucassaragosa/Desktop/BN100.ttarch") -- YES I KNOW. the file is 200mb lol

for i, file in pairs(TTE_ArchiveListFiles(archive)) do
    if ends_with(file, ".d3dtx") then
        TTE_Log("Loading " .. file)
        TTE_OpenMetaStream(file, archive) -- open to test.
    end
end