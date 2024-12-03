set(TARGET_NAME ToolLibrary)

set(TELLTALE_LIB_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(TELLTALE_LIB_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# Create a library from the sources
add_library(ToolLibrary STATIC
    ${TELLTALE_LIB_HEADERS_DIR}/Meta/Meta.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Platform/Win64.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Scheduler/JobScheduler.hpp
    ${TELLTALE_LIB_HEADERS_DIR}/Config.hpp
    ${TELLTALE_LIB_SOURCES_DIR}/Platform/Win64.cpp
    ${TELLTALE_LIB_SOURCES_DIR}/Scheduler/JobScheduler.cpp
)

# Enable platform specific defines
if(UNIX)
    if(Apple)
        target_compile_definitions(ToolLibrary PRIVATE MACOS=1)
    else()
        target_compile_definitions(ToolLibrary PRIVATE LINUX=1)
    endif()
elseif(WIN32)
    target_compile_definitions(ToolLibrary PRIVATE _WIN64=1)
endif()

set_target_properties(ToolLibrary PROPERTIES FOLDER "TelltaleEditor")

target_include_directories(ToolLibrary PUBLIC ${TELLTALE_LIB_HEADERS_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Define DEBUG and RELEASE macros depending on cmake build type 
    target_compile_definitions(ToolLibrary PRIVATE DEBUG=1)
else()
    target_compile_definitions(ToolLibrary PRIVATE RELEASE=1)
endif()
