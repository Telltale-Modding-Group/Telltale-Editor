set(module editor) # Create variable module equal to editor
add_subdirectory(Source) # Add Source CMakeLists.txt
#target_include_directories(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include) # Add header files

if(CMAKE_BUILD_TYPE STREQUAL "Debug") # Define DEBUG and RELEASE macros depending on cmake build type
    target_compile_definitions(${module} PRIVATE DEBUG=1)
else()
    target_compile_definitions(${module} PRIVATE RELEASE=1)
endif()
    

if(UNIX)
    if(Apple)
        target_compile_definitions(${module} PRIVATE MACOS=1)
    else()
        target_compile_definitions(${module} PRIVATE LINUX=1)
    endif()
elseif(WIN32)
    target_compile_definitions(${module} PRIVATE _WIN64=1)
endif()