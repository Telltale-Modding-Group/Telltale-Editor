#pragma once

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>

#include <Core/Callbacks.hpp>

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

    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    /**
     Gets a property. Use the intrinsic template version to skip having to coerce to a C++ type.
     */
    static Meta::ClassInstance Get(Meta::ClassInstance prop, String key);
    
    /**
     Sets a property in the property set prop. Use the intrinsic template version to skip having to create the instance.
     */
    static void Set(Meta::ClassInstance prop, String key, Meta::ClassInstance value);

    /**
     Adds a callback which will be called with the property given changes.  The property must exist otherwise this is ignored
     */
    static void AddCallback(Meta::ClassInstance prop, String property, Ptr<FunctionBase> pCallback);
    
    static Bool ExistsKey(Meta::ClassInstance prop, String keyName, Bool bSearchParents);
    
    /**
     The property must be coercable. See types above.
     */
    template<typename T>
    static T Get(Meta::ClassInstance prop, String key)
    {
        Meta::ClassInstance inst = Get(prop, key);
        T value{};
        if(inst)
            Meta::ImportPODInstance(value, inst);
        return value;
    }
    
    /**
     See PropertySet normal version. This sets it as a C++ type. You must pass in the raw class name (without 'class' 'struct' specifiers). If the class name in the game uses those specifiers,
     they will be added and used if the version without cannot be found. T must be coercable. See types above.
     */
    template<typename T>
    static void Set(Meta::ClassInstance prop, String key, T value, String rawClassName)
    {
        auto keyInfo = prop._GetInternalChildrenRefs()->find(Symbol(key));
        if(keyInfo != prop._GetInternalChildrenRefs()->end())
        {
            Meta::ImportPODInstance<T>(value, keyInfo->second);
            keyInfo->second.ExecuteCallbacks();
        }
        else
        {
            U32 clazzID = Meta::FindClass(rawClassName, 0);
            if(clazzID == 0)
                clazzID = Meta::FindClass("class " + rawClassName, 0);
            TTE_ASSERT(clazzID != 0, "At PropertySet<T>(): the class name %s was not found in the current game registry!", rawClassName.c_str());
            Meta::ClassInstance clazz = Meta::CreateInstance(clazzID);
            Meta::ImportPODInstance<T>(value, clazz);
            prop._GetInternalChildrenRefs()->operator[](key) = std::move(clazz);
        }
    }
    
};
