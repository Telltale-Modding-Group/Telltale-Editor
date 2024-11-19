set(SOURCES # define a list of sources
    lapi.c
    lauxlib.c
    lbaselib.c
    lbitlib.c
    lcode.c
    lcorolib.c
    lctype.c
    ldblib.c
    ldebug.c
    ldo.c
    ldump.c
    lfunc.c
    lgc.c
    linit.c
    liolib.c
    llex.c
    lmathlib.c
    lmem.c
    loadlib.c
    lobject.c
    lopcodes.c
    loslib.c
    lparser.c
    lstate.c
    lstring.c
    lstrlib.c
    ltable.c
    ltablib.c
    ltm.c
    lua.c
    lundump.c
    lvm.c
    lzio.c
)

target_include_directories(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) # Add header files
target_sources(${module} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES}) # Add sources to library