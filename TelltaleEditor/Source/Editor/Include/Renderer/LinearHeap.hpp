#pragma once

#include <Core/Config.hpp>

/**
 * Linear heap is a classic game engine design. A paged memory allocated that supports holding destructors and calling them when a page is released.
 * This is not just a memory allocator, but an OOP supported GC.
 * This is from my TelltaleToolLib library and is the same implementation as Telltales.
 */
class LinearHeap {
    
    struct Page {
        U32 _Size;
        U32 _Index;
        Page* _Next;
    };
    
    struct ObjWrapperBase {
        
        ObjWrapperBase* _Next;
        
        inline virtual ~ObjWrapperBase() {}
        
    };
    
    template<typename T>
    struct ObjWrapper : ObjWrapperBase {
        
        T _Obj;
        
        inline virtual ~ObjWrapper() {}
        
    };
    
    template<typename T>
    struct ObjArrayWrapper : ObjWrapperBase {
        
        U32 _Size;
        T* _Array;
        
        inline virtual ~ObjArrayWrapper() {}
        
    };
    
    struct Context {
        
        ObjWrapperBase* _FirstObject, * _LastObject;
        Page* _Page;
        U32 _PagePos;
        U32 _ObjCount;
        Context* _Next;
        
    };
    
    U32 _PageCount; // number of active pages
    U32 _TotalMemUsed; // total bytes used so far
    U32 _PageSize; // page size, constant and large, eg 0x100000
    U32 _CurrentPos; // current position in the current page
    Context* _ContextStack; // context stack array
    Context _BaseContext; // base context stack
    Page* _BasePage, * _CurrentPage; // page info
    
    inline Page* _AllocatePage(U32 size = 0){
        size = MAX(size, _PageSize);
        Page* pg = (Page*)TTE_ALLOC(size, MEMORY_TAG_RENDERER);
        if (!pg)
            return 0;
        memset(pg, 0, size);
        pg->_Index = _PageCount++;
        pg->_Size = size;
        pg->_Next = 0;
        _TotalMemUsed += size;
        return pg;
    }
    
    inline void _ReleasePageList(Page* pPage) {
        while(pPage){
            Page* next = pPage->_Next;
            TTE_FREE(pPage);
            pPage = next;
        }
    }
    
    inline void _CallDestructors(Context& context){
        ObjWrapperBase* pObject = context._FirstObject;
        while(pObject){
            ObjWrapperBase* next = pObject->_Next;
            pObject->~ObjWrapperBase();
            pObject = next;
        }
        context._LastObject = context._FirstObject = NULL;
        context._ObjCount = 0;
    }
    
    inline void _AddObject(ObjWrapperBase* pObj){
        if(_ContextStack->_LastObject)
            _ContextStack->_LastObject->_Next = pObj;
        if (!_ContextStack->_FirstObject)
            _ContextStack->_FirstObject = pObj;
        _ContextStack->_LastObject = pObj;
        ++_ContextStack->_ObjCount;
    }
    
    inline void _PopContextInt(){
        _CallDestructors(*_ContextStack);
        _ContextStack->_FirstObject = _ContextStack->_LastObject = 0;
        _ContextStack->_ObjCount = 0;
        _CurrentPage = _ContextStack->_Page;
        _CurrentPos = _ContextStack->_PagePos;
        _ContextStack = _ContextStack->_Next;
        if(_ContextStack == nullptr)
            _ContextStack = &_BaseContext;
    }
    
public:
    
    // Construct with optional non-default page size
    inline LinearHeap(U32 pageSize = 0x100000) : _ContextStack(&_BaseContext), _CurrentPage(0), _BasePage(0),
        _TotalMemUsed(0), _PageSize(pageSize), _CurrentPos(0), _PageCount(0) {}
    
    // destruct normally.
    inline ~LinearHeap() {
        ReleaseAll();
    }
    
    /**
     * Allocate size number of bytes with a optional align. Align can be from 1 to 8 and must be a power of two (1,2,4,8.).
     */
    inline U8* Alloc(U32 size, U32 align = 1){
        Page* currentPage = _CurrentPage;
        U8* pRet = 0;
        if(currentPage){
            U32 alignedOffset = (_CurrentPos + (align - 1)) & ~(align - 1);
            if (alignedOffset + size <= currentPage->_Size){
                //OK found a page!
                pRet = (U8*)currentPage + sizeof(Page) + alignedOffset;
                _CurrentPos = size + alignedOffset;
            }else{
                Page* pNext = _AllocatePage(size);
                _CurrentPos = size;
                pNext->_Next = currentPage;
                _CurrentPage = pNext;
                pRet = (U8*)pNext + sizeof(Page);
            }
        }else{
            _CurrentPage = currentPage = _AllocatePage(size);
            pRet = (U8*)currentPage + sizeof(Page);
            _CurrentPos = size;
        }
        return pRet;
    }
    
    /**
     * Allocates and returns T. Calls default ctor. The destructor will be called when you pop the current context.
     */
    template<typename T>
    inline T* New() {
        ObjWrapper<T>* pAlloc = (ObjWrapper<T>*)Alloc(sizeof(ObjWrapper<T>), alignof(ObjWrapper<T>));
        pAlloc->_Next = 0;
        new ((char*)(&pAlloc->_Obj)) T();
        _AddObject(pAlloc);
        return &pAlloc->_Obj;
    }
    
    /**
     * See New. Allocates an array of numElem consecutively in memory and returns it. Destructors will be called when the current context is popped.
     */
    template<typename T>
    inline T* NewArray(U32 numElem) {
        ObjArrayWrapper<T>* pArrayWrapper = (ObjArrayWrapper<T>*)Alloc(sizeof(ObjArrayWrapper<T>), alignof(ObjArrayWrapper<T>));
        pArrayWrapper->_Array = NewArrayNoDestruct<T>(numElem);
        pArrayWrapper->_Next = 0;
        _AddObject(pArrayWrapper);
        return pArrayWrapper->_Array;
    }
    
    /**
     * Creates a new instance of T in this linear heap. Its constructor IS called, but destructor will not be called at all by this class so you will need to if you wish.
     */
    template<typename T>
    inline T* NewNoDestruct() {
        U8* pAlloc = Alloc(sizeof(T), alignof(T));
        new (pAlloc) T();
        return (T*)pAlloc;
    }
    
    /**
     * See NewNoDestruct. Array version. Consecutive in memory.
     */
    template<typename T>
    inline T* NewArrayNoDestruct(U32 numElem) {
        U8* pAlloc = Alloc(sizeof(T) * numElem, alignof(T));
        for (U32 i = 0; i < numElem; i++)
            new (pAlloc + (i * sizeof(T))) T();
        return (T*)pAlloc;
    }
    
    /**
     * Pushes a destructor context. This means all objects returned from New and NewArray after this call (without NoDestruct) will have their destructors called at the next PopContext,
     * which will pop this newly pushed context.
     */
    inline void PushContext() {
        Context* pContext = NewNoDestruct<Context>();
        pContext->_Next = _ContextStack;
        _ContextStack = pContext;
    }
    
    /**
     * Pops a context. This will call destructors of any objects created using New or NewArray between the last call of PushContext and this call.
     */
    inline void PopContext() {
        if(_ContextStack){
            _PopContextInt();
            if (!_CurrentPage)
                _CurrentPage = _BasePage;
        }
    }
    
    /**
     * Gets the index of the base page in this linear heap. Return -1 if no pages.
     */
    inline int GetBasePageIndex() {
        return _BasePage ? _BasePage->_Index : -1;
    }
    
    /**
     * Returns the number of bytes free in the current page - anymore would require a new allocation.
     */
    inline int GetCurrentPageBytesFree(){
        return _CurrentPage ? _CurrentPage->_Size - _CurrentPos : _PageSize;
    }
    
    /**
     * Gets the page index for a given pointer, checking the pointer is in the range of one the pages. Else returns -1.
     */
    inline int GetPageIndexForAlloc(void* pAlloc){
        for(Page* i = _BasePage; i; i = i->_Next){
            if (pAlloc >= ((U8*)i + sizeof(Page)) && pAlloc <= ((U8*)i + sizeof(Page) + i->_Size))
                return i->_Index;
        }
        return -1;
    }
    
    /**
     * Frees only the first page in this linear heap.
     */
    inline void FreeFirstPage() {
        if(_BasePage){
            Page* b = _BasePage->_Next;
            TTE_FREE(_BasePage);
            _BasePage = b;
            _PageCount--;
        }
        if(!_PageCount){
            _TotalMemUsed = 0;
            _CurrentPage = 0;
        }
    }
    
    /**
     * Pops all contexts in this linear heap, calling all destructors.
     */
    inline void FreeAll() {
        while (_ContextStack != &_BaseContext)
            PopContext();
    }
    
    /**
     * Releases all memory associated with this linear heap, along with popping all contexts and calling destructors.
     */
    inline void ReleaseAll(){
        FreeAll();
        _ReleasePageList(_BasePage);
    }
    
    /**
     * Returns a string allocated with the contents of the given constant string. Length can be 0 in which the length will be calculated. The modifiable string is returned.
     */
    inline U8* StringIntern(CString s, U32 length = 0){
        if (!length)
            length = (U32)strlen(s);
        U8* pAlloc = Alloc(length + 1, 1);
        memcpy(pAlloc, s, length);
        pAlloc[length] = 0;
        return pAlloc;
    }
    
};
