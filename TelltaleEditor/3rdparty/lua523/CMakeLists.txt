set(lua523_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua523)

# Create a library from the sources
add_library(lua523 STATIC 
${lua523_SOURCE_DIR}/lapi.c
${lua523_SOURCE_DIR}/lauxlib.c
${lua523_SOURCE_DIR}/lbaselib.c
${lua523_SOURCE_DIR}/lcode.c
${lua523_SOURCE_DIR}/lcorolib.c
${lua523_SOURCE_DIR}/lctype.c
${lua523_SOURCE_DIR}/ldblib.c
${lua523_SOURCE_DIR}/ldebug.c
${lua523_SOURCE_DIR}/ldo.c
${lua523_SOURCE_DIR}/ldump.c
${lua523_SOURCE_DIR}/lfunc.c
${lua523_SOURCE_DIR}/lgc.c
${lua523_SOURCE_DIR}/linit.c
${lua523_SOURCE_DIR}/liolib.c
${lua523_SOURCE_DIR}/llex.c
${lua523_SOURCE_DIR}/lmathlib.c
${lua523_SOURCE_DIR}/lmem.c
${lua523_SOURCE_DIR}/loadlib.c
${lua523_SOURCE_DIR}/lobject.c
${lua523_SOURCE_DIR}/lopcodes.c
${lua523_SOURCE_DIR}/loslib.c
${lua523_SOURCE_DIR}/lparser.c
${lua523_SOURCE_DIR}/lstate.c
${lua523_SOURCE_DIR}/lstring.c
${lua523_SOURCE_DIR}/lstrlib.c
${lua523_SOURCE_DIR}/ltable.c
${lua523_SOURCE_DIR}/ltablib.c
${lua523_SOURCE_DIR}/ltm.c
${lua523_SOURCE_DIR}/lundump.c
${lua523_SOURCE_DIR}/lvm.c
${lua523_SOURCE_DIR}/lzio.c
)

target_include_directories(lua523 PUBLIC ${lua_SOURCE_DIR}) 
ignore_warnings(lua523 0)