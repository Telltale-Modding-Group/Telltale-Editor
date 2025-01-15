set(third_party_folder "ThirdParty")

if(NOT TARGET zlib)
    include(zlib/CMakeLists.txt) # Probably have to change to zlib.cmake
    set_target_properties(zlib PROPERTIES FOLDER ${third_party_folder}/zlib)
endif()

if(NOT TARGET lua523)
    include(lua523/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua523 PROPERTIES FOLDER ${third_party_folder}/lua523)
endif()

if(NOT TARGET lua514)
    include(lua514/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua514 PROPERTIES FOLDER ${third_party_folder}/lua514)
endif()

if(NOT TARGET lua502)
    include(lua502/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua502 PROPERTIES FOLDER ${third_party_folder}/lua502)
endif()

add_subdirectory(sdl3)