set(THIRD_PARTY_FOLDER "ThirdParty")

if(NOT TARGET zlib)
    include(zlib/CMakeLists.txt) # Probably have to change to zlib.cmake
    set_target_properties(zlib PROPERTIES FOLDER ${THIRD_PARTY_FOLDER})
endif()

if(NOT TARGET lua523)
    include(lua523/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua523 PROPERTIES FOLDER ${THIRD_PARTY_FOLDER})
endif()

if(NOT TARGET lua514)
    include(lua514/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua514 PROPERTIES FOLDER ${THIRD_PARTY_FOLDER})
endif()

if(NOT TARGET lua502)
    include(lua502/CMakeLists.txt) # Probably have to change to lua.cmake
    set_target_properties(lua502 PROPERTIES FOLDER ${THIRD_PARTY_FOLDER})
endif()

if(NOT TARGET SDL3::SDL3-static)
    # TODO! Fix folder hierarchy
    set(CMAKE_FOLDER ${THIRD_PARTY_FOLDER}/SDL3) # Apply FOLDER to all targets in the SDL3 folder
    add_subdirectory(SDL3 EXCLUDE_FROM_ALL)
    unset(CMAKE_FOLDER) # Unset it
endif()

if(NOT TARGET SDL3_shadercross::SDL3_shadercross-static)
    # TODO! Fix folder hierarchy
    set(CMAKE_FOLDER ${THIRD_PARTY_FOLDER}/SDL3_shadercross) # Apply FOLDER to all targets in the SDL3 folder
    set(SDLSHADERCROSS_VENDORED on)
    add_subdirectory(SDL_shadercross EXCLUDE_FROM_ALL)
    unset(CMAKE_FOLDER) # Unset it
endif()
