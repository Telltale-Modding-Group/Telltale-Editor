#include <Meta/Meta.hpp>
#include <Core/Context.hpp>
#include <Scripting/LuaManager.hpp>

#include <map>

static ToolContext* GlobalContext = nullptr;

ToolContext* CreateToolContext()
{

    if(GlobalContext == nullptr){
        GlobalContext = (ToolContext*)TTE_ALLOC(sizeof(ToolContext), MEMORY_TAG_TOOL_CONTEXT); // alloc raw and construct, as its private.
        new (GlobalContext) ToolContext();
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
    
    if(GlobalContext)
        TTE_FREE(GlobalContext);
    
    GlobalContext = nullptr;
    
}

namespace Meta {
    
    // Registered game
    struct RegGame {
        String Name, ID;
        StreamVersion MetaVersion;
    };
    
    static std::vector<RegGame> Games{};
    static std::map<U32, Class> Classes{};
    static U32 GameIndex{}; // Current game index
    
    namespace L { // Lua functions
        
        U32 luaMetaRegisterGame(LuaManager& man){
            RegGame reg{};
            ScriptManager::TableGet(man, "Name");
            reg.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "ID");
            reg.ID = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "MetaVersion");
            
            String v = ScriptManager::PopString(man);
            
            reg.MetaVersion = v == "MSV6" ? MSV6 : v == "MSV5" ? MSV5 : v == "MSV4" ? MSV4 : v == "MCOM" ? MCOM : v == "MTRE" ? MTRE : v == "MBIN" ? MBIN : v == "MBES" ? MBES : (StreamVersion)-1;
            
            TTE_ASSERT(reg.MetaVersion != (StreamVersion)-1, "Invalid meta version");
            
            Games.push_back(std::move(reg));
            return 0;
        }
        
        // Generates new registerable meta library lua API
        LuaFunctionCollection luaMeta() {
            LuaFunctionCollection Col{};
            Col.Functions.push_back({"MetaRegisterGame",&luaMetaRegisterGame});
            return Col;
        }
        
    }
    
    void InitGame()
    {
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        GameSnapshot snap = GetToolContext()->GetSnapshot();
        
        ScriptManager::GetGlobal(GetToolContext()->GetLibraryLVM(), "RegisterAll", true);
        
        if(GetToolContext()->GetLibraryLVM().Type(-1) != LuaType::FUNCTION){
            GetToolContext()->GetLibraryLVM().Pop(1);
            TTE_ASSERT(false,"Cannot switch to new snapshot. Classes registration script is invalid");
            return;
        }else{
            GetToolContext()->GetLibraryLVM().PushLString(snap.ID);
            GetToolContext()->GetLibraryLVM().PushLString(snap.Platform);
            GetToolContext()->GetLibraryLVM().PushLString(snap.Vendor);
            ScriptManager::Execute(GetToolContext()->GetLibraryLVM(), 3, 1);
        }
        
        TTE_LOG("Meta fully initialised with snapshot of %s", snap.ID.c_str());
    }
    
    void RelGame()
    {
        Classes.clear();
    }
    
    void Initialise()
    {
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        
        ScriptManager::RegisterCollection(GetToolContext()->GetLibraryLVM(), L::luaMeta()); // Register meta scripting API
        
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(),
                               GetToolContext()->LoadLibraryStringResource("Games.lua")); // Initialise games
        
        String src = GetToolContext()->LoadLibraryStringResource("Classes.lua");
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(),  src);
        src.clear();
        
        TTE_LOG("Meta partially initialised with %d games", (U32)Games.size());
    }
    
    void Shutdown()
    {
        Games.clear();
        
        TTE_LOG("Meta shutdown");
    }
    
}
