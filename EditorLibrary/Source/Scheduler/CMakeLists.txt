set(SOURCES # define a list of sources
    JobScheduler.cpp
)

target_sources(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}) # Add EditorUI sources
add_subdirectory(Platform/${TTE_TARGET_PLATFORM})