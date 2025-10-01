#include <Resource/PropertySet.hpp>
#include <cstdio>

// get prop at stack index.
static Meta::ClassInstance luaProp(LuaManager& man, I32 index)
{
    if(man.Type(index) == LuaType::STRING)
    {
        Symbol file = ScriptManager::ToSymbol(man, index);
        HandlePropertySet hProp{};
        return hProp.GetObject(ResourceRegistry::GetBoundRegistry(man), true);
    }
    else if(man.Type(index) == LuaType::NIL)
        return {};
    return Meta::AcquireScriptInstance(man, index);
}

#define ENSUREPROP(prop, index) Meta::ClassInstance prop = luaProp(man, index); \
if(!prop) { TTE_LOG("ScriptError at PropertySet API: argument %d property set was not valid or was unloaded", index); return 0; }

static U32 luaPropertyExists(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    man.PushBool(PropertySet::ExistsKey(prop, kn, man.GetTop() == 3 ? man.ToBool(3) : true, ResourceRegistry::GetBoundRegistry(man)));
    return 1;
}

static U32 luaPropertyHasGlobal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol p = ScriptManager::ToSymbol(man, 2);
    man.PushBool(PropertySet::IsMyParent(prop, p, man.GetTop() == 3 ? man.ToBool(3) : true, ResourceRegistry::GetBoundRegistry(man)));
    return 1;
}

static U32 luaPropertyIsLocal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol p = ScriptManager::ToSymbol(man, 2);
    man.PushBool(PropertySet::IsMyParent(prop, p, man.GetTop() == 3 ? man.ToBool(3) : true, ResourceRegistry::GetBoundRegistry(man)));
    return 1;
}

static U32 luaPropertyMakeLocal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    man.PushBool(PropertySet::PromoteKeyToLocal(prop, kn, ResourceRegistry::GetBoundRegistry(man)));
    return 1;
}

static U32 luaPropertyAddGlobal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    PropertySet::AddParent(prop, kn, ResourceRegistry::GetBoundRegistry(man));
    return 0;
}

static U32 luaPropertyCreate(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    String tn = man.ToString(3);
    if(tn == "int")
        tn = "int32";
    U32 clz = Meta::FindClass(tn, 0);
    if(clz == 0)
    {
        TTE_LOG("ScriptError(PropertyCreate): Type name \"%s\" is invalid", tn.c_str());
        return 0;
    }
    Meta::ClassInstance value = Meta::CreateInstance(clz);
    if(man.GetTop() == 4)
    {
        if(!Meta::CoerceLuaToMeta(man, 4, value))
            TTE_LOG("Failed to assign property %s from script: argument type was invalid", GetRuntimeSymbols().FindOrHashString(kn).c_str());
    }
    PropertySet::Set(prop, kn, value, ResourceRegistry::GetBoundRegistry(man), PropertySet::SetPropertyMode::DIRECT);
    return 0;
}

static U32 luaPropertySet(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    Meta::ClassInstance value = PropertySet::Get(prop, kn, true, ResourceRegistry::GetBoundRegistry(man));
    if(!value)
    {
        TTE_LOG("Failed to assigned property %s: does not exist. Use PropertyCreate first!", GetRuntimeSymbols().FindOrHashString(kn).c_str());
        return 0;
    }
    if(!Meta::CoerceLuaToMeta(man, 3, value))
    {
        TTE_LOG("Failed to assign property %s from script: argument type was invalid", GetRuntimeSymbols().FindOrHashString(kn).c_str());
    }
    else
    {
        PropertySet::MarkModified(prop, kn, ResourceRegistry::GetBoundRegistry(man)); // on key modified (Call cbs)
    }
    return 0;
}

static U32 luaPropertyGet(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    prop = PropertySet::Get(prop, kn, true, ResourceRegistry::GetBoundRegistry(man));
    if(!prop)
    {
        man.PushNil();
        return 1;
    }
    if(!Meta::CoerceMetaToLua(man, prop))
    {
        man.PushNil();
        TTE_LOG("At PropertyGet: property value type %s is not coercable into lua as of yet. Please contact us (returning nil)", Meta::GetClass(prop.GetClassID()).Name.c_str());
    }
    return 1;
}

static U32 luaPropertySetIndex(LuaManager& man)
{
    return luaPropertyGet(man);
}

static U32 luaPropertySetNewIndex(LuaManager& man)
{
    return luaPropertySet(man);
}

static U32 luaPropertySetToString(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    String nm = ResourceRegistry::GetBoundRegistry(man)->LocateResource(prop._GetInternal());
    if(nm.length() == 0)
    {
        U8 Buf[128];
        sprintf((char*)Buf, "<anonymous at address: %p>", prop._GetInternal());
        man.PushLString(String((CString)Buf));
    }
    else
        man.PushLString(nm);
    return 1;
}

static U32 luaPropertySetMetatable(LuaManager& man)
{
    man.PushTable();
    
    man.PushLString("__index");
    man.PushFn(&luaPropertySetIndex);
    man.SetTable(-3);
    
    man.PushLString("__newindex");
    man.PushFn(&luaPropertySetNewIndex);
    man.SetTable(-3);
    
    man.PushLString("__tostring");
    man.PushFn(&luaPropertySetToString);
    man.SetTable(-3);
    
    man.RegisterConstantMetaTable(LuaConstantMetaTable::PROP);
    
    return 0;
}

static U32 luaPropertyDontSaveInSaveGames(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    PropertySet::GetFlags(prop) |= PropertySet::Flag::DONT_STORE_IN_SAVE_GAMES;
    return 0;
}

static U32 luaPropertySetImport(LuaManager& man)
{
    ENSUREPROP(dest, 1);
    ENSUREPROP(source, 2);
    if(man.GetTop() == 3)
    {
        ENSUREPROP(keys, 3);
        PropertySet::ImportKeysValuesAndParents(dest, source, true, true, keys, true, false, ResourceRegistry::GetBoundRegistry(man));
    }
    else
    {
        PropertySet::ImportKeysValuesAndParents(dest, source, true, true, Meta::ClassInstance{}, true, false, ResourceRegistry::GetBoundRegistry(man));
    }
    return 0;
}

static U32 luaPropertyPrintKeys(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    std::set<Symbol> keys{};
    PropertySet::GetKeys(prop, keys, true, ResourceRegistry::GetBoundRegistry(man));
    for(auto key: keys)
    {
        String symbol = SymbolTable::FindOrHashString(key);
        TTE_LOG("%s", symbol.c_str());
    }
    return 0;
}

static U32 luaPropertyRemove(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    PropertySet::RemoveKey(prop, kn);
    return 0;
}

static U32 luaPropertyRemoveGlobal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol p{};
    Ptr<ResourceRegistry> pRegistry = ResourceRegistry::GetBoundRegistry(man);
    if(man.Type(2) == LuaType::STRING)
        p = ScriptManager::ToSymbol(man, 2);
    else
        p = pRegistry->LocateResource(Meta::AcquireScriptInstance(man, 2)._GetInternal());
    Bool bDiscardLocals = man.GetTop() > 2 ? man.ToInteger(3) == 2 : true; // 0 being 'prompt' in the engine. maybe do this? (message box and UI needed)
    PropertySet::RemoveParent(prop, p, bDiscardLocals, pRegistry);
    return 0;
}

static U32 luaPropertyGetKeyPropertySet(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    HandlePropertySet introduced = PropertySet::GetPropertySetKeyIsIntroducedFrom(prop, kn, ResourceRegistry::GetBoundRegistry(man));
    if(introduced)
    {
        Meta::ImportCoercableLuaValue(introduced, man);
    }
    else
    {
        man.PushNil();
    }
    return 1;
}

static U32 luaPropertyGetValuePropertySet(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    HandlePropertySet introduced = PropertySet::GetPropertySetValueIsRetrievedFrom(prop, kn, ResourceRegistry::GetBoundRegistry(man), true);
    if(introduced)
    {
        Meta::ImportCoercableLuaValue(introduced, man);
    }
    else
    {
        man.PushNil();
    }
    return 1;
}

static U32 luaPropertyKeys(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    man.PushTable();
    std::set<Symbol> keys{};
    PropertySet::GetKeys(prop, keys, true, ResourceRegistry::GetBoundRegistry(man));
    I32 index = 1;
    for(auto key: keys)
    {
        String symbol = SymbolTable::FindOrHashString(key);
        man.PushInteger(index++);
        man.PushLString(symbol);
        man.SetTable(-3);
    }
    return 1;
}

static U32 luaPropertyGetKeyType(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    prop = PropertySet::Get(prop, kn, true, ResourceRegistry::GetBoundRegistry(man));
    if(prop)
    {
        String tn = Meta::GetClass(prop.GetClassID()).Name;
        man.PushLString(tn);
    }
    else
        man.PushNil();
    return 1;
}

static U32 luaPropertyClearKeys(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    PropertySet::ClearKeys(prop);
    return 0;
}

static U32 luaPropertyGetGlobals(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    man.PushTable();
    std::set<HandlePropertySet> parents{};
    PropertySet::GetParents(prop, parents, true, ResourceRegistry::GetBoundRegistry(man));
    I32 index = 1;
    for(auto p: parents)
    {
        String symbol = SymbolTable::FindOrHashString(p.GetObject());
        man.PushInteger(index++);
        man.PushLString(symbol);
        man.SetTable(-3);
    }
    return 1;
}

static U32 luaPropertyIsContainer(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    prop = PropertySet::Get(prop, kn, true, ResourceRegistry::GetBoundRegistry(man));
    if(prop)
    {
        man.PushBool((Meta::GetClass(prop.GetClassID()).Flags & Meta::CLASS_CONTAINER) != 0);
    }
    else
        man.PushBool(false);
    return 1;
}

static U32 luaPropertyMoveGlobal(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    PropertySet::MoveParentToFront(prop, kn);
    return 0;
}

static U32 luaPropertyNumKeys(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    man.PushUnsignedInteger(PropertySet::GetNumKeys(prop, man.ToBool(2), ResourceRegistry::GetBoundRegistry(man)));
    return 1;
}

static U32 luaPropertyRuntime(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    if((PropertySet::GetFlags(prop) & PropertySet::RUNTIME_PROP) == 0)
        man.PushNil();
    return 1;
}

static U32 luaPropertyModified(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    PropertySet::CallAllCallbacks(prop, ResourceRegistry::GetBoundRegistry(man));
    return 0;
}

static U32 luaPropertyEmpty(LuaManager& man)
{
    return 0;
}

static U32 luaPropertyAddCallback(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol kn = ScriptManager::ToSymbol(man, 2);
    Ptr<LuaPropertyKeyCallback> pCallback = TTE_NEW_PTR(LuaPropertyKeyCallback, MEMORY_TAG_CALLBACK, man);
    if(man.Type(-1) == LuaType::STRING)
    {
        String function = man.ToString(3);
        pCallback->SetFunction(function);
    }
    else
    {
        if(man.Type(-1) != LuaType::FUNCTION)
        {
            TTE_LOG("ScriptError(PropertyAddKeyCallback): not a function");
            return 0;
        }
        pCallback->SetFunction(3);
    }
    pCallback->Tag = kn;
    PropertySet::AddCallback(prop, kn, std::move(pCallback));
    return 0;
}

static U32 luaPropertyAddMultiKeyCallback(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    ENSUREPROP(keysFrom, 2);
    Bool bSearchParents = man.GetTop() >= 4 ? man.ToBool(4) : true;
    std::set<Symbol> keys{};
    if(!keysFrom || keysFrom._GetInternal() == prop._GetInternal())
    {
        PropertySet::GetKeys(prop, keys, bSearchParents, ResourceRegistry::GetBoundRegistry(man));
    }
    else
    {
        PropertySet::GetKeys(keysFrom, keys, bSearchParents, ResourceRegistry::GetBoundRegistry(man));
    }
    for(auto key: keys)
    {
        Ptr<LuaPropertyKeyCallback> pCallback = TTE_NEW_PTR(LuaPropertyKeyCallback, MEMORY_TAG_CALLBACK, man);
        if(man.Type(-1) == LuaType::STRING)
        {
            String function = man.ToString(3);
            pCallback->SetFunction(function);
        }
        else
        {
            if(man.Type(-1) != LuaType::FUNCTION)
            {
                TTE_LOG("ScriptError(PropertyAddMultiKeyCallback): not a function");
                return 0;
            }
            pCallback->SetFunction(3);
        }
        pCallback->Tag = key;
        PropertySet::AddCallback(prop, key, std::move(pCallback));
    }
    return 0;
}

static U32 luaPropertyRemoveKeyCallback(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    Symbol key = ScriptManager::ToSymbol(man, 2);
    if(man.GetTop() == 3)
    {
        LuaPropertyKeyCallback dummy{man};
        // has lua function to check against
        if(man.Type(-1) == LuaType::STRING)
        {
            String function = man.ToString(3);
            dummy.SetFunction(function);
        }
        else
        {
            if(man.Type(-1) != LuaType::FUNCTION)
            {
                TTE_LOG("ScriptError(PropertyRemoveKeyCallback): not a function");
                return 0;
            }
            dummy.SetFunction(3);
        }
        PropertySet::RemoveCallback(prop, key, TTE_PROXY_PTR(&dummy, FunctionBase));
    }
    else
    {
        PropertySet::RemoveCallback(prop, key, nullptr);
    }
    return 0;
}

static U32 luaPropertyRemoveMultiKeyCallback(LuaManager& man)
{
    ENSUREPROP(prop, 1);
    ENSUREPROP(keysFrom, 2);
    Bool bSearchParents = man.GetTop() >= 4 ? man.ToBool(4) : true;
    std::set<Symbol> keys{};
    if(!keysFrom || keysFrom._GetInternal() == prop._GetInternal())
    {
        PropertySet::GetKeys(prop, keys, bSearchParents, ResourceRegistry::GetBoundRegistry(man));
    }
    else
    {
        PropertySet::GetKeys(keysFrom, keys, bSearchParents, ResourceRegistry::GetBoundRegistry(man));
    }
    LuaPropertyKeyCallback dummy{man};
    Bool bSearchCallback = false;
    if(man.GetTop() >= 3)
    {
        bSearchCallback = true;
        // has lua function to check against
        if(man.Type(-1) == LuaType::STRING)
        {
            String function = man.ToString(3);
            dummy.SetFunction(function);
        }
        else
        {
            if(man.Type(-1) != LuaType::FUNCTION)
            {
                TTE_LOG("ScriptError(PropertyRemoveMultiKeyCallback): not a function");
                return 0;
            }
            dummy.SetFunction(3);
        }
    }
    for(auto key: keys)
    {
        PropertySet::RemoveCallback(prop, key, bSearchCallback ? TTE_PROXY_PTR(&dummy, FunctionBase) : nullptr);
    }
    return 0;
}

void PropertySet::InjectScriptAPI(LuaFunctionCollection& Col)
{
    PUSH_FUNC(Col, "PropertyRemoveMultiKeyCallback", &luaPropertyRemoveMultiKeyCallback,
              "nil PropertyRemoveMultiKeyCallback(propertySet,keysFromPropertySet,luaFuncName)",
              "Remove a luaFunction that was registered for changes to a key in keysFromPropertySet when modified. "
              "If keysFromPropertySet is nil, then the callback is removed for each key in propertySet."
              " Optional 4th parameter to search parents (default true).");
    PUSH_FUNC(Col, "PropertyRemoveKeyCallback", &luaPropertyRemoveKeyCallback, "nil PropertyRemoveKeyCallback(propertySet,key,luaFuncName)",
              "Remove the luaFunction registered to be called whenever the key in the given property set is modified.");
    PUSH_FUNC(Col, "PropertyAddMultiKeyCallback", &luaPropertyAddCallback, "nil PropertyAddMultiKeyCallback(propertySet,keysFromPropertySet,luaFuncName)",
              "Add a luaFunction to be called whenever a key in keysFromPropertySet is modified. "
              "If keysFromPropertySet is nil, then the callback will occur for any key modified in propertySet. "
              " The function gets called with the key and the new data for the key. Optional 4th argument to use all non-local parent keys when traversing keys from."
              " It is default true. The keys from property set is not cached in any way and the keys with adding callbacks are taken at this call."
              " Keys from can also be nil meaning the callback is added to every single value in the property set at the time of this call.");
    PUSH_FUNC(Col, "PropertyAddKeyCallback", &luaPropertyAddCallback, "nil PropertyAddKeyCallback(propertySet, key, luaFuncName)",
              "Adds a callback to be called every time the key is modified. Pass in the lua function name for compatibility, while in most new games"
              " the function can also be the function lua object. The function gets called with the key and the new data for the key.");
    PUSH_FUNC(Col, "PropertySetModified", &luaPropertyModified, "nil PropertySetModified(propertySet)", "Sets the property set as modified, calling every "
              "local key in the property set's callback.");
    PUSH_FUNC(Col, "PropertyClearKeyCallbacks", &luaPropertyEmpty, "nil PropertyClearKeyCallbacks(propertySet)", "Does nothing (in the engine either).");
    PUSH_FUNC(Col, "PropertyIsRuntime", &luaPropertyRuntime, "propertySet PropertyIsRuntime(propertySet)",
              "Returns the input property set if the property set is a runtime property set, else nil.");
    PUSH_FUNC(Col, "PropertyNumKeys", &luaPropertyNumKeys, "int PropertyNumKeys(propertySet, countGlobals)",
              "Returns the number of keys in the property set. Set the second argument to true to count global parent keys as well.");
    PUSH_FUNC(Col, "PropertyMoveGlobalToFront", &luaPropertyMoveGlobal, "nil PropertyMoveGlobalToFront(propertySet, parentName)",
              "Moves the given parent property to the front of the parent array. Does not need to contain it, if it does it is moved, else added to the front."
              " This means that it will be searched first for any non local keys.");
    PUSH_FUNC(Col, "PropertyClearKeys", &luaPropertyClearKeys, "nil PropertyClearKeys(propertySet)", "Clears all local property keys.");
    PUSH_FUNC(Col, "PropertyGetGlobals", &luaPropertyGetGlobals, "table PropertyGetGlobals(propertySet)", "Gets all global parent property file names.");
    PUSH_FUNC(Col, "PropertyIsContainer", &luaPropertyIsContainer, "bool PropertyIsContainer(propertySet, keyName)", "Tests if the given property is a container"
              " type, such as an array or map.");
    Col.GenerateMetaTables.push_back(&luaPropertySetMetatable);
    PUSH_FUNC(Col, "PropertyExists", &luaPropertyExists, "bool PropertyExists(prop, keyName)", "Returns true if the given property exists, false otherwise."
              " Optional 3rd parameter to check parents, default true.");
    PUSH_FUNC(Col, "PropertyHasGlobal", &luaPropertyHasGlobal, "bool PropertyHasGlobal(propertySet,parentPropertySet)",
              "Does this property set use this parent? Optional 3rd parameter to check parents, default true");
    PUSH_FUNC(Col, "PropertyIsLocal", &luaPropertyIsLocal, "bool PropertyIsLocal(propertySet,key)", "Is the key 'local' to the property set?"
              " This means that it exists in the property set and not through one of the parents.");
    PUSH_FUNC(Col, "PropertyMakeLocal", &luaPropertyMakeLocal, "bool PropertyMakeLocal(propertySet,key)",
              "Make the key 'local' to the property set. See IsLocal.");
    PUSH_FUNC(Col, "PropertyAddGlobal", &luaPropertyAddGlobal, "nil PropertyAddGlobal(propertySet,parentPropertySet)",
              "Add a global parent to a property set. Parent property set should be a symbol or string filename.");
    PUSH_FUNC(Col, "PropertyCreate", &luaPropertyCreate, "nil PropertyCreate(propertySet,keyName,typeName,--[[optional]] value)",
              "Add a value to the property set. Valid type names are: \"bool\", \"Color\", etc."
              " Optional 4th argument is the property value to assign. Don't include and 'class ' etc specifiers, for compatibility with all games (you can if you really want)");
    PUSH_FUNC(Col, "PropertySet", &luaPropertySet, "nil PropertySet(propertySet,keyName,value)", "Set a value in a property set. The key name must exist.");
    PUSH_FUNC(Col, "PropertyGet", &luaPropertyGet, "value PropertyGet(propertySet,keyName)", "Get a value from the property set");
    PUSH_FUNC(Col, "PropertyDontSaveInSaveGames", &luaPropertyDontSaveInSaveGames, "nil PropertyDontSaveInSaveGames(propertySet)",
              "Flags this property set to ignore runtime changes (so they aren't saved in save games.");
    PUSH_FUNC(Col, "PropertyImportKeyValues", &luaPropertySetImport,
              "nil PropertyImportKeyValues(destPropertySet,sourcePropertySet,keysFromPropertySet)",
              "Import all the keys in property set \"keysFromPropertySet\" from property set\"sourcePropertySe"
              "t\" to property set \"destPropertySet\". 'keysfromPropertySet' is the filter. "); // technically 3rd is optional but older games require it.
    PUSH_FUNC(Col, "PropertyPrintKeys", &luaPropertyPrintKeys, "nil PropertyPrintKeys(propertySet)", "Print the keys of the property set");
    PUSH_FUNC(Col, "PropertyGetKeyType", &luaPropertyGetKeyType, "string PropertyGetKeyType(propertySet, key)", "Returns the type name of the property value type");
    PUSH_FUNC(Col, "PropertyRemove", &luaPropertyRemove, "nil PropertyRemove(propertySet,keyName)", "Remove a value from the property set");
    PUSH_FUNC(Col, "PropertyRemoveGlobal", &luaPropertyRemoveGlobal, "nil PropertyRemoveGlobal(propertySet,parentPropertySet)",
              "Remove a global parent from a property set. Optional 3rd parameter being 1 to keep any local keys overriden by the global parent (default discards)");
    PUSH_FUNC(Col, "PropertyGetKeyPropertySet", &luaPropertyGetKeyPropertySet,
              "propertySet PropertyGetKeyPropertySet(propertySet,keyName)", "Get the property set a key is introduced from");
    PUSH_FUNC(Col, "PropertyGetValuePropertySet", &luaPropertyGetValuePropertySet, "propertySet PropertyGetValuePropertySet(propertySet,keyName)",
              "Get the property set a value is retrieved from. This is different to PropertyGetKeyPropertySet, where it finds the prop where the key is "
              "introduced from, while this finds the local'est' property set (closest to input set) there is an overriding value in for the key.");
    PUSH_FUNC(Col, "PropertyKeys", &luaPropertyKeys, "table PropertyKeys(propertySet)", "Get all the keys in a property set.");
}

void PropertySet::MoveParentToFront(Meta::ClassInstance prop, Symbol parent)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if (parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for (U32 i = 0; i < array.GetSize(); ++i)
        {
            HandlePropertySet hParent{};
            if(hParent.GetObject() == parent)
            {
                if(i == 0)
                    return; // already ok (at front)
                Meta::ClassInstance u{};
                array.PopValue(i, u);
                break;
            }
        }
        Meta::ClassInstance global = Meta::CreateInstance(Meta::FindClass(PropertySet::ClassHandle, 0)); // not a common class (its Handle<>) so OK
        Meta::ImportCoercableInstance(parent, global);
        array.Insert({}, std::move(global), 0, false, false);
    }
}

void PropertySet::RemoveCallback(Meta::ClassInstance prop, Symbol key, Ptr<FunctionBase> pMatchingCallback)
{
    if(key.GetCRC64() == 0 && !pMatchingCallback)
        return;
    InternalData& data = *((InternalData*)prop._GetInternalPropertySetData());
    Bool bRemoved;
    do
    {
        bRemoved = false;
        for(auto it = data.KeyCallbacks.begin(); it != data.KeyCallbacks.end(); it++)
        {
            const Ptr<FunctionBase>& pCallback = *it;
            if((key.GetCRC64() == 0 || pCallback->Tag == key) && (!pMatchingCallback || pCallback->Equals(*pMatchingCallback)))
            {
                data.KeyCallbacks.erase(it);
                bRemoved = true;
                break;
            }
        }
    } while(bRemoved);
}

HandlePropertySet PropertySet::GetPropertySetValueIsRetrievedFrom(Meta::ClassInstance prop, Symbol keyName, Ptr<ResourceRegistry> pRegistry, Bool bCheckMyself)
{
    if(bCheckMyself && ExistsKey(prop, keyName, false, pRegistry))
    {
        HandlePropertySet hMyself{};
        hMyself.SetObject(pRegistry->LocateResource(prop._GetInternal()));
        return hMyself;
    }
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if (parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for (U32 i = 0; i < array.GetSize(); ++i)
        {
            Meta::ClassInstance parent = array.GetValue(i);
            HandlePropertySet hParent{};
            Meta::ExtractCoercableInstance(hParent, parent);
            Meta::ClassInstance resolvedParent = hParent.GetObject(pRegistry, true);
            if (!resolvedParent)
                continue;
            HandlePropertySet found = GetPropertySetValueIsRetrievedFrom(resolvedParent, keyName, pRegistry, true);
            if (found)
                return found;
        }
    }
    
    return {};
}

HandlePropertySet _DoGetPSKIntroducedFrom(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry>& pRegistry, Symbol MyName)
{
    // If the key exists at this level and wasn't inherited, this is the origin.
    if (PropertySet::ExistsKey(prop, KeyName, false, pRegistry) && !PropertySet::ExistsParentKey(prop, KeyName, pRegistry))
    {
        HandlePropertySet h{};
        h.SetObject(MyName);
        return h;
    }
    
    // Traverse mParentList
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if (parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for (U32 i = 0; i < array.GetSize(); ++i)
        {
            Meta::ClassInstance parent = array.GetValue(i);
            HandlePropertySet hParent;
            Meta::ExtractCoercableInstance(hParent, parent);
            Meta::ClassInstance resolvedParent = hParent.GetObject(pRegistry, true);
            if (!resolvedParent)
                continue;
            
            HandlePropertySet found = _DoGetPSKIntroducedFrom(resolvedParent, KeyName, pRegistry, hParent.GetObject());
            if (found)
                return found;
        }
    }
    
    // Key not introduced in this branch
    return HandlePropertySet{};
}


HandlePropertySet PropertySet::GetPropertySetKeyIsIntroducedFrom(Meta::ClassInstance prop, Symbol keyName, Ptr<ResourceRegistry> reg)
{
    return _DoGetPSKIntroducedFrom(prop, keyName, reg, reg->LocateResource(prop._GetInternal()));
}

void PropertySet::ImportKeysValuesAndParents(Meta::ClassInstance prop, Meta::ClassInstance source,
                                             Bool bSearchParents, Bool bImportParents, Meta::ClassInstance filter,
                                             Bool bOverwriteExistingKeys, Bool bInvertFilter, Ptr<ResourceRegistry> pRegistry)
{
    std::set<Symbol> srcKeys;
    std::set<HandlePropertySet> srcParents;
    std::set<Symbol> filterKeys;
    std::set<HandlePropertySet> filterParents;
    std::set<HandlePropertySet> curParents;
    if(bImportParents)
        GetParents(prop, curParents, true, pRegistry);
    
    // --- SOURCE INFO ---
    GetKeys(source, srcKeys, bSearchParents, pRegistry);
    if (bSearchParents || bImportParents)
        GetParents(source, srcParents, bSearchParents, pRegistry);
    
    // --- FILTER INFO ---
    if (filter)
    {
        GetKeys(filter, filterKeys, bSearchParents, pRegistry);
        if (bSearchParents || bImportParents)
            GetParents(filter, filterParents, bSearchParents, pRegistry);
    }
    
    // --- IMPORT KEYS ---
    for (const Symbol& srcKey : srcKeys)
    {
        Bool inFilter = filter ? filterKeys.find(srcKey) != filterKeys.end() : true;
        Bool shouldImport = filter ? (bInvertFilter ? !inFilter : inFilter) : true;
        
        if (!shouldImport)
            continue;
        
        if (!bOverwriteExistingKeys && PropertySet::ExistsKey(prop, srcKey, bSearchParents, pRegistry))
            continue;
        
        PropertySet::Set(prop, srcKey, PropertySet::Get(source, srcKey, bSearchParents, pRegistry), pRegistry);
    }
    
    // --- HANDLE PARENTS ---
    if (bSearchParents || bImportParents)
    {
        for (auto srcParentHandle : srcParents)
        {
            Meta::ClassInstance srcParent = srcParentHandle.GetObject(pRegistry, true);
            if (!srcParent || srcParent == prop)
                continue;
            
            // RECURSE: search into parent for keys only
            if (bSearchParents)
            {
                PropertySet::ImportKeysValuesAndParents(prop, srcParent,true, bImportParents,filter, false, false, pRegistry);
            }
            
            // ADD PARENT: only if bImportParents is true
            if (bImportParents && curParents.find(srcParentHandle) == curParents.end())
            {
                AddParent(prop, srcParentHandle.GetObject(), pRegistry);
            }
        }
    }
}

void PropertySet::GetParents(Meta::ClassInstance prop, std::set<HandlePropertySet>& parents, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if(parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for(U32 i = 0; i < array.GetSize(); i++)
        {
            prop = array.GetValue(i);
            HandlePropertySet hProp{};
            Meta::ExtractCoercableInstance(hProp, prop);
            Symbol name = hProp.GetObject();
            if(bSearchParents && name)
                GetParents(hProp.GetObject(pRegistry, true), parents, true, pRegistry);
            if(name.GetCRC64())
                parents.insert(std::move(hProp));
        }
    }
}

void PropertySet::AddCallback(Meta::ClassInstance prop, Symbol key, Ptr<FunctionBase> pCallback)
{
    auto& callbacks = ((InternalData*)prop._GetInternalPropertySetData())->KeyCallbacks;
    if(pCallback)
    {
        pCallback->Tag = key;
        auto it = callbacks.find(pCallback);
        if(it != callbacks.end())
        {
            pCallback->Next = std::move(*it);
            callbacks.erase(it);
        }
        callbacks.insert(std::move(pCallback));
    }
}

Meta::ClassInstance PropertySet::Get(Meta::ClassInstance prop, Symbol key, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    if(!prop)
        return {};
    auto it = prop._GetInternalChildrenRefs()->find(key);
    if(it == prop._GetInternalChildrenRefs()->end())
    {
        Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
        if(parentMember)
        {
            Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
            for(U32 i = 0; i < array.GetSize(); i++)
            {
                prop = array.GetValue(i);
                HandlePropertySet hProp{};
                Meta::ExtractCoercableInstance(hProp, prop);
                prop = hProp.GetObject(pRegistry, true);
                prop = Get(prop, key, true, pRegistry);
                if(prop)
                    return prop;
            }
        }
        return {};
    }
    return it->second;
}

void PropertySet::Set(Meta::ClassInstance prop, Symbol key, Meta::ClassInstance value, Ptr<ResourceRegistry> pRegistry, SetPropertyMode mode)
{
    if(mode == SetPropertyMode::DIRECT)
    {
        auto keyInfo = prop._GetInternalChildrenRefs()->find(key);
        if(keyInfo == prop._GetInternalChildrenRefs()->end())
        {
            keyInfo = prop._GetInternalChildrenRefs()->emplace(key, Meta::ClassInstance{}).first;
        }
        keyInfo->second = std::move(value);
    }
    else
    {
        if(mode == SetPropertyMode::COPY)
        {
            Meta::CopyInstance(prop, value, key);
        }
        else
        {
            Meta::MoveInstance(prop, value, key); // assigned in maker.
        }
    }
    MarkModified(prop, key, pRegistry);
}

void PropertySet::MarkModified(Meta::ClassInstance prop, Symbol Key, Ptr<ResourceRegistry> pRegistry)
{
    auto& callbacks = ((InternalData*)prop._GetInternalPropertySetData())->KeyCallbacks;
    FunctionDummy D{};
    Ptr<FunctionBase> dummy = TTE_PROXY_PTR(&D, FunctionDummy);
    dummy->Tag = Key;
    auto it = callbacks.find(dummy);
    if(it != callbacks.end())
    {
        Meta::ClassInstance value = Get(prop, Key, true, pRegistry);
        FunctionBase* callback = it->get();
        while(callback)
        {
            callback->CallMeta(value, {}, {}, {});
            callback = callback->Next.get();
        }
    }
}

U32& PropertySet::GetFlags(Meta::ClassInstance prop)
{
    return Meta::GetMember<U32>(prop, "mFlags");
}

Bool PropertySet::ExistsKey(Meta::ClassInstance prop, Symbol keyName, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    Symbol kn = keyName;
    for(auto& local: *prop._GetInternalChildrenRefs())
    {
        if(kn == local.first)
            return true;
    }
    if(bSearchParents)
    {
        std::set<HandlePropertySet> parents{};
        GetParents(prop, parents, true, pRegistry);
        for(auto prnt: parents)
        {
            Meta::ClassInstance pProp = prnt.GetObject(pRegistry, true);
            if(pProp && pProp != prop && ExistsKey(pProp, keyName, false, pRegistry))
                return true;
        }
    }
    return false;
}

Bool PropertySet::IsMyParent(Meta::ClassInstance prop, Symbol parent, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    std::set<HandlePropertySet> parents{};
    GetParents(prop, parents, bSearchParents, pRegistry);
    for(auto prnt: parents)
        if(prnt.GetObject() == parent)
            return true;
    return false;
}

void PropertySet::ClearKeys(Meta::ClassInstance prop)
{
    prop._GetInternalChildrenRefs()->clear();
}

void PropertySet::ClearParents(Meta::ClassInstance prop)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if(parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        array.Clear();
    }
}

void PropertySet::AddParent(Meta::ClassInstance prop, Symbol parent, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if(parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for(U32 i = 0; i < array.GetSize(); i++)
        {
            prop = array.GetValue(i);
            HandlePropertySet hProp{};
            Meta::ExtractCoercableInstance(hProp, prop);
            Symbol name = hProp.GetObject();
            if(name == parent)
                return;
        }
        HandlePropertySet hParent{};
        hParent.SetObject(parent);
        Meta::ClassInstance handle = Meta::CreateInstance(Meta::FindClass(PropertySet::ClassHandle, 0));
        Meta::ImportCoercableInstance(hParent, handle);
        array.PushValue(handle, false);
        Meta::ClassInstance resolvedParent = hParent.GetObject(pRegistry, true);
        HandlePropertySet hMyself{};
        hMyself.SetObject(pRegistry->LocateResource(prop._GetInternal()));
        if(resolvedParent && hMyself.GetObject())
        {
            AddChild(resolvedParent, hMyself);
        }
    }
}

void PropertySet::AddChild(Meta::ClassInstance prop, HandlePropertySet child)
{
    auto& children = ((InternalData*)prop._GetInternalPropertySetData())->Children;
    for(auto& ch: children)
        if(ch.GetObject() == child.GetObject())
            return;
    children.push_back(std::move(child));
}

void PropertySet::CallAllCallbacks(Meta::ClassInstance prop, Ptr<ResourceRegistry> pRegistry)
{
    auto& callbacks = ((InternalData*)prop._GetInternalPropertySetData())->KeyCallbacks;
    for(const auto& cb: callbacks)
    {
        Meta::ClassInstance value = Get(prop, cb->Tag, true, pRegistry);
        if(value) // Important! The callback can exist before or even without the key existing.
        {
            FunctionBase* pFunction = cb.get();
            while (pFunction)
            {
                pFunction->CallMeta(value, {}, {}, {});
                pFunction = pFunction->Next.get();
            }
        }
    }
}

Bool PropertySet::ContainsAllKeys(Meta::ClassInstance prop, Meta::ClassInstance rhs)
{
    for(auto& ch: *rhs._GetInternalChildrenRefs())
    {
        if(!ExistsKey(prop, ch.first, false, nullptr))
            return false;
    }
    return true;
}

Bool PropertySet::ExistsParentKey(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if(parentMember)
    {
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for(U32 i = 0; i < array.GetSize(); i++)
        {
            prop = array.GetValue(i);
            HandlePropertySet hProp{};
            Meta::ExtractCoercableInstance(hProp, prop);
            if(ExistsKey(hProp.GetObject(pRegistry, true), KeyName, true, pRegistry))
                return true;
        }
    }
    return false;
}

void PropertySet::GetKeys(Meta::ClassInstance prop, std::set<Symbol> &keys, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    for(auto& ch: *prop._GetInternalChildrenRefs())
    {
        keys.insert(ch.first);
    }
    if(bSearchParents)
    {
        Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
        if(parentMember)
        {
            Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
            for(U32 i = 0; i < array.GetSize(); i++)
            {
                prop = array.GetValue(i);
                HandlePropertySet hProp{};
                Meta::ExtractCoercableInstance(hProp, prop);
                GetKeys(hProp.GetObject(pRegistry, true), keys, true, pRegistry);
            }
        }
    }
}

U32 PropertySet::GetNumKeys(Meta::ClassInstance prop, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
{
    std::set<Symbol> keys{};
    GetKeys(prop, keys, bSearchParents, pRegistry);
    return (U32)keys.size();
}

Bool PropertySet::IsKeyLocal(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry> pRegistry)
{
    return !ExistsParentKey(prop, KeyName, pRegistry);
}

Bool PropertySet::PromoteKeyToLocal(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance clazz = Get(prop, KeyName, true, pRegistry);
    if(clazz)
    {
        for(auto& ch: *prop._GetInternalChildrenRefs())
        {
            if(ch.first == KeyName)
                return false; // already local
        }
        prop._GetInternalChildrenRefs()->operator[](KeyName) = Meta::CopyInstance(clazz);
        return true;
    }
    return false;
}

void PropertySet::RemoveKey(Meta::ClassInstance prop, Symbol KeyName)
{
    auto it = prop._GetInternalChildrenRefs()->find(KeyName);
    if(it != prop._GetInternalChildrenRefs()->end())
        prop._GetInternalChildrenRefs()->erase(it);
}

void PropertySet::RemoveParent(Meta::ClassInstance prop, Symbol parent, Bool bDiscardLocalKeys, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    if(parentMember)
    {
        HandlePropertySet hProp{};
        Bool bRemoved = false;
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for(U32 i = 0; i < array.GetSize(); i++)
        {
            prop = array.GetValue(i);
            Meta::ExtractCoercableInstance(hProp, prop);
            Symbol name = hProp.GetObject();
            if(name == parent)
            {
                Meta::ClassInstance old{};
                array.PopValue(i, old);
                bRemoved = true;
                break;
            }
        }
        if(bRemoved && bDiscardLocalKeys)
        {
            Meta::ClassInstance parentProp = hProp.GetObject(pRegistry, true);
            std::set<Symbol> keys{};
            GetKeys(parentProp, keys, true, pRegistry);
            for(Symbol k: keys)
                RemoveKey(prop, k);
        }
    }
}

void PropertySet::PostLoad(Meta::ClassInstance prop, Ptr<ResourceRegistry> pRegistry)
{
    Meta::ClassInstance parentMember = Meta::GetMember(prop, "mParentList", true);
    Symbol myName = pRegistry->LocateResource(prop._GetInternal());
    if(parentMember)
    {
        HandlePropertySet hProp{};
        Meta::ClassInstanceCollection& array = Meta::CastToCollection(parentMember);
        for(U32 i = 0; i < array.GetSize(); i++)
        {
            prop = array.GetValue(i);
            Meta::ExtractCoercableInstance(hProp, prop);
            Symbol name = hProp.GetObject();
            if(name != myName)
            {
                Meta::ClassInstance parent = hProp.GetObject(pRegistry, true);
                if(parent)
                {
                    HandlePropertySet hMyself{};
                    hMyself.SetObject(myName);
                    AddChild(parent, hMyself);
                }
            }
        }
    }
}
