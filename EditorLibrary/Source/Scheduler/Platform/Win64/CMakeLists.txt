set(SOURCES # define a list of sources
    Thread.cpp
)

target_sources(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES})