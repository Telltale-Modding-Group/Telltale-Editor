-- TESTING DEVELOPMENT MOD SCRIPT. USERS CAN PROVIDE THEIR OWN, THIS ONE IS FOR TESTING.

TTE_Switch("BN100", "MacOS", "")

archive = TTE_OpenTTArchive("../../Dev/data.ttarch") -- pass in the relative path for exe. we need this for testing in the editor.

prop = TTE_OpenMetaStream("poker_text_button.prop", archive) -- BN100 is set, but loading a texas archive, ehhh not good, but the games are pretty much same

--print("Done! The file type is: " .. select(1, MetaGetClass(prop)))
