set(lua502_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua502)

# Create a library from the sources
add_library(lua502 STATIC 
${lua502_SOURCE_DIR}/lapi.c
${lua502_SOURCE_DIR}/lauxlib.c
${lua502_SOURCE_DIR}/lbaselib.c
${lua502_SOURCE_DIR}/lcode.c
${lua502_SOURCE_DIR}/ldblib.c
${lua502_SOURCE_DIR}/ldebug.c
${lua502_SOURCE_DIR}/ldo.c
${lua502_SOURCE_DIR}/ldump.c
${lua502_SOURCE_DIR}/lfunc.c
${lua502_SOURCE_DIR}/lgc.c
${lua502_SOURCE_DIR}/liolib.c
${lua502_SOURCE_DIR}/llex.c
${lua502_SOURCE_DIR}/lmathlib.c
${lua502_SOURCE_DIR}/lmem.c
${lua502_SOURCE_DIR}/loadlib.c
${lua502_SOURCE_DIR}/lobject.c
${lua502_SOURCE_DIR}/lopcodes.c
${lua502_SOURCE_DIR}/lparser.c
${lua502_SOURCE_DIR}/lstate.c
${lua502_SOURCE_DIR}/lstring.c
${lua502_SOURCE_DIR}/lstrlib.c
${lua502_SOURCE_DIR}/ltable.c
${lua502_SOURCE_DIR}/ltablib.c
${lua502_SOURCE_DIR}/ltm.c
${lua502_SOURCE_DIR}/lundump.c
${lua502_SOURCE_DIR}/lvm.c
${lua502_SOURCE_DIR}/lzio.c
)

target_include_directories(lua502 PUBLIC ${lua_SOURCE_DIR}) 
ignore_warnings(lua502 0)