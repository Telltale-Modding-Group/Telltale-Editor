set(TARGET_NAME ToolLibrary)

set(TELLTALE_LIB_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(TELLTALE_LIB_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Create a library from the sources
add_library(ToolLibrary STATIC
    ${TELLTALE_LIB_HEADERS_DIR}/Meta/Meta.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Platform/Win64.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Platform/MacOS.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Platform/Linux.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Scheduler/JobScheduler.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Config.hpp
    ${TELLTALE_LIB_SOURCES_DIR}/Platform/Win64.cpp
    ${TELLTALE_LIB_SOURCES_DIR}/Scheduler/JobScheduler.cpp
)

# Enable platform specific defines
set_build_type(${TARGET_NAME})
set_platform_type(${TARGET_NAME})

set_target_properties(ToolLibrary PROPERTIES FOLDER "TelltaleEditor")

target_include_directories(ToolLibrary PUBLIC ${TELLTALE_LIB_HEADERS_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
