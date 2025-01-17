add_subdirectory(Platform/${TTE_TARGET_PLATFORM})
target_sources(${TARGET_NAME} PRIVATE Symbol.cpp Memory.cpp Math.cpp)