set(zlib_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib)

# Create a library from the sources
add_library(zlib STATIC 
${zlib_SOURCE_DIR}/adler32.c
${zlib_SOURCE_DIR}/compress.c
${zlib_SOURCE_DIR}/crc32.c
${zlib_SOURCE_DIR}/deflate.c
${zlib_SOURCE_DIR}/infback.c
${zlib_SOURCE_DIR}/inffast.c
${zlib_SOURCE_DIR}/inflate.c
${zlib_SOURCE_DIR}/inftrees.c
${zlib_SOURCE_DIR}/trees.c
)

target_include_directories(zlib PUBLIC ${zlib_SOURCE_DIR}) 