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
    // For information about the LockContext argument, see LuaManager::CallFunction
    inline void Execute(LuaManager& man, U32 Nargs, U32 Nresults, Bool LockContext)
    {
        man.CallFunction(Nargs, Nresults, LockContext);
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
    
    // Call the given function name, with no arguments. For information avout Lockcontext argument, see LuaManager::CallFunction
    inline void CallFunction(LuaManager& man, const String& name, Bool LockContext)
    {
        
        // get the function
        GetGlobal(man, name, true);
        
        // if a function, call
        if(man.Type(-1) == LuaType::FUNCTION)
            man.CallFunction(0, LUA_MULTRET, LockContext);
        else
            man.SetTop(-2); // pop non function
        
    }
    
    // Run lua source code on the VM
    inline void RunText(LuaManager& man, const String& code, CString Chunkname = "defaultrun.lua")
    {
        man.RunText(code.c_str(), (U32)code.length(), Chunkname);
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
    
    inline U32 PopUnsignedInteger(LuaManager& man){
        U32 ret = (U32)man.ToInteger(-1);
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
    
    // Swaps the top two elements
    inline void SwapTopElements(LuaManager& man)
    {
        man.Insert(-2);
    }
    
    // INTERNAL. DON'T USE.
    template<typename T>
    inline U32 __InternalDeleter(LuaManager& man)
    {
        TTE_DEL(((T*)man.ToPointer(-1)));
        return 0;
    }
    
    // returns 0 if invalid, or not found.
    inline U32 GetScriptObjectTag(LuaManager& man, I32 stackIndex)
    {
        if(!man.GetMetatable(stackIndex))
            return 0; // not found
        man.PushLString("__ttetag");
        man.GetTable(-2);
        U32 tag = man.ToInteger(-1);
        man.Pop(2);
        return tag;
    }
    
    // TTE_DEL (with its destructor) will be called when the lua GC decides there are no more references to this object pushed.
    // pass in a tag integer, so you can check the type in the future with GetScriptObjectTag. Cannot be 0!
    template<typename T>
    inline void PushScriptOwned(LuaManager& man, T* obj, U32 tag)
    {
        TTE_ASSERT(tag != 0, "Tag value is not allowed to be 0");
        man.PushOpaque(obj); // push
        
        man.PushTable(); // meta table
        
        man.PushLString("__gc");
        man.PushFn(&__InternalDeleter<T>);
        man.SetTable(-3); // set gc method
        
        if(tag != 0)
        {
            man.PushLString("__ttetag");
            man.PushUnsignedInteger(tag);
            man.SetTable(-3);
        }
        
        man.SetMetaTable(-2);
    }
    
}
