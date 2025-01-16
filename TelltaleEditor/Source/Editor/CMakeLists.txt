set(TARGET_NAME Editor)

file(GLOB_RECURSE EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp) # Recursively get all headers
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC ToolLibrary)

target_include_directories(${TARGET_NAME} PUBLIC "${THIRD_PARTY_DIR}/sdl3")

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 14 OUTPUT_NAME "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Release")

function(add_copy_cmd fname thirddir target_name output_dir)
    add_custom_command(
        TARGET ${target_name} POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "${thirddir}/${fname}" 
        "${output_dir}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/${fname}"
    )
endfunction()

# copy dynamic libraries to output build directory
if(UNIX)
    if(APPLE)
        add_copy_cmd("oodle_mac_arm_x86.dylib" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
        add_copy_cmd("libsdl3_mac_arm64.dylib" "${THIRD_PARTY_DIR}/sdl3" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
        target_link_libraries(${TARGET_NAME} PRIVATE "${THIRD_PARTY_DIR}/sdl3/libsdl3_mac_arm64.dylib")
    else()
        add_copy_cmd("oodle_linux_arm.so" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
        add_copy_cmd("oodle_linux_x86.so" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
      	add_copy_cmd("libsdl3_linux_x86.so" "${THIRD_PARTY_DIR}/sdl3" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
        target_link_libraries(${TARGET_NAME} PRIVATE "${THIRD_PARTY_DIR}/sdl3/libsdl3_linux_x86.so")
    endif()
elseif(WIN32)
    add_copy_cmd("oodle_win_x86.dll" "${THIRD_PARTY_DIR}/oodle" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
    add_copy_cmd("libsdl3_win64_x86.dll" "${THIRD_PARTY_DIR}/sdl3" ${TARGET_NAME} "${TELLTALE_EDITOR_ROOT_DIR}/Bin")
    target_link_libraries(${TARGET_NAME} PRIVATE "${THIRD_PARTY_DIR}/sdl3/libsdl3_win64_x86.dll")
endif()

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)
