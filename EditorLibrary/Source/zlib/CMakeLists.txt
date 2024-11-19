set(SOURCES # define a list of sources
    adler32.c
    compress.c
    crc32.c
    deflate.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)

target_include_directories(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) # Add header files
target_sources(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}) # Add sources to library