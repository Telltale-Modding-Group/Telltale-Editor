#pragma once

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>
#include <Core/Callbacks.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <set>
#include <vector>

// Key callback override to pass in the key.
struct LuaPropertyKeyCallback : LuaFunctionImpl<2>
{
    
    inline LuaPropertyKeyCallback(LuaManagerRef&& man) : LuaFunctionImpl<2>(std::move(man)) {}
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* pArg4, U32 classArg4) override
    {
        U32 sym = Meta::FindClass("class Symbol", 0);
        LuaFunctionImpl::CallErased(&Tag, sym, pArg1, classArg1, nullptr, 0, nullptr, 0);
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override
    {
        U8 SymbolInstanceBuffer[128]{0};
        Meta::ClassInstance keyArg = Meta::CreateTemporaryInstance(SymbolInstanceBuffer, 128, Meta::FindClass("class Symbol", 0));
        Meta::ImportCoercableInstance(Tag, keyArg);
        LuaFunctionImpl::CallMeta(keyArg, arg1, {}, {});
    }
    
    inline virtual U32 GetNumArguments() const override
    {
        return 1;
    }
    
};

// .PROP FILES. PropertySet's are managed here as Meta::ClassInstance's. Use the namespace below to use its functionality.

/*
 Types which can be coerced (eg C++ structs to Meta classes), which can be used as template parameters in this namespace functionality
 Any intrinsic data type (float, int, short, byte...)
 String
 Handle<T>
 Symbol
 Vector2
 Vector3
 Vector4
 Transform
 Quaternion
 Color
 Polar
 BoundingBox
 Sphere
 
 */
class PropertySet
{
public:
    
    static constexpr CString ClassHandle = "Handle<PropertySet>";
    
    struct KeyCallbackTracked
    {
        Ptr<FunctionBase> MyCallback;
        U32 TrackingRefID = 0; // for removing all callbacks with this ID etc.
    };
    
    struct KeyCallbackComparator
    {
        
        inline Bool operator()(const KeyCallbackTracked& lhs, const KeyCallbackTracked& rhs) const
        {
            return lhs.MyCallback->Tag < rhs.MyCallback->Tag;
        }
        
    };
    
    struct InternalData
    {
        
        std::set<KeyCallbackTracked, KeyCallbackComparator> KeyCallbacks;
        std::vector<HandlePropertySet> Children;
        
        void Move(InternalData& dest);
        void Clone(InternalData& dest) const; // clones a copy into the dest.
        
    };
    
    enum Flag
    {
        DONT_STORE_IN_SAVE_GAMES = 0x200,
        RUNTIME_PROP = 0x10,
    };
    
    enum class SetPropertyMode
    {
        DIRECT, // Directly assign the value parameter. No move, use the memory. Use only if you just constructed and it wont be used elsewhere.
        COPY, // Copy into a new value leaving this value unaffected.
        MOVE, // Move into a new value, leaving this value empty.
    };

    static void InjectScriptAPI(LuaFunctionCollection& Col);
    
    /**
     Gets a property. Use the intrinsic template version to skip having to coerce to a C++ type.
     */
    static Meta::ClassInstance Get(Meta::ClassInstance prop, Symbol key, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Sets a property in the property set prop. Use the intrinsic template version to skip having to create the instance. Last argument is the set mode. See the enum above.
     */
    static void Set(Meta::ClassInstance prop, Symbol key, Meta::ClassInstance value,  Ptr<ResourceRegistry> pRegistry, SetPropertyMode mode = SetPropertyMode::COPY);
    
    /**
     Clears all the callbacks for the given property set. Give the optional tracking reference to clear IDs matching that, or 0 to clear all.
     */
    static void ClearCallbacks(Meta::ClassInstance prop, U32 withTrackingRef = 0);

    /**
     Adds a callback which will be called with the property given changes.  The property does not have to exist yet.
     Optional tracking reference to delete all callbacks with that reference. Tracking reference 0 is untrackable (ie clear callbacks with 0 will clear all, not just ones with the ref = 0)
     */
    static void AddCallback(Meta::ClassInstance prop, Symbol property, Ptr<FunctionBase> pCallback, U32 trackingRef = 0);
    
    /**
     Returns if the given property key exists in the property set. Optionally state wether to search all parent properties too.
     */
    static Bool ExistsKey(Meta::ClassInstance prop, Symbol keyName, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Gets all parents of the given property set.
     */
    static void GetParents(Meta::ClassInstance prop, std::set<HandlePropertySet>& parents, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Tests if the given property set parent file name is a parent of the given prop.
     */
    static Bool IsMyParent(Meta::ClassInstance prop, Symbol parent, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Clears all key-value mappings in the given property set.
     */
    static void ClearKeys(Meta::ClassInstance prop);
    
    /**
     Clear all the parents of the given prop.
     */
    static void ClearParents(Meta::ClassInstance prop);
    
    /**
     Adds a parent to the given property set.
     */
    static void AddParent(Meta::ClassInstance prop, Symbol parent, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Marks the given local property as modified.
     */
    static void MarkModified(Meta::ClassInstance prop, Symbol Key, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Moves the given property file to the front. It will be searched first. If it doesnt not exist, it will be added
     */
    static void MoveParentToFront(Meta::ClassInstance, Symbol parent);
    
    /**
     Calls all key callbacks, such as all values were changed.
     */
    static void CallAllCallbacks(Meta::ClassInstance prop, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Tests if prop has all of the keys of rhs in the prop. Used for logic groups and rules. Does not search parents.
    */
    static Bool ContainsAllKeys(Meta::ClassInstance prop, Meta::ClassInstance rhs);
    
    /**
     Returns true if any of the parents contain a property with KeyName (and their parents recursively).
     */
    static Bool ExistsParentKey(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Gets all key names of the given property set and puts then into the output keys set.
     */
    static void GetKeys(Meta::ClassInstance prop, std::set<Symbol>& keys, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Returns the number of keys in the property set.
     */
    static U32 GetNumKeys(Meta::ClassInstance prop, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry);

    /**
     * Gets the number of parents in the given property set. Does not search any parents
     */
    static U32 GetNumParents(Meta::ClassInstance prop);
    
    /**
     Returns if the given property is local to the property set and not from a parent property set. Change this by making it local or setting this property.
     */
    static Bool IsKeyLocal(Meta::ClassInstance prop, Symbol KeyName, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Promotes the given key such that it will now directly exist in this property set and not through a parent. Its value is pertained.
     Returns true if it was made local.
     */
    static Bool PromoteKeyToLocal(Meta::ClassInstance prop, Symbol KeyName , Ptr<ResourceRegistry> pRegistry);
    
    /**
     Removes a property key. Does not remove any parent keys.
     */
    static void RemoveKey(Meta::ClassInstance prop, Symbol KeyName);
    
    /**
     Removes the parent from the property set. Set discard local keys to true to remove local keys from the property set which were overriding parent keys.
     */
    static void RemoveParent(Meta::ClassInstance prop, Symbol parent, Bool bDiscardLocalKeys, Ptr<ResourceRegistry> pRegistry);
    
    /**
     Removes the given callback from the property set. Key can be an empty symbol meaning matching callback is checked if Equals return true, vise verce can be null and key is checked. If none are valid this call is ignored.
     */
    static void RemoveCallback(Meta::ClassInstance prop, Symbol key, Ptr<FunctionBase> pMatchingCallback);
    
    /**
     Returns a reference to the flags of the property set.
     */
    static U32& GetFlags(Meta::ClassInstance prop);
    
    /**
     Imports keys. Optionally provide a filter set. With the filter: If invert is false, only keys source key-value pairs with *keys* existing in the filter will be imported. If true, the opposite (only keys not in the filter)
     */
    static void ImportKeysValuesAndParents(Meta::ClassInstance prop, Meta::ClassInstance source, Bool bSearchParents, Bool bImportParents, Meta::ClassInstance filter,
                                           Bool bOverwriteExistingKeys, Bool bInvertFilter, Ptr<ResourceRegistry> reg);
    
    /**
     Returns a handle to the property set which the key is introduced from, ie highest parent containing it.
     */
    static HandlePropertySet GetPropertySetKeyIsIntroducedFrom(Meta::ClassInstance prop, Symbol keyName , Ptr<ResourceRegistry> reg);
    
    /**
     Returns a handle to the property set which the key is retreived from, ie the lowest in the heirarchy with an overriding value (myself, them above etc)
     */
    static HandlePropertySet GetPropertySetValueIsRetrievedFrom(Meta::ClassInstance prop, Symbol keyName , Ptr<ResourceRegistry> reg, Bool bCheckMyself);
    
    /**
     The property must be coercable. See types above.
     */
    template<typename T>
    static T Get(Meta::ClassInstance prop, Symbol key, Bool bSearchParents, Ptr<ResourceRegistry> pRegistry)
    {
        Meta::ClassInstance inst = Get(prop, key, bSearchParents, pRegistry);
        T value{};
        if(inst)
            Meta::ImportCoercableInstance(value, inst);
        return value;
    }
    
    /**
     See PropertySet normal version. This sets it as a C++ type. You must pass in the raw class name (without 'class' 'struct' specifiers). If the class name in the game uses those specifiers,
     they will be added and used if the version without cannot be found. T must be coercable. See types above.
     */
    template<typename T>
    static void Set(Meta::ClassInstance prop, Symbol key, T value, String rawClassName, Ptr<ResourceRegistry> pRegistry)
    {
        U32 clazzID = Meta::FindClass(rawClassName, 0);
        if(clazzID == 0)
            clazzID = Meta::FindClass("class " + rawClassName, 0);
        TTE_ASSERT(clazzID != 0, "At PropertySet<T>(): the class name %s was not found in the current game registry!", rawClassName.c_str());
        Meta::ClassInstance clazz = Meta::CreateInstance(clazzID);
        Meta::ImportCoercableInstance<T>(value, clazz);
        Set(prop, key, std::move(clazz), pRegistry, SetPropertyMode::DIRECT);
    }
    
private:
    
    static void PostLoad(Meta::ClassInstance prop, Ptr<ResourceRegistry> pRegistry); // post load. adds children
    
    static void AddChild(Meta::ClassInstance prop, HandlePropertySet hChild); // adds child
    
};
