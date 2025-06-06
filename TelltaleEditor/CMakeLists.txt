set(EDITOR_ROOT_DIR "${TELLTALE_EDITOR_ROOT_DIR}/TelltaleEditor")
set(THIRD_PARTY_DIR "${EDITOR_ROOT_DIR}/3rdparty") # Also known as vendor
set(EDITOR_ASSET_DIR "/Assets")


if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") # Specific VS things
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TelltaleEditor)
endif()

if(UNIX) # Setting cmake variables depending on platform
    set(TTE_TARGET_PLATFORM Unix)
    if(APPLE)
        set(TTE_TARGET_UNIX MacOS)
    else()
        set(TTE_TARGET_UNIX Linux)
    endif()
elseif(WIN32)
    set(TTE_TARGET_PLATFORM Win64)
endif()

function(set_build_type target) # Set the build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Define DEBUG and RELEASE macros depending on cmake build type 
        target_compile_definitions(${target} PRIVATE DEBUG=1)
    else()
        target_compile_definitions(${target} PRIVATE RELEASE=1)
    endif()
endfunction()

function(set_platform_type target) # Enable platform specific defines
    if(UNIX)
        if(APPLE)
            target_compile_definitions(${target} PRIVATE MACOS=1)
        else()
            target_compile_definitions(${target} PRIVATE LINUX=1)
        endif()
    elseif(WIN32)
        target_compile_definitions(${target} PRIVATE _WIN64=1)
    endif()
endfunction()

function (ignore_warnings target is_cpp) # ignore specific warnings, but disallow vla
	set(FINAL_WARNINGS "")

	if(is_cpp EQUAL 1) # cpp specific all platform warnings
		if(UNIX)
			set(FINAL_WARNINGS "${FINAL_WARNINGS}-Wno-reorder;")
		endif()
	endif()

	if(UNIX)
		if(APPLE)
			set(FINAL_WARNINGS "${FINAL_WARNINGS}-Wno-format-extra-args;-Wno-deprecated;-Werror=vla;")
		else()
            set(FINAL_WARNINGS "${FINAL_WARNINGS}-Wno-format-extra-args;-Wno-deprecated;-Werror=vla;")
		endif()
	elseif(WIN32)
		set(FINAL_WARNINGS "${FINAL_WARNINGS}") # any msvc specific stuff in the future
	endif()

	target_compile_options(${target} PRIVATE ${FINAL_WARNINGS})
endfunction()

add_subdirectory(3rdparty) # Add the dependencies/libraries
add_subdirectory(Source/ToolLibrary) # Add the Source Directory for both the library and UI
add_subdirectory(Source/Editor) # Add the Source Directory for both the library and UI
