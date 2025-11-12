set(TARGET_NAME Editor)

file(GLOB_RECURSE EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp) # Recursively get all headers
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
file(GLOB_RECURSE EDITOR_INLINES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.inl) # Recursively get all inlines
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_INLINES})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_INLINES})

target_link_libraries(${TARGET_NAME} PRIVATE
    ToolLibrary
    SDL3::SDL3-static
    imgui
    nfd
)

# Include D3D libraries
if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE d3d12.lib d3dcompiler.lib)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
    if(BUILD_TYPE_UPPER STREQUAL "DEBUG")
        target_link_libraries(${TARGET_NAME} PRIVATE "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Bin/WinPixEventRuntime.lib")
    endif()
    target_include_directories(${TARGET_NAME} PUBLIC "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Include")
elseif(UNIX AND NOT APPLE)
    # TODO GLSLANG
elseif(APPLE)
    # TODO METAL COMP
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)

    if(NOT METAL_FRAMEWORK)
        message(FATAL_ERROR "Cannot build Telltale Editor: Metal framework not found. Please ensure the Metal SDK is installed, and make sure MetalKit is as well!")
    endif()

    if(NOT METALKIT_FRAMEWORK)
        message(FATAL_ERROR "Cannot build Telltale Editor: MetalKit framework not found! Please ensure this SDK is installed!")
    endif()

    target_link_libraries(${TARGET_NAME} PRIVATE
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Release")

# Windows debugger directory
if(WIN32)
set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
endif()

function(add_copy_cmd fname thirddir target_name output_dir)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${thirddir}/${fname}"
        "${output_dir}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/${fname}"
    )
endfunction()

function(add_copy_cmd_debug fname thirddir target_name output_dir)
    if(CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
        if(BUILD_TYPE_UPPER STREQUAL "DEBUG")
            add_custom_command(
                TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${thirddir}/${fname}"
                    "${output_dir}/Debug/${fname}"
            )
        endif()
    endif()
endfunction()

# copy dynamic libraries to output build directory
if(UNIX)
    if(APPLE)
        add_copy_cmd("oodle_mac_arm_x86.dylib" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
    else()
        add_copy_cmd("oodle_linux_arm.so" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
        add_copy_cmd("oodle_linux_x86.so" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
    endif()
elseif(WIN32)
    add_copy_cmd("oodle_win_x86.dll" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
endif()

if(WIN32)
    add_copy_cmd_debug("WinPixEventRuntime.dll" "${THIRD_PARTY_DIR}/WinPIXEventRuntime/Bin" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
endif()

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)
