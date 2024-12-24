#pragma once

#include <Scripting/LuaManager.hpp>
#include <vector>

// High level LUA scripting API. This builds upon the Lua Manager which leads with the
// lua API for different lua versions supported by the range of Telltale Games.

// This has no state at all, lua manager and lua_State captures it all in the stack.

#define LUA_MULTRET (-1)

// Registerable C function
struct LuaFunctionRegObject
{
    String Name;
    LuaCFunction Function = nullptr;
};

// Collection of registerable C functions providing C API.
struct LuaFunctionCollection
{
    String Name;
    std::vector<LuaFunctionRegObject> Functions;
};

// Provides high level scripting access. Most of the functions are the same as Telltales actual ScriptManager API.
namespace ScriptManager {
    
    // Execute the function on the stack followed by its arguments pushed after. Function & args all popped. Nresults is number of arguments
    // pushed onto the stack, ensured to be padded with NILs if needed - or truncated. Pass LUA_MULTRET for whatever the function returns.
    inline void Execute(LuaManager& man, U32 Nargs, U32 Nresults)
    {
        man.CallFunction(Nargs, Nresults);
    }
    
    // Pushes onto the stack the global with the given name, or nil.
    inline void GetGlobal(LuaManager& man, const String& name, Bool bRaw)
    {
        man.PushEnv();
        man.PushLString(name);
        man.GetTable(-2, bRaw);
        man.Remove(-2);
    }
    
    // Sets a global, popping from the stack the global to be set.
    inline void SetGlobal(LuaManager& man, const String& name, Bool bRaw)
    {
        man.PushEnv(); // just a bunch of stack shuffling
        man.Insert(-2);
        man.PushLString(name);
        man.Insert(-2);
        man.SetTable(-3, bRaw);//table,key,val
    }
    
    // Registers (overrides if existing) a function collection (ie system of functions) to the lua environment.
    inline void RegisterCollection(LuaManager& man, const LuaFunctionCollection& collection)
    {
        
        if(collection.Name.length() > 0)
            man.PushTable(); // new table
        else
            man.PushEnv(); // no container name, pushing everything to _ENV/_G
        
        for(auto& regObj : collection.Functions){
            
            // Register each function
            man.PushLString(regObj.Name);
            man.PushFn(regObj.Function);
            man.SetTable(-3, true);
            
        }
        
        // set global
        if(collection.Name.length() > 0)
            SetGlobal(man, collection.Name, true);
        
        man.Pop(1); // pop globals/new table
        
    }
    
    // Loads a chunk of lua code but does not run it, pushing it onto the top of the stack.
    // Pass in the name for debugging purposes, eg the file name.
    // If this returns false, nothing is pushed and error messages are logged to console.
    inline Bool LoadChunk(LuaManager& man, const String& name, const String& text){
        return man.LoadChunk(name, (const U8*)text.c_str(), (U32)text.length(), false);
    }
    
    // Call the given function name, with no arguments.
    inline void CallFunction(LuaManager& man, const String& name)
    {
        
        // get the function
        GetGlobal(man, name, true);
        
        // if a function, call
        if(man.Type(-1) == LuaType::FUNCTION)
            man.CallFunction(0, LUA_MULTRET);
        else
            man.SetTop(-2); // pop non function
        
    }
    
    // Run lua source code on the VM
    inline void RunText(LuaManager& man, const String& code)
    {
        man.RunText(code.c_str(), (U32)code.length());
    }
    
    inline String PopString(LuaManager& man) {
        String ret = man.ToString(-1);
        man.Pop(1);
        return ret;
    }
    
    inline Bool PopBool(LuaManager& man){
        Bool ret = man.ToBool(-1);
        man.Pop(1);
        return ret;
    }
    
    inline Float PopFloat(LuaManager& man){
        Float ret = man.ToFloat(-1);
        man.Pop(1);
        return ret;
    }
    
    inline I32 PopInteger(LuaManager& man){
        I32 ret = man.ToInteger(-1);
        man.Pop(1);
        return ret;
    }
    
    inline void* PopOpaque(LuaManager& man){
        void* ret = man.ToPointer(-1);
        man.Pop(1);
        return ret;
    }
    
    // Top of the stack should be the table. Pushes the element in the table at the given key. DOES NOT pop table.
    inline void TableGet(LuaManager& man, String key) {
        man.PushLString(std::move(key));
        man.GetTable(-2);
    }
    
}
