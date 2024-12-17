/*
** $Id: lua.h,v 1.175b 2003/03/18 12:31:39 roberto Exp $
** Lua - An Extensible Extension Language
** Tecgraf: Computer Graphics Technology Group, PUC-Rio, Brazil
** http://www.lua.org	mailto:info@lua.org
** See Copyright Notice at the end of this file
*/


#ifndef lua_h
#define lua_h

#include <stdarg.h>
#include <stddef.h>

// BY TELLALE EDITOR

#define luaA_pushobject  L502_luaA_pushobject
#define luaC_collectgarbage  L502_luaC_collectgarbage
#define luaD_call  L502_luaD_call
#define luaD_growstack  L502_luaD_growstack
#define luaD_pcall  L502_luaD_pcall
#define luaD_protectedparser  L502_luaD_protectedparser
#define luaE_newthread  L502_luaE_newthread
#define luaF_newCclosure  L502_luaF_newCclosure
#define luaG_errormsg  L502_luaG_errormsg
#define luaH_get  L502_luaH_get
#define luaH_getnum  L502_luaH_getnum
#define luaH_new  L502_luaH_new
#define luaH_next  L502_luaH_next
#define luaH_set  L502_luaH_set
#define luaH_setnum  L502_luaH_setnum
#define luaO_pushvfstring  L502_luaO_pushvfstring
#define luaO_rawequalObj  L502_luaO_rawequalObj
#define luaS_newlstr  L502_luaS_newlstr
#define luaS_newudata  L502_luaS_newudata
#define luaT_typenames  L502_luaT_typenames
#define luaU_dump  L502_luaU_dump
#define luaV_concat  L502_luaV_concat
#define luaV_equalval  L502_luaV_equalval
#define luaV_gettable  L502_luaV_gettable
#define luaV_lessthan  L502_luaV_lessthan
#define luaV_settable  L502_luaV_settable
#define luaV_tonumber  L502_luaV_tonumber
#define luaV_tostring  L502_luaV_tostring
#define luaZ_init  L502_luaZ_init
#define luaZ_lookahead  L502_luaZ_lookahead
#define lua_atpanic  L502_lua_atpanic
#define lua_call  L502_lua_call
#define lua_checkstack  L502_lua_checkstack
#define lua_concat  L502_lua_concat
#define lua_cpcall  L502_lua_cpcall
#define lua_dump  L502_lua_dump
#define lua_equal  L502_lua_equal
#define lua_error  L502_lua_error
#define lua_getfenv  L502_lua_getfenv
#define lua_getgccount  L502_lua_getgccount
#define lua_getgcthreshold  L502_lua_getgcthreshold
#define lua_getmetatable  L502_lua_getmetatable
#define lua_gettable  L502_lua_gettable
#define lua_gettop  L502_lua_gettop
#define lua_getupvalue  L502_lua_getupvalue
#define lua_ident  L502_lua_ident
#define lua_insert  L502_lua_insert
#define lua_iscfunction  L502_lua_iscfunction
#define lua_isnumber  L502_lua_isnumber
#define lua_isstring  L502_lua_isstring
#define lua_isuserdata  L502_lua_isuserdata
#define lua_lessthan  L502_lua_lessthan
#define lua_load  L502_lua_load
#define lua_newtable  L502_lua_newtable
#define lua_newthread  L502_lua_newthread
#define lua_newuserdata  L502_lua_newuserdata
#define lua_next  L502_lua_next
#define lua_pcall  L502_lua_pcall
#define lua_pushboolean  L502_lua_pushboolean
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushfstring  L502_lua_pushfstring
#define lua_pushlightuserdata  L502_lua_pushlightuserdata
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushstring  L502_lua_pushstring
#define lua_pushupvalues  L502_lua_pushupvalues
#define lua_pushvalue  L502_lua_pushvalue
#define lua_pushvfstring  L502_lua_pushvfstring
#define lua_rawequal  L502_lua_rawequal
#define lua_rawget  L502_lua_rawget
#define lua_rawgeti  L502_lua_rawgeti
#define lua_rawset  L502_lua_rawset
#define lua_rawseti  L502_lua_rawseti
#define lua_remove  L502_lua_remove
#define lua_replace  L502_lua_replace
#define lua_setfenv  L502_lua_setfenv
#define lua_setgcthreshold  L502_lua_setgcthreshold
#define lua_setmetatable  L502_lua_setmetatable
#define lua_settable  L502_lua_settable
#define lua_settop  L502_lua_settop
#define lua_setupvalue  L502_lua_setupvalue
#define lua_strlen  L502_lua_strlen
#define lua_toboolean  L502_lua_toboolean
#define lua_tocfunction  L502_lua_tocfunction
#define lua_tonumber  L502_lua_tonumber
#define lua_topointer  L502_lua_topointer
#define lua_tostring  L502_lua_tostring
#define lua_tothread  L502_lua_tothread
#define lua_touserdata  L502_lua_touserdata
#define lua_type  L502_lua_type
#define lua_typename  L502_lua_typename
#define lua_version  L502_lua_version
#define lua_xmove  L502_lua_xmove
#define luaL_addlstring  L502_luaL_addlstring
#define luaL_addstring  L502_luaL_addstring
#define luaL_addvalue  L502_luaL_addvalue
#define luaL_argerror  L502_luaL_argerror
#define luaL_buffinit  L502_luaL_buffinit
#define luaL_callmeta  L502_luaL_callmeta
#define luaL_checkany  L502_luaL_checkany
#define luaL_checklstring  L502_luaL_checklstring
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_checkstack  L502_luaL_checkstack
#define luaL_checktype  L502_luaL_checktype
#define luaL_checkudata  L502_luaL_checkudata
#define luaL_error  L502_luaL_error
#define luaL_findstring  L502_luaL_findstring
#define luaL_getmetafield  L502_luaL_getmetafield
#define luaL_getmetatable  L502_luaL_getmetatable
#define luaL_getn  L502_luaL_getn
#define luaL_loadbuffer  L502_luaL_loadbuffer
#define luaL_loadfile  L502_luaL_loadfile
#define luaL_newmetatable  L502_luaL_newmetatable
#define luaL_openlib  L502_luaL_openlib
#define luaL_optlstring  L502_luaL_optlstring
#define luaL_optnumber  L502_luaL_optnumber
#define luaL_prepbuffer  L502_luaL_prepbuffer
#define luaL_pushresult  L502_luaL_pushresult
#define luaL_ref  L502_luaL_ref
#define luaL_setn  L502_luaL_setn
#define luaL_typerror  L502_luaL_typerror
#define luaL_unref  L502_luaL_unref
#define luaL_where  L502_luaL_where
#define lua_call  L502_lua_call
#define lua_checkstack  L502_lua_checkstack
#define lua_concat  L502_lua_concat
#define lua_dobuffer  L502_lua_dobuffer
#define lua_dofile  L502_lua_dofile
#define lua_dostring  L502_lua_dostring
#define lua_error  L502_lua_error
#define lua_getinfo  L502_lua_getinfo
#define lua_getmetatable  L502_lua_getmetatable
#define lua_getstack  L502_lua_getstack
#define lua_gettable  L502_lua_gettable
#define lua_gettop  L502_lua_gettop
#define lua_insert  L502_lua_insert
#define lua_isnumber  L502_lua_isnumber
#define lua_load  L502_lua_load
#define lua_newtable  L502_lua_newtable
#define lua_pcall  L502_lua_pcall
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushfstring  L502_lua_pushfstring
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushstring  L502_lua_pushstring
#define lua_pushvalue  L502_lua_pushvalue
#define lua_pushvfstring  L502_lua_pushvfstring
#define lua_rawget  L502_lua_rawget
#define lua_rawgeti  L502_lua_rawgeti
#define lua_rawset  L502_lua_rawset
#define lua_rawseti  L502_lua_rawseti
#define lua_remove  L502_lua_remove
#define lua_setmetatable  L502_lua_setmetatable
#define lua_settable  L502_lua_settable
#define lua_settop  L502_lua_settop
#define lua_strlen  L502_lua_strlen
#define lua_tonumber  L502_lua_tonumber
#define lua_tostring  L502_lua_tostring
#define lua_touserdata  L502_lua_touserdata
#define lua_type  L502_lua_type
#define lua_typename  L502_lua_typename
#define luaL_argerror  L502_luaL_argerror
#define luaL_callmeta  L502_luaL_callmeta
#define luaL_checkany  L502_luaL_checkany
#define luaL_checklstring  L502_luaL_checklstring
#define luaL_checkstack  L502_luaL_checkstack
#define luaL_checktype  L502_luaL_checktype
#define luaL_error  L502_luaL_error
#define luaL_getmetafield  L502_luaL_getmetafield
#define luaL_getn  L502_luaL_getn
#define luaL_loadbuffer  L502_luaL_loadbuffer
#define luaL_loadfile  L502_luaL_loadfile
#define luaL_openlib  L502_luaL_openlib
#define luaL_optlstring  L502_luaL_optlstring
#define luaL_optnumber  L502_luaL_optnumber
#define luaL_where  L502_luaL_where
#define lua_call  L502_lua_call
#define lua_checkstack  L502_lua_checkstack
#define lua_concat  L502_lua_concat
#define lua_error  L502_lua_error
#define lua_getfenv  L502_lua_getfenv
#define lua_getgccount  L502_lua_getgccount
#define lua_getgcthreshold  L502_lua_getgcthreshold
#define lua_getinfo  L502_lua_getinfo
#define lua_getmetatable  L502_lua_getmetatable
#define lua_getstack  L502_lua_getstack
#define lua_gettable  L502_lua_gettable
#define lua_gettop  L502_lua_gettop
#define lua_insert  L502_lua_insert
#define lua_iscfunction  L502_lua_iscfunction
#define lua_isnumber  L502_lua_isnumber
#define lua_isstring  L502_lua_isstring
#define lua_newtable  L502_lua_newtable
#define lua_newthread  L502_lua_newthread
#define lua_newuserdata  L502_lua_newuserdata
#define lua_next  L502_lua_next
#define lua_pcall  L502_lua_pcall
#define lua_pushboolean  L502_lua_pushboolean
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushstring  L502_lua_pushstring
#define lua_pushvalue  L502_lua_pushvalue
#define lua_rawequal  L502_lua_rawequal
#define lua_rawget  L502_lua_rawget
#define lua_rawgeti  L502_lua_rawgeti
#define lua_rawset  L502_lua_rawset
#define lua_replace  L502_lua_replace
#define lua_resume  L502_lua_resume
#define lua_setfenv  L502_lua_setfenv
#define lua_setgcthreshold  L502_lua_setgcthreshold
#define lua_setmetatable  L502_lua_setmetatable
#define lua_settable  L502_lua_settable
#define lua_settop  L502_lua_settop
#define lua_toboolean  L502_lua_toboolean
#define lua_tonumber  L502_lua_tonumber
#define lua_topointer  L502_lua_topointer
#define lua_tostring  L502_lua_tostring
#define lua_tothread  L502_lua_tothread
#define lua_touserdata  L502_lua_touserdata
#define lua_type  L502_lua_type
#define lua_typename  L502_lua_typename
#define lua_xmove  L502_lua_xmove
#define lua_yield  L502_lua_yield
#define luaopen_base  L502_luaopen_base
#define luaH_get  L502_luaH_get
#define luaH_set  L502_luaH_set
#define luaK_checkstack  L502_luaK_checkstack
#define luaK_code  L502_luaK_code
#define luaK_codeABC  L502_luaK_codeABC
#define luaK_codeABx  L502_luaK_codeABx
#define luaK_concat  L502_luaK_concat
#define luaK_dischargevars  L502_luaK_dischargevars
#define luaK_exp2RK  L502_luaK_exp2RK
#define luaK_exp2anyreg  L502_luaK_exp2anyreg
#define luaK_exp2nextreg  L502_luaK_exp2nextreg
#define luaK_exp2val  L502_luaK_exp2val
#define luaK_fixline  L502_luaK_fixline
#define luaK_getlabel  L502_luaK_getlabel
#define luaK_goiffalse  L502_luaK_goiffalse
#define luaK_goiftrue  L502_luaK_goiftrue
#define luaK_indexed  L502_luaK_indexed
#define luaK_infix  L502_luaK_infix
#define luaK_jump  L502_luaK_jump
#define luaK_nil  L502_luaK_nil
#define luaK_numberK  L502_luaK_numberK
#define luaK_patchlist  L502_luaK_patchlist
#define luaK_patchtohere  L502_luaK_patchtohere
#define luaK_posfix  L502_luaK_posfix
#define luaK_prefix  L502_luaK_prefix
#define luaK_reserveregs  L502_luaK_reserveregs
#define luaK_self  L502_luaK_self
#define luaK_setcallreturns  L502_luaK_setcallreturns
#define luaK_storevar  L502_luaK_storevar
#define luaK_stringK  L502_luaK_stringK
#define luaM_growaux  L502_luaM_growaux
#define luaP_opmodes  L502_luaP_opmodes
#define luaX_syntaxerror  L502_luaX_syntaxerror
#define luaL_argerror  L502_luaL_argerror
#define luaL_checkany  L502_luaL_checkany
#define luaL_checklstring  L502_luaL_checklstring
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_checktype  L502_luaL_checktype
#define luaL_openlib  L502_luaL_openlib
#define luaL_optlstring  L502_luaL_optlstring
#define luaL_optnumber  L502_luaL_optnumber
#define lua_call  L502_lua_call
#define lua_concat  L502_lua_concat
#define lua_dostring  L502_lua_dostring
#define lua_gethook  L502_lua_gethook
#define lua_gethookcount  L502_lua_gethookcount
#define lua_gethookmask  L502_lua_gethookmask
#define lua_getinfo  L502_lua_getinfo
#define lua_getlocal  L502_lua_getlocal
#define lua_getstack  L502_lua_getstack
#define lua_gettop  L502_lua_gettop
#define lua_getupvalue  L502_lua_getupvalue
#define lua_insert  L502_lua_insert
#define lua_iscfunction  L502_lua_iscfunction
#define lua_isnumber  L502_lua_isnumber
#define lua_isstring  L502_lua_isstring
#define lua_newtable  L502_lua_newtable
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushfstring  L502_lua_pushfstring
#define lua_pushlightuserdata  L502_lua_pushlightuserdata
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushstring  L502_lua_pushstring
#define lua_pushvalue  L502_lua_pushvalue
#define lua_rawget  L502_lua_rawget
#define lua_rawset  L502_lua_rawset
#define lua_sethook  L502_lua_sethook
#define lua_setlocal  L502_lua_setlocal
#define lua_settable  L502_lua_settable
#define lua_settop  L502_lua_settop
#define lua_setupvalue  L502_lua_setupvalue
#define lua_tonumber  L502_lua_tonumber
#define lua_tostring  L502_lua_tostring
#define lua_type  L502_lua_type
#define luaopen_debug  L502_luaopen_debug
#define luaA_pushobject  L502_luaA_pushobject
#define luaD_call  L502_luaD_call
#define luaD_growstack  L502_luaD_growstack
#define luaD_throw  L502_luaD_throw
#define luaF_getlocalname  L502_luaF_getlocalname
#define luaG_aritherror  L502_luaG_aritherror
#define luaG_checkcode  L502_luaG_checkcode
#define luaG_concaterror  L502_luaG_concaterror
#define luaG_errormsg  L502_luaG_errormsg
#define luaG_inithooks  L502_luaG_inithooks
#define luaG_ordererror  L502_luaG_ordererror
#define luaG_runerror  L502_luaG_runerror
#define luaG_typeerror  L502_luaG_typeerror
#define luaO_chunkid  L502_luaO_chunkid
#define luaO_pushfstring  L502_luaO_pushfstring
#define luaO_pushvfstring  L502_luaO_pushvfstring
#define luaO_rawequalObj  L502_luaO_rawequalObj
#define luaP_opmodes  L502_luaP_opmodes
#define luaT_typenames  L502_luaT_typenames
#define luaV_tonumber  L502_luaV_tonumber
#define lua_gethook  L502_lua_gethook
#define lua_gethookcount  L502_lua_gethookcount
#define lua_gethookmask  L502_lua_gethookmask
#define lua_getinfo  L502_lua_getinfo
#define lua_getlocal  L502_lua_getlocal
#define lua_getstack  L502_lua_getstack
#define lua_sethook  L502_lua_sethook
#define lua_setlocal  L502_lua_setlocal
#define luaC_collectgarbage  L502_luaC_collectgarbage
#define luaD_call  L502_luaD_call
#define luaD_callhook  L502_luaD_callhook
#define luaD_growstack  L502_luaD_growstack
#define luaD_pcall  L502_luaD_pcall
#define luaD_poscall  L502_luaD_poscall
#define luaD_precall  L502_luaD_precall
#define luaD_protectedparser  L502_luaD_protectedparser
#define luaD_rawrunprotected  L502_luaD_rawrunprotected
#define luaD_reallocCI  L502_luaD_reallocCI
#define luaD_reallocstack  L502_luaD_reallocstack
#define luaD_throw  L502_luaD_throw
#define luaF_close  L502_luaF_close
#define luaF_newLclosure  L502_luaF_newLclosure
#define luaG_runerror  L502_luaG_runerror
#define luaG_typeerror  L502_luaG_typeerror
#define luaH_new  L502_luaH_new
#define luaH_set  L502_luaH_set
#define luaH_setnum  L502_luaH_setnum
#define luaM_realloc  L502_luaM_realloc
#define luaS_newlstr  L502_luaS_newlstr
#define luaT_gettmbyobj  L502_luaT_gettmbyobj
#define luaU_undump  L502_luaU_undump
#define luaV_execute  L502_luaV_execute
#define luaY_parser  L502_luaY_parser
#define lua_resume  L502_lua_resume
#define lua_yield  L502_lua_yield
#define luaU_dump  L502_luaU_dump
#define luaU_endianness  L502_luaU_endianness
#define luaC_link  L502_luaC_link
#define luaF_close  L502_luaF_close
#define luaF_findupval  L502_luaF_findupval
#define luaF_freeclosure  L502_luaF_freeclosure
#define luaF_freeproto  L502_luaF_freeproto
#define luaF_getlocalname  L502_luaF_getlocalname
#define luaF_newCclosure  L502_luaF_newCclosure
#define luaF_newLclosure  L502_luaF_newLclosure
#define luaF_newproto  L502_luaF_newproto
#define luaM_realloc  L502_luaM_realloc
#define luaC_callGCTM  L502_luaC_callGCTM
#define luaC_collectgarbage  L502_luaC_collectgarbage
#define luaC_link  L502_luaC_link
#define luaC_separateudata  L502_luaC_separateudata
#define luaC_sweep  L502_luaC_sweep
#define luaD_call  L502_luaD_call
#define luaD_reallocCI  L502_luaD_reallocCI
#define luaD_reallocstack  L502_luaD_reallocstack
#define luaE_freethread  L502_luaE_freethread
#define luaF_freeclosure  L502_luaF_freeclosure
#define luaF_freeproto  L502_luaF_freeproto
#define luaH_free  L502_luaH_free
#define luaM_realloc  L502_luaM_realloc
#define luaS_resize  L502_luaS_resize
#define luaT_gettm  L502_luaT_gettm
#define luaL_argerror  L502_luaL_argerror
#define luaL_buffinit  L502_luaL_buffinit
#define luaL_checklstring  L502_luaL_checklstring
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_checkstack  L502_luaL_checkstack
#define luaL_checktype  L502_luaL_checktype
#define luaL_checkudata  L502_luaL_checkudata
#define luaL_error  L502_luaL_error
#define luaL_findstring  L502_luaL_findstring
#define luaL_getmetatable  L502_luaL_getmetatable
#define luaL_newmetatable  L502_luaL_newmetatable
#define luaL_openlib  L502_luaL_openlib
#define luaL_optlstring  L502_luaL_optlstring
#define luaL_optnumber  L502_luaL_optnumber
#define luaL_prepbuffer  L502_luaL_prepbuffer
#define luaL_pushresult  L502_luaL_pushresult
#define lua_gettable  L502_lua_gettable
#define lua_gettop  L502_lua_gettop
#define lua_isnumber  L502_lua_isnumber
#define lua_newtable  L502_lua_newtable
#define lua_newuserdata  L502_lua_newuserdata
#define lua_pushboolean  L502_lua_pushboolean
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushfstring  L502_lua_pushfstring
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushstring  L502_lua_pushstring
#define lua_pushvalue  L502_lua_pushvalue
#define lua_rawget  L502_lua_rawget
#define lua_rawset  L502_lua_rawset
#define lua_setmetatable  L502_lua_setmetatable
#define lua_settable  L502_lua_settable
#define lua_settop  L502_lua_settop
#define lua_strlen  L502_lua_strlen
#define lua_toboolean  L502_lua_toboolean
#define lua_tonumber  L502_lua_tonumber
#define lua_tostring  L502_lua_tostring
#define lua_touserdata  L502_lua_touserdata
#define lua_type  L502_lua_type
#define luaopen_io  L502_luaopen_io
#define luaD_throw  L502_luaD_throw
#define luaO_chunkid  L502_luaO_chunkid
#define luaO_pushfstring  L502_luaO_pushfstring
#define luaO_str2d  L502_luaO_str2d
#define luaS_newlstr  L502_luaS_newlstr
#define luaX_checklimit  L502_luaX_checklimit
#define luaX_errorline  L502_luaX_errorline
#define luaX_init  L502_luaX_init
#define luaX_lex  L502_luaX_lex
#define luaX_setinput  L502_luaX_setinput
#define luaX_syntaxerror  L502_luaX_syntaxerror
#define luaX_token2str  L502_luaX_token2str
#define luaZ_fill  L502_luaZ_fill
#define luaZ_openspace  L502_luaZ_openspace
#define luaL_argerror  L502_luaL_argerror
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_error  L502_luaL_error
#define luaL_openlib  L502_luaL_openlib
#define lua_gettop  L502_lua_gettop
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnumber  L502_lua_pushnumber
#define lua_settable  L502_lua_settable
#define luaopen_math  L502_luaopen_math
#define luaD_throw  L502_luaD_throw
#define luaG_runerror  L502_luaG_runerror
#define luaM_growaux  L502_luaM_growaux
#define luaM_realloc  L502_luaM_realloc
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushstring  L502_lua_pushstring
#define lua_settable  L502_lua_settable
#define luaopen_loadlib  L502_luaopen_loadlib
#define luaD_growstack  L502_luaD_growstack
#define luaO_chunkid  L502_luaO_chunkid
#define luaO_int2fb  L502_luaO_int2fb
#define luaO_log2  L502_luaO_log2
#define luaO_nilobject  L502_luaO_nilobject
#define luaO_pushfstring  L502_luaO_pushfstring
#define luaO_pushvfstring  L502_luaO_pushvfstring
#define luaO_rawequalObj  L502_luaO_rawequalObj
#define luaO_str2d  L502_luaO_str2d
#define luaS_newlstr  L502_luaS_newlstr
#define luaV_concat  L502_luaV_concat
#define luaP_opmodes  L502_luaP_opmodes
#define luaF_newproto  L502_luaF_newproto
#define luaH_new  L502_luaH_new
#define luaK_checkstack  L502_luaK_checkstack
#define luaK_code  L502_luaK_code
#define luaK_codeABC  L502_luaK_codeABC
#define luaK_codeABx  L502_luaK_codeABx
#define luaK_concat  L502_luaK_concat
#define luaK_dischargevars  L502_luaK_dischargevars
#define luaK_exp2RK  L502_luaK_exp2RK
#define luaK_exp2anyreg  L502_luaK_exp2anyreg
#define luaK_exp2nextreg  L502_luaK_exp2nextreg
#define luaK_exp2val  L502_luaK_exp2val
#define luaK_fixline  L502_luaK_fixline
#define luaK_getlabel  L502_luaK_getlabel
#define luaK_goiffalse  L502_luaK_goiffalse
#define luaK_goiftrue  L502_luaK_goiftrue
#define luaK_indexed  L502_luaK_indexed
#define luaK_infix  L502_luaK_infix
#define luaK_jump  L502_luaK_jump
#define luaK_nil  L502_luaK_nil
#define luaK_numberK  L502_luaK_numberK
#define luaK_patchlist  L502_luaK_patchlist
#define luaK_patchtohere  L502_luaK_patchtohere
#define luaK_posfix  L502_luaK_posfix
#define luaK_prefix  L502_luaK_prefix
#define luaK_reserveregs  L502_luaK_reserveregs
#define luaK_self  L502_luaK_self
#define luaK_setcallreturns  L502_luaK_setcallreturns
#define luaK_storevar  L502_luaK_storevar
#define luaK_stringK  L502_luaK_stringK
#define luaM_growaux  L502_luaM_growaux
#define luaM_realloc  L502_luaM_realloc
#define luaO_int2fb  L502_luaO_int2fb
#define luaO_log2  L502_luaO_log2
#define luaO_pushfstring  L502_luaO_pushfstring
#define luaS_newlstr  L502_luaS_newlstr
#define luaX_checklimit  L502_luaX_checklimit
#define luaX_lex  L502_luaX_lex
#define luaX_setinput  L502_luaX_setinput
#define luaX_syntaxerror  L502_luaX_syntaxerror
#define luaX_token2str  L502_luaX_token2str
#define luaY_parser  L502_luaY_parser
#define luaC_callGCTM  L502_luaC_callGCTM
#define luaC_link  L502_luaC_link
#define luaC_separateudata  L502_luaC_separateudata
#define luaC_sweep  L502_luaC_sweep
#define luaD_rawrunprotected  L502_luaD_rawrunprotected
#define luaD_throw  L502_luaD_throw
#define luaE_freethread  L502_luaE_freethread
#define luaE_newthread  L502_luaE_newthread
#define luaF_close  L502_luaF_close
#define luaH_new  L502_luaH_new
#define luaM_realloc  L502_luaM_realloc
#define luaS_freeall  L502_luaS_freeall
#define luaS_newlstr  L502_luaS_newlstr
#define luaS_resize  L502_luaS_resize
#define luaT_init  L502_luaT_init
#define luaX_init  L502_luaX_init
#define lua_close  L502_lua_close
#define lua_open  L502_lua_open
#define luaM_realloc  L502_luaM_realloc
#define luaS_freeall  L502_luaS_freeall
#define luaS_newlstr  L502_luaS_newlstr
#define luaS_newudata  L502_luaS_newudata
#define luaS_resize  L502_luaS_resize
#define luaL_addlstring  L502_luaL_addlstring
#define luaL_addvalue  L502_luaL_addvalue
#define luaL_argerror  L502_luaL_argerror
#define luaL_buffinit  L502_luaL_buffinit
#define luaL_checklstring  L502_luaL_checklstring
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_checkstack  L502_luaL_checkstack
#define luaL_checktype  L502_luaL_checktype
#define luaL_error  L502_luaL_error
#define luaL_openlib  L502_luaL_openlib
#define luaL_optnumber  L502_luaL_optnumber
#define luaL_prepbuffer  L502_luaL_prepbuffer
#define luaL_pushresult  L502_luaL_pushresult
#define lua_call  L502_lua_call
#define lua_dump  L502_lua_dump
#define lua_gettop  L502_lua_gettop
#define lua_isstring  L502_lua_isstring
#define lua_pushcclosure  L502_lua_pushcclosure
#define lua_pushlstring  L502_lua_pushlstring
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushvalue  L502_lua_pushvalue
#define lua_replace  L502_lua_replace
#define lua_settop  L502_lua_settop
#define lua_strlen  L502_lua_strlen
#define lua_toboolean  L502_lua_toboolean
#define lua_tonumber  L502_lua_tonumber
#define lua_tostring  L502_lua_tostring
#define lua_type  L502_lua_type
#define luaopen_string  L502_luaopen_string
#define luaC_link  L502_luaC_link
#define luaG_runerror  L502_luaG_runerror
#define luaH_free  L502_luaH_free
#define luaH_get  L502_luaH_get
#define luaH_getnum  L502_luaH_getnum
#define luaH_getstr  L502_luaH_getstr
#define luaH_mainposition  L502_luaH_mainposition
#define luaH_new  L502_luaH_new
#define luaH_next  L502_luaH_next
#define luaH_set  L502_luaH_set
#define luaH_setnum  L502_luaH_setnum
#define luaM_realloc  L502_luaM_realloc
#define luaO_log2  L502_luaO_log2
#define luaO_nilobject  L502_luaO_nilobject
#define luaO_rawequalObj  L502_luaO_rawequalObj
#define luaL_addlstring  L502_luaL_addlstring
#define luaL_addvalue  L502_luaL_addvalue
#define luaL_argerror  L502_luaL_argerror
#define luaL_buffinit  L502_luaL_buffinit
#define luaL_checknumber  L502_luaL_checknumber
#define luaL_checkstack  L502_luaL_checkstack
#define luaL_checktype  L502_luaL_checktype
#define luaL_error  L502_luaL_error
#define luaL_getn  L502_luaL_getn
#define luaL_openlib  L502_luaL_openlib
#define luaL_optlstring  L502_luaL_optlstring
#define luaL_optnumber  L502_luaL_optnumber
#define luaL_pushresult  L502_luaL_pushresult
#define luaL_setn  L502_luaL_setn
#define lua_call  L502_lua_call
#define lua_gettop  L502_lua_gettop
#define lua_isstring  L502_lua_isstring
#define lua_lessthan  L502_lua_lessthan
#define lua_next  L502_lua_next
#define lua_pushnil  L502_lua_pushnil
#define lua_pushnumber  L502_lua_pushnumber
#define lua_pushvalue  L502_lua_pushvalue
#define lua_rawgeti  L502_lua_rawgeti
#define lua_rawseti  L502_lua_rawseti
#define lua_settop  L502_lua_settop
#define lua_toboolean  L502_lua_toboolean
#define lua_type  L502_lua_type
#define luaopen_table  L502_luaopen_table
#define luaH_getstr  L502_luaH_getstr
#define luaO_nilobject  L502_luaO_nilobject
#define luaS_newlstr  L502_luaS_newlstr
#define luaT_gettm  L502_luaT_gettm
#define luaT_gettmbyobj  L502_luaT_gettmbyobj
#define luaT_init  L502_luaT_init
#define luaT_typenames  L502_luaT_typenames
#define luaF_newproto  L502_luaF_newproto
#define luaG_checkcode  L502_luaG_checkcode
#define luaG_runerror  L502_luaG_runerror
#define luaM_realloc  L502_luaM_realloc
#define luaS_newlstr  L502_luaS_newlstr
#define luaU_endianness  L502_luaU_endianness
#define luaU_undump  L502_luaU_undump
#define luaZ_fill  L502_luaZ_fill
#define luaZ_openspace  L502_luaZ_openspace
#define luaZ_read  L502_luaZ_read
#define luaC_collectgarbage  L502_luaC_collectgarbage
#define luaD_call  L502_luaD_call
#define luaD_callhook  L502_luaD_callhook
#define luaD_growstack  L502_luaD_growstack
#define luaD_poscall  L502_luaD_poscall
#define luaD_precall  L502_luaD_precall
#define luaF_close  L502_luaF_close
#define luaF_findupval  L502_luaF_findupval
#define luaF_newLclosure  L502_luaF_newLclosure
#define luaG_aritherror  L502_luaG_aritherror
#define luaG_concaterror  L502_luaG_concaterror
#define luaG_inithooks  L502_luaG_inithooks
#define luaG_ordererror  L502_luaG_ordererror
#define luaG_runerror  L502_luaG_runerror
#define luaG_typeerror  L502_luaG_typeerror
#define luaH_get  L502_luaH_get
#define luaH_getstr  L502_luaH_getstr
#define luaH_new  L502_luaH_new
#define luaH_set  L502_luaH_set
#define luaH_setnum  L502_luaH_setnum
#define luaO_nilobject  L502_luaO_nilobject
#define luaO_rawequalObj  L502_luaO_rawequalObj
#define luaO_str2d  L502_luaO_str2d
#define luaS_newlstr  L502_luaS_newlstr
#define luaT_gettm  L502_luaT_gettm
#define luaT_gettmbyobj  L502_luaT_gettmbyobj
#define luaV_concat  L502_luaV_concat
#define luaV_equalval  L502_luaV_equalval
#define luaV_execute  L502_luaV_execute
#define luaV_gettable  L502_luaV_gettable
#define luaV_lessthan  L502_luaV_lessthan
#define luaV_settable  L502_luaV_settable
#define luaV_tonumber  L502_luaV_tonumber
#define luaV_tostring  L502_luaV_tostring
#define luaZ_openspace  L502_luaZ_openspace
#define luaM_realloc  L502_luaM_realloc
#define luaZ_fill  L502_luaZ_fill
#define luaZ_init  L502_luaZ_init
#define luaZ_lookahead  L502_luaZ_lookahead
#define luaZ_openspace  L502_luaZ_openspace
#define luaZ_read  L502_luaZ_read

#define LUA_VERSION	"Lua 5.0.2"
#define LUA_COPYRIGHT	"Copyright (C) 1994-2004 Tecgraf, PUC-Rio"
#define LUA_AUTHORS 	"R. Ierusalimschy, L. H. de Figueiredo & W. Celes"



/* option for multiple returns in `lua_pcall' and `lua_call' */
#define LUA_MULTRET	(-1)


/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_GLOBALSINDEX	(-10001)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))


/* error codes for `lua_load' and `lua_pcall' */
#define LUA_ERRRUN	1
#define LUA_ERRFILE	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5


typedef struct lua_State lua_State;

typedef int (*lua_CFunction) (lua_State *L);


/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*lua_Chunkreader) (lua_State *L, void *ud, size_t *sz);

typedef int (*lua_Chunkwriter) (lua_State *L, const void* p,
                                size_t sz, void* ud);


/*
** basic types
*/
#define LUA_TNONE	(-1)

#define LUA_TNIL	0
#define LUA_TBOOLEAN	1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER	3
#define LUA_TSTRING	4
#define LUA_TTABLE	5
#define LUA_TFUNCTION	6
#define LUA_TUSERDATA	7
#define LUA_TTHREAD	8


/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20


/*
** generic extra include file
*/
#ifdef LUA_USER_H
#include LUA_USER_H
#endif


/* type of numbers in Lua */
#ifndef LUA_NUMBER
typedef double lua_Number;
#else
typedef LUA_NUMBER lua_Number;
#endif


/* mark for all API functions */
#ifndef LUA_API
#define LUA_API		extern
#endif


/*
** state manipulation
*/
LUA_API lua_State *lua_open (void);
LUA_API void       lua_close (lua_State *L);
LUA_API lua_State *lua_newthread (lua_State *L);

LUA_API lua_CFunction lua_atpanic (lua_State *L, lua_CFunction panicf);


/*
** basic stack manipulation
*/
LUA_API int   lua_gettop (lua_State *L);
LUA_API void  lua_settop (lua_State *L, int idx);
LUA_API void  lua_pushvalue (lua_State *L, int idx);
LUA_API void  lua_remove (lua_State *L, int idx);
LUA_API void  lua_insert (lua_State *L, int idx);
LUA_API void  lua_replace (lua_State *L, int idx);
LUA_API int   lua_checkstack (lua_State *L, int sz);

LUA_API void  lua_xmove (lua_State *from, lua_State *to, int n);


/*
** access functions (stack -> C)
*/

LUA_API int             lua_isnumber (lua_State *L, int idx);
LUA_API int             lua_isstring (lua_State *L, int idx);
LUA_API int             lua_iscfunction (lua_State *L, int idx);
LUA_API int             lua_isuserdata (lua_State *L, int idx);
LUA_API int             lua_type (lua_State *L, int idx);
LUA_API const char     *lua_typename (lua_State *L, int tp);

LUA_API int            lua_equal (lua_State *L, int idx1, int idx2);
LUA_API int            lua_rawequal (lua_State *L, int idx1, int idx2);
LUA_API int            lua_lessthan (lua_State *L, int idx1, int idx2);

LUA_API lua_Number      lua_tonumber (lua_State *L, int idx);
LUA_API int             lua_toboolean (lua_State *L, int idx);
LUA_API const char     *lua_tostring (lua_State *L, int idx);
LUA_API size_t          lua_strlen (lua_State *L, int idx);
LUA_API lua_CFunction   lua_tocfunction (lua_State *L, int idx);
LUA_API void	       *lua_touserdata (lua_State *L, int idx);
LUA_API lua_State      *lua_tothread (lua_State *L, int idx);
LUA_API const void     *lua_topointer (lua_State *L, int idx);


/*
** push functions (C -> stack)
*/
LUA_API void  lua_pushnil (lua_State *L);
LUA_API void  lua_pushnumber (lua_State *L, lua_Number n);
LUA_API void  lua_pushlstring (lua_State *L, const char *s, size_t l);
LUA_API void  lua_pushstring (lua_State *L, const char *s);
LUA_API const char *lua_pushvfstring (lua_State *L, const char *fmt,
                                                    va_list argp);
LUA_API const char *lua_pushfstring (lua_State *L, const char *fmt, ...);
LUA_API void  lua_pushcclosure (lua_State *L, lua_CFunction fn, int n);
LUA_API void  lua_pushboolean (lua_State *L, int b);
LUA_API void  lua_pushlightuserdata (lua_State *L, void *p);


/*
** get functions (Lua -> stack)
*/
LUA_API void  lua_gettable (lua_State *L, int idx);
LUA_API void  lua_rawget (lua_State *L, int idx);
LUA_API void  lua_rawgeti (lua_State *L, int idx, int n);
LUA_API void  lua_newtable (lua_State *L);
LUA_API void *lua_newuserdata (lua_State *L, size_t sz);
LUA_API int   lua_getmetatable (lua_State *L, int objindex);
LUA_API void  lua_getfenv (lua_State *L, int idx);


/*
** set functions (stack -> Lua)
*/
LUA_API void  lua_settable (lua_State *L, int idx);
LUA_API void  lua_rawset (lua_State *L, int idx);
LUA_API void  lua_rawseti (lua_State *L, int idx, int n);
LUA_API int   lua_setmetatable (lua_State *L, int objindex);
LUA_API int   lua_setfenv (lua_State *L, int idx);


/*
** `load' and `call' functions (load and run Lua code)
*/
LUA_API void  lua_call (lua_State *L, int nargs, int nresults);
LUA_API int   lua_pcall (lua_State *L, int nargs, int nresults, int errfunc);
LUA_API int lua_cpcall (lua_State *L, lua_CFunction func, void *ud);
LUA_API int   lua_load (lua_State *L, lua_Chunkreader reader, void *dt,
                        const char *chunkname);

LUA_API int lua_dump (lua_State *L, lua_Chunkwriter writer, void *data);


/*
** coroutine functions
*/
LUA_API int  lua_yield (lua_State *L, int nresults);
LUA_API int  lua_resume (lua_State *L, int narg);

/*
** garbage-collection functions
*/
LUA_API int   lua_getgcthreshold (lua_State *L);
LUA_API int   lua_getgccount (lua_State *L);
LUA_API void  lua_setgcthreshold (lua_State *L, int newthreshold);

/*
** miscellaneous functions
*/

LUA_API const char *lua_version (void);

LUA_API int   lua_error (lua_State *L);

LUA_API int   lua_next (lua_State *L, int idx);

LUA_API void  lua_concat (lua_State *L, int n);



/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define lua_boxpointer(L,u) \
	(*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))

#define lua_unboxpointer(L,i)	(*(void **)(lua_touserdata(L, i)))

#define lua_pop(L,n)		lua_settop(L, -(n)-1)

#define lua_register(L,n,f) \
	(lua_pushstring(L, n), \
	 lua_pushcfunction(L, f), \
	 lua_settable(L, LUA_GLOBALSINDEX))

#define lua_pushcfunction(L,f)	lua_pushcclosure(L, f, 0)

#define lua_isfunction(L,n)	(lua_type(L,n) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L,n) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L,n) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L,n) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L,n) == LUA_TBOOLEAN)
#define lua_isnone(L,n)		(lua_type(L,n) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L,n) <= 0)

#define lua_pushliteral(L, s)	\
	lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)



/*
** compatibility macros and functions
*/


LUA_API int lua_pushupvalues (lua_State *L);

#define lua_getregistry(L)	lua_pushvalue(L, LUA_REGISTRYINDEX)
#define lua_setglobal(L,s)	\
   (lua_pushstring(L, s), lua_insert(L, -2), lua_settable(L, LUA_GLOBALSINDEX))

#define lua_getglobal(L,s)	\
		(lua_pushstring(L, s), lua_gettable(L, LUA_GLOBALSINDEX))


/* compatibility with ref system */

/* pre-defined references */
#define LUA_NOREF	(-2)
#define LUA_REFNIL	(-1)

#define lua_ref(L,lock)	((lock) ? luaL_ref(L, LUA_REGISTRYINDEX) : \
      (lua_pushstring(L, "unlocked references are obsolete"), lua_error(L), 0))

#define lua_unref(L,ref)	luaL_unref(L, LUA_REGISTRYINDEX, (ref))

#define lua_getref(L,ref)	lua_rawgeti(L, LUA_REGISTRYINDEX, ref)



/*
** {======================================================================
** useful definitions for Lua kernel and libraries
** =======================================================================
*/

/* formats for Lua numbers */
#ifndef LUA_NUMBER_SCAN
#define LUA_NUMBER_SCAN		"%lf"
#endif

#ifndef LUA_NUMBER_FMT
#define LUA_NUMBER_FMT		"%.14g"
#endif

/* }====================================================================== */


/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3
#define LUA_HOOKTAILRET 4


/*
** Event masks
*/
#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET	(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)

typedef struct lua_Debug lua_Debug;  /* activation record */

typedef void (*lua_Hook) (lua_State *L, lua_Debug *ar);


LUA_API int lua_getstack (lua_State *L, int level, lua_Debug *ar);
LUA_API int lua_getinfo (lua_State *L, const char *what, lua_Debug *ar);
LUA_API const char *lua_getlocal (lua_State *L, const lua_Debug *ar, int n);
LUA_API const char *lua_setlocal (lua_State *L, const lua_Debug *ar, int n);
LUA_API const char *lua_getupvalue (lua_State *L, int funcindex, int n);
LUA_API const char *lua_setupvalue (lua_State *L, int funcindex, int n);

LUA_API int lua_sethook (lua_State *L, lua_Hook func, int mask, int count);
LUA_API lua_Hook lua_gethook (lua_State *L);
LUA_API int lua_gethookmask (lua_State *L);
LUA_API int lua_gethookcount (lua_State *L);


#define LUA_IDSIZE	60

struct lua_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `Lua', `C', `main', `tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  char short_src[LUA_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2004 Tecgraf, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif
