set(TARGET_NAME Editor)

file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h) # Recursively get all headers
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC ToolLibrary)
target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 14 OUTPUT_NAME "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Debug")
set_target_properties(${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${TELLTALE_EDITOR_ROOT_DIR}/Bin/Release")

# copy oodle dynamic libraries to output build directory
if(UNIX)
	if(APPLE)
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
			"${THIRD_PARTY_DIR}/oodle/oodle_mac_arm_x86.dylib" 
			"${TELLTALE_EDITOR_ROOT_DIR}/Bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/oodle_mac_arm_x86.dylib")
    else()
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
			"${THIRD_PARTY_DIR}/oodle/oodle_linux_arm.so" 
			"${TELLTALE_EDITOR_ROOT_DIR}/Bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/oodle_linux_arm.so")

		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
			"${THIRD_PARTY_DIR}/oodle/oodle_linux_x86.so" 
			"${TELLTALE_EDITOR_ROOT_DIR}/Bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/oodle_linux_x86.so")
    endif()
elseif(WIN32)
	add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different 
		"${THIRD_PARTY_DIR}/oodle/oodle_win_x86.dll" 
		"${TELLTALE_EDITOR_ROOT_DIR}/Bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/oodle_win.dll")
endif()

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)
