#pragma once

#include <map>
#include <type_traits>

class ResourceRegistry;

template<typename T> inline Ptr<Handleable> AllocateCommon(Ptr<ResourceRegistry> registry);

namespace Meta // Implementation detail for coersion of c++ => meta, meta => c++, c++ => lua, lua => c++, meta => lua, lua => meta
{
    
    // ============= ============= ============= ============= C++ <-> META <-> LUA =============  =============  =============  =============
    
    namespace _Impl
    {
        
        using LuaToMetaFn =  void (LuaManager& man, I32 index, ClassInstance& inst);
        using MetaToLuaFn = void(LuaManager& man, ClassInstance& inst);
        
        struct _CoersionOps
        {
            
            // FOR POD TYPE CONVERSION
            void (*ExtractMeta)(void* out, ClassInstance& inst) = nullptr;
            void (*ImportMeta)(const void* in, ClassInstance& inst) = nullptr;
            void (*ImportLua)(const void* in, LuaManager& man) = nullptr;
            void (*ExtractLua)(void*, LuaManager& man, I32 stackIndex);
            MetaToLuaFn* MetaToLua = nullptr;
            LuaToMetaFn* LuaToMeta = nullptr;
            
            // FOR COMMON CLASSES
            CommonClassAllocator* Allocator = nullptr;
            
            CString ClassName = nullptr;
        };
        
        template<typename T, Bool Common = false>
        struct _CoersionAdapter
        {
            
            static inline void _ConvertLua(LuaManager& man, ClassInstance& inst)
            {
                T temporary{};
                _Coersion<T>::Extract(temporary, inst);
                _Coersion<T>::ImportLua(temporary, man);
            }
            
            static inline void _ConvertMeta(LuaManager& man, I32 index, ClassInstance& inst)
            {
                T temporary{};
                _Coersion<T>::ExtractLua(temporary, man, index);
                _Coersion<T>::Import(temporary, inst);
            }
            
            static inline void _ExtractMeta(void* out, ClassInstance& inst)
            {
                _Coersion<T>::Extract(*((T*)out), inst);
            }
            
            static inline void _ImportMeta(const void* in, ClassInstance& inst)
            {
                _Coersion<T>::Import(*((const T*)in), inst);
            }
            
            static inline  void _ExtractLua(void* out, LuaManager& man, I32 index)
            {
                _Coersion<T>::ExtractLua(*((T*)out), man, index);
            }
            
            static inline void _ImportLua(const void* in, LuaManager& man)
            {
                _Coersion<T>::ImportLua(*((const T*)in), man);
            }
            
        };
        
        template<typename T>
        struct _CoersionAdapter<T, true>
        {
            
            static inline void _ConvertLua(LuaManager& man, ClassInstance& inst)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
            static inline void _ConvertMeta(LuaManager& man, I32 index, ClassInstance& inst)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
            static inline void _ExtractMeta(void* out, ClassInstance& inst)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
            static inline void _ImportMeta(const void* in, ClassInstance& inst)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
            static inline void _ExtractLua(void* out, LuaManager& man, I32 index)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
            static inline void _ImportLua(const void* in, LuaManager& man)
            {
                TTE_ASSERT(false, "Common classes cannot be coerced this way. Please use normalisation API.");
            }
            
        };
        
#ifdef _META_COERSION_IMPL
        
        std::vector<_CoersionOps>& _CoersionRegistry()
        {
            static std::vector<_CoersionOps> registry{};
            return registry;
        }
        
#else
        
        std::vector<_CoersionOps>& _CoersionRegistry();
        
#endif
        
        template<typename T, Bool Common = false>
        struct _CoersionClassName
        {
            static constexpr CString ClassName = "";
        };
        
        template<typename T>
        struct _CoersionClassName<T, true>
        {
            static constexpr CString ClassName = T::Class;
        };
        
        template<typename T, Bool _Ext>
        struct _CoersionCommonSelector
        {
            
            static constexpr Bool IsCommon = false;
            
            static inline CommonClassAllocator* _SelectAl()
            {
                return nullptr;
            }
            
        };
        
        template<typename T>
        struct _CoersionCommonSelector<T, true>
        {
            
            static constexpr Bool IsCommon = true;
            
            static inline CommonClassAllocator* _SelectAl()
            {
                return &AllocateCommon<T>;
            }
            
        };
        
        template<typename T>
        using _CoersionSelectAllocator = _CoersionCommonSelector<T, std::is_base_of_v<Handleable, T>>;
        
        template<typename T>
        using _CoersionSelectAdapter = _CoersionAdapter<T, std::is_base_of_v<Handleable, T>>;
        
        template<typename T>
        struct _CoersionRegistrar
        {
            
            static inline Bool _Registered = false;
            
            struct _Registrar
            {
                inline _Registrar()
                {
                    _CoersionOps ops{};
                    ops.ExtractMeta = &_CoersionSelectAdapter<T>::_ExtractMeta;
                    ops.ImportMeta = &_CoersionSelectAdapter<T>::_ImportMeta;
                    ops.ExtractLua = &_CoersionSelectAdapter<T>::_ExtractLua;
                    ops.ImportLua = &_CoersionSelectAdapter<T>::_ImportLua;
                    ops.MetaToLua = &_CoersionSelectAdapter<T>::_ConvertLua;
                    ops.LuaToMeta = &_CoersionSelectAdapter<T>::_ConvertMeta;
                    ops.ClassName = _CoersionClassName<T, std::is_base_of_v<Handleable, T>>::ClassName;
                    ops.Allocator = _CoersionSelectAllocator<T>::_SelectAl();
                    _CoersionRegistry().push_back(std::move(ops));
                    _Registered = true;
                }
                
            };
            
            static inline _Registrar _MyRegistrar{};
            
            inline static void _ForceInstantiate()
            {
                if(!_Registered)
                    (void)_MyRegistrar;
            }
            
        };
        
        template<Bool _Valid = true> struct _CoerceBase
        {
            static constexpr Bool IsValid = _Valid;
        };
        
        template<typename T>
        struct _CoersionLuaIntrin
        { // these dont have to be defined but should be
            
            // DEFAULTS IF this type cant be used with lua (eg bounding box). we will include them anyway for modders.
            
            static inline void ImportLua(const T& in, LuaManager& man)
            {
                TTE_LOG("WARNING: No lua coersion implementation defined for T");
                man.PushNil();
            }
            
            static inline void ExtractLua(T& out, LuaManager& man, I32 stackIndex)
            {
                TTE_LOG("WARNING: No lua coersion implementation defined for T");
            }
            
        };
        
        template<typename T>
        struct _Coersion : _CoerceBase<false>
        { // these MUST be defined
            
            static constexpr CString ClassName = "";
            static constexpr Bool IsValid = false;
            
            static inline void Extract(T& dest, ClassInstance& src)
            {
                TTE_ASSERT(false, "No implementation defined for T");
            }
            
            static inline void Import(const T& src, ClassInstance& dest)
            {
                TTE_ASSERT(false, "No implementation defined for T");
            }
            
        };
        
        // INLINED EXTRACTORS TO HELP COERCE TYPES FROM META SYSTEM INTO C++ WHICH DON'T CHANGE OVER GAMES
        
        // LUA COERSION
        
#define _COERCABLE_LUA_HELPER(_T, PushFn, FromFn, CastBase) template<> struct _CoersionLuaIntrin<_T> { \
static inline void ImportLua(const _T& in, LuaManager& man) { man.PushFn((CastBase)in); } \
static inline void ExtractLua(_T& out, LuaManager& man, I32 stackIndex) { out = (CastBase)man.FromFn(stackIndex); } }
        
        _COERCABLE_LUA_HELPER(Bool, PushBool, ToBool, Bool);
        _COERCABLE_LUA_HELPER(String, PushLString, ToString, String);
        _COERCABLE_LUA_HELPER(I32, PushInteger, ToInteger, I32);
        _COERCABLE_LUA_HELPER(U32, PushInteger, ToInteger, U32);
        _COERCABLE_LUA_HELPER(U16, PushInteger, ToInteger, U32);
        _COERCABLE_LUA_HELPER(U8, PushInteger, ToInteger, U32);
        _COERCABLE_LUA_HELPER(I16, PushInteger, ToInteger, I16);
        _COERCABLE_LUA_HELPER(I8, PushInteger, ToInteger, I8);
        _COERCABLE_LUA_HELPER(Float, PushFloat, ToFloat, Float);
        
        template<>
        struct _CoersionLuaIntrin<Symbol>
        {
            
            static inline void ImportLua(const Symbol& in, LuaManager& man)
            {
                ScriptManager::PushSymbol(man, in);
            }
            
            static inline void ExtractLua(Symbol& out, LuaManager& man, I32 stackIndex)
            {
                out = ScriptManager::ToSymbol(man, stackIndex);
            }
            
        };
        
        // META COERSION
        
#define _COERCABLE_HELPER(_T, Cls) template<> struct _CoersionClassName<_T> { static constexpr CString ClassName = Cls; }; \
template struct _CoersionRegistrar<_T>; \
template<> struct _Coersion<_T> : _CoerceBase<>, _CoersionLuaIntrin<_T>, _CoersionRegistrar<_T> { \
static inline void Extract(_T& out, ClassInstance& inst)        { out = COERCE(inst._GetInternal(), _T); } \
static inline void Import(const _T& src, ClassInstance& inst)   { COERCE(inst._GetInternal(), _T) = src; } };
        
        _COERCABLE_HELPER(Bool,"bool")
        _COERCABLE_HELPER(String,"String")
        _COERCABLE_HELPER(I32, "int32;int")
        _COERCABLE_HELPER(U32, "uint32;uint;long;unsigned int")
        _COERCABLE_HELPER(U8,"uint8;unsigned char")
        _COERCABLE_HELPER(I8,"int8;char;signed char")
        _COERCABLE_HELPER(U16,"uint16;unsigned short;wchar_t")
        _COERCABLE_HELPER(I16,"int16;short")
        _COERCABLE_HELPER(I64,"int64;__int64;long long")
        _COERCABLE_HELPER(U64,"uint64;unsigned __int64;unsigned long long")
        _COERCABLE_HELPER(Symbol,"Symbol")
        _COERCABLE_HELPER(Float, "float")
        //_COERCABLE_HELPER(double) Disable doubles. Not used in the engine, prefer floats.
        
#undef _COERCABLE_HELPER
        
        template<> struct _Coersion<Placeholder> : _CoerceBase<>
        {
            static constexpr CString ClassName = "";
            static inline void Extract(Placeholder& out, ClassInstance& inst)
            {
            }
            
            static inline void Import(const Placeholder& in, ClassInstance& inst)
            {
            }
            
            static inline void ImportLua(const Placeholder& in, LuaManager& man)
            {
            }
            
            static inline void ExtractLua(Placeholder& out, LuaManager& man, I32 stackIndex)
            {
            }
            
        };
        
#define DECL_COERSION(T, ClsName) \
template<> struct _CoersionClassName<T> { static constexpr CString ClassName = ClsName; }; \
template struct _CoersionRegistrar<T>; \
template<> struct _Coersion<T> : _CoerceBase<>, _CoersionRegistrar<T>, _CoersionLuaIntrin<T>
        
#define IMPL_COERSION_FN(T, Fn, Ret) Ret Meta::_Impl::_Coersion<T>::Fn
        
        // Does not register to registrar as we dont know the T.
#define DECL_COERSION_TP(TemplateType, ClsName) \
template<typename T> struct _CoersionClassName<TemplateType<T>> { static constexpr CString ClassName = ClsName; }; \
template<typename T> struct _Coersion<TemplateType<T>> : _CoerceBase<>, _CoersionLuaIntrin<TemplateType<T>>
        
#define COERSION_IMPORT_MEMBER(MetaMember, PushFn, CMember) man.PushLString(MetaMember); man.PushFn(in.CMember); man.SetTable(-3)
#define COERSION_EXPORT_MEMBER(MetaMember, PopFn, CMemberBinding) man.PushLString(MetaMember); man.GetTable(stackIndex); out.CMemberBinding = PopFn(man)
        
        DECL_COERSION(Vector2, "Vector2")
        {
            //static constexpr CString ClassName = "class Vector2";
            static inline void Extract(Vector2& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ExtractCoercableInstance(out.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ExtractCoercableInstance(out.y, temp);
            }
            static inline void Import(const Vector2& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ImportCoercableInstance(in.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ImportCoercableInstance(in.y, temp);
            }
            static inline void ImportLua(const Vector2& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("x", PushFloat, x);
                COERSION_IMPORT_MEMBER("y", PushFloat, y);
            }
            static inline void ExtractLua(Vector2& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("x", ScriptManager::PopFloat, x);
                COERSION_EXPORT_MEMBER("y", ScriptManager::PopFloat, y);
            }
        };
        
        DECL_COERSION(Vector3, "Vector3")
        {
            static inline void Extract(Vector3& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ExtractCoercableInstance(out.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ExtractCoercableInstance(out.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ExtractCoercableInstance(out.z, temp);
            }
            static inline void Import(const Vector3& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ImportCoercableInstance(in.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ImportCoercableInstance(in.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ImportCoercableInstance(in.z, temp);
            }
            static inline void ImportLua(const Vector3& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("x", PushFloat, x);
                COERSION_IMPORT_MEMBER("y", PushFloat, y);
                COERSION_IMPORT_MEMBER("z", PushFloat, z);
            }
            static inline void ExtractLua(Vector3& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("x", ScriptManager::PopFloat, x);
                COERSION_EXPORT_MEMBER("y", ScriptManager::PopFloat, y);
                COERSION_EXPORT_MEMBER("z", ScriptManager::PopFloat, z);
            }
        };
        
        DECL_COERSION(Vector4, "Vector4")
        {
            static inline void Extract(Vector4& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ExtractCoercableInstance(out.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ExtractCoercableInstance(out.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ExtractCoercableInstance(out.z, temp);
                temp = Meta::GetMember(inst, "w", true);
                ExtractCoercableInstance(out.w, temp);
            }
            static inline void Import(const Vector4& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ImportCoercableInstance(in.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ImportCoercableInstance(in.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ImportCoercableInstance(in.z, temp);
                temp = Meta::GetMember(inst, "w", true);
                ImportCoercableInstance(in.w, temp);
            }
            static inline void ImportLua(const Vector4& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("x", PushFloat, x);
                COERSION_IMPORT_MEMBER("y", PushFloat, y);
                COERSION_IMPORT_MEMBER("z", PushFloat, z);
                COERSION_IMPORT_MEMBER("w", PushFloat, w);
            }
            static inline void ExtractLua(Vector4& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("x", ScriptManager::PopFloat, x);
                COERSION_EXPORT_MEMBER("y", ScriptManager::PopFloat, y);
                COERSION_EXPORT_MEMBER("z", ScriptManager::PopFloat, z);
                COERSION_EXPORT_MEMBER("w", ScriptManager::PopFloat, w);
            }
        };
        
        DECL_COERSION(Quaternion, "Quaternion")
        {
            static inline void Extract(Quaternion& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ExtractCoercableInstance(out.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ExtractCoercableInstance(out.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ExtractCoercableInstance(out.z, temp);
                temp = Meta::GetMember(inst, "w", true);
                ExtractCoercableInstance(out.w, temp);
            }
            static inline void Import(const Quaternion& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "x", true);
                ImportCoercableInstance(in.x, temp);
                temp = Meta::GetMember(inst, "y", true);
                ImportCoercableInstance(in.y, temp);
                temp = Meta::GetMember(inst, "z", true);
                ImportCoercableInstance(in.z, temp);
                temp = Meta::GetMember(inst, "w", true);
                ImportCoercableInstance(in.w, temp);
            }
            static inline void ImportLua(const Quaternion& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("x", PushFloat, x);
                COERSION_IMPORT_MEMBER("y", PushFloat, y);
                COERSION_IMPORT_MEMBER("z", PushFloat, z);
                COERSION_IMPORT_MEMBER("w", PushFloat, w);
            }
            static inline void ExtractLua(Quaternion& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("x", ScriptManager::PopFloat, x);
                COERSION_EXPORT_MEMBER("y", ScriptManager::PopFloat, y);
                COERSION_EXPORT_MEMBER("z", ScriptManager::PopFloat, z);
                COERSION_EXPORT_MEMBER("w", ScriptManager::PopFloat, w);
            }
        };
        
        DECL_COERSION(Colour, "Color")
        {
            static inline void Extract(Colour& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "r", true);
                ExtractCoercableInstance(out.r, temp);
                temp = Meta::GetMember(inst, "g", true);
                ExtractCoercableInstance(out.g, temp);
                temp = Meta::GetMember(inst, "b", true);
                ExtractCoercableInstance(out.b, temp);
                temp = Meta::GetMember(inst, "a", true);
                ExtractCoercableInstance(out.a, temp);
            }
            static inline void Import(const Colour& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "r", true);
                ImportCoercableInstance(in.r, temp);
                temp = Meta::GetMember(inst, "g", true);
                ImportCoercableInstance(in.g, temp);
                temp = Meta::GetMember(inst, "b", true);
                ImportCoercableInstance(in.b, temp);
                temp = Meta::GetMember(inst, "a", true);
                ImportCoercableInstance(in.a, temp);
            }
            static inline void ImportLua(const Colour& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("r", PushFloat, r);
                COERSION_IMPORT_MEMBER("g", PushFloat, g);
                COERSION_IMPORT_MEMBER("b", PushFloat, b);
                COERSION_IMPORT_MEMBER("a", PushFloat, a);
            }
            static inline void ExtractLua(Colour& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("r", ScriptManager::PopFloat, r);
                COERSION_EXPORT_MEMBER("g", ScriptManager::PopFloat, g);
                COERSION_EXPORT_MEMBER("b", ScriptManager::PopFloat, b);
                COERSION_EXPORT_MEMBER("a", ScriptManager::PopFloat, a);
            }
        };
        
        DECL_COERSION(BoundingBox, "BoundingBox")
        {
            static inline void Extract(BoundingBox& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mMin", true);
                ExtractCoercableInstance(out._Min, temp);
                temp = Meta::GetMember(inst, "mMax", true);
                ExtractCoercableInstance(out._Max, temp);
            }
            static inline void Import(const BoundingBox& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mMin", true);
                ImportCoercableInstance(in._Min, temp);
                temp = Meta::GetMember(inst, "mMax", true);
                ImportCoercableInstance(in._Max, temp);
            }
            static inline void ImportLua(const BoundingBox& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("MinX", PushFloat, _Min.x);
                COERSION_IMPORT_MEMBER("MinY", PushFloat, _Min.y);
                COERSION_IMPORT_MEMBER("MinZ", PushFloat, _Min.z);
                COERSION_IMPORT_MEMBER("MaxX", PushFloat, _Max.x);
                COERSION_IMPORT_MEMBER("MaxY", PushFloat, _Max.y);
                COERSION_IMPORT_MEMBER("MaxZ", PushFloat, _Max.z);
            }
            static inline void ExtractLua(BoundingBox& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("MinX", ScriptManager::PopFloat, _Min.x);
                COERSION_EXPORT_MEMBER("MinY", ScriptManager::PopFloat, _Min.y);
                COERSION_EXPORT_MEMBER("MinZ", ScriptManager::PopFloat, _Min.z);
                COERSION_EXPORT_MEMBER("MaxX", ScriptManager::PopFloat, _Max.x);
                COERSION_EXPORT_MEMBER("MaxY", ScriptManager::PopFloat, _Max.y);
                COERSION_EXPORT_MEMBER("MaxZ", ScriptManager::PopFloat, _Max.z);
            }
        };
        
        DECL_COERSION(Sphere, "Sphere")
        {
            static inline void Extract(Sphere& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mRadius", true);
                ExtractCoercableInstance(out._Radius, temp);
                temp = Meta::GetMember(inst, "mCenter", true);
                ExtractCoercableInstance(out._Center, temp);
            }
            static inline void Import(const Sphere& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mRadius", true);
                ImportCoercableInstance(in._Radius, temp);
                temp = Meta::GetMember(inst, "mCenter", true);
                ImportCoercableInstance(in._Center, temp);
            }
            static inline void ImportLua(const Sphere& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("CenterX", PushFloat, _Center.x);
                COERSION_IMPORT_MEMBER("CenterY", PushFloat, _Center.y);
                COERSION_IMPORT_MEMBER("CenterZ", PushFloat, _Center.z);
                COERSION_IMPORT_MEMBER("Radius", PushFloat, _Radius);
            }
            static inline void ExtractLua(Sphere& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("CenterX", ScriptManager::PopFloat, _Center.x);
                COERSION_EXPORT_MEMBER("CenterY", ScriptManager::PopFloat, _Center.y);
                COERSION_EXPORT_MEMBER("CenterZ", ScriptManager::PopFloat, _Center.z);
                COERSION_EXPORT_MEMBER("MaxX", ScriptManager::PopFloat, _Radius);
            }
        };
        
        DECL_COERSION(Transform, "Transform")
        {
            static inline void Extract(Transform& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mRot", true);
                ExtractCoercableInstance(out._Rot, temp);
                temp = Meta::GetMember(inst, "mTrans", true);
                ExtractCoercableInstance(out._Trans, temp);
            }
            static inline void Import(const Transform& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mRot", true);
                ImportCoercableInstance(in._Rot, temp);
                temp = Meta::GetMember(inst, "mTrans", true);
                ImportCoercableInstance(in._Trans, temp);
            }
            static inline void ImportLua(const Transform& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("TransX", PushFloat, _Trans.x);
                COERSION_IMPORT_MEMBER("TransY", PushFloat, _Trans.y);
                COERSION_IMPORT_MEMBER("TransZ", PushFloat, _Trans.z);
                COERSION_IMPORT_MEMBER("RotX", PushFloat, _Rot.x);
                COERSION_IMPORT_MEMBER("RotY", PushFloat, _Rot.y);
                COERSION_IMPORT_MEMBER("RotZ", PushFloat, _Rot.z);
                COERSION_IMPORT_MEMBER("RotW", PushFloat, _Rot.w);
            }
            static inline void ExtractLua(Transform& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("TransX", ScriptManager::PopFloat, _Trans.x);
                COERSION_EXPORT_MEMBER("TransY", ScriptManager::PopFloat, _Trans.y);
                COERSION_EXPORT_MEMBER("TransZ", ScriptManager::PopFloat, _Trans.z);
                COERSION_EXPORT_MEMBER("RotX", ScriptManager::PopFloat, _Rot.x);
                COERSION_EXPORT_MEMBER("RotY", ScriptManager::PopFloat, _Rot.y);
                COERSION_EXPORT_MEMBER("RotZ", ScriptManager::PopFloat, _Rot.z);
                COERSION_EXPORT_MEMBER("RotW", ScriptManager::PopFloat, _Rot.w);
            }
        };
        
        DECL_COERSION(Polar, "Polar")
        {
            static inline void Extract(Polar& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mR", true);
                ExtractCoercableInstance(out.R, temp);
                temp = Meta::GetMember(inst, "mTheta", true);
                ExtractCoercableInstance(out.Theta, temp);
                temp = Meta::GetMember(inst, "mPhi", true);
                ExtractCoercableInstance(out.Phi, temp);
            }
            static inline void Import(const Polar& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "mR", true);
                ImportCoercableInstance(in.R, temp);
                temp = Meta::GetMember(inst, "mTheta", true);
                ImportCoercableInstance(in.Theta, temp);
                temp = Meta::GetMember(inst, "mPhi", true);
                ImportCoercableInstance(in.Phi, temp);
            }
            static inline void ImportLua(const Polar& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("R",         PushFloat, R);
                COERSION_IMPORT_MEMBER("Theta",     PushFloat, Theta);
                COERSION_IMPORT_MEMBER("Phi",       PushFloat, Phi);
            }
            static inline void ExtractLua(Polar& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("R",         ScriptManager::PopFloat, R);
                COERSION_EXPORT_MEMBER("Theta",     ScriptManager::PopFloat, Theta);
                COERSION_EXPORT_MEMBER("Phi",       ScriptManager::PopFloat, Phi);
            }
        };
        
        DECL_COERSION(TRect<float>, "TRect<float>")
        {
            static inline void Extract(TRect<Float>& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "right", true);
                ExtractCoercableInstance(out.right, temp);
                temp = Meta::GetMember(inst, "top", true);
                ExtractCoercableInstance(out.top, temp);
                temp = Meta::GetMember(inst, "bottom", true);
                ExtractCoercableInstance(out.bottom, temp);
                temp = Meta::GetMember(inst, "left", true);
                ExtractCoercableInstance(out.left, temp);
            }
            static inline void Import(const TRect<Float>& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "top", true);
                ImportCoercableInstance(in.top, temp);
                temp = Meta::GetMember(inst, "bottom", true);
                ImportCoercableInstance(in.bottom, temp);
                temp = Meta::GetMember(inst, "left", true);
                ImportCoercableInstance(in.left, temp);
                temp = Meta::GetMember(inst, "right", true);
                ImportCoercableInstance(in.right, temp);
            }
            static inline void ImportLua(const TRect<Float>& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("Top", PushFloat, top);
                COERSION_IMPORT_MEMBER("Bottom", PushFloat, bottom);
                COERSION_IMPORT_MEMBER("Left", PushFloat, left);
                COERSION_IMPORT_MEMBER("Right", PushFloat, right);
            }
            static inline void ExtractLua(TRect<Float>& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("Top", ScriptManager::PopFloat, top);
                COERSION_EXPORT_MEMBER("Bottom",ScriptManager::PopFloat, bottom);
                COERSION_EXPORT_MEMBER("Left",  ScriptManager::PopFloat, left);
                COERSION_EXPORT_MEMBER("Right",  ScriptManager::PopFloat, right);
            }
        };
        
        DECL_COERSION(Rect, "Rect")
        {
            static inline void Extract(Rect& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "right", true);
                ExtractCoercableInstance(out.right, temp);
                temp = Meta::GetMember(inst, "top", true);
                ExtractCoercableInstance(out.top, temp);
                temp = Meta::GetMember(inst, "bottom", true);
                ExtractCoercableInstance(out.bottom, temp);
                temp = Meta::GetMember(inst, "left", true);
                ExtractCoercableInstance(out.left, temp);
            }
            static inline void Import(const Rect& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "top", true);
                ImportCoercableInstance(in.top, temp);
                temp = Meta::GetMember(inst, "bottom", true);
                ImportCoercableInstance(in.bottom, temp);
                temp = Meta::GetMember(inst, "left", true);
                ImportCoercableInstance(in.left, temp);
                temp = Meta::GetMember(inst, "right", true);
                ImportCoercableInstance(in.right, temp);
            }
            static inline void ImportLua(const Rect& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("Top", PushInteger, top);
                COERSION_IMPORT_MEMBER("Bottom", PushInteger, bottom);
                COERSION_IMPORT_MEMBER("Left", PushInteger, left);
                COERSION_IMPORT_MEMBER("Right", PushInteger, right);
            }
            static inline void ExtractLua(Rect& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("Top", ScriptManager::PopInteger, top);
                COERSION_EXPORT_MEMBER("Bottom",ScriptManager::PopInteger, bottom);
                COERSION_EXPORT_MEMBER("Left",  ScriptManager::PopInteger, left);
                COERSION_EXPORT_MEMBER("Right",  ScriptManager::PopInteger, right);
            }
        };
        
        DECL_COERSION(TRange<float>, "TRange<float>")
        {
            static inline void Extract(TRange<Float>& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "min", true);
                ExtractCoercableInstance(out.min, temp);
                temp = Meta::GetMember(inst, "max", true);
                ExtractCoercableInstance(out.max, temp);
            }
            static inline void Import(const TRange<Float>& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "min", true);
                ImportCoercableInstance(in.min, temp);
                temp = Meta::GetMember(inst, "max", true);
                ImportCoercableInstance(in.max, temp);
            }
            static inline void ImportLua(const TRange<Float>& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("Min", PushFloat, min);
                COERSION_IMPORT_MEMBER("Max", PushFloat, max);
            }
            static inline void ExtractLua(TRange<Float>& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("Min", ScriptManager::PopFloat, min);
                COERSION_EXPORT_MEMBER("Max",ScriptManager::PopFloat, max);
            }
        };
        
        DECL_COERSION(TRange<U32>, "TRange<unsigned int>")
        {
            static inline void Extract(TRange<U32>& out, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "min", true);
                ExtractCoercableInstance(out.min, temp);
                temp = Meta::GetMember(inst, "max", true);
                ExtractCoercableInstance(out.max, temp);
            }
            static inline void Import(const TRange<U32>& in, ClassInstance& inst)
            {
                ClassInstance temp = Meta::GetMember(inst, "min", true);
                ImportCoercableInstance(in.min, temp);
                temp = Meta::GetMember(inst, "max", true);
                ImportCoercableInstance(in.max, temp);
            }
            static inline void ImportLua(const TRange<U32>& in, LuaManager& man)
            {
                man.PushTable();
                COERSION_IMPORT_MEMBER("Min", PushUnsignedInteger, min);
                COERSION_IMPORT_MEMBER("Max", PushUnsignedInteger, max);
            }
            static inline void ExtractLua(TRange<U32>& out, LuaManager& man, I32 stackIndex)
            {
                stackIndex = man.ToAbsolute(stackIndex);
                COERSION_EXPORT_MEMBER("Min", ScriptManager::PopUnsignedInteger, min);
                COERSION_EXPORT_MEMBER("Max",ScriptManager::PopUnsignedInteger, max);
            }
        };
        
        DECL_COERSION(AnimOrChore, "AnimOrChore")
        {
            
            // All defined in Meta.cpp (rely on handle, not defined yet)
            
            static void Extract(AnimOrChore& out, ClassInstance& inst);
            static void Import(const AnimOrChore& in, ClassInstance& inst);
            static void ImportLua(const AnimOrChore& in, LuaManager& man);
            static void ExtractLua(AnimOrChore& out, LuaManager& man, I32 stackIndex);
            
        };
        
    }
    
}
