set(third_party_folder "ThirdParty")

if(NOT TARGET zlib)
    include(zlib/CMakeLists.txt) # Probably have to change to zlib.cmake
    set_target_properties(zlib PROPERTIES FOLDER ${third_party_folder}/zlib)
endif()

if(NOT TARGET lua)
    include(lua52/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua PROPERTIES FOLDER ${third_party_folder}/lua)
endif()
