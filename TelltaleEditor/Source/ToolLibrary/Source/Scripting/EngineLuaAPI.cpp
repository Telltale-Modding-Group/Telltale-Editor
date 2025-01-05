#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>

using namespace Meta; // include all meta from this TU

// LOWER LEVEL SCRIPTING API FOR LUA FUNCTIONS DEFINED IN THE GAME ENGINE (eg ContainerGetNumElements)

// LIBRARY SCRIPTING API IS IN NORMAL LIBRARYLUAAPI.CPP. THESE FUNCTIONS ARE DEFINED PER GAME IN THEIR GAME LUA SCRIPTS.

static U32 luaContainerGetNumElements(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 1, "Requires one argument");
    
    U32 num = 0; // return value
    
    ClassInstance inst = AcquireScriptInstance(man, -1);
    
    if(inst && IsCollection(inst))
        num = CastToCollection(inst).GetSize();
    else
        TTE_LOG("At ContainerGetNumElements: container was null or invalid");
    
    man.PushUnsignedInteger(num); // push num elements
    
    return 1;
}

static U32 luaContainerInsertElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        collection.PushValue(AcquireScriptInstance(man, -1), true); // copy value to container
    }
    else
    {
        TTE_LOG("At ContainerInsertElement: container was null or invalid");
    }
    
    return 0;
}

static U32 luaContainerRemoveElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        I32 index = man.ToInteger(-1);
        if(index >= 0 && index < (I32)collection.GetSize())
            collection.PopValue((U32)index, inst); // put into inst, as we dont need it anymore
    }
    else
        TTE_LOG("At ContainerRemoveElement: container was null or invalid");
    
    return 0;
}

static U32 luaContainerGetElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    ClassInstance inst = AcquireScriptInstance(man, -2);
    
    if(inst && IsCollection(inst))
    {
        ClassInstanceCollection& collection = CastToCollection(inst);
        I32 index = man.ToInteger(-1);
        inst = collection.GetValue((U32)index);
        // tool uses weak references
        inst.PushScriptRef(man);
    }
    else
    {
        TTE_LOG("At ContainerGetElement: container was null or invalid");
        man.PushNil();
    }
    
    return 1;
}

LuaFunctionCollection luaGameEngine() {
    LuaFunctionCollection col{};
    
    // 'LuaContainer'
    col.Functions.push_back({"_ContainerGetNumElements", &luaContainerGetNumElements});
    col.Functions.push_back({"_ContainerRemoveElement", &luaContainerRemoveElement});
    col.Functions.push_back({"_ContainerInsertElement", &luaContainerInsertElement});
    col.Functions.push_back({"_ContainerGetElement", &luaContainerGetElement});
    
    return col;
}
