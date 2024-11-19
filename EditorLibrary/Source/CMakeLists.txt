set(SOURCES # define a list of sources
    TestExport.cpp
)

add_subdirectory(zlib) # Add zlib CMakeLists.txt
add_subdirectory(lua52) # Add lua52 CMakeLists.txt
target_sources(editorlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}) # Add sources to library