set(TARGET_NAME ToolLibrary)

set(TELLTALE_LIB_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(TELLTALE_LIB_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Create a library from the sources
add_library(${TARGET_NAME} STATIC)

add_subdirectory(${TELLTALE_LIB_SOURCES_DIR}/Scheduler)

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

ignore_warnings(${TARGET_NAME} 1)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")

target_include_directories(${TARGET_NAME} PUBLIC ${TELLTALE_LIB_HEADERS_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
