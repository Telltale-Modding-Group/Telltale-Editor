#include <Core/Context.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Meta/Meta.hpp>

thread_local Bool _IsMain = false; // Used in meta.

Bool IsCallingFromMain()
{
    return _IsMain;
}

// ===================================================================         TOOL CONTEXT
// ===================================================================

static ToolContext* GlobalContext = nullptr;

ToolContext* CreateToolContext(LuaFunctionCollection API)
{
    
    if(GlobalContext == nullptr){
        _IsMain = true; // CreateToolContext sets this current thread as the main thread
        GlobalContext = (ToolContext*)TTE_ALLOC(sizeof(ToolContext), MEMORY_TAG_TOOL_CONTEXT); // alloc raw and construct, as its private.
        new (GlobalContext) ToolContext(std::move(API)); // construct after as some asserts require
    }else{
        TTE_ASSERT(false, "Trying to create more than one ToolContext. Only one instance is allowed per process.");
    }
    
    return GlobalContext;
}

ToolContext* GetToolContext()
{
    return GlobalContext;
}

void DestroyToolContext()
{
    TTE_ASSERT(_IsMain, "Must only be called from main thread");
    
    if(GlobalContext)
        TTE_DEL(GlobalContext);
    
    GlobalContext = nullptr;
    
}

LuaManager& ToolContext::GetGameLVM()
{
    TTE_ASSERT(GetActiveGame(), "No active game!");
    return _L[(U32)Meta::GetInternalState().GetActiveGame().LVersion];
}

Ptr<ResourceRegistry> ToolContext::CreateResourceRegistry()
{
    if(GetActiveGame() == nullptr)
    {
        TTE_LOG("Cannot create a resource registry as there is no current game set.");
        return nullptr;
    }
    
    Ptr<ResourceRegistry> registry = TTE_NEW_PTR(ResourceRegistry, MEMORY_TAG_RESOURCE_REGISTRY, GetGameLVM());
    
    Ptr<GameDependentObject> asDependent = std::static_pointer_cast<GameDependentObject>(registry);
    {
        std::lock_guard<std::mutex> G{_DependentsLock};
        _SwitchDependents.push_back(asDependent);
    }
    
    return registry;
}

void ToolContext::Switch(GameSnapshot snapshot)
{
    if(snapshot.ID.empty() || !Meta::_Impl::_CheckPlatform(snapshot.Platform))
    {
        TTE_ASSERT(false, "The platform '%s' is not a valid platform name. See valid platforms", snapshot.Platform.c_str());
        return;
    }
    if(_Setup)
        Release();
    
    _Snapshot = snapshot;
    TTE_ASSERT(_Snapshot.ID.length() && _Snapshot.Platform.length(), "Game or platform not specified");
    
    Meta::InitGame();
    JobScheduler::Initialise(_PerStateCollection);
    
    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/Files_" + snapshot.ID + ".symmap");
    GameSymbols.SerialiseIn(symbols); // reload
    
    _Setup = true;
}

ToolContext::ToolContext(LuaFunctionCollection PerState)
{
    DataStreamManager::Initialise();
    Compression::Initialise();
    _L[0].Initialise(LuaVersion::LUA_5_2_3);
    _L[1].Initialise(LuaVersion::LUA_5_1_4);
    _L[2].Initialise(LuaVersion::LUA_5_0_2);
    Meta::Initialise();
    
    // higher level lua api for each thread, including this context
    _PerStateCollection = std::move(PerState);
    ScriptManager::RegisterCollection(GetLibraryLVM(), _PerStateCollection);
    
    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/RuntimeSymbols.symmap"); // Load runtime symbols
    RuntimeSymbols.SerialiseIn(symbols);
}

String ToolContext::LoadLibraryStringResource(String name)
{
    DataStreamRef stream = LoadLibraryResource(name);
    if(!stream)
        return ""; // Stream could not be opened
    
    String result{};
    U8* tmp = TTE_ALLOC(stream->GetSize() + 1, MEMORY_TAG_TEMPORARY); // temp buffer to read string from
    
    if(!stream->Read(tmp, stream->GetSize())){
        TTE_FREE(tmp); // free buffer
        return ""; // could not read all bytes
    }
    
    tmp[stream->GetSize()] = 0; // add null terminator
    result = (CString)tmp; // cast raw U8 bytes to c char string
    
    TTE_FREE(tmp); // Free buffer
    
    return result;
}

void ToolContext::Release()
{
    if(_Setup)
    {
        
        U32 errors{};
        for(auto& dependent: _SwitchDependents)
        {
            Ptr<GameDependentObject> alive = dependent.lock();
            if(alive)
            {
                errors++;
                TTE_LOG("FATAL: Game dependent object is still alive at a game switch: %s", alive->ObjName);
            }
        }
        if(errors)
            TTE_ASSERT(false, "Found %d alive game dependent objects before a game switch.", errors);
        _SwitchDependents.clear();
        
        JobScheduler::Shutdown();
        Meta::RelGame();
        Blowfish::Shutdown();
        GameSymbols.Clear();
        
        _Setup = false;
        
    }
}
