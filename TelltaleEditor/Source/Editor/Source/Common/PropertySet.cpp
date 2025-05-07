#include <Common/PropertySet.hpp>

void PropertySet::RegisterScriptAPI(LuaFunctionCollection& Col)
{
    
}

void PropertySet::AddCallback(Meta::ClassInstance prop, String key, Ptr<FunctionBase> pCallback)
{
    auto keyInfo = prop._GetInternalChildrenRefs()->find(Symbol(key));
    if(keyInfo != prop._GetInternalChildrenRefs()->end())
    {
        keyInfo->second.AddCallback(std::move(pCallback));
    }
}

Meta::ClassInstance PropertySet::Get(Meta::ClassInstance prop, String key)
{
    if(!prop)
        return {};
    auto it = prop._GetInternalChildrenRefs()->find(key);
    if(it == prop._GetInternalChildrenRefs()->end())
        return {};
    return it->second;
}

void PropertySet::Set(Meta::ClassInstance prop, String key, Meta::ClassInstance value)
{
    auto keyInfo = prop._GetInternalChildrenRefs()->find(Symbol(key));
    if(keyInfo != prop._GetInternalChildrenRefs()->end())
    {
        value._Callbacks = std::move(keyInfo->second._Callbacks); // keep callbacks
        keyInfo->second = std::move(value);
        keyInfo->second.ExecuteCallbacks();
    }
    else
    {
        prop._GetInternalChildrenRefs()->operator[](key) = std::move(value);
    }
}

Bool PropertySet::ExistsKey(Meta::ClassInstance prop, String keyName, Bool bSearchParents)
{
    Symbol kn = keyName;
    for(auto& local: *prop._GetInternalChildrenRefs())
    {
        if(kn == local.first)
            return true;
    }
    TTE_ASSERT(!bSearchParents, "impl");
    return false;
}
