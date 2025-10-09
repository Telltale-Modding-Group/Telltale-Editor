add_library(nfd STATIC)

set(nfd_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NFD)

target_sources(nfd
    PRIVATE
        ${nfd_SOURCE_DIR}/src/nfd_common.c
        ${nfd_SOURCE_DIR}/src/common.h
        ${nfd_SOURCE_DIR}/src/nfd_common.h
)

if (APPLE)
    message(STATUS "-nfd for macos detected")
    target_sources(nfd PRIVATE ${nfd_SOURCE_DIR}/src/nfd_cocoa.m)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    target_link_libraries(nfd PRIVATE ${FOUNDATION_FRAMEWORK} ${APPKIT_FRAMEWORK})

elseif (WIN32)
    message(STATUS "-nfd for win detected")
    target_sources(nfd PRIVATE ${nfd_SOURCE_DIR}/src/nfd_win.cpp)
    target_link_libraries(nfd PRIVATE ole32 uuid comdlg32)
    set_target_properties(nfd PROPERTIES LINKER_LANGUAGE CXX)
elseif (UNIX)
    message(STATUS "-nfd for unix / linux")
    target_sources(nfd PRIVATE ${nfd_SOURCE_DIR}/src/nfd_zenity.c ${nfd_SOURCE_DIR}/src/simple_exec.h)
endif()

target_include_directories(nfd PUBLIC  ${nfd_SOURCE_DIR}/include)
ignore_warnings(nfd 0)