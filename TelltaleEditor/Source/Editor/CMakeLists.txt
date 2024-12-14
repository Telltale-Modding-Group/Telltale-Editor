set(TARGET_NAME Editor)

file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h) # Recursively get all headers
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC lua523)
target_link_libraries(${TARGET_NAME} PUBLIC lua514)
target_link_libraries(${TARGET_NAME} PUBLIC lua502)
target_link_libraries(${TARGET_NAME} PUBLIC zlib)
target_link_libraries(${TARGET_NAME} PUBLIC ToolLibrary)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 14 OUTPUT_NAME "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)
