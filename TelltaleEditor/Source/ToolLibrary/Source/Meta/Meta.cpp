#include <Meta/Meta.hpp>
#include <Core/Context.hpp>

namespace Meta {
    
    // Registered game
    struct RegGame {
        String Name, ID;
    };
    
    static std::vector<RegGame> Games{};
    
    namespace L { // Lua functions
        
        U32 luaMetaRegisterGame(LuaManager& man){
            RegGame reg{};
            ScriptManager::TableGet(man, "Name");
            reg.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "ID");
            reg.ID = ScriptManager::PopString(man);
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
    
    // ScriptManager in development. Once that is up to scratch we can implement the game independent meta system
    
    void Initialise(ToolContext& context)
    {
        ScriptManager::RegisterCollection(context.GetLibraryLVM(), L::luaMeta()); // Register meta scripting API
        
        ScriptManager::RunText(context.GetLibraryLVM(), context.LoadLibraryStringResource("Games.lua")); // Initialise games
        
        TTE_LOG("Meta initialised with %d games", (U32)Games.size());
    }
    
    void Shutdown(ToolContext& ctx)
    {
        Games.clear();
        
        TTE_LOG("Meta shutdown");
    }
    
}
