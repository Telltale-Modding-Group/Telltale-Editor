set(EDITOR_ROOT_DIR "${TELLTALE_EDITOR_ROOT_DIR}/TelltaleEditor")
set(THIRD_PARTY_DIR "${EDITOR_ROOT_DIR}/3rdparty") # Also known as vendor
set(EDITOR_ASSET_DIR "/Assets")


if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") # Specific VS things
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TelltaleEditor)
endif()

if(UNIX) # Setting cmake variables depending on platform
    set(TTE_TARGET_PLATFORM Unix)
    if(Apple)
        set(TTE_TARGET_UNIX MacOS)
    else()
        set(TTE_TARGET_UNIX Linux)
    endif()
elseif(WIN32)
    set(TTE_TARGET_PLATFORM Win64)
endif()

add_subdirectory(3rdparty) # Add the dependencies/libraries
add_subdirectory(Source/ToolLibrary) # Add the Source Directory for both the library and UI
add_subdirectory(Source/Editor) # Add the Source Directory for both the library and UI
