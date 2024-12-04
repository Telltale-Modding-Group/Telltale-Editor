cmake_minimum_required(VERSION 3.19)
project(TelltaleEditor VERSION 0.0.1)

set(CMAKE_BUILD_TYPE Debug) # Set the build type to debug by default
set(CMAKE_CXX_STANDARD 17) # Use C++17
set(BUILD_SHARED_LIBS OFF) # Build static libraries
set(CMAKE_CXX_FLAGS_DEBUG "-g") # debug info compiler flag
set(CMAKE_CXX_FLAGS_RELEASE "-O3") # optimization compiler flag
include(CMakeDependentOption)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR) # Prevent in-source builds
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set(TELLTALE_EDITOR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${TELLTALE_EDITOR_ROOT_DIR}/bin")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")

set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Enable IDE folders
add_subdirectory(TelltaleEditor)
