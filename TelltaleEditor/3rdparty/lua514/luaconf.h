/*
** $Id: luaconf.h,v 1.82.1.7 2008/02/11 16:25:08 roberto Exp $
** Configuration file for Lua
** See Copyright Notice in lua.h
*/


#ifndef lconfig_h
#define lconfig_h

#include <limits.h>
#include <stddef.h>

// BY TELLTALE EDITOR THE FOLLOWING MACROS


#define luaA_pushobject  L514_luaA_pushobject
#define lua_atpanic  L514_lua_atpanic
#define lua_call  L514_lua_call
#define lua_checkstack  L514_lua_checkstack
#define lua_concat  L514_lua_concat
#define lua_cpcall  L514_lua_cpcall
#define lua_createtable  L514_lua_createtable
#define lua_dump  L514_lua_dump
#define lua_equal  L514_lua_equal
#define lua_error  L514_lua_error
#define lua_gc  L514_lua_gc
#define lua_getallocf  L514_lua_getallocf
#define lua_getfenv  L514_lua_getfenv
#define lua_getfield  L514_lua_getfield
#define lua_getmetatable  L514_lua_getmetatable
#define lua_gettable  L514_lua_gettable
#define lua_gettop  L514_lua_gettop
#define lua_getupvalue  L514_lua_getupvalue
#define lua_insert  L514_lua_insert
#define lua_iscfunction  L514_lua_iscfunction
#define lua_isnumber  L514_lua_isnumber
#define lua_isstring  L514_lua_isstring
#define lua_isuserdata  L514_lua_isuserdata
#define lua_lessthan  L514_lua_lessthan
#define lua_load  L514_lua_load
#define lua_newthread  L514_lua_newthread
#define lua_newuserdata  L514_lua_newuserdata
#define lua_next  L514_lua_next
#define lua_objlen  L514_lua_objlen
#define lua_pcall  L514_lua_pcall
#define lua_pushboolean  L514_lua_pushboolean
#define lua_pushcclosure  L514_lua_pushcclosure
#define lua_pushfstring  L514_lua_pushfstring
#define lua_pushinteger  L514_lua_pushinteger
#define lua_pushlightuserdata  L514_lua_pushlightuserdata
#define lua_pushlstring  L514_lua_pushlstring
#define lua_pushnil  L514_lua_pushnil
#define lua_pushnumber  L514_lua_pushnumber
#define lua_pushstring  L514_lua_pushstring
#define lua_pushthread  L514_lua_pushthread
#define lua_pushvalue  L514_lua_pushvalue
#define lua_pushvfstring  L514_lua_pushvfstring
#define lua_rawequal  L514_lua_rawequal
#define lua_rawget  L514_lua_rawget
#define lua_rawgeti  L514_lua_rawgeti
#define lua_rawset  L514_lua_rawset
#define lua_rawseti  L514_lua_rawseti
#define lua_remove  L514_lua_remove
#define lua_replace  L514_lua_replace
#define lua_setallocf  L514_lua_setallocf
#define lua_setfenv  L514_lua_setfenv
#define lua_setfield  L514_lua_setfield
#define lua_setlevel  L514_lua_setlevel
#define lua_setmetatable  L514_lua_setmetatable
#define lua_settable  L514_lua_settable
#define lua_settop  L514_lua_settop
#define lua_setupvalue  L514_lua_setupvalue
#define lua_status  L514_lua_status
#define lua_toboolean  L514_lua_toboolean
#define lua_tocfunction  L514_lua_tocfunction
#define lua_tointeger  L514_lua_tointeger
#define lua_tolstring  L514_lua_tolstring
#define lua_tonumber  L514_lua_tonumber
#define lua_topointer  L514_lua_topointer
#define lua_tothread  L514_lua_tothread
#define lua_touserdata  L514_lua_touserdata
#define lua_type  L514_lua_type
#define lua_typename  L514_lua_typename
#define lua_xmove  L514_lua_xmove
#define luaL_addlstring  L514_luaL_addlstring
#define luaL_addstring  L514_luaL_addstring
#define luaL_addvalue  L514_luaL_addvalue
#define luaL_argerror  L514_luaL_argerror
#define luaL_buffinit  L514_luaL_buffinit
#define luaL_callmeta  L514_luaL_callmeta
#define luaL_checkany  L514_luaL_checkany
#define luaL_checkinteger  L514_luaL_checkinteger
#define luaL_checklstring  L514_luaL_checklstring
#define luaL_checknumber  L514_luaL_checknumber
#define luaL_checkoption  L514_luaL_checkoption
#define luaL_checkstack  L514_luaL_checkstack
#define luaL_checktype  L514_luaL_checktype
#define luaL_checkudata  L514_luaL_checkudata
#define luaL_error  L514_luaL_error
#define luaL_findtable  L514_luaL_findtable
#define luaL_getmetafield  L514_luaL_getmetafield
#define luaL_gsub  L514_luaL_gsub
#define luaL_loadbuffer  L514_luaL_loadbuffer
#define luaL_loadfile  L514_luaL_loadfile
#define luaL_loadstring  L514_luaL_loadstring
#define luaL_newmetatable  L514_luaL_newmetatable
#define luaL_newstate  L514_luaL_newstate
#define luaL_openlib  L514_luaL_openlib
#define luaL_optinteger  L514_luaL_optinteger
#define luaL_optlstring  L514_luaL_optlstring
#define luaL_optnumber  L514_luaL_optnumber
#define luaL_prepbuffer  L514_luaL_prepbuffer
#define luaL_pushresult  L514_luaL_pushresult
#define luaL_ref  L514_luaL_ref
#define luaL_register  L514_luaL_register
#define luaL_typerror  L514_luaL_typerror
#define luaL_unref  L514_luaL_unref
#define luaL_where  L514_luaL_where
#define luaopen_base  L514_luaopen_base
#define luaK_checkstack  L514_luaK_checkstack
#define luaK_codeABC  L514_luaK_codeABC
#define luaK_codeABx  L514_luaK_codeABx
#define luaK_concat  L514_luaK_concat
#define luaK_dischargevars  L514_luaK_dischargevars
#define luaK_exp2RK  L514_luaK_exp2RK
#define luaK_exp2anyreg  L514_luaK_exp2anyreg
#define luaK_exp2nextreg  L514_luaK_exp2nextreg
#define luaK_exp2val  L514_luaK_exp2val
#define luaK_fixline  L514_luaK_fixline
#define luaK_getlabel  L514_luaK_getlabel
#define luaK_goiftrue  L514_luaK_goiftrue
#define luaK_indexed  L514_luaK_indexed
#define luaK_infix  L514_luaK_infix
#define luaK_jump  L514_luaK_jump
#define luaK_nil  L514_luaK_nil
#define luaK_numberK  L514_luaK_numberK
#define luaK_patchlist  L514_luaK_patchlist
#define luaK_patchtohere  L514_luaK_patchtohere
#define luaK_posfix  L514_luaK_posfix
#define luaK_prefix  L514_luaK_prefix
#define luaK_reserveregs  L514_luaK_reserveregs
#define luaK_ret  L514_luaK_ret
#define luaK_self  L514_luaK_self
#define luaK_setlist  L514_luaK_setlist
#define luaK_setoneret  L514_luaK_setoneret
#define luaK_setreturns  L514_luaK_setreturns
#define luaK_storevar  L514_luaK_storevar
#define luaK_stringK  L514_luaK_stringK
#define luaopen_debug  L514_luaopen_debug
#define luaG_aritherror  L514_luaG_aritherror
#define luaG_checkcode  L514_luaG_checkcode
#define luaG_checkopenop  L514_luaG_checkopenop
#define luaG_concaterror  L514_luaG_concaterror
#define luaG_errormsg  L514_luaG_errormsg
#define luaG_ordererror  L514_luaG_ordererror
#define luaG_runerror  L514_luaG_runerror
#define luaG_typeerror  L514_luaG_typeerror
#define lua_gethook  L514_lua_gethook
#define lua_gethookcount  L514_lua_gethookcount
#define lua_gethookmask  L514_lua_gethookmask
#define lua_getinfo  L514_lua_getinfo
#define lua_getlocal  L514_lua_getlocal
#define lua_getstack  L514_lua_getstack
#define lua_sethook  L514_lua_sethook
#define lua_setlocal  L514_lua_setlocal
#define luaD_call  L514_luaD_call
#define luaD_callhook  L514_luaD_callhook
#define luaD_growstack  L514_luaD_growstack
#define luaD_pcall  L514_luaD_pcall
#define luaD_poscall  L514_luaD_poscall
#define luaD_precall  L514_luaD_precall
#define luaD_protectedparser  L514_luaD_protectedparser
#define luaD_rawrunprotected  L514_luaD_rawrunprotected
#define luaD_reallocCI  L514_luaD_reallocCI
#define luaD_reallocstack  L514_luaD_reallocstack
#define luaD_seterrorobj  L514_luaD_seterrorobj
#define luaD_throw  L514_luaD_throw
#define lua_resume  L514_lua_resume
#define lua_yield  L514_lua_yield
#define luaU_dump  L514_luaU_dump
#define luaF_close  L514_luaF_close
#define luaF_findupval  L514_luaF_findupval
#define luaF_freeclosure  L514_luaF_freeclosure
#define luaF_freeproto  L514_luaF_freeproto
#define luaF_freeupval  L514_luaF_freeupval
#define luaF_getlocalname  L514_luaF_getlocalname
#define luaF_newCclosure  L514_luaF_newCclosure
#define luaF_newLclosure  L514_luaF_newLclosure
#define luaF_newproto  L514_luaF_newproto
#define luaF_newupval  L514_luaF_newupval
#define luaC_barrierback  L514_luaC_barrierback
#define luaC_barrierf  L514_luaC_barrierf
#define luaC_callGCTM  L514_luaC_callGCTM
#define luaC_freeall  L514_luaC_freeall
#define luaC_fullgc  L514_luaC_fullgc
#define luaC_link  L514_luaC_link
#define luaC_linkupval  L514_luaC_linkupval
#define luaC_separateudata  L514_luaC_separateudata
#define luaC_step  L514_luaC_step
#define luaL_openlibs  L514_luaL_openlibs
#define luaopen_io  L514_luaopen_io
#define luaX_init  L514_luaX_init
#define luaX_lexerror  L514_luaX_lexerror
#define luaX_lookahead  L514_luaX_lookahead
#define luaX_newstring  L514_luaX_newstring
#define luaX_next  L514_luaX_next
#define luaX_setinput  L514_luaX_setinput
#define luaX_syntaxerror  L514_luaX_syntaxerror
#define luaX_token2str  L514_luaX_token2str
#define luaopen_math  L514_luaopen_math
#define luaM_growaux_  L514_luaM_growaux_
#define luaM_realloc_  L514_luaM_realloc_
#define luaM_toobig  L514_luaM_toobig
#define luaopen_package  L514_luaopen_package
#define luaO_chunkid  L514_luaO_chunkid
#define luaO_fb2int  L514_luaO_fb2int
#define luaO_int2fb  L514_luaO_int2fb
#define luaO_log2  L514_luaO_log2
#define luaO_pushfstring  L514_luaO_pushfstring
#define luaO_pushvfstring  L514_luaO_pushvfstring
#define luaO_rawequalObj  L514_luaO_rawequalObj
#define luaO_str2d  L514_luaO_str2d
#define luaopen_os  L514_luaopen_os
#define luaY_parser  L514_luaY_parser
#define luaE_freethread  L514_luaE_freethread
#define luaE_newthread  L514_luaE_newthread
#define lua_close  L514_lua_close
#define lua_newstate  L514_lua_newstate
#define luaS_newlstr  L514_luaS_newlstr
#define luaS_newudata  L514_luaS_newudata
#define luaS_resize  L514_luaS_resize
#define luaopen_string  L514_luaopen_string
#define luaH_free  L514_luaH_free
#define luaH_get  L514_luaH_get
#define luaH_getn  L514_luaH_getn
#define luaH_getnum  L514_luaH_getnum
#define luaH_getstr  L514_luaH_getstr
#define luaH_new  L514_luaH_new
#define luaH_next  L514_luaH_next
#define luaH_resizearray  L514_luaH_resizearray
#define luaH_set  L514_luaH_set
#define luaH_setnum  L514_luaH_setnum
#define luaH_setstr  L514_luaH_setstr
#define luaopen_table  L514_luaopen_table
#define luaT_gettm  L514_luaT_gettm
#define luaT_gettmbyobj  L514_luaT_gettmbyobj
#define luaT_init  L514_luaT_init
#define luaU_header  L514_luaU_header
#define luaU_undump  L514_luaU_undump
#define luaV_concat  L514_luaV_concat
#define luaV_equalval  L514_luaV_equalval
#define luaV_execute  L514_luaV_execute
#define luaV_gettable  L514_luaV_gettable
#define luaV_lessthan  L514_luaV_lessthan
#define luaV_settable  L514_luaV_settable
#define luaV_tonumber  L514_luaV_tonumber
#define luaV_tostring  L514_luaV_tostring
#define luaZ_fill  L514_luaZ_fill
#define luaZ_init  L514_luaZ_init
#define luaZ_lookahead  L514_luaZ_lookahead
#define luaZ_openspace  L514_luaZ_openspace
#define luaZ_read  L514_luaZ_read

/*
** ==================================================================
** Search for "@@" to find all configurable definitions.
** ===================================================================
*/


/*
@@ LUA_ANSI controls the use of non-ansi features.
** CHANGE it (define it) if you want Lua to avoid the use of any
** non-ansi feature or library.
*/
#if defined(__STRICT_ANSI__)
#define LUA_ANSI
#endif


#if !defined(LUA_ANSI) && defined(_WIN32)
#define LUA_WIN
#endif

#if defined(LUA_USE_LINUX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* needs an extra library: -ldl */
#define LUA_USE_READLINE	/* needs some extra libraries */
#endif

#if defined(LUA_USE_MACOSX)
#define LUA_USE_POSIX
#define LUA_DL_DYLD		/* does not need extra library */
#endif



/*
@@ LUA_USE_POSIX includes all functionallity listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(LUA_USE_POSIX)
#define LUA_USE_MKSTEMP
#define LUA_USE_ISATTY
#define LUA_USE_POPEN
#define LUA_USE_ULONGJMP
#endif


/*
@@ LUA_PATH and LUA_CPATH are the names of the environment variables that
@* Lua check to set its paths.
@@ LUA_INIT is the name of the environment variable that Lua
@* checks for initialization code.
** CHANGE them if you want different names.
*/
#define LUA_PATH        "LUA_PATH"
#define LUA_CPATH       "LUA_CPATH"
#define LUA_INIT	"LUA_INIT"


/*
@@ LUA_PATH_DEFAULT is the default path that Lua uses to look for
@* Lua libraries.
@@ LUA_CPATH_DEFAULT is the default path that Lua uses to look for
@* C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/
#if defined(_WIN32)
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define LUA_LDIR	"!\\lua\\"
#define LUA_CDIR	"!\\"
#define LUA_PATH_DEFAULT  \
		".\\?.lua;"  LUA_LDIR"?.lua;"  LUA_LDIR"?\\init.lua;" \
		             LUA_CDIR"?.lua;"  LUA_CDIR"?\\init.lua"
#define LUA_CPATH_DEFAULT \
	".\\?.dll;"  LUA_CDIR"?.dll;" LUA_CDIR"loadall.dll"

#else
#define LUA_ROOT	"/usr/local/"
#define LUA_LDIR	LUA_ROOT "share/lua/5.1/"
#define LUA_CDIR	LUA_ROOT "lib/lua/5.1/"
#define LUA_PATH_DEFAULT  \
		"./?.lua;"  LUA_LDIR"?.lua;"  LUA_LDIR"?/init.lua;" \
		            LUA_CDIR"?.lua;"  LUA_CDIR"?/init.lua"
#define LUA_CPATH_DEFAULT \
	"./?.so;"  LUA_CDIR"?.so;" LUA_CDIR"loadall.so"
#endif


/*
@@ LUA_DIRSEP is the directory separator (for submodules).
** CHANGE it if your machine does not use "/" as the directory separator
** and is not Windows. (On Windows Lua automatically uses "\".)
*/
#if defined(_WIN32)
#define LUA_DIRSEP	"\\"
#else
#define LUA_DIRSEP	"/"
#endif


/*
@@ LUA_PATHSEP is the character that separates templates in a path.
@@ LUA_PATH_MARK is the string that marks the substitution points in a
@* template.
@@ LUA_EXECDIR in a Windows path is replaced by the executable's
@* directory.
@@ LUA_IGMARK is a mark to ignore all before it when bulding the
@* luaopen_ function name.
** CHANGE them if for some reason your system cannot use those
** characters. (E.g., if one of those characters is a common character
** in file/directory names.) Probably you do not need to change them.
*/
#define LUA_PATHSEP	";"
#define LUA_PATH_MARK	"?"
#define LUA_EXECDIR	"!"
#define LUA_IGMARK	"-"


/*
@@ LUA_INTEGER is the integral type used by lua_pushinteger/lua_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define LUA_INTEGER	ptrdiff_t


/*
@@ LUA_API is a mark for all core API functions.
@@ LUALIB_API is a mark for all standard library functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** LUA_BUILD_AS_DLL to get it).
*/
#if defined(LUA_BUILD_AS_DLL)

#if defined(LUA_CORE) || defined(LUA_LIB)
#define LUA_API __declspec(dllexport)
#else
#define LUA_API __declspec(dllimport)
#endif

#else

#define LUA_API		extern

#endif

/* more often than not the libs go together with the core */
#define LUALIB_API	LUA_API


/*
@@ LUAI_FUNC is a mark for all extern functions that are not to be
@* exported to outside modules.
@@ LUAI_DATA is a mark for all extern (const) variables that are not to
@* be exported to outside modules.
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when Lua is compiled as a shared library.
*/
#if defined(luaall_c)
#define LUAI_FUNC	static
#define LUAI_DATA	/* empty */

#elif defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
      defined(__ELF__)
#define LUAI_FUNC	__attribute__((visibility("hidden"))) extern
#define LUAI_DATA	LUAI_FUNC

#else
#define LUAI_FUNC	extern
#define LUAI_DATA	extern
#endif



/*
@@ LUA_QL describes how error messages quote program elements.
** CHANGE it if you want a different appearance.
*/
#define LUA_QL(x)	"'" x "'"
#define LUA_QS		LUA_QL("%s")


/*
@@ LUA_IDSIZE gives the maximum size for the description of the source
@* of a function in debug information.
** CHANGE it if you want a different size.
*/
#define LUA_IDSIZE	60


/*
** {==================================================================
** Stand-alone configuration
** ===================================================================
*/

#if defined(lua_c) || defined(luaall_c)

/*
@@ lua_stdin_is_tty detects whether the standard input is a 'tty' (that
@* is, whether we're running lua interactively).
** CHANGE it if you have a better definition for non-POSIX/non-Windows
** systems.
*/
#if defined(LUA_USE_ISATTY)
#include <unistd.h>
#define lua_stdin_is_tty()	isatty(0)
#elif defined(LUA_WIN)
#include <io.h>
#include <stdio.h>
#define lua_stdin_is_tty()	_isatty(_fileno(stdin))
#else
#define lua_stdin_is_tty()	1  /* assume stdin is a tty */
#endif


/*
@@ LUA_PROMPT is the default prompt used by stand-alone Lua.
@@ LUA_PROMPT2 is the default continuation prompt used by stand-alone Lua.
** CHANGE them if you want different prompts. (You can also change the
** prompts dynamically, assigning to globals _PROMPT/_PROMPT2.)
*/
#define LUA_PROMPT		"> "
#define LUA_PROMPT2		">> "


/*
@@ LUA_PROGNAME is the default name for the stand-alone Lua program.
** CHANGE it if your stand-alone interpreter has a different name and
** your system is not able to detect that name automatically.
*/
#define LUA_PROGNAME		"lua"


/*
@@ LUA_MAXINPUT is the maximum length for an input line in the
@* stand-alone interpreter.
** CHANGE it if you need longer lines.
*/
#define LUA_MAXINPUT	512


/*
@@ lua_readline defines how to show a prompt and then read a line from
@* the standard input.
@@ lua_saveline defines how to "save" a read line in a "history".
@@ lua_freeline defines how to free a line read by lua_readline.
** CHANGE them if you want to improve this functionality (e.g., by using
** GNU readline and history facilities).
*/
#if defined(LUA_USE_READLINE)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define lua_readline(L,b,p)	((void)L, ((b)=readline(p)) != NULL)
#define lua_saveline(L,idx) \
	if (lua_strlen(L,idx) > 0)  /* non-empty line? */ \
	  add_history(lua_tostring(L, idx));  /* add it to history */
#define lua_freeline(L,b)	((void)L, free(b))
#else
#define lua_readline(L,b,p)	\
	((void)L, fputs(p, stdout), fflush(stdout),  /* show prompt */ \
	fgets(b, LUA_MAXINPUT, stdin) != NULL)  /* get line */
#define lua_saveline(L,idx)	{ (void)L; (void)idx; }
#define lua_freeline(L,b)	{ (void)L; (void)b; }
#endif

#endif

/* }================================================================== */


/*
@@ LUAI_GCPAUSE defines the default pause between garbage-collector cycles
@* as a percentage.
** CHANGE it if you want the GC to run faster or slower (higher values
** mean larger pauses which mean slower collection.) You can also change
** this value dynamically.
*/
#define LUAI_GCPAUSE	200  /* 200% (wait memory to double before next GC) */


/*
@@ LUAI_GCMUL defines the default speed of garbage collection relative to
@* memory allocation as a percentage.
** CHANGE it if you want to change the granularity of the garbage
** collection. (Higher values mean coarser collections. 0 represents
** infinity, where each step performs a full collection.) You can also
** change this value dynamically.
*/
#define LUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */



/*
@@ LUA_COMPAT_GETN controls compatibility with old getn behavior.
** CHANGE it (define it) if you want exact compatibility with the
** behavior of setn/getn in Lua 5.0.
*/
#undef LUA_COMPAT_GETN

/*
@@ LUA_COMPAT_LOADLIB controls compatibility about global loadlib.
** CHANGE it to undefined as soon as you do not need a global 'loadlib'
** function (the function is still available as 'package.loadlib').
*/
#undef LUA_COMPAT_LOADLIB

/*
@@ LUA_COMPAT_VARARG controls compatibility with old vararg feature.
** CHANGE it to undefined as soon as your programs use only '...' to
** access vararg parameters (instead of the old 'arg' table).
*/
#define LUA_COMPAT_VARARG

/*
@@ LUA_COMPAT_MOD controls compatibility with old math.mod function.
** CHANGE it to undefined as soon as your programs use 'math.fmod' or
** the new '%' operator instead of 'math.mod'.
*/
#define LUA_COMPAT_MOD

/*
@@ LUA_COMPAT_LSTR controls compatibility with old long string nesting
@* facility.
** CHANGE it to 2 if you want the old behaviour, or undefine it to turn
** off the advisory error when nesting [[...]].
*/
#define LUA_COMPAT_LSTR		1

/*
@@ LUA_COMPAT_GFIND controls compatibility with old 'string.gfind' name.
** CHANGE it to undefined as soon as you rename 'string.gfind' to
** 'string.gmatch'.
*/
#define LUA_COMPAT_GFIND

/*
@@ LUA_COMPAT_OPENLIB controls compatibility with old 'luaL_openlib'
@* behavior.
** CHANGE it to undefined as soon as you replace to 'luaL_register'
** your uses of 'luaL_openlib'
*/
#define LUA_COMPAT_OPENLIB



/*
@@ luai_apicheck is the assert macro used by the Lua-C API.
** CHANGE luai_apicheck if you want Lua to perform some checks in the
** parameters it gets from API calls. This may slow down the interpreter
** a bit, but may be quite useful when debugging C code that interfaces
** with Lua. A useful redefinition is to use assert.h.
*/
#if defined(LUA_USE_APICHECK)
#include <assert.h>
#define luai_apicheck(L,o)	{ (void)L; assert(o); }
#else
#define luai_apicheck(L,o)	{ (void)L; }
#endif


/*
@@ LUAI_BITSINT defines the number of bits in an int.
** CHANGE here if Lua cannot automatically detect the number of bits of
** your machine. Probably you do not need to change this.
*/
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760
#define LUAI_BITSINT	16
#elif INT_MAX > 2147483640L
/* int has at least 32 bits */
#define LUAI_BITSINT	32
#else
#error "you must define LUA_BITSINT with number of bits in an integer"
#endif


/*
@@ LUAI_UINT32 is an unsigned integer with at least 32 bits.
@@ LUAI_INT32 is an signed integer with at least 32 bits.
@@ LUAI_UMEM is an unsigned integer big enough to count the total
@* memory used by Lua.
@@ LUAI_MEM is a signed integer big enough to count the total memory
@* used by Lua.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. (The definitions in the 'else'
** part always works, but may waste space on machines with 64-bit
** longs.) Probably you do not need to change this.
*/
#if LUAI_BITSINT >= 32
#define LUAI_UINT32	unsigned int
#define LUAI_INT32	int
#define LUAI_MAXINT32	INT_MAX
#define LUAI_UMEM	size_t
#define LUAI_MEM	ptrdiff_t
#else
/* 16-bit ints */
#define LUAI_UINT32	unsigned long
#define LUAI_INT32	long
#define LUAI_MAXINT32	LONG_MAX
#define LUAI_UMEM	unsigned long
#define LUAI_MEM	long
#endif


/*
@@ LUAI_MAXCALLS limits the number of nested calls.
** CHANGE it if you need really deep recursive calls. This limit is
** arbitrary; its only purpose is to stop infinite recursion before
** exhausting memory.
*/
#define LUAI_MAXCALLS	20000


/*
@@ LUAI_MAXCSTACK limits the number of Lua stack slots that a C function
@* can use.
** CHANGE it if you need lots of (Lua) stack space for your C
** functions. This limit is arbitrary; its only purpose is to stop C
** functions to consume unlimited stack space. (must be smaller than
** -LUA_REGISTRYINDEX)
*/
#define LUAI_MAXCSTACK	8000



/*
** {==================================================================
** CHANGE (to smaller values) the following definitions if your system
** has a small C stack. (Or you may want to change them to larger
** values if your system has a large C stack and these limits are
** too rigid for you.) Some of these constants control the size of
** stack-allocated arrays used by the compiler or the interpreter, while
** others limit the maximum number of recursive calls that the compiler
** or the interpreter can perform. Values too large may cause a C stack
** overflow for some forms of deep constructs.
** ===================================================================
*/


/*
@@ LUAI_MAXCCALLS is the maximum depth for nested C calls (short) and
@* syntactical nested non-terminals in a program.
*/
#define LUAI_MAXCCALLS		200


/*
@@ LUAI_MAXVARS is the maximum number of local variables per function
@* (must be smaller than 250).
*/
#define LUAI_MAXVARS		200


/*
@@ LUAI_MAXUPVALUES is the maximum number of upvalues per function
@* (must be smaller than 250).
*/
#define LUAI_MAXUPVALUES	60


/*
@@ LUAL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
*/
#define LUAL_BUFFERSIZE		BUFSIZ

/* }================================================================== */




/*
** {==================================================================
@@ LUA_NUMBER is the type of numbers in Lua.
** CHANGE the following definitions only if you want to build Lua
** with a number type different from double. You may also need to
** change lua_number2int & lua_number2integer.
** ===================================================================
*/

#define LUA_NUMBER_DOUBLE
#define LUA_NUMBER	double

/*
@@ LUAI_UACNUMBER is the result of an 'usual argument conversion'
@* over a number.
*/
#define LUAI_UACNUMBER	double


/*
@@ LUA_NUMBER_SCAN is the format for reading numbers.
@@ LUA_NUMBER_FMT is the format for writing numbers.
@@ lua_number2str converts a number to a string.
@@ LUAI_MAXNUMBER2STR is maximum size of previous conversion.
@@ lua_str2number converts a string to a number.
*/
#define LUA_NUMBER_SCAN		"%lf"
#define LUA_NUMBER_FMT		"%.14g"
#define lua_number2str(s,n)	sprintf((s), LUA_NUMBER_FMT, (n))
#define LUAI_MAXNUMBER2STR	32 /* 16 digits, sign, point, and \0 */
#define lua_str2number(s,p)	strtod((s), (p))


/*
@@ The luai_num* macros define the primitive operations over numbers.
*/
#if defined(LUA_CORE)
#include <math.h>
#define luai_numadd(a,b)	((a)+(b))
#define luai_numsub(a,b)	((a)-(b))
#define luai_nummul(a,b)	((a)*(b))
#define luai_numdiv(a,b)	((a)/(b))
#define luai_nummod(a,b)	((a) - floor((a)/(b))*(b))
#define luai_numpow(a,b)	(pow(a,b))
#define luai_numunm(a)		(-(a))
#define luai_numeq(a,b)		((a)==(b))
#define luai_numlt(a,b)		((a)<(b))
#define luai_numle(a,b)		((a)<=(b))
#define luai_numisnan(a)	(!luai_numeq((a), (a)))
#endif


/*
@@ lua_number2int is a macro to convert lua_Number to int.
@@ lua_number2integer is a macro to convert lua_Number to lua_Integer.
** CHANGE them if you know a faster way to convert a lua_Number to
** int (with any rounding method and without throwing errors) in your
** system. In Pentium machines, a naive typecast from double to int
** in C is extremely slow, so any alternative is worth trying.
*/

/* On a Pentium, resort to a trick */
#if defined(LUA_NUMBER_DOUBLE) && !defined(LUA_ANSI) && !defined(__SSE2__) && \
    (defined(__i386) || defined (_M_IX86) || defined(__i386__))

/* On a Microsoft compiler, use assembler */
#if defined(_MSC_VER)

#define lua_number2int(i,d)   __asm fld d   __asm fistp i
#define lua_number2integer(i,n)		lua_number2int(i, n)

/* the next trick should work on any Pentium, but sometimes clashes
   with a DirectX idiosyncrasy */
#else

union luai_Cast { double l_d; long l_l; };
#define lua_number2int(i,d) \
  { volatile union luai_Cast u; u.l_d = (d) + 6755399441055744.0; (i) = u.l_l; }
#define lua_number2integer(i,n)		lua_number2int(i, n)

#endif


/* this option always works, but may be slow */
#else
#define lua_number2int(i,d)	((i)=(int)(d))
#define lua_number2integer(i,d)	((i)=(lua_Integer)(d))

#endif

/* }================================================================== */


/*
@@ LUAI_USER_ALIGNMENT_T is a type that requires maximum alignment.
** CHANGE it if your system requires alignments larger than double. (For
** instance, if your system supports long doubles and they must be
** aligned in 16-byte boundaries, then you should add long double in the
** union.) Probably you do not need to change this.
*/
#define LUAI_USER_ALIGNMENT_T	union { double u; void *s; long l; }


/*
@@ LUAI_THROW/LUAI_TRY define how Lua does exception handling.
** CHANGE them if you prefer to use longjmp/setjmp even with C++
** or if want/don't to use _longjmp/_setjmp instead of regular
** longjmp/setjmp. By default, Lua handles errors with exceptions when
** compiling as C++ code, with _longjmp/_setjmp when asked to use them,
** and with longjmp/setjmp otherwise.
*/
#if defined(__cplusplus)
/* C++ exceptions */
#define LUAI_THROW(L,c)	throw(c)
#define LUAI_TRY(L,c,a)	try { a } catch(...) \
	{ if ((c)->status == 0) (c)->status = -1; }
#define luai_jmpbuf	int  /* dummy variable */

#elif defined(LUA_USE_ULONGJMP)
/* in Unix, try _longjmp/_setjmp (more efficient) */
#define LUAI_THROW(L,c)	_longjmp((c)->b, 1)
#define LUAI_TRY(L,c,a)	if (_setjmp((c)->b) == 0) { a }
#define luai_jmpbuf	jmp_buf

#else
/* default handling with long jumps */
#define LUAI_THROW(L,c)	longjmp((c)->b, 1)
#define LUAI_TRY(L,c,a)	if (setjmp((c)->b) == 0) { a }
#define luai_jmpbuf	jmp_buf

#endif


/*
@@ LUA_MAXCAPTURES is the maximum number of captures that a pattern
@* can do during pattern-matching.
** CHANGE it if you need more captures. This limit is arbitrary.
*/
#define LUA_MAXCAPTURES		32


/*
@@ lua_tmpnam is the function that the OS library uses to create a
@* temporary name.
@@ LUA_TMPNAMBUFSIZE is the maximum size of a name created by lua_tmpnam.
** CHANGE them if you have an alternative to tmpnam (which is considered
** insecure) or if you want the original tmpnam anyway.  By default, Lua
** uses tmpnam except when POSIX is available, where it uses mkstemp.
*/
#if defined(loslib_c) || defined(luaall_c)

#if defined(LUA_USE_MKSTEMP)
#include <unistd.h>
#define LUA_TMPNAMBUFSIZE	32
#define lua_tmpnam(b,e)	{ \
	strcpy(b, "/tmp/lua_XXXXXX"); \
	e = mkstemp(b); \
	if (e != -1) close(e); \
	e = (e == -1); }

#else
#define LUA_TMPNAMBUFSIZE	L_tmpnam
#define lua_tmpnam(b,e)		{ e = (tmpnam(b) == NULL); }
#endif

#endif


/*
@@ lua_popen spawns a new process connected to the current one through
@* the file streams.
** CHANGE it if you have a way to implement it in your system.
*/
#if defined(LUA_USE_POPEN)

#define lua_popen(L,c,m)	((void)L, fflush(NULL), popen(c,m))
#define lua_pclose(L,file)	((void)L, (pclose(file) != -1))

#elif defined(LUA_WIN)

#define lua_popen(L,c,m)	((void)L, _popen(c,m))
#define lua_pclose(L,file)	((void)L, (_pclose(file) != -1))

#else

#define lua_popen(L,c,m)	((void)((void)c, m),  \
		luaL_error(L, LUA_QL("popen") " not supported"), (FILE*)0)
#define lua_pclose(L,file)		((void)((void)L, file), 0)

#endif

/*
@@ LUA_DL_* define which dynamic-library system Lua should use.
** CHANGE here if Lua has problems choosing the appropriate
** dynamic-library system for your platform (either Windows' DLL, Mac's
** dyld, or Unix's dlopen). If your system is some kind of Unix, there
** is a good chance that it has dlopen, so LUA_DL_DLOPEN will work for
** it.  To use dlopen you also need to adapt the src/Makefile (probably
** adding -ldl to the linker options), so Lua does not select it
** automatically.  (When you change the makefile to add -ldl, you must
** also add -DLUA_USE_DLOPEN.)
** If you do not want any kind of dynamic library, undefine all these
** options.
** By default, _WIN32 gets LUA_DL_DLL and MAC OS X gets LUA_DL_DYLD.
*/
#if defined(LUA_USE_DLOPEN)
#define LUA_DL_DLOPEN
#endif

#if defined(LUA_WIN)
#define LUA_DL_DLL
#endif


/*
@@ LUAI_EXTRASPACE allows you to add user-specific data in a lua_State
@* (the data goes just *before* the lua_State pointer).
** CHANGE (define) this if you really need that. This value must be
** a multiple of the maximum alignment required for your machine.
*/
#define LUAI_EXTRASPACE		0


/*
@@ luai_userstate* allow user-specific actions on threads.
** CHANGE them if you defined LUAI_EXTRASPACE and need to do something
** extra when a thread is created/deleted/resumed/yielded.
*/
#define luai_userstateopen(L)		((void)L)
#define luai_userstateclose(L)		((void)L)
#define luai_userstatethread(L,L1)	((void)L)
#define luai_userstatefree(L)		((void)L)
#define luai_userstateresume(L,n)	((void)L)
#define luai_userstateyield(L,n)	((void)L)


/*
@@ LUA_INTFRMLEN is the length modifier for integer conversions
@* in 'string.format'.
@@ LUA_INTFRM_T is the integer type correspoding to the previous length
@* modifier.
** CHANGE them if your system supports long long or does not support long.
*/

#if defined(LUA_USELONGLONG)

#define LUA_INTFRMLEN		"ll"
#define LUA_INTFRM_T		long long

#else

#define LUA_INTFRMLEN		"l"
#define LUA_INTFRM_T		long

#endif



/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/



#endif

