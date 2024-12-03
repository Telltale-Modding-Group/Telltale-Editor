set(TARGET_NAME Editor)

file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h) # Recursively get all headers
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp) # Recursively get all sources
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES})

target_link_libraries(${TARGET_NAME} PUBLIC lua)
target_link_libraries(${TARGET_NAME} PUBLIC zlib)
target_link_libraries(${TARGET_NAME} PUBLIC ToolLibrary)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "TelltaleEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")

if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Define DEBUG and RELEASE macros depending on cmake build type 
    target_compile_definitions(Editor PRIVATE DEBUG=1)
else()
    target_compile_definitions(Editor PRIVATE RELEASE=1)
endif()
