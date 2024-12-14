/*
** $Id: luaconf.h,v 1.176.1.1 2013/04/12 18:48:47 roberto Exp roberto $
** Configuration file for Lua
** See Copyright Notice in lua.h
*/


#ifndef lconfig_h
#define lconfig_h

#include <limits.h>
#include <stddef.h>

// THE FOLLOWING IS ADDED BY TELLTALE EDITOR.

#define lua_absindex  L523_lua_absindex
#define lua_arith  L523_lua_arith
#define lua_atpanic  L523_lua_atpanic
#define lua_callk  L523_lua_callk
#define lua_checkstack  L523_lua_checkstack
#define lua_compare  L523_lua_compare
#define lua_concat  L523_lua_concat
#define lua_copy  L523_lua_copy
#define lua_createtable  L523_lua_createtable
#define lua_dump  L523_lua_dump
#define lua_error  L523_lua_error
#define lua_gc  L523_lua_gc
#define lua_getallocf  L523_lua_getallocf
#define lua_getctx  L523_lua_getctx
#define lua_getfield  L523_lua_getfield
#define lua_getglobal  L523_lua_getglobal
#define lua_getmetatable  L523_lua_getmetatable
#define lua_gettable  L523_lua_gettable
#define lua_gettop  L523_lua_gettop
#define lua_getupvalue  L523_lua_getupvalue
#define lua_getuservalue  L523_lua_getuservalue
#define lua_insert  L523_lua_insert
#define lua_iscfunction  L523_lua_iscfunction
#define lua_isnumber  L523_lua_isnumber
#define lua_isstring  L523_lua_isstring
#define lua_isuserdata  L523_lua_isuserdata
#define lua_len  L523_lua_len
#define lua_load  L523_lua_load
#define lua_newuserdata  L523_lua_newuserdata
#define lua_next  L523_lua_next
#define lua_pcallk  L523_lua_pcallk
#define lua_pushboolean  L523_lua_pushboolean
#define lua_pushcclosure  L523_lua_pushcclosure
#define lua_pushfstring  L523_lua_pushfstring
#define lua_pushinteger  L523_lua_pushinteger
#define lua_pushlightuserdata  L523_lua_pushlightuserdata
#define lua_pushlstring  L523_lua_pushlstring
#define lua_pushnil  L523_lua_pushnil
#define lua_pushnumber  L523_lua_pushnumber
#define lua_pushstring  L523_lua_pushstring
#define lua_pushthread  L523_lua_pushthread
#define lua_pushunsigned  L523_lua_pushunsigned
#define lua_pushvalue  L523_lua_pushvalue
#define lua_pushvfstring  L523_lua_pushvfstring
#define lua_rawequal  L523_lua_rawequal
#define lua_rawget  L523_lua_rawget
#define lua_rawgeti  L523_lua_rawgeti
#define lua_rawgetp  L523_lua_rawgetp
#define lua_rawlen  L523_lua_rawlen
#define lua_rawset  L523_lua_rawset
#define lua_rawseti  L523_lua_rawseti
#define lua_rawsetp  L523_lua_rawsetp
#define lua_remove  L523_lua_remove
#define lua_replace  L523_lua_replace
#define lua_setallocf  L523_lua_setallocf
#define lua_setfield  L523_lua_setfield
#define lua_setglobal  L523_lua_setglobal
#define lua_setmetatable  L523_lua_setmetatable
#define lua_settable  L523_lua_settable
#define lua_settop  L523_lua_settop
#define lua_setupvalue  L523_lua_setupvalue
#define lua_setuservalue  L523_lua_setuservalue
#define lua_status  L523_lua_status
#define lua_toboolean  L523_lua_toboolean
#define lua_tocfunction  L523_lua_tocfunction
#define lua_tointegerx  L523_lua_tointegerx
#define lua_tolstring  L523_lua_tolstring
#define lua_tonumberx  L523_lua_tonumberx
#define lua_topointer  L523_lua_topointer
#define lua_tothread  L523_lua_tothread
#define lua_tounsignedx  L523_lua_tounsignedx
#define lua_touserdata  L523_lua_touserdata
#define lua_type  L523_lua_type
#define lua_typename  L523_lua_typename
#define lua_upvalueid  L523_lua_upvalueid
#define lua_upvaluejoin  L523_lua_upvaluejoin
#define lua_version  L523_lua_version
#define lua_xmove  L523_lua_xmove
#define luaL_addlstring  L523_luaL_addlstring
#define luaL_addstring  L523_luaL_addstring
#define luaL_addvalue  L523_luaL_addvalue
#define luaL_argerror  L523_luaL_argerror
#define luaL_buffinit  L523_luaL_buffinit
#define luaL_buffinitsize  L523_luaL_buffinitsize
#define luaL_callmeta  L523_luaL_callmeta
#define luaL_checkany  L523_luaL_checkany
#define luaL_checkinteger  L523_luaL_checkinteger
#define luaL_checklstring  L523_luaL_checklstring
#define luaL_checknumber  L523_luaL_checknumber
#define luaL_checkoption  L523_luaL_checkoption
#define luaL_checkstack  L523_luaL_checkstack
#define luaL_checktype  L523_luaL_checktype
#define luaL_checkudata  L523_luaL_checkudata
#define luaL_checkunsigned  L523_luaL_checkunsigned
#define luaL_checkversion_  L523_luaL_checkversion_
#define luaL_error  L523_luaL_error
#define luaL_execresult  L523_luaL_execresult
#define luaL_fileresult  L523_luaL_fileresult
#define luaL_getmetafield  L523_luaL_getmetafield
#define luaL_getsubtable  L523_luaL_getsubtable
#define luaL_gsub  L523_luaL_gsub
#define luaL_len  L523_luaL_len
#define luaL_loadbufferx  L523_luaL_loadbufferx
#define luaL_loadfilex  L523_luaL_loadfilex
#define luaL_loadstring  L523_luaL_loadstring
#define luaL_newmetatable  L523_luaL_newmetatable
#define luaL_newstate  L523_luaL_newstate
#define luaL_optinteger  L523_luaL_optinteger
#define luaL_optlstring  L523_luaL_optlstring
#define luaL_optnumber  L523_luaL_optnumber
#define luaL_optunsigned  L523_luaL_optunsigned
#define luaL_prepbuffsize  L523_luaL_prepbuffsize
#define luaL_pushresult  L523_luaL_pushresult
#define luaL_pushresultsize  L523_luaL_pushresultsize
#define luaL_ref  L523_luaL_ref
#define luaL_requiref  L523_luaL_requiref
#define luaL_setfuncs  L523_luaL_setfuncs
#define luaL_setmetatable  L523_luaL_setmetatable
#define luaL_testudata  L523_luaL_testudata
#define luaL_tolstring  L523_luaL_tolstring
#define luaL_traceback  L523_luaL_traceback
#define luaL_unref  L523_luaL_unref
#define luaL_where  L523_luaL_where
#define luaopen_base  L523_luaopen_base
#define luaK_checkstack  L523_luaK_checkstack
#define luaK_codeABC  L523_luaK_codeABC
#define luaK_codeABx  L523_luaK_codeABx
#define luaK_codek  L523_luaK_codek
#define luaK_concat  L523_luaK_concat
#define luaK_dischargevars  L523_luaK_dischargevars
#define luaK_exp2RK  L523_luaK_exp2RK
#define luaK_exp2anyreg  L523_luaK_exp2anyreg
#define luaK_exp2anyregup  L523_luaK_exp2anyregup
#define luaK_exp2nextreg  L523_luaK_exp2nextreg
#define luaK_exp2val  L523_luaK_exp2val
#define luaK_fixline  L523_luaK_fixline
#define luaK_getlabel  L523_luaK_getlabel
#define luaK_goiffalse  L523_luaK_goiffalse
#define luaK_goiftrue  L523_luaK_goiftrue
#define luaK_indexed  L523_luaK_indexed
#define luaK_infix  L523_luaK_infix
#define luaK_jump  L523_luaK_jump
#define luaK_nil  L523_luaK_nil
#define luaK_numberK  L523_luaK_numberK
#define luaK_patchclose  L523_luaK_patchclose
#define luaK_patchlist  L523_luaK_patchlist
#define luaK_patchtohere  L523_luaK_patchtohere
#define luaK_posfix  L523_luaK_posfix
#define luaK_prefix  L523_luaK_prefix
#define luaK_reserveregs  L523_luaK_reserveregs
#define luaK_ret  L523_luaK_ret
#define luaK_self  L523_luaK_self
#define luaK_setlist  L523_luaK_setlist
#define luaK_setoneret  L523_luaK_setoneret
#define luaK_setreturns  L523_luaK_setreturns
#define luaK_storevar  L523_luaK_storevar
#define luaK_stringK  L523_luaK_stringK
#define luaopen_coroutine  L523_luaopen_coroutine
#define luaopen_debug  L523_luaopen_debug
#define luaG_aritherror  L523_luaG_aritherror
#define luaG_concaterror  L523_luaG_concaterror
#define luaG_errormsg  L523_luaG_errormsg
#define luaG_ordererror  L523_luaG_ordererror
#define luaG_runerror  L523_luaG_runerror
#define luaG_typeerror  L523_luaG_typeerror
#define lua_gethook  L523_lua_gethook
#define lua_gethookcount  L523_lua_gethookcount
#define lua_gethookmask  L523_lua_gethookmask
#define lua_getinfo  L523_lua_getinfo
#define lua_getlocal  L523_lua_getlocal
#define lua_getstack  L523_lua_getstack
#define lua_sethook  L523_lua_sethook
#define lua_setlocal  L523_lua_setlocal
#define luaD_call  L523_luaD_call
#define luaD_growstack  L523_luaD_growstack
#define luaD_hook  L523_luaD_hook
#define luaD_pcall  L523_luaD_pcall
#define luaD_poscall  L523_luaD_poscall
#define luaD_precall  L523_luaD_precall
#define luaD_protectedparser  L523_luaD_protectedparser
#define luaD_rawrunprotected  L523_luaD_rawrunprotected
#define luaD_reallocstack  L523_luaD_reallocstack
#define luaD_shrinkstack  L523_luaD_shrinkstack
#define luaD_throw  L523_luaD_throw
#define lua_resume  L523_lua_resume
#define lua_yieldk  L523_lua_yieldk
#define luaU_dump  L523_luaU_dump
#define luaF_close  L523_luaF_close
#define luaF_findupval  L523_luaF_findupval
#define luaF_freeproto  L523_luaF_freeproto
#define luaF_freeupval  L523_luaF_freeupval
#define luaF_getlocalname  L523_luaF_getlocalname
#define luaF_newCclosure  L523_luaF_newCclosure
#define luaF_newLclosure  L523_luaF_newLclosure
#define luaF_newproto  L523_luaF_newproto
#define luaF_newupval  L523_luaF_newupval
#define luaC_barrier_  L523_luaC_barrier_
#define luaC_barrierback_  L523_luaC_barrierback_
#define luaC_barrierproto_  L523_luaC_barrierproto_
#define luaC_changemode  L523_luaC_changemode
#define luaC_checkfinalizer  L523_luaC_checkfinalizer
#define luaC_checkupvalcolor  L523_luaC_checkupvalcolor
#define luaC_forcestep  L523_luaC_forcestep
#define luaC_freeallobjects  L523_luaC_freeallobjects
#define luaC_fullgc  L523_luaC_fullgc
#define luaC_newobj  L523_luaC_newobj
#define luaC_runtilstate  L523_luaC_runtilstate
#define luaC_step  L523_luaC_step
#define luaL_openlibs  L523_luaL_openlibs
#define luaopen_io  L523_luaopen_io
#define luaX_init  L523_luaX_init
#define luaX_lookahead  L523_luaX_lookahead
#define luaX_newstring  L523_luaX_newstring
#define luaX_next  L523_luaX_next
#define luaX_setinput  L523_luaX_setinput
#define luaX_syntaxerror  L523_luaX_syntaxerror
#define luaX_token2str  L523_luaX_token2str
#define luaopen_math  L523_luaopen_math
#define luaM_growaux_  L523_luaM_growaux_
#define luaM_realloc_  L523_luaM_realloc_
#define luaM_toobig  L523_luaM_toobig
#define luaopen_package  L523_luaopen_package
#define luaO_arith  L523_luaO_arith
#define luaO_ceillog2  L523_luaO_ceillog2
#define luaO_chunkid  L523_luaO_chunkid
#define luaO_fb2int  L523_luaO_fb2int
#define luaO_hexavalue  L523_luaO_hexavalue
#define luaO_int2fb  L523_luaO_int2fb
#define luaO_pushfstring  L523_luaO_pushfstring
#define luaO_pushvfstring  L523_luaO_pushvfstring
#define luaO_str2d  L523_luaO_str2d
#define luaopen_os  L523_luaopen_os
#define luaY_parser  L523_luaY_parser
#define luaE_extendCI  L523_luaE_extendCI
#define luaE_freeCI  L523_luaE_freeCI
#define luaE_freethread  L523_luaE_freethread
#define luaE_setdebt  L523_luaE_setdebt
#define lua_close  L523_lua_close
#define lua_newstate  L523_lua_newstate
#define lua_newthread  L523_lua_newthread
#define luaS_eqlngstr  L523_luaS_eqlngstr
#define luaS_eqstr  L523_luaS_eqstr
#define luaS_hash  L523_luaS_hash
#define luaS_new  L523_luaS_new
#define luaS_newlstr  L523_luaS_newlstr
#define luaS_newudata  L523_luaS_newudata
#define luaS_resize  L523_luaS_resize
#define luaopen_string  L523_luaopen_string
#define luaH_free  L523_luaH_free
#define luaH_get  L523_luaH_get
#define luaH_getint  L523_luaH_getint
#define luaH_getn  L523_luaH_getn
#define luaH_getstr  L523_luaH_getstr
#define luaH_new  L523_luaH_new
#define luaH_newkey  L523_luaH_newkey
#define luaH_next  L523_luaH_next
#define luaH_resize  L523_luaH_resize
#define luaH_resizearray  L523_luaH_resizearray
#define luaH_set  L523_luaH_set
#define luaH_setint  L523_luaH_setint
#define luaopen_table  L523_luaopen_table
#define luaT_gettm  L523_luaT_gettm
#define luaT_gettmbyobj  L523_luaT_gettmbyobj
#define luaT_init  L523_luaT_init
#define luaU_header  L523_luaU_header
#define luaU_undump  L523_luaU_undump
#define luaV_arith  L523_luaV_arith
#define luaV_concat  L523_luaV_concat
#define luaV_equalobj_  L523_luaV_equalobj_
#define luaV_execute  L523_luaV_execute
#define luaV_finishOp  L523_luaV_finishOp
#define luaV_gettable  L523_luaV_gettable
#define luaV_lessequal  L523_luaV_lessequal
#define luaV_lessthan  L523_luaV_lessthan
#define luaV_objlen  L523_luaV_objlen
#define luaV_settable  L523_luaV_settable
#define luaV_tonumber  L523_luaV_tonumber
#define luaV_tostring  L523_luaV_tostring
#define luaZ_fill  L523_luaZ_fill
#define luaZ_init  L523_luaZ_init
#define luaZ_openspace  L523_luaZ_openspace
#define luaZ_read  L523_luaZ_read

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
#if !defined(LUA_ANSI) && defined(__STRICT_ANSI__)
#define LUA_ANSI
#endif


#if !defined(LUA_ANSI) && defined(_WIN32) && !defined(_WIN32_WCE)
#define LUA_WIN		/* enable goodies for regular Windows platforms */
#endif

#if defined(LUA_WIN)
#define LUA_DL_DLL
#define LUA_USE_AFORMAT		/* assume 'printf' handles 'aA' specifiers */
#endif



#if defined(LUA_USE_LINUX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* needs an extra library: -ldl */
#define LUA_USE_READLINE	/* needs some extra libraries */
#define LUA_USE_STRTODHEX	/* assume 'strtod' handles hex formats */
#define LUA_USE_AFORMAT		/* assume 'printf' handles 'aA' specifiers */
#define LUA_USE_LONGLONG	/* assume support for long long */
#endif

#if defined(LUA_USE_MACOSX)
#define LUA_USE_POSIX
#define LUA_USE_DLOPEN		/* does not need -ldl */
#define LUA_USE_READLINE	/* needs an extra library: -lreadline */
#define LUA_USE_STRTODHEX	/* assume 'strtod' handles hex formats */
#define LUA_USE_AFORMAT		/* assume 'printf' handles 'aA' specifiers */
#define LUA_USE_LONGLONG	/* assume support for long long */
#endif



/*
@@ LUA_USE_POSIX includes all functionality listed as X/Open System
@* Interfaces Extension (XSI).
** CHANGE it (define it) if your system is XSI compatible.
*/
#if defined(LUA_USE_POSIX)
#define LUA_USE_MKSTEMP
#define LUA_USE_ISATTY
#define LUA_USE_POPEN
#define LUA_USE_ULONGJMP
#define LUA_USE_GMTIME_R
#endif



/*
@@ LUA_PATH_DEFAULT is the default path that Lua uses to look for
@* Lua libraries.
@@ LUA_CPATH_DEFAULT is the default path that Lua uses to look for
@* C libraries.
** CHANGE them if your machine has a non-conventional directory
** hierarchy or if you want to install your libraries in
** non-conventional directories.
*/
#if defined(_WIN32)	/* { */
/*
** In Windows, any exclamation mark ('!') in the path is replaced by the
** path of the directory of the executable file of the current process.
*/
#define LUA_LDIR	"!\\lua\\"
#define LUA_CDIR	"!\\"
#define LUA_PATH_DEFAULT  \
		LUA_LDIR"?.lua;"  LUA_LDIR"?\\init.lua;" \
		LUA_CDIR"?.lua;"  LUA_CDIR"?\\init.lua;" ".\\?.lua"
#define LUA_CPATH_DEFAULT \
		LUA_CDIR"?.dll;" LUA_CDIR"loadall.dll;" ".\\?.dll"

#else			/* }{ */

#define LUA_VDIR	LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "/"
#define LUA_ROOT	"/usr/local/"
#define LUA_LDIR	LUA_ROOT "share/lua/" LUA_VDIR
#define LUA_CDIR	LUA_ROOT "lib/lua/" LUA_VDIR
#define LUA_PATH_DEFAULT  \
		LUA_LDIR"?.lua;"  LUA_LDIR"?/init.lua;" \
		LUA_CDIR"?.lua;"  LUA_CDIR"?/init.lua;" "./?.lua"
#define LUA_CPATH_DEFAULT \
		LUA_CDIR"?.so;" LUA_CDIR"loadall.so;" "./?.so"
#endif			/* } */


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
@@ LUA_ENV is the name of the variable that holds the current
@@ environment, used to access global names.
** CHANGE it if you do not like this name.
*/
#define LUA_ENV		"_ENV"


/*
@@ LUA_API is a mark for all core API functions.
@@ LUALIB_API is a mark for all auxiliary library functions.
@@ LUAMOD_API is a mark for all standard library opening functions.
** CHANGE them if you need to define those functions in some special way.
** For instance, if you want to create one Windows DLL with the core and
** the libraries, you may want to use the following definition (define
** LUA_BUILD_AS_DLL to get it).
*/
#if defined(LUA_BUILD_AS_DLL)	/* { */

#if defined(LUA_CORE) || defined(LUA_LIB)	/* { */
#define LUA_API __declspec(dllexport)
#else						/* }{ */
#define LUA_API __declspec(dllimport)
#endif						/* } */

#else				/* }{ */

#define LUA_API		extern

#endif				/* } */


/* more often than not the libs go together with the core */
#define LUALIB_API	LUA_API
#define LUAMOD_API	LUALIB_API


/*
@@ LUAI_FUNC is a mark for all extern functions that are not to be
@* exported to outside modules.
@@ LUAI_DDEF and LUAI_DDEC are marks for all extern (const) variables
@* that are not to be exported to outside modules (LUAI_DDEF for
@* definitions and LUAI_DDEC for declarations).
** CHANGE them if you need to mark them in some special way. Elf/gcc
** (versions 3.2 and later) mark them as "hidden" to optimize access
** when Lua is compiled as a shared library. Not all elf targets support
** this attribute. Unfortunately, gcc does not offer a way to check
** whether the target offers that support, and those without support
** give a warning about it. To avoid these warnings, change to the
** default definition.
*/
#if defined(__GNUC__) && ((__GNUC__*100 + __GNUC_MINOR__) >= 302) && \
    defined(__ELF__)		/* { */
#define LUAI_FUNC	__attribute__((visibility("hidden"))) extern
#define LUAI_DDEC	LUAI_FUNC
#define LUAI_DDEF	/* empty */

#else				/* }{ */
#define LUAI_FUNC	extern
#define LUAI_DDEC	extern
#define LUAI_DDEF	/* empty */
#endif				/* } */



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
@@ luai_writestring/luai_writeline define how 'print' prints its results.
** They are only used in libraries and the stand-alone program. (The #if
** avoids including 'stdio.h' everywhere.)
*/
#if defined(LUA_LIB) || defined(lua_c)
#include <stdio.h>
#define luai_writestring(s,l)	fwrite((s), sizeof(char), (l), stdout)
#define luai_writeline()	(luai_writestring("\n", 1), fflush(stdout))
#endif

/*
@@ luai_writestringerror defines how to print error messages.
** (A format string with one argument is enough for Lua...)
*/
#define luai_writestringerror(s,p) \
	(fprintf(stderr, (s), (p)), fflush(stderr))


/*
@@ LUAI_MAXSHORTLEN is the maximum length for short strings, that is,
** strings that are internalized. (Cannot be smaller than reserved words
** or tags for metamethods, as these strings must be internalized;
** #("function") = 8, #("__newindex") = 10.)
*/
#define LUAI_MAXSHORTLEN        40



/*
** {==================================================================
** Compatibility with previous versions
** ===================================================================
*/

/*
@@ LUA_COMPAT_ALL controls all compatibility options.
** You can define it to get all options, or change specific options
** to fit your specific needs.
*/
#if defined(LUA_COMPAT_ALL)	/* { */

/*
@@ LUA_COMPAT_UNPACK controls the presence of global 'unpack'.
** You can replace it with 'table.unpack'.
*/
#define LUA_COMPAT_UNPACK

/*
@@ LUA_COMPAT_LOADERS controls the presence of table 'package.loaders'.
** You can replace it with 'package.searchers'.
*/
#define LUA_COMPAT_LOADERS

/*
@@ macro 'lua_cpcall' emulates deprecated function lua_cpcall.
** You can call your C function directly (with light C functions).
*/
#define lua_cpcall(L,f,u)  \
	(lua_pushcfunction(L, (f)), \
	 lua_pushlightuserdata(L,(u)), \
	 lua_pcall(L,1,0,0))


/*
@@ LUA_COMPAT_LOG10 defines the function 'log10' in the math library.
** You can rewrite 'log10(x)' as 'log(x, 10)'.
*/
#define LUA_COMPAT_LOG10

/*
@@ LUA_COMPAT_LOADSTRING defines the function 'loadstring' in the base
** library. You can rewrite 'loadstring(s)' as 'load(s)'.
*/
#define LUA_COMPAT_LOADSTRING

/*
@@ LUA_COMPAT_MAXN defines the function 'maxn' in the table library.
*/
#define LUA_COMPAT_MAXN

/*
@@ The following macros supply trivial compatibility for some
** changes in the API. The macros themselves document how to
** change your code to avoid using them.
*/
#define lua_strlen(L,i)		lua_rawlen(L, (i))

#define lua_objlen(L,i)		lua_rawlen(L, (i))

#define lua_equal(L,idx1,idx2)		lua_compare(L,(idx1),(idx2),LUA_OPEQ)
#define lua_lessthan(L,idx1,idx2)	lua_compare(L,(idx1),(idx2),LUA_OPLT)

/*
@@ LUA_COMPAT_MODULE controls compatibility with previous
** module functions 'module' (Lua) and 'luaL_register' (C).
*/
#define LUA_COMPAT_MODULE

#endif				/* } */

/* }================================================================== */



/*
@@ LUAI_BITSINT defines the number of bits in an int.
** CHANGE here if Lua cannot automatically detect the number of bits of
** your machine. Probably you do not need to change this.
*/
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760		/* { */
#define LUAI_BITSINT	16
#elif INT_MAX > 2147483640L	/* }{ */
/* int has at least 32 bits */
#define LUAI_BITSINT	32
#else				/* }{ */
#error "you must define LUA_BITSINT with number of bits in an integer"
#endif				/* } */


/*
@@ LUA_INT32 is a signed integer with exactly 32 bits.
@@ LUAI_UMEM is an unsigned integer big enough to count the total
@* memory used by Lua.
@@ LUAI_MEM is a signed integer big enough to count the total memory
@* used by Lua.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. Probably you do not need to change
** this.
*/
#if LUAI_BITSINT >= 32		/* { */
#define LUA_INT32	int
#define LUAI_UMEM	size_t
#define LUAI_MEM	ptrdiff_t
#else				/* }{ */
/* 16-bit ints */
#define LUA_INT32	long
#define LUAI_UMEM	unsigned long
#define LUAI_MEM	long
#endif				/* } */


/*
@@ LUAI_MAXSTACK limits the size of the Lua stack.
** CHANGE it if you need a different limit. This limit is arbitrary;
** its only purpose is to stop Lua from consuming unlimited stack
** space (and to reserve some numbers for pseudo-indices).
*/
#if LUAI_BITSINT >= 32
#define LUAI_MAXSTACK		1000000
#else
#define LUAI_MAXSTACK		15000
#endif

/* reserve some space for error handling */
#define LUAI_FIRSTPSEUDOIDX	(-LUAI_MAXSTACK - 1000)




/*
@@ LUAL_BUFFERSIZE is the buffer size used by the lauxlib buffer system.
** CHANGE it if it uses too much C-stack space.
*/
#define LUAL_BUFFERSIZE		BUFSIZ




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
*/
#define LUA_NUMBER_SCAN		"%lf"
#define LUA_NUMBER_FMT		"%.14g"
#define lua_number2str(s,n)	sprintf((s), LUA_NUMBER_FMT, (n))
#define LUAI_MAXNUMBER2STR	32 /* 16 digits, sign, point, and \0 */


/*
@@ l_mathop allows the addition of an 'l' or 'f' to all math operations
*/
#define l_mathop(x)		(x)


/*
@@ lua_str2number converts a decimal numeric string to a number.
@@ lua_strx2number converts an hexadecimal numeric string to a number.
** In C99, 'strtod' does both conversions. C89, however, has no function
** to convert floating hexadecimal strings to numbers. For these
** systems, you can leave 'lua_strx2number' undefined and Lua will
** provide its own implementation.
*/
#define lua_str2number(s,p)	strtod((s), (p))

#if defined(LUA_USE_STRTODHEX)
#define lua_strx2number(s,p)	strtod((s), (p))
#endif


/*
@@ The luai_num* macros define the primitive operations over numbers.
*/

/* the following operations need the math library */
#if defined(lobject_c) || defined(lvm_c)
#include <math.h>
#define luai_nummod(L,a,b)	((a) - l_mathop(floor)((a)/(b))*(b))
#define luai_numpow(L,a,b)	(l_mathop(pow)(a,b))
#endif

/* these are quite standard operations */
#if defined(LUA_CORE)
#define luai_numadd(L,a,b)	((a)+(b))
#define luai_numsub(L,a,b)	((a)-(b))
#define luai_nummul(L,a,b)	((a)*(b))
#define luai_numdiv(L,a,b)	((a)/(b))
#define luai_numunm(L,a)	(-(a))
#define luai_numeq(a,b)		((a)==(b))
#define luai_numlt(L,a,b)	((a)<(b))
#define luai_numle(L,a,b)	((a)<=(b))
#define luai_numisnan(L,a)	(!luai_numeq((a), (a)))
#endif



/*
@@ LUA_INTEGER is the integral type used by lua_pushinteger/lua_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define LUA_INTEGER	ptrdiff_t

/*
@@ LUA_UNSIGNED is the integral type used by lua_pushunsigned/lua_tounsigned.
** It must have at least 32 bits.
*/
#define LUA_UNSIGNED	unsigned LUA_INT32



/*
** Some tricks with doubles
*/

#if defined(LUA_NUMBER_DOUBLE) && !defined(LUA_ANSI)	/* { */
/*
** The next definitions activate some tricks to speed up the
** conversion from doubles to integer types, mainly to LUA_UNSIGNED.
**
@@ LUA_MSASMTRICK uses Microsoft assembler to avoid clashes with a
** DirectX idiosyncrasy.
**
@@ LUA_IEEE754TRICK uses a trick that should work on any machine
** using IEEE754 with a 32-bit integer type.
**
@@ LUA_IEEELL extends the trick to LUA_INTEGER; should only be
** defined when LUA_INTEGER is a 32-bit integer.
**
@@ LUA_IEEEENDIAN is the endianness of doubles in your machine
** (0 for little endian, 1 for big endian); if not defined, Lua will
** check it dynamically for LUA_IEEE754TRICK (but not for LUA_NANTRICK).
**
@@ LUA_NANTRICK controls the use of a trick to pack all types into
** a single double value, using NaN values to represent non-number
** values. The trick only works on 32-bit machines (ints and pointers
** are 32-bit values) with numbers represented as IEEE 754-2008 doubles
** with conventional endianess (12345678 or 87654321), in CPUs that do
** not produce signaling NaN values (all NaNs are quiet).
*/

/* Microsoft compiler on a Pentium (32 bit) ? */
#if defined(LUA_WIN) && defined(_MSC_VER) && defined(_M_IX86)	/* { */

#define LUA_MSASMTRICK
#define LUA_IEEEENDIAN		0
#define LUA_NANTRICK


/* pentium 32 bits? */
#elif defined(__i386__) || defined(__i386) || defined(__X86__) /* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEELL
#define LUA_IEEEENDIAN		0
#define LUA_NANTRICK

/* pentium 64 bits? */
#elif defined(__x86_64)						/* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEEENDIAN		0

#elif defined(__POWERPC__) || defined(__ppc__)			/* }{ */

#define LUA_IEEE754TRICK
#define LUA_IEEEENDIAN		1

#else								/* }{ */

/* assume IEEE754 and a 32-bit integer type */
#define LUA_IEEE754TRICK

#endif								/* } */

#endif							/* } */

/* }================================================================== */




/* =================================================================== */

/*
** Local configuration. You can use this space to add your redefinitions
** without modifying the main part of the file.
*/



#endif

