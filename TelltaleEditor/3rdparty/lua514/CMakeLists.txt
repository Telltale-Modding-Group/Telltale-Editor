set(lua514_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua514)

# Create a library from the sources
add_library(lua514 STATIC 
${lua514_SOURCE_DIR}/lapi.c
${lua514_SOURCE_DIR}/lauxlib.c
${lua514_SOURCE_DIR}/lbaselib.c
${lua514_SOURCE_DIR}/lcode.c
${lua514_SOURCE_DIR}/ldblib.c
${lua514_SOURCE_DIR}/ldebug.c
${lua514_SOURCE_DIR}/ldo.c
${lua514_SOURCE_DIR}/ldump.c
${lua514_SOURCE_DIR}/lfunc.c
${lua514_SOURCE_DIR}/lgc.c
${lua514_SOURCE_DIR}/linit.c
${lua514_SOURCE_DIR}/liolib.c
${lua514_SOURCE_DIR}/llex.c
${lua514_SOURCE_DIR}/lmathlib.c
${lua514_SOURCE_DIR}/lmem.c
${lua514_SOURCE_DIR}/loadlib.c
${lua514_SOURCE_DIR}/lobject.c
${lua514_SOURCE_DIR}/lopcodes.c
${lua514_SOURCE_DIR}/loslib.c
${lua514_SOURCE_DIR}/lparser.c
${lua514_SOURCE_DIR}/lstate.c
${lua514_SOURCE_DIR}/lstring.c
${lua514_SOURCE_DIR}/lstrlib.c
${lua514_SOURCE_DIR}/ltable.c
${lua514_SOURCE_DIR}/ltablib.c
${lua514_SOURCE_DIR}/ltm.c
${lua514_SOURCE_DIR}/lundump.c
${lua514_SOURCE_DIR}/lvm.c
${lua514_SOURCE_DIR}/lzio.c
)

target_include_directories(lua514 PUBLIC ${lua_SOURCE_DIR}) 
ignore_warnings(lua514 0)