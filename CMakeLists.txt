cmake_minimum_required(VERSION 3.13)
project(TelltaleEditor)

set(CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_STANDARD 23) # Use C++23

set(CMAKE_CXX_FLAGS_DEBUG "-g") # debug info compiler flag
set(CMAKE_CXX_FLAGS_RELEASE "-O3") # optimization compiler flag

add_executable(editor) # Executable built from EditorUI
add_library(editorlib) # Library built from EditorLibrary

add_subdirectory(EditorLibrary/Source) # Add EditorLibrary CMakeLists.txt
add_subdirectory(EditorUI/Source) # Add EditorUI CMakeLists.txt

target_link_libraries(editor editorlib) # Link editor library to executable
target_compile_options(editor PRIVATE -Wall -Werror=vla) # Show all warnings and prohibit variable-length arrays
target_compile_options(editorlib PRIVATE -Wall -Werror=vla)