cmake_minimum_required(VERSION 3.13)
project(TelltaleEditor)

set(CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 17) # Use C++17

set(CMAKE_CXX_FLAGS_DEBUG "-g") # debug info compiler flag
set(CMAKE_CXX_FLAGS_RELEASE "-O3") # optimization compiler flag

add_executable(editor) # Executable built from EditorUI
add_library(editorlib) # Library built from EditorLibrary

if(UNIX) # Setting cmake variables depending on platform
    set(TTE_TARGET_PLATFORM Unix)
    if(Apple)
        set(TTE_TARGET_UNIX MacOS)
    else()
        set(TTE_TARGET_UNIX Linux)
    endif()
elseif(WIN32)
    set(TTE_TARGET_PLATFORM Win64)
endif()

target_include_directories(editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EditorLibrary/Include) # Add header files

add_subdirectory(EditorLibrary) # Add EditorLibrary CMakeLists.txt
add_subdirectory(EditorUI) # Add EditorUI CMakeLists.txt

target_link_libraries(editor editorlib) # Link editor library to executable
target_compile_options(editor PRIVATE -Wall -Werror=vla) # Show all warnings and prohibit variable-length arrays
target_compile_options(editorlib PRIVATE -Wall -Werror=vla)

