#pragma once

#include <fastdelegate/FastDelegate.h>

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>

// CALLBACKS HELPERS FOR META SYSTEM

// Base polymorphic class for functions and methods. Functions have no associated class instance. Methods do.
struct FunctionBase
{
  
    // Call with C++ argument
    virtual void CallErased(void* pArg1) = 0;
    
    // Call with Meta system argument
    virtual void CallMeta(Meta::ClassInstance& arg1) = 0;
    
    // Comparison
    virtual Bool Equals(const FunctionBase& rhs) const = 0;
    
    virtual ~FunctionBase() = default;
    
};

// Meta system args bound -> a C++ instance. Member function
template<typename Object, typename Arg1>
struct Method : FunctionBase
{
    
    WeakPtr<Object> MethodObject;
    fastdelegate::FastDelegate1<Arg1> Delegate;
    
    inline virtual void CallErased(void* pArg1) override final
    {
        Call(*((Arg1*)pArg1));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance& arg1) override final
    {
        // Convert meta type pArg1 into the C++ Arg1 type
        Arg1 value{};
        Meta::ExtractPODInstance(value, arg1);
        Call(std::move(value));
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const Method* pMethod = dynamic_cast<const Method*>(&rhs);
        return pMethod && pMethod->MethodObject.lock() == MethodObject.lock() && Delegate == pMethod->Delegate;
    }
    
    inline void Call(Arg1 arg1)
    {
        Ptr<Object> acquired{};
        if(!IsWeakPtrUnbound(MethodObject))
        {
            if(MethodObject.expired())
                return;
            acquired = MethodObject.lock(); // keep here. delegate pointer lifetime
        }
        Delegate(arg1);
    }
    
    Method(Ptr<Object> pObject, void (Object::*pMethod)(Arg1))
    {
        MethodObject = pObject;
        Delegate.bind(pObject.get(), pMethod);
    }
    
    ~Method() = default;
    
};

// Meta system args bound -> a C++ instance. Non-member function
template<typename Arg1>
struct Function : FunctionBase
{
    
    fastdelegate::FastDelegate1<Arg1> Delegate;
    
    inline virtual void CallErased(void* pArg1) override final
    {
        Call(*((Arg1*)pArg1));
    }
    
    inline virtual void CallMeta(Meta::ClassInstance& arg1) override final
    {
        // Convert meta type pArg1 into the C++ Arg1 type
        Arg1 value{};
        Meta::ExtractPODInstance(value, arg1);
        Call(std::move(value));
    }
    
    inline virtual Bool Equals(const FunctionBase& rhs) const override final
    {
        const Function* pMethod = dynamic_cast<const Function*>(&rhs);
        return Delegate == pMethod->Delegate;
    }
    
    inline void Call(Arg1 arg1)
    {
        Delegate(arg1);
    }
    
    // Non member function
    inline Function(void (*pFunc)(Arg1))
    {
        Delegate.bind(pFunc);
    }
    
    ~Function() = default;
    
};

// Annoying fix to be able to pass types with commas in them into macros by wrapping in function as its argument.
template<typename T> struct _TemplArgFnWrapper;
template<typename T, typename U> struct _TemplArgFnWrapper<T(U)> { typedef U _FTy; };

#define ALLOCATE_METHOD_CALLBACK(PtrObj, MemFn, MethodClass, ArgType) \
 TTE_NEW_PTR(_TemplArgFnWrapper<void((Method<MethodClass, ArgType>))>::_FTy, MEMORY_TAG_CALLBACK, PtrObj, MemFn)

#define ALLOCATE_FUNCTION_CALLBACK(Fn, ArgType) TTE_NEW_PTR(FunctionMethod<ArgType>, MEMORY_TAG_CALLBACK, Fn)
