cmake_minimum_required(VERSION 3.19)
project(TelltaleEditor VERSION 0.0.1)

set(CMAKE_BUILD_TYPE Debug) # Set the build type to debug by default
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF) # Build static libraries
include(CMakeDependentOption)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR) # Prevent in-source builds
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

if(NOT WIN32)
    # GCC/Clang (Linux, macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    # MSVC (Windows)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od")    # /Zi = debug info, /Od = no optimization
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG") # linker flag for .pdb generation
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")     # /O2 = optimize for speed
endif()

set(TELLTALE_EDITOR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${TELLTALE_EDITOR_ROOT_DIR}/BinInstall")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${TELLTALE_EDITOR_ROOT_DIR}/Bin/_Int) # internal
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${TELLTALE_EDITOR_ROOT_DIR}/Bin/_Int) # internal
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${TELLTALE_EDITOR_ROOT_DIR}/Bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Enable IDE folders
add_subdirectory(TelltaleEditor)
