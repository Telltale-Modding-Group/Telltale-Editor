#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>

using namespace Meta; // include all meta from this TU

// LOWER LEVEL SCRIPTING API FOR LUA FUNCTIONS DEFINED IN THE GAME ENGINE (eg ContainerGetNumElements)

// LIBRARY SCRIPTING API IS IN NORMAL META.CPP. THESE FUNCTIONS ARE DEFINED PER GAME IN THEIR GAME LUA SCRIPTS.

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
    
    
    
    return 1;
}

static U32 luaContainerRemoveElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    
    
    return 1;
}

static U32 luaContainerGetElement(LuaManager& man)
{
    TTE_ASSERT(man.GetTop() == 2, "Requires two args");
    
    
    
    return 1;
}

LuaFunctionCollection luaMetaGameEngine() {
    LuaFunctionCollection col{};
    
    // 'LuaContainer'
    col.Functions.push_back({"MetaContainerGetNumElements", &luaContainerGetNumElements});
    col.Functions.push_back({"MetaContainerRemoveElement", &luaContainerRemoveElement});
    col.Functions.push_back({"MetaContainerInsertElement", &luaContainerInsertElement});
    col.Functions.push_back({"MetaContainerGetElement", &luaContainerGetElement});
    
    return col;
}
