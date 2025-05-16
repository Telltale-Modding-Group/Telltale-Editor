#include <Scripting/ScriptManager.hpp>
#include <Runtime/SceneRuntime.hpp>
#include <Symbols.hpp>

#include <vector>

// This file contains pretty much all of the Telltale Tool LUA API, with documentation.

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><> SECTION <====> UTILITIES <><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><<><><><><><><>><><><><><><><><><>

template<U32 Mn, U32 Mx, U32 NRet>
struct LUAArgError
{
    
    static U32 Execute(LuaManager& man, CString Fn)
    {
        TTE_LOG("ScriptError(%s): expected between %d and %d arguments, but only %d given", Fn, Mn, Mx, man.GetTop());
        for(U32 i = 0; i < NRet; i++)
            man.PushNil();
        return NRet;
    }
    
};

template<U32 N, U32 NRet>
struct LUAArgError<N, N, NRet>
{
    
    static U32 Execute(LuaManager& man, CString Fn)
    {
        TTE_LOG("ScriptError(%s): Expected %d arguments but only %d given", Fn, N, man.GetTop());
        for(U32 i = 0; i < NRet; i++)
            man.PushNil();
        return NRet;
    }
    
};

template<U32 NRet>
struct LUAFnCtx
{
    
    CString Name;
    LuaManager& Man;
    I32 PassedArgs;
    
    LUAFnCtx(LuaManager& man, CString n) : Man(man), Name(n)
    {
        PassedArgs = man.GetTop();
    }
    
    ~LUAFnCtx()
    {
        if(Man.GetTop() != PassedArgs + NRet)
        {
            TTE_LOG("ScriptError(%s): Should have returned %d values, but is returning %d", NRet, Man.GetTop() - PassedArgs);
        }
    }
    
};

struct StaticLUAFunction
{
    CString Name = nullptr;
    CString Decl = nullptr;
    CString Desc = nullptr;
    LuaCFunction Proc = nullptr;
};

static U32 _FunctionCount = 0;
static StaticLUAFunction _Functions[2048]{};

static void _RegisterLuaFn(CString name, CString decl, CString desc, LuaCFunction proc)
{
    TTE_ASSERT(_FunctionCount < 2048, "Increase size!!"); // really?
    _Functions[_FunctionCount++] = { name, decl, desc, proc };
}

template <auto _LF>
struct LuaFunctionRegObjectWrapper
{
    LuaFunctionRegObjectWrapper(CString name, CString decl, CString desc)
    {
        _RegisterLuaFn(name, decl, desc, _LF);
    }
};

// Define new function with name, declaration and description,
#define DEFINEFN(Name, Decl, Desc) \
static U32 Name(LuaManager& man); \
static LuaFunctionRegObjectWrapper<Name> _Reg_##Name{#Name, Decl, Desc}; \
static U32 Name(LuaManager& man)

// Sets function information about how many arguments and return values the function takes.
#define FNINFO(Name, MinArgs, MaxArgs, NReturns) \
LUAFnCtx<NReturns> _Fn(man, Name); if(man.GetTop() < MinArgs || man.GetTop() > MaxArgs) { return LUAArgError<MinArgs, MaxArgs, NReturns>::Execute(man, Name); }

// Sets local variable SceneRuntime* pSceneRuntime. If not attached to lua manager, fails. Used for functions which can only be executed within a scene runtime env.
#define RUNTIME_API() ScriptManager::GetGlobal(man, "_SceneRuntime", true); /* DON'T CHANGE THE KEY. USED IN OTHER PARTS. IF SO USE SEARCH ALL.*/ \
if(man.Type(-1) != LuaType::LIGHT_OPAQUE) { TTE_LOG("ScriptError(%s): No attached scene runtime! You cannot call this function at this time,", _Fn.Name); return 0; } \
SceneRuntime* pSceneRuntime = (SceneRuntime*)ScriptManager::PopOpaque(man);

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><> SECTION <====> SCENE API <><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><<><><><><><><>><><><><><><><><><><><>

DEFINEFN(SceneOpen, "nil SceneOpen(fileName)", "Open the scene file. Optional 2nd parameter is the name of the entry function. "
         "The default entry function is the scene file name without the extension. EngineOnSceneOpenFinished(scene) is called once the scene has been opened."
         " The difference between this function and SceneAdd functions is that this will load shaders and set the world scene.")
{
    FNINFO("SceneOpen", 1, 2, 0);
    RUNTIME_API();
    
    // ARGS
    String fileName = man.ToString(1);
    String entryFn{};
    if(man.GetTop() == 2)
        entryFn = man.ToString(2);
    else
        entryFn = FileGetName(fileName) + "()";
    
    // FUNC
    SceneMessage openMsg{};
    openMsg.Type = SceneMessageType::OPEN_SCENE;
    String* args = pSceneRuntime->AllocateMessageArguments<String>(2);
    openMsg.Arguments = args;
    args[0] = fileName;
    args[1] = entryFn;
    pSceneRuntime->SendMessage(openMsg);
    
    return 0;
}

static void SceneAddHelper(LuaManager& man, SceneRuntime* pSceneRuntime, I32 fileNameIdx, I32 options, I32 entryPoint, I32 entryPointStrArgument)
{
    // ARGS
    String fileName = man.ToString(fileNameIdx);
    String entryFn{};
    if(man.GetTop() >= entryPoint)
        entryFn = man.ToString(2);
    else
        entryFn = FileGetName(fileName);
    if(man.GetTop() >= entryPointStrArgument)
    {
        entryFn += "(\"";
        entryFn += man.ToString(entryPointStrArgument);
        entryFn += "\")";
    }
    else
    {
        entryFn += "()";
    }
    I32 aPriority = 1000;
    Bool bCbs = true;
    if(man.GetTop() >= options) // options table iterate
    {
        ITERATE_TABLE(it, options)
        {
            String key = man.ToString(it.KeyIndex());
            if(CompareCaseInsensitive(key, "callCallbacks"))
                bCbs = man.ToBool(it.ValueIndex());
            else if(CompareCaseInsensitive(key, "async"))
                ; // unused here for now
            else if(CompareCaseInsensitive(key, "agentPriority"))
                aPriority = man.ToInteger(it.ValueIndex());
        }
    }
    
    // FUNC
    SceneMessage openMsg{};
    openMsg.Type = SceneMessageType::ADD_SCENE;
    AddSceneInfo* args = pSceneRuntime->AllocateMessageArguments<AddSceneInfo>(1);
    openMsg.Arguments = args;
    args->FileName = fileName;
    args->EntryPoint = entryFn;
    pSceneRuntime->SendMessage(openMsg);
}

DEFINEFN(SceneAdd, "nil SceneAdd(fileName)", "Add the scene file. Optional 2nd parameter is the name of the entry function."
         " Optional 3rd parameter is the the string argument to pass into the entry point (else no args). "
        "EngineOnSceneAddFinished is called with the scene as the argument.")
{
    FNINFO("SceneAdd", 1, 2, 0);
    RUNTIME_API();
    
    SceneAddHelper(man, pSceneRuntime, 1, 0, 2, 3);
    
    return 0;
}

DEFINEFN(SceneAddWithOptions, "nil SceneAddWithOptions(fileName, optionsTable)", "Add the scene file with options. Optional 3rd and 4th arguments"
         " are the entry point function name and entry point functino argument string (if not, passed with no arguments). Options table can contain "
         "callCallbacks (default true), async (default true), agentPriority (default 1000). Agent priority must be positive.")
{
    FNINFO("SceneAddWithOptions", 2, 4, 0);
    RUNTIME_API();
    
    SceneAddHelper(man, pSceneRuntime, 1, 2, 3, 4);
    
    return 0;
}

DEFINEFN(SceneAddAgent, "nil SceneAddAgent(scene,agent_name,agent_props)", "Add an agent to the scene. Agent props is the handle "
         "(string filename / symbol string hash, upper hex 16 chars padded), for the agent properties. Empty string for defaults.")
{
    FNINFO("SceneAddAgent", 3, 3, 0);
    RUNTIME_API();
    
    Scene* pScene = ScriptManager::GetScriptObject<Scene>(man, 1);
    String agentName = man.ToString(2);
    Symbol prop = ScriptManager::ToSymbol(man, 3);
    
    if(pScene && !pScene->ExistsAgent(agentName))
    {
        HandlePropertySet hProp{};
        hProp.SetObject(pSceneRuntime->GetResourceRegistry(), prop, false, false);
        pScene->AddAgent(agentName, {}, hProp.GetObject(pSceneRuntime->GetResourceRegistry(), true));
    }
    
    return 0;
}

DEFINEFN(SceneCreate, "nil SceneCreate(fileName)", "Creates a new scene file that is empty, fileName should include path. "
         "Optional 2nd param is the prop file for the scene agent to use. This just saves the new scene file and does not return or load it.")
{
    FNINFO("SceneCreate", 1, 2, 0);
    RUNTIME_API();
    
    String file = man.ToString(1);
    String parent = man.GetTop() == 2 ? man.ToString(2) : kScenePropName;
    U32 sceneCls = Meta::FindClass("class Scene", 0);
    
    Scene s{pSceneRuntime->GetResourceRegistry()};
    String name = FileGetName(file);
    s.SetName(name);
    
    
    return 0;
}

// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><> SECTION <====> NEXT API! <><><><><><><><><><><><><><><><><><><><><><><><><><><>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><<><><><><><><>><><><><><><><><><><><>

void luaCompleteGameEngine(LuaFunctionCollection& Col)
{
    for(U32 i = 0; i < _FunctionCount; i++)
        PUSH_FUNC(Col, _Functions[i].Name, _Functions[i].Proc, _Functions[i].Decl, _Functions[i].Desc);
}
