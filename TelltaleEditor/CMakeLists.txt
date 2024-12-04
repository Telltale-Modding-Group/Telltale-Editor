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

function(set_build_type type) # Set the build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Define DEBUG and RELEASE macros depending on cmake build type 
        target_compile_definitions(${type} PRIVATE DEBUG=1)
    else()
        target_compile_definitions(${type}  PRIVATE RELEASE=1)
    endif()
endfunction()

function(set_platform_type type) # Enable platform specific defines
    if(UNIX)
        if(Apple)
            target_compile_definitions(${type}  PRIVATE MACOS=1)
        else()
            target_compile_definitions(${type}  PRIVATE LINUX=1)
        endif()
    elseif(WIN32)
        target_compile_definitions(${type}  PRIVATE _WIN64=1)
    endif()
endfunction()

add_subdirectory(3rdparty) # Add the dependencies/libraries
add_subdirectory(Source/ToolLibrary) # Add the Source Directory for both the library and UI
add_subdirectory(Source/Editor) # Add the Source Directory for both the library and UI
