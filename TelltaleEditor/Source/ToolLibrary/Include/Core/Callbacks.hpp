#pragma once

#include <fastdelegate/FastDelegate.h>
#include <type_traits>

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>

#include <Scripting/ScriptManager.hpp>

// ===================================================================         BASE
// ===================================================================

// Base polymorphic class for functions and methods. Functions have no associated class instance. Methods do.
struct FunctionBase
{
 
    Ptr<FunctionBase> Next = nullptr; // If used as linked list.
    Symbol Tag = Symbol{}; // Misc tag
  
    // Call with C++ argument
    virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* Arg4, U32 classArg4) = 0;
    
    // Call with Meta system argument
    virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) = 0;
    
    // Comparison
    virtual Bool Equals(const FunctionBase& rhs) const = 0;
    
    // If expired (eg weak ptr to method obj)
    virtual Bool Expired() const = 0;
    
    virtual U32 GetNumArguments() const = 0;
    
    virtual ~FunctionBase() = default;
    
};

// ===================================================================         CALLBACK LIST
// ===================================================================

// Array of callbacks.
class Callbacks
{
    
    Ptr<FunctionBase> _Cbs;
    
public:
    
    inline void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* Arg4, U32 classArg4)
    {
        // Call and purge any expired callbacks
        Ptr<FunctionBase> fn = _Cbs, prev = {};
        while (fn)
        {
            if(fn->Expired())
            {
                if(fn == _Cbs)
                {
                    fn = _Cbs = fn->Next;
                }
                else
                {
                    if(prev)
                        prev->Next = fn->Next;
                    fn = fn->Next;
                }
            }
            else
            {
                prev = fn;
                fn->CallErased(pArg1, classArg1, pArg2, classArg2, pArg3, classArg3, Arg4, classArg4);
                fn = fn->Next;
            }
        }
    }
    
    inline void PushCallback(Ptr<FunctionBase> pCallback)
    {
        pCallback->Next = _Cbs;
        _Cbs = std::move(pCallback);
    }

    inline void RemoveCallbacks(Symbol callbackTag) // rem all with tag
    {
        Ptr<FunctionBase> fn = _Cbs, prev = {};
        while (fn)
        {
            if(fn->Tag == callbackTag)
            {
                if(fn == _Cbs)
                {
                    fn = _Cbs = fn->Next;
                }
                else
                {
                    if(prev)
                        prev->Next = fn->Next;
                    fn = fn->Next;
                }
            }
            else
            {
                prev = fn;
                fn = fn->Next;
            }
        }
    }
    
    inline void Clear()
    {
        Ptr<FunctionBase> fn = _Cbs;
        while(fn)
        {
            fn = std::move(fn->Next);
        }
        _Cbs.reset();
    }
    
};

// ===================================================================         DUMMY
// ===================================================================

struct FunctionDummyImpl : FunctionBase
{
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* Arg4, U32 classArg4) override {}
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override {}
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override{ return false; }
    
    inline virtual U32 GetNumArguments() const override { return 0; }
    
    inline virtual Bool Expired() const override { return false; }
    
    inline virtual ~FunctionDummyImpl() {}
    
};

using FunctionDummy = FunctionDummyImpl;

#define CALLBACK_ARGUMENT_ASSERT_NOENUM(A) static_assert(!std::is_enum<A>::value, "Enums cannot be used as callback parameters. Please use Enum<E> for meta coersion support")

// ===================================================================         LUA CALLBACKS (META -> LUA)
// ===================================================================

template<U32 NumArgs>
struct LuaFunctionImpl : FunctionBase
{
    
    static_assert(NumArgs >= 0 && NumArgs <= 4, "Number of arguments must be between 0 and 4");
    
    LuaManagerRef ManagerRef;
    I32 RegistryIndex = -1;
    
    inline LuaFunctionImpl(LuaManagerRef&& mref) : ManagerRef(std::move(mref)) {}
    
    // Set the function by name
    inline void SetFunction(const String& functionName)
    {
        LuaManager& man = ManagerRef.Get();
        ScriptManager::GetGlobal(man, functionName, true);
        if(man.Type(-1) == LuaType::FUNCTION)
        {
            Clear();
            RegistryIndex = man.RefReg();
        }
        else
        {
            man.Pop(1);
        }
    }
    
    inline virtual Bool Expired() const override
    {
        return ManagerRef.Expired();
    }
    
    // Set the function by stack index
    inline void SetFunction(I32 stackIndex)
    {
        Clear();
        LuaManager& man = ManagerRef.Get();
        man.PushCopy(stackIndex);
        RegistryIndex = man.RefReg();
    }
    
    // Resets the function.
    inline void Clear()
    {
        if(RegistryIndex != -1 && !ManagerRef.Expired())
        {
            ManagerRef.Get().UnrefReg(RegistryIndex);
        }
        RegistryIndex = -1;
    }
    
    inline virtual ~LuaFunctionImpl()
    {
        Clear();
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override
    {
        const LuaFunctionImpl* pImpl = dynamic_cast<const LuaFunctionImpl*>(&rhs);
        if(pImpl->ManagerRef.Expired())
            return false;// going to assume the managers are the same. (very very unlikely not)
        if(pImpl->RegistryIndex == -1 || RegistryIndex == -1)
            return false; // not set
        LuaManager& man = pImpl->ManagerRef.Get();
        man.GetReg(RegistryIndex);
        man.GetReg(pImpl->RegistryIndex);
        Bool bEqual = man.Compare(-1, -2, LuaOp::EQ);
        man.SetTop(-3); // pop2
        return bEqual;
    }
    
    inline virtual U32 GetNumArguments() const override
    {
        return NumArgs;
    }
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* pArg4, U32 classArg4) override
    {
        if(ManagerRef.Expired())
        {
            TTE_LOG("WARNING: Calling Lua function callback but manager reference expired.");
            return;
        }
        LuaManager& man = ManagerRef.Get();
        if(NumArgs >= 1) TTE_ASSERT(pArg1, "Argument 1 not provided");
        if(NumArgs >= 2) TTE_ASSERT(pArg2, "Argument 2 not provided");
        if(NumArgs >= 3) TTE_ASSERT(pArg3, "Argument 3 not provided");
        if(NumArgs == 4) TTE_ASSERT(pArg4, "Argument 4 not provided");
        if(RegistryIndex != -1)
        {
            man.GetReg(RegistryIndex);
            if(NumArgs >= 1) TTE_ASSERT(Meta::CoerceTypeErasedToLua(man, pArg1, classArg1), "Argument 1 could not be coerced into Lua");
            if(NumArgs >= 2) TTE_ASSERT(Meta::CoerceTypeErasedToLua(man, pArg2, classArg2), "Argument 2 could not be coerced into Lua");
            if(NumArgs >= 3) TTE_ASSERT(Meta::CoerceTypeErasedToLua(man, pArg2, classArg3), "Argument 3 could not be coerced into Lua");
            if(NumArgs == 4) TTE_ASSERT(Meta::CoerceTypeErasedToLua(man, pArg3, classArg4), "Argument 4 could not be coerced into Lua");
            ScriptManager::Execute(man, NumArgs, 0, false);
        }
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override
    {
        if(ManagerRef.Expired())
        {
            TTE_LOG("WARNING: Calling Lua function callback but manager reference expired.");
            return;
        }
        LuaManager& man = ManagerRef.Get();
        if(NumArgs >= 1) TTE_ASSERT(arg1, "Argument 1 not provided");
        if(NumArgs >= 2) TTE_ASSERT(arg2, "Argument 2 not provided");
        if(NumArgs >= 3) TTE_ASSERT(arg3, "Argument 3 not provided");
        if(NumArgs == 4) TTE_ASSERT(arg4, "Argument 4 not provided");
        if(RegistryIndex != -1)
        {
            man.GetReg(RegistryIndex);
            if(NumArgs >= 1) TTE_ASSERT(Meta::CoerceMetaToLua(man, arg1), "Argument 1 could not be coerced into Lua");
            if(NumArgs >= 2) TTE_ASSERT(Meta::CoerceMetaToLua(man, arg2), "Argument 2 could not be coerced into Lua");
            if(NumArgs >= 3) TTE_ASSERT(Meta::CoerceMetaToLua(man, arg3), "Argument 3 could not be coerced into Lua");
            if(NumArgs >= 4) TTE_ASSERT(Meta::CoerceMetaToLua(man, arg4), "Argument 4 could not be coerced into Lua");
            ScriptManager::Execute(man, NumArgs, 0, false);
        }
    }
    
};

template<U32 NumArguments>
using LuaFunction = LuaFunctionImpl<NumArguments>;

// ===================================================================         METHOD (CLASS ATTACHED)
// ===================================================================

template<typename Object, Bool Checked>
struct MethodLock
{

    Ptr<Object> MyObject;

    inline MethodLock(const WeakPtr<Object> wk)
    {
        if (!IsWeakPtrUnbound(wk))
        {
            if(wk.expired())
            {
                TTE_LOG("WARNING: Method object has expired for unchecked MethodImplBase<TObj>");
            }
            else
            {
                MyObject = wk.lock();
            }
        }
    }

    ~MethodLock() = default;

};

template<typename T>
struct MethodLock<T, false>
{
    inline MethodLock(T*) {}
};

template<typename Object, Bool Checked = true>
class MethodImplBase : public FunctionBase
{
protected:
    
    WeakPtr<Object> _MethodObject;

public:

    static constexpr Bool MyChecked = true;

    inline MethodImplBase(Ptr<Object> pObject) : _MethodObject(pObject) {}

    inline MethodImplBase(Object* pObject) : _MethodObject()
    {
        TTE_ASSERT(false, "Incorrect constructor used! Please check the macro invocation.");
    }

    inline Bool CompareBase(const MethodImplBase& rhs) const
    {
        return _MethodObject.lock() == rhs._MethodObject.lock();
    }
    
    inline virtual Bool Expired() const override
    {
        return _MethodObject.expired();
    }
    
};

template<typename Object>
class MethodImplBase<Object, false> : public FunctionBase
{
protected:

    Object* _MethodObject;

public:

    static constexpr Bool MyChecked = false;

    inline MethodImplBase(Object* pObject) : _MethodObject(pObject)
    {
    }

    inline MethodImplBase(Ptr<Object> pObject) : _MethodObject(pObject.get())
    {
    }

    inline Bool CompareBase(const MethodImplBase& rhs) const
    {
        return _MethodObject == rhs._MethodObject;
    }
    
    inline virtual Bool Expired() const override
    {
        return false; // raw ptr
    }

};

template<typename Object>
struct _MethodImplBaseSelector
{
    static constexpr Bool _MyChecked = false;
};

template<typename Object>
struct _MethodImplBaseSelector<Ptr<Object>>
{
    static constexpr Bool _MyChecked = true;
};

#define CALLBACK_TEST_CHECKED(PtrObj) _MethodImplBaseSelector<std::decay<decltype(PtrObj)>::type>::_MyChecked

template <typename T>
struct _MethodFnTraits;

//  R (Cls::*)(Arg)
template <typename Cls, typename Arg, typename R>
struct _MethodFnTraits<R(Cls::*)(Arg)> 
{
    using _Clz = Cls;
    using _ArgT = Arg;
};

// R (Cls::*)(Arg) const
template <typename Cls, typename Arg, typename R>
struct _MethodFnTraits<R(Cls::*)(Arg) const> 
{
    using _Clz = Cls;
    using _ArgT = Arg;
};

template<typename Object, Bool Checked, typename Arg1 = Placeholder, typename Arg2 = Placeholder, typename Arg3 = Placeholder, typename Arg4 = Placeholder>
struct MethodImpl;

template<typename Object, Bool Checked> // 0 ARGS
struct MethodImpl<Object, Checked, Placeholder, Placeholder, Placeholder, Placeholder> : MethodImplBase<Object, Checked>
{

    using _MethodImplBase = MethodImplBase<Object, Checked>;
    
    fastdelegate::FastDelegate0<> Delegate;
    
    virtual U32 GetNumArguments() const override final
    {
        return 0;
    }
    
    ~MethodImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* pArg4, U32 classArg4) override final
    {
        Call();
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override final
    {
        Call();
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const _MethodImplBase* pMethod = dynamic_cast<const _MethodImplBase*>(&rhs);
        return pMethod && _MethodImplBase::CompareBase(*pMethod) && this->Delegate == pMethod->Delegate;
    }
    
    inline MethodImpl(Ptr<Object> pObject, void (Object::*pMethod)()) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject.get(), pMethod);
    }

    inline MethodImpl(Object* pObject, void (Object::* pMethod)()) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject, pMethod);
    }
    
protected:
    
    inline void Call()
    {
        MethodLock<Object, Checked> _Lock{ this->_MethodObject };
        Delegate();
    }
    
};

template<typename Object, Bool Checked, typename Arg1> // 1 ARG
struct MethodImpl<Object, Checked, Arg1, Placeholder, Placeholder, Placeholder> : MethodImplBase<Object, Checked>
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    
    using _MethodImplBase = MethodImplBase<Object, Checked>;
    
    fastdelegate::FastDelegate1<Arg1> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 1; }
    
    ~MethodImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void*, U32, void*, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1, "Argument to 1-argument callback was NULL.");
        Call(*static_cast<Arg1*>(pArg1));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance, Meta::ClassInstance, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        if(value1.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Call(std::move(*value1.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pMethod = dynamic_cast<const _MethodImplBase*>(&rhs);
        return pMethod && _MethodImplBase::CompareBase(*pMethod) && this->Delegate == ((const MethodImpl*)pMethod)->Delegate;
    }
    
    inline MethodImpl(Ptr<Object> pObject, void (Object::*pMethod)(Arg1)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject.get(), pMethod);
    }

    inline MethodImpl(Object* pObject, void (Object::* pMethod)(Arg1)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject, pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1)
    {
        MethodLock<Object, Checked> _Lock{this->_MethodObject};
        Delegate(std::move(arg1));
    }
    
};

template<typename Object, Bool Checked, typename Arg1, typename Arg2> // 2 ARGS
struct MethodImpl<Object, Checked, Arg1, Arg2, Placeholder, Placeholder> : MethodImplBase<Object, Checked>
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);
    
    using _MethodImplBase = MethodImplBase<Object, Checked>;
    
    fastdelegate::FastDelegate2<Arg1, Arg2> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 2; }
    
    ~MethodImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void* pArg2, U32, void*, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1 && pArg2, "Argument(s) to 2-argument callback were NULL.");
        Call(*static_cast<Arg1*>(pArg1), *static_cast<Arg2*>(pArg2));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        if(value1.Get() && value2.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Call(std::move(*value1.Get()), std::move(*value2.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pMethod = dynamic_cast<const _MethodImplBase*>(&rhs);
        return pMethod && _MethodImplBase::CompareBase(*pMethod) && this->Delegate == ((const MethodImpl*)pMethod)->Delegate;
    }
    
    inline MethodImpl(Ptr<Object> pObject, void (Object::*pMethod)(Arg1, Arg2)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject.get(), pMethod);
    }

    inline MethodImpl(Object* pObject, void (Object::* pMethod)(Arg1, Arg2)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject, pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2)
    {
        MethodLock<Object, Checked> _Lock{ this->_MethodObject };
        Delegate(std::move(arg1), std::move(arg2));
    }
    
};

template<typename Object, Bool Checked, typename Arg1, typename Arg2, typename Arg3> // 3 ARGS
struct MethodImpl<Object, Checked, Arg1, Arg2, Arg3, Placeholder> : MethodImplBase<Object, Checked>
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg3);
    
    using _MethodImplBase = MethodImplBase<Object, Checked>;
    
    fastdelegate::FastDelegate3<Arg1, Arg2, Arg3> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 3; }
    ~MethodImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void* pArg2, U32, void* pArg3, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1 && pArg2 && pArg3, "Argument(s) to 3-argument callback were NULL.");
        Call(*static_cast<Arg1*>(pArg1), *static_cast<Arg2*>(pArg2), *static_cast<Arg3*>(pArg3));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        OptionalDefaultConstructible<Arg3> value3{};
        if(value1.Get() && value2.Get() && value3.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Meta::ExtractCoercableInstance(*value3.Get(), arg3);
            Call(std::move(*value1.Get()), std::move(*value2.Get()), std::move(*value3.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pMethod = dynamic_cast<const _MethodImplBase*>(&rhs);
        return pMethod && _MethodImplBase::CompareBase(*pMethod) && this->Delegate == ((const MethodImpl*)pMethod)->Delegate;
    }
    
    inline MethodImpl(Ptr<Object> pObject, void (Object::*pMethod)(Arg1, Arg2, Arg3)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject.get(), pMethod);
    }

    inline MethodImpl(Object* pObject, void (Object::* pMethod)(Arg1, Arg2, Arg3)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject, pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        MethodLock<Object, Checked> _Lock{ this->_MethodObject };
        Delegate(std::move(arg1), std::move(arg2), std::move(arg3));
    }
    
};

template<typename Object, Bool Checked, typename Arg1, typename Arg2, typename Arg3, typename Arg4> // 4 ARGS
struct MethodImpl : MethodImplBase<Object, Checked>
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg3);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg4);
    
    using _MethodImplBase = MethodImplBase<Object, Checked>;
    
    fastdelegate::FastDelegate4<Arg1, Arg2, Arg3, Arg4> Delegate;
    
    virtual U32 GetNumArguments() const override final
    {
        return 4;
    }
    
    ~MethodImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* pArg4, U32 classArg4) override final
    {
        TTE_ASSERT(pArg1 && pArg2 && pArg3 && pArg4, "One or more of arguments to 4-argument callback were NULL.");
        Call(*((Arg1*)pArg1), *((Arg2*)pArg2), *((Arg3*)pArg3), *((Arg4*)pArg4));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override final
    {
        // Convert meta type pArg into the C++ Arg type
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        OptionalDefaultConstructible<Arg3> value3{};
        OptionalDefaultConstructible<Arg4> value4{};
        if(value1.Get() && value2.Get() && value3.Get() && value4.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Meta::ExtractCoercableInstance(*value3.Get(), arg3);
            Meta::ExtractCoercableInstance(*value4.Get(), arg4);
            Call(std::move(*value1.Get()), std::move(*value2.Get()), std::move(*value3.Get()), std::move(*value4.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const _MethodImplBase* pMethod = dynamic_cast<const _MethodImplBase*>(&rhs);
        return pMethod && _MethodImplBase::CompareBase(*pMethod) && this->Delegate == ((const MethodImpl*)pMethod)->Delegate;
    }
    
    inline MethodImpl(Ptr<Object> pObject, void (Object::*pMethod)(Arg1, Arg2, Arg3, Arg4)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject.get(), pMethod);
    }

    inline MethodImpl(Object* pObject, void (Object::* pMethod)(Arg1, Arg2, Arg3, Arg4)) : _MethodImplBase(pObject)
    {
        Delegate.bind(pObject, pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        MethodLock<Object, Checked> _Lock{ this->_MethodObject };
        Delegate(std::move(arg1), std::move(arg2), std::move(arg3), std::move(arg4));
    }
    
};

// Meta system args bound -> a C++ instance. Member function
template<typename Object, Bool Checked, typename A1 = Placeholder, typename A2 = Placeholder, typename A3 = Placeholder, typename A4 = Placeholder>
using Method = MethodImpl<Object, Checked, A1, A2, A3, A4>;

// ===================================================================         FUNCTION (NO CLASS ATTACH)
// ===================================================================

template<typename Arg1 = Placeholder, typename Arg2 = Placeholder, typename Arg3 = Placeholder, typename Arg4 = Placeholder>
struct FunctionImpl;

template<> // 0 ARGS
struct FunctionImpl<Placeholder, Placeholder, Placeholder, Placeholder> : FunctionBase
{
    
    fastdelegate::FastDelegate0<> Delegate;
    
    virtual U32 GetNumArguments() const override final
    {
        return 0;
    }
    
    ~FunctionImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32 classArg1, void* pArg2, U32 classArg2, void* pArg3, U32 classArg3, void* pArg4, U32 classArg4) override final
    {
        Call();
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override final
    {
        Call();
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const FunctionImpl* pMethod = dynamic_cast<const FunctionImpl*>(&rhs);
        return Delegate == pMethod->Delegate;
    }
    
    inline FunctionImpl(void (*pMethod)())
    {
        Delegate.bind(pMethod);
    }
    
protected:
    
    inline void Call()
    {
        Delegate();
    }
    
};

template<typename Arg1> // 1 ARG
struct FunctionImpl<Arg1, Placeholder, Placeholder, Placeholder> : FunctionBase
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    
    fastdelegate::FastDelegate1<Arg1> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 1; }
    ~FunctionImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void*, U32, void*, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1, "Argument to 1-argument function was NULL.");
        Call(*((Arg1*)pArg1));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance, Meta::ClassInstance, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        if(value1.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Call(std::move(*value1.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pFunc = dynamic_cast<const FunctionImpl*>(&rhs);
        return pFunc && this->Delegate == pFunc->Delegate;
    }
    
    inline FunctionImpl(void (*pMethod)(Arg1))
    {
        Delegate.bind(pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1)
    {
        Delegate(std::move(arg1));
    }
    
};

template<typename Arg1, typename Arg2> // 2 ARGS
struct FunctionImpl<Arg1, Arg2, Placeholder, Placeholder> : FunctionBase
{
    
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);

    fastdelegate::FastDelegate2<Arg1, Arg2> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 2; }
    ~FunctionImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void* pArg2, U32, void*, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1 && pArg2, "One or more arguments to 2-argument function were NULL.");
        Call(*((Arg1*)pArg1), *((Arg2*)pArg2));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        if(value1.Get() && value2.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Call(std::move(*value1.Get()), std::move(*value2.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pFunc = dynamic_cast<const FunctionImpl*>(&rhs);
        return pFunc && this->Delegate == pFunc->Delegate;
    }
    
    inline FunctionImpl(void (*pMethod)(Arg1, Arg2))
    {
        Delegate.bind(pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2)
    {
        Delegate(std::move(arg1), std::move(arg2));
    }
    
};

template<typename Arg1, typename Arg2, typename Arg3> // 3 ARGS
struct FunctionImpl<Arg1, Arg2, Arg3, Placeholder> : FunctionBase
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg3);
    
    fastdelegate::FastDelegate3<Arg1, Arg2, Arg3> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 3; }
    ~FunctionImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void* pArg2, U32, void* pArg3, U32, void*, U32) override final
    {
        TTE_ASSERT(pArg1 && pArg2 && pArg3, "One or more arguments to 3-argument function were NULL.");
        Call(*((Arg1*)pArg1), *((Arg2*)pArg2), *((Arg3*)pArg3));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        OptionalDefaultConstructible<Arg3> value3{};
        if(value1.Get() && value2.Get() && value3.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Meta::ExtractCoercableInstance(*value3.Get(), arg3);
            Call(std::move(*value1.Get()), std::move(*value2.Get()), std::move(*value3.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pFunc = dynamic_cast<const FunctionImpl*>(&rhs);
        return pFunc && this->Delegate == pFunc->Delegate;
    }
    
    inline FunctionImpl(void (*pMethod)(Arg1, Arg2, Arg3))
    {
        Delegate.bind(pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2, Arg3 arg3)
    {
        Delegate(std::move(arg1), std::move(arg2), std::move(arg3));
    }
    
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4> // 4 ARGS
struct FunctionImpl : FunctionBase
{

    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg1);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg2);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg3);
    CALLBACK_ARGUMENT_ASSERT_NOENUM(Arg4);

    fastdelegate::FastDelegate4<Arg1, Arg2, Arg3, Arg4> Delegate;
    
    virtual U32 GetNumArguments() const override final { return 4; }
    ~FunctionImpl() = default;
    
    inline virtual void CallErased(void* pArg1, U32, void* pArg2, U32, void* pArg3, U32, void* pArg4, U32) override final
    {
        TTE_ASSERT(pArg1 && pArg2 && pArg3 && pArg4, "One or more arguments to 4-argument function were NULL.");
        Call(*((Arg1*)pArg1), *((Arg2*)pArg2), *((Arg3*)pArg3), *((Arg4*)pArg4));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance arg1, Meta::ClassInstance arg2, Meta::ClassInstance arg3, Meta::ClassInstance arg4) override final
    {
        OptionalDefaultConstructible<Arg1> value1{};
        OptionalDefaultConstructible<Arg2> value2{};
        OptionalDefaultConstructible<Arg3> value3{};
        OptionalDefaultConstructible<Arg4> value4{};
        if(value1.Get() && value2.Get() && value3.Get() && value4.Get())
        {
            Meta::ExtractCoercableInstance(*value1.Get(), arg1);
            Meta::ExtractCoercableInstance(*value2.Get(), arg2);
            Meta::ExtractCoercableInstance(*value3.Get(), arg3);
            Meta::ExtractCoercableInstance(*value4.Get(), arg4);
            Call(std::move(*value1.Get()), std::move(*value2.Get()), std::move(*value3.Get()), std::move(*value4.Get()));
        }
        else
        {
            TTE_ASSERT(false, "Cannot use CallMeta: one or more of the arguments to this callback are not default contructible");
        }
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const auto* pFunc = dynamic_cast<const FunctionImpl*>(&rhs);
        return pFunc && this->Delegate == pFunc->Delegate;
    }
    
    inline FunctionImpl(void (*pMethod)(Arg1, Arg2, Arg3, Arg4))
    {
        Delegate.bind(pMethod);
    }
    
protected:
    
    inline void Call(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
    {
        Delegate(std::move(arg1), std::move(arg2), std::move(arg3), std::move(arg4));
    }
    
};


// Meta system args bound -> a C++ instance. Non-member function
template<typename A1 = Placeholder, typename A2 = Placeholder, typename A3 = Placeholder, typename A4 = Placeholder>
using Function = FunctionImpl<A1, A2, A3, A4>;

// ===================================================================         UTIL
// ===================================================================

// Annoying fix to be able to pass types with commas in them into macros by wrapping in function as its argument.
template<typename T> struct _TemplArgFnWrapper;
template<typename T, typename U> struct _TemplArgFnWrapper<T(U)> { typedef U _FTy; };

#define ALLOCATE_LUA_CALLBACK(NumArgs, Manager) TTE_NEW_PTR(LuaFunction<NumArgs>, MEMORY_TAG_CALLBACK, Manager)

// Allocate class function callback with no arguments
#define ALLOCATE_METHOD_CALLBACK_0(PtrObj, MemFn, MethodClass) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Method<MethodClass MACRO_COMMA CALLBACK_TEST_CHECKED(PtrObj)>)>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, &MethodClass::MemFn)

// Allocate class function callback with one argument
#define ALLOCATE_METHOD_CALLBACK_1(PtrObj, MemFn, MethodClass, ArgType) \
 TTE_NEW_PTR(_TemplArgFnWrapper<void(Method<MethodClass MACRO_COMMA CALLBACK_TEST_CHECKED(PtrObj) MACRO_COMMA ArgType>)>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, &MethodClass::MemFn)

// Allocate class function callback with 2 arguments
#define ALLOCATE_METHOD_CALLBACK_2(PtrObj, MemFn, MethodClass, ArgType1, ArgType2) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Method<MethodClass MACRO_COMMA CALLBACK_TEST_CHECKED(PtrObj) MACRO_COMMA ArgType1 MACRO_COMMA ArgType2 \
                                    >)>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, &MethodClass::MemFn)

// Allocate class function callback with 3 arguments
#define ALLOCATE_METHOD_CALLBACK_3(PtrObj, MemFn, MethodClass, ArgType1, ArgType2, ArgType3) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Method<MethodClass MACRO_COMMA CALLBACK_TEST_CHECKED(PtrObj) MACRO_COMMA ArgType1 MACRO_COMMA ArgType2 \
                                    MACRO_COMMA ArgType3>)>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, &MethodClass::MemFn)

// Allocate class function callback with 4 arguments
#define ALLOCATE_METHOD_CALLBACK_4(PtrObj, MemFn, MethodClass, ArgType1, ArgType2, ArgType3, ArgType4) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Method<MethodClass MACRO_COMMA CALLBACK_TEST_CHECKED(PtrObj) MACRO_COMMA ArgType1 MACRO_COMMA ArgType2 \
                                    MACRO_COMMA ArgType3 MACRO_COMMA ArgType4>)>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, &MethodClass::MemFn)

// Allocate static/free function callback with no arguments
#define ALLOCATE_FUNCTION_CALLBACK_0(Fn) \
TTE_NEW_PTR(Function<>, MEMORY_TAG_CALLBACK, Fn)

// Allocate static/free function callback with 1 argument
#define ALLOCATE_FUNCTION_CALLBACK_1(Fn, ArgType) \
TTE_NEW_PTR(Function<ArgType>, MEMORY_TAG_CALLBACK, Fn)

// Allocate static/free function callback with 2 arguments
#define ALLOCATE_FUNCTION_CALLBACK_2(Fn, ArgType1, ArgType2) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Function<ArgType1 MACRO_COMMA ArgType2>)>::_FTy, MEMORY_TAG_CALLBACK, Fn)

// Allocate static/free function callback with 3 arguments
#define ALLOCATE_FUNCTION_CALLBACK_3(Fn, ArgType1, ArgType2, ArgType3) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Function<ArgType1 MACRO_COMMA ArgType2 MACRO_COMMA ArgType3>)>::_FTy, MEMORY_TAG_CALLBACK, Fn)

// Allocate static/free function callback with 4 arguments
#define ALLOCATE_FUNCTION_CALLBACK_4(Fn, ArgType1, ArgType2, ArgType3, ArgType4) \
TTE_NEW_PTR(_TemplArgFnWrapper<void(Function<ArgType1 MACRO_COMMA ArgType2 MACRO_COMMA ArgType3 MACRO_COMMA ArgType4>)>::_FTy, MEMORY_TAG_CALLBACK, Fn)


