set(SOURCES # define a list of sources
    Main.cpp
)
set(module editor) # Create variable module equal to editor

target_sources(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}) # Add EditorUI sources