#pragma once

#include <Scripting/LuaManager.hpp>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <Core/Symbol.hpp>

// High level LUA scripting API. This builds upon the Lua Manager which leads with the
// lua API for different lua versions supported by the range of Telltale Games.

// This has no state at all, lua manager and lua_State captures it all in the stack.

/**
 Gets this current thread LVM. If this is the main thread, gets the current tool context LVM. If a worker thread, then returns the worker thread. Any other thread will fail an assert.
 This will ALWAYS return a VM which uses the latest Lua script version telltale uses, Lua 5.2.3!
 */
LuaManager& GetThreadLVM();

#define LUA_MULTRET (-1)

// Registerable C function
struct LuaFunctionRegObject
{
    String Name;
    LuaCFunction Function = nullptr;
    String Declaration, Description;
};

// Collection of registerable C functions providing C API.
struct LuaFunctionCollection
{
    
    String Name;
    std::vector<LuaFunctionRegObject> Functions;
    
    std::unordered_map<String, String> StringGlobals; // global name to value to reg
    std::unordered_map<String, U32> IntegerGlobals; // global name to value to reg
    
    std::unordered_map<String, String> GlobalDescriptions; // global name to description
    
    std::vector<LuaCFunction> GenerateMetaTables; // generates meta table for scripts, see lua manager LuaConstantMetaTable
    
};

#define PUSH_FUNC(Col, Name, Fn, Decl, Desc) Col.Functions.push_back({Name, Fn, Decl, Desc})

#define PUSH_GLOBAL_S(Col, Name, Str, Desc) Col.StringGlobals[Name] = Str; Col.GlobalDescriptions[Name] = Desc

#define PUSH_GLOBAL_I(Col, Name, Val, Desc) Col.IntegerGlobals[Name] = Val; Col.GlobalDescriptions[Name] = Desc

#define PUSH_GLOBAL_DESC(Col, Name, Desc) Col.GlobalDescriptions[Name] = Desc

// Helper macro to iterate over a lua table. 'it' is the variable name for the iterator. table is the table index on the stack (can be relative to this macro invocation).
#define ITERATE_TABLE(it, table) for(auto it = ScriptManager::TableIterator(man, table); it != ScriptManager::TableIterator(man, table, true); ++it)

// Provides high level scripting access. Most of the functions are the same as Telltales actual ScriptManager API.
namespace ScriptManager
{
    
    // Decrypts scripts into a new readable stream.
    DataStreamRef DecryptScript(DataStreamRef& src);
    
    // Encrypts scripts into a new telltale stream. Specify if the given file is a .lenc file, as opposed to .lua.
    DataStreamRef EncryptScript(DataStreamRef& src, Bool bLenc);
    
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
    void RegisterCollection(LuaManager& man, const LuaFunctionCollection& collection);
    
    // Loads a chunk of lua code but does not run it, pushing it onto the top of the stack.
    // Pass in the name for debugging purposes, eg the file name.
    // If this returns false, nothing is pushed and error messages are logged to console.
    inline Bool LoadChunk(LuaManager& man, const String& name, const String& text){
        return man.LoadChunk(name, (const U8*)text.c_str(), (U32)text.length(), LoadChunkMode::SOURCE);
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
    inline void RunText(LuaManager& man, const String& code, CString Chunkname, Bool lockContext)
    {
        man.RunText(code.c_str(), (U32)code.length(), lockContext, Chunkname);
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
        T** ppObj = reinterpret_cast<T**>(man.ToPointer(-1));
        if (ppObj && *ppObj)
        {
            TTE_DEL(*ppObj);
            *ppObj = nullptr;
        }
        return 0;
    }
    
    // INTERNAL. DON'T USE.
    inline U32 __InternalDeleterWeak(LuaManager& man)
    {
        man.GetMetatable(-1);
        man.PushLString("__ttedel");
        man.GetTable(-2);
        if(man.Type(-1) != LuaType::NIL)
        {
            man.Pop(2); // pop nonnil and mt
        }
        else
        {
            man.Pop(1); // pop nil
            man.PushLString("__ttedel");
            man.PushBool(true);
            man.SetTable(-3);
            man.Pop(1); // pop mt
            WeakPtr<Handleable>& ptr = *((WeakPtr<Handleable>*)man.ToPointer(-1));
            ptr.~WeakPtr<Handleable>();
        }
        return 0;
    }
    
    // returns 0 if invalid, or not found.
    inline U32 GetScriptObjectTag(LuaManager& man, I32 stackIndex)
    {
        if(man.Type(stackIndex) == LuaType::FULL_OPAQUE)
        {
            stackIndex = man.ToAbsolute(stackIndex);
            man.GetMetatable(stackIndex);
            man.PushLString("__ttetag");
            man.GetTable(-2);
            U32 tag = (U32)PopInteger(man);
            man.Pop(1);
            return tag & 0x7FFF'FFFF;
        }
        return 0;
    }
    
    // TTE_DEL (with its destructor) will be called when the lua GC decides there are no more references to this object pushed.
    // pass in a tag integer, so you can check the type in the future with GetScriptObjectTag. Cannot be 0!
    template<typename T>
    inline void PushScriptOwned(LuaManager& man, T* obj, U32 tag)
    {
        TTE_ASSERT(tag != 0, "Tag value is not allowed to be 0");
        
        T** ppObj = (T**)man.CreateUserData(sizeof(T*));
        *ppObj = obj;
        
        man.PushTable(); // meta table
        
        man.PushLString("__ttetag");
        man.PushUnsignedInteger(tag);
        man.SetTable(-3);
        
        man.PushLString("__gc");
        man.PushFn(&__InternalDeleter<T>);
        man.SetTable(-3); // set gc method
        
        man.SetMetaTable(-2);
    }
    
    // Pushes the weak pointer. All accesses will check its valid here first.
    inline void PushWeakScriptReference(LuaManager& man, WeakPtr<Handleable> obj)
    {
        WeakPtr<Handleable>* pWeak = new (man.CreateUserData(sizeof(WeakPtr<Handleable>))) WeakPtr<Handleable>(std::move(obj));
        
        man.PushTable(); // meta table
        
        man.PushLString("__ttetag");
        man.PushUnsignedInteger((U32)ObjectTag::HANDLEABLE | 0x8000'0000u); // top bit => means weak
        man.SetTable(-3);
        
        man.PushLString("__gc");
        man.PushFn(&__InternalDeleterWeak);
        man.SetTable(-3); // set gc method
        
        man.SetMetaTable(-2);
    }
    
    // Gets the underlying object pointer for a script owned object. Its type is not checked, check get get object tag.
    template<typename T>
    inline T* GetScriptObject(LuaManager& man, I32 stackIndex)
    {
        man.GetMetatable(stackIndex);
        man.PushLString("__ttetag");
        man.GetTable(-2);
        U32 tag = (U32)PopInteger(man);
        man.Pop(1);
        if(tag >> 31)
        {
            WeakPtr<T>& wk = *((WeakPtr<T>*)man.ToPointer(stackIndex));
            return wk.expired() ? nullptr : wk.lock().get();
        }
        return *(T**)man.ToPointer(stackIndex);
    }
    
    // Must have been pushed using PushWeakScriptOwned.
    template<typename T>
    inline WeakPtr<T> GetScriptObjectWeak(LuaManager& man, I32 stackIndex)
    {
        man.GetMetatable(stackIndex);
        man.PushLString("__ttetag");
        man.GetTable(-2);
        U32 tag = (U32)PopInteger(man);
        TTE_ASSERT((tag >> 31) == 1, "Can only be used with objects pushed as a weak pointer.");
        man.Pop(1);
        WeakPtr<T>& wk = *((WeakPtr<T>*)man.ToPointer(stackIndex));
        return wk;
    }
    
    // Converts a string. If it is in a valid hex format for symbols, that hash is used, else the string is hashed.
    Symbol ToSymbol(LuaManager& man, I32 i);
    // Tries to find it from the global symbol table first, if not then returns a hex string.
    void PushSymbol(LuaManager& man, Symbol value);

    // Use macro helper to iterate.
    class TableIterator {
    public:
        
        inline TableIterator(LuaManager& man, I32 tableIndex, Bool isEnd = false)
        : _Man(man), _TableIndex(man.ToAbsolute(tableIndex)), _Finished(isEnd)
        {
            if (!_Finished) {
                _Man.PushNil();
                _Finished = !_Man.TableNext(_TableIndex);
                _State();
            }
        }
        
        // Move to next item
        inline TableIterator& operator++()
        {
            if (_Finished)
                return *this;
            _Man.Pop(1); // Pop value, leave key
            _Finished = !_Man.TableNext(_TableIndex);
            _State();
            return *this;
        }
        
        Bool operator!=(const TableIterator& other) const
        {
            return _Finished != other._Finished;
        }
        
        // Provide absolute indices for key/value
        inline I32 KeyIndex() const
        {
            return _KeyIndex;
        }
        
        inline I32 ValueIndex() const
        {
            return _ValueIndex;
        }
        
    private:
        
        inline void _State()
        {
            if (_Finished)
            {
                _KeyIndex = _ValueIndex = -1;
            }
            else
            {
                _KeyIndex = _Man.GetTop() - 1;
                _ValueIndex = _Man.GetTop();
            }
        }
        
        LuaManager& _Man;
        I32 _TableIndex;
        I32 _KeyIndex = -1;
        I32 _ValueIndex = -1;
        Bool _Finished = true;
        
    };
    
}

LuaFunctionCollection luaGameEngine(Bool bWorker); // actual engine game api in EngineLUAApi.cpp
LuaFunctionCollection luaLibraryAPI(Bool bWorker); // actual api in LibraryLUAApi.cpp

inline void InjectFullLuaAPI(LuaManager& man, Bool bWorker)
{
    LuaFunctionCollection Col = luaGameEngine(bWorker); // adds resource api, prop
    ScriptManager::RegisterCollection(man, Col);
    Col = luaLibraryAPI(bWorker);
    ScriptManager::RegisterCollection(man, Col);
    CString dumpTable =
    R"(
    function TableToString(o)
       if type(o) == 'table' then
          local s = '{ '
          for k,v in pairs(o) do
             if type(k) ~= 'number' then k = '"'..k..'"' end
             s = s .. '['..k..'] = ' .. TableToString(v) .. ', '
          end
          return s .. '} '
       else
          return tostring(o)
       end
    end
    )";
    man.RunText(dumpTable, (U32)strlen(dumpTable), false, "TableToString.lua");
}
