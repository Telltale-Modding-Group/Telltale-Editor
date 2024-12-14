set(TARGET_NAME ToolLibrary)

set(TELLTALE_LIB_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(TELLTALE_LIB_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Create a library from the sources
add_library(${TARGET_NAME} STATIC)

# SOURCE

add_subdirectory(${TELLTALE_LIB_SOURCES_DIR}/Scheduler)
add_subdirectory(${TELLTALE_LIB_SOURCES_DIR}/Core)
add_subdirectory(${TELLTALE_LIB_SOURCES_DIR}/Scripting)

# INCLUDE

add_subdirectory(${TELLTALE_LIB_HEADERS_DIR}/Scheduler)
add_subdirectory(${TELLTALE_LIB_HEADERS_DIR}/Core)
add_subdirectory(${TELLTALE_LIB_HEADERS_DIR}/Meta)
add_subdirectory(${TELLTALE_LIB_HEADERS_DIR}/Scripting)

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "TelltaleEditor")
ignore_warnings(${TARGET_NAME} 1)

target_include_directories(${TARGET_NAME} PUBLIC ${TELLTALE_LIB_HEADERS_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# Add include dirs 3rd parties

target_include_directories(${TARGET_NAME} PUBLIC ${THIRD_PARTY_DIR})