-- TESTING DEVELOPMENT MOD SCRIPT. USERS CAN PROVIDE THEIR OWN, THIS ONE IS FOR TESTING.

TTE_Switch("BN100", "MacOS", "") -- set to boneville macos version for menuitem.prop testing

archive = TTE_OpenTTArchive("../../Dev/data.ttarch") -- pass in the relative path for exe. we need this for testing in the editor.

-- print some files
filenames = TTE_ArchiveListFiles(archive)
for _,v in pairs(filenames) do
    if string.match(v, ".+%.vers$") then
        print("found " .. v)
    end
end

prop = TTE_OpenMetaStream("poker_text_button.prop", archive) -- BN100 is set, but loading a texas archive, ehhh not good, but the games are pretty much same

print("Done! The file type is: " .. select(1, MetaGetClass(prop)))
