#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Resource/DataStream.hpp>
#include <Scripting/LuaManager.hpp>
#include <vector>
#include <set>
#include <functional>

// maximum number of versions of a given typename (eg int) with different version CRCs allowed (normally theres only 1 so any more is not likely)
#define MAX_VERSION_NUMBER 10

// Meta system is all inside this namespace. This is a reflection system initialised by the lua scripts.
// ClassID is the combined hash of the type name and version crc of that type.
namespace Meta {
    
    // ======================================== META BINARY TYPES ========================================
    
    // Sections of binary stream
    enum StreamSection {
        STREAM_SECTION_MAIN = 0,
        STREAM_SECTION_ASYNC = 1,
        STREAM_SECTION_DEBUG = 2,
        STREAM_SECTION_COUNT = 3,
    };
    
    // Binary stream versions
    enum StreamVersion {
        MBIN = 0, // Meta BINary
        MBES = 1, // Meta Binary Encrypted Stream. This is not a valid version to write with, encrypt archives if needed.
        MTRE = 2, // Meta Type References
        MCOM = 3, // Meta Compressed
        MSV4 = 4, // Meta Stream Version 4 (unused)
        MSV5 = 5, // 5
        MSV6 = 6, // 6
    };
    
    // A binary meta stream. Used internally.
    struct Stream {
        
        struct Section {
            DataStreamRef Data; // Section binary data stream
            Bool Compressed; // If section is compressed
            std::vector<U64> Blocks; // For blocks. In read, stores the sizes, in write stores the block offset initial.
        };
        
        StreamVersion Version;
        Section Sect[STREAM_SECTION_COUNT];
        std::vector<U32> VersionInf; // vector of class IDs
        
        StreamSection CurrentSection = STREAM_SECTION_MAIN;
        
        inline Bool Write(const U8* Buffer, U64 BufferLength)
        {
            return Sect[CurrentSection].Data->Write(Buffer, BufferLength);
        }
        
        inline Bool Read(U8* Buffer, U64 BufferLength)
        {
            return Sect[CurrentSection].Data->Read(Buffer, BufferLength);
        }
        
    };
    
    // ======================================== INTERNAL META TYPES ========================================
    
    enum MetaMemberFlag {
        MEMBER_MEMORY_DISABLE = 1, // this member is not actually in memory but just exists for the version hashing
        MEMBER_ENUM = 2, // this member is an enum
        MEMBER_FLAG = 4, // this member is a flags bitfield
        MEMBER_BASE = 8, // this member is a base class
        MEMBER_SERIALISE_DISABLE = 16, // skip serialisation for this member. this also excludes this member from version hash
        MEMBER_VERSION_HASH_DISABLE = 32, // dont include in any version hashes, although is still in memory and serialised
    };
    
    enum MetaClassFlag {
        CLASS_INTRINSIC = 1, // this class is an intrinsic type so is not added to meta class header
        CLASS_ALLOW_ASYNC = 2, // allow this class to be serialised asynchronously, ie its a very large class.
        CLASS_CONTAINER = 4, // this class is a container type (map/set/array/etc)
        CLASS_ABSTRACT = 8, // this class is abstract (Baseclass_ prefix for ex.) so instances of it cannot be created.
        CLASS_NON_BLOCKED = 16, // this class is not blocked in serialisation
    };
    
    // A type member
    struct Member {
        
        String Name;
        U32 Flags;
        U32 ClassID; // member type
        
        U32 RTOffset = 0; // runtime offset in memory
        
    };
    
    class ClassInstance;
    
    // Weak reference to the parent. Deep into member trees, these point to the top level class, eg: array of materials , top level is D3DMesh
    using ParentWeakReference = std::weak_ptr<U8>;
    
    // A type class. This as well as Member are used internally. Refer to classes using the index (U32 - internal version CRC).
    // Refer to members by name string
    struct Class {
        
        // MEMBERS REQUIRED.
        String Name = "";
        U32 Flags = 0;
        
        // CREATED INTERNALLY.
        U64 TypeHash = 0;
        
        U32 ClassID;
        U32 VersionCRC;
        U32 VersionNumber; // similar to version crc, but a number (normally 0, 1 etc). Associate a ID for multiple verisons of the same typename.
        
        // USE FOR CONTAINER TYPES
        U32 ArraySize = 0; // if this is an SArray<T,N>, this is N. Value only set of SArrays
        U32 ArrayKeyClass = 0; // if this is a collection, this is the key value class
        U32 ArrayValClass = 0; // if this is a collection, this ia the value class
        
        // RUNTIME DATA, INTERNAL.
        U32 RTSize = 0; // runtime internal size of the class, automatically generated unless intrinsic.
        
        // C++ IMPL FOR INTRINSICS/CONTAINERS/NON-POD
        void (*Constructor)(void* pMemory, U32 ClassID, ParentWeakReference host) = nullptr;
        void (*Destructor)(void* pMemory, U32 ClassID) = nullptr;
        void (*CopyConstruct)(const void* pSrc, void* pDst, ParentWeakReference host) = nullptr;
        void (*MoveConstruct)(void* pSrc, void* pDst, ParentWeakReference host) = nullptr;
        
        // OTHER OPERATIONS (MAINLY FOR INTRINSICS)
        Bool (*LessThan)(const void* pLHS, const void* pRHS) = nullptr; // less than operator on two instances
        Bool (*Equals)(const void* pLHS, const void* pRHS) = nullptr; // compare two instances
        String (*ToString)(const void* pMemory) = nullptr; // converts to string
        
        // serialiser (needed for intrinsics/containers) function. iswrite if write, else reading.
        Bool (*Serialise)(Stream& stream, ClassInstance& host, Class* clazz, void* pInstance, Bool IsWrite) = nullptr;
        String SerialiseScriptFn = ""; // custom serialise overrider, function name in scripts
        
        // MEMBERS ARRAY
        std::vector<Member> Members;
        
    };
    
    // compilsed serialisation script
    struct CompiledSerialiserScript
    {
        U8* Binary = nullptr;
        U32 Size = 0;
    };
    
    // Internal implementation
    namespace _Impl {
        
        Class* _GetClass(U32 i); // gets class ptr from index. MUST exist.
        
        U32 _ClassChildrenArrayOff(Class& clazz); // internal children refs vector offset
        
        U32 _ClassRuntimeSize(Class& clazz, ParentWeakReference& parentRef); // internal runtime total size of class with parent
        
        U32 _Register(LuaManager& man, Class&& c, I32 classTableStackIndex); // register new class and calc CRCs
        
        U32 _DoLuaVersionCRC(LuaManager& man, I32 classTableStackIndex); // calculate version crc32
        
        void _DoConstruct(Class* pClass, U8* pMemory, ParentWeakReference host); // internally construct type into memory
        
        void _DoDestruct(Class* pClass, U8* pMemory); // internally call destruct
        
        void _DoCopyConstruct(Class* pClass, U8* pDst, const U8* pSrc, ParentWeakReference host); // internally copy type
        
        void _DoMoveConstruct(Class* pClass, U8* pDst, U8* pSrc, ParentWeakReference host); // internally move type
        
        String _PerformToString(U8* pMemory, Class* pClass);
        
        // for c++ controlled: host is empty. if script object: host MUST be a reference to a c++ controlled one.
        ClassInstance _MakeInstance(U32 ClassID, ClassInstance& host); // allocates but does not construct anything in the memory
        
        // some serialisers have 'host' argument: top level class object
        Bool _Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        // serialises a given type to the stream
        
        Bool _DefaultSerialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        // default serialise (member by member)
        
        // internal serialisers
        
        Bool SerialiseString(Stream& stream, ClassInstance& host, Class*, void* pMemory, Bool IsWrite);
        
        Bool SerialiseCollection(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        
        Bool SerialiseBool(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        
        Bool SerialiseSymbol(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        
        // internal type defaults
        
        void CtorCol(void* pMemory, U32 Array, ParentWeakReference host);
        
        void DtorCol(void* pMemory, U32);
        
        void CopyCol(const void* pSrc, void* pDst, ParentWeakReference host);
        
        void MoveCol(void* pSrc, void* pDst, ParentWeakReference host);
        
        
        void CtorString(void* pMemory, U32, ParentWeakReference host);
        
        void DtorString(void* pMemory, U32);
        
        void CopyString(const void* pSrc, void* pDst, ParentWeakReference host);
        
        void MoveString(void* pSrc, void* pDst, ParentWeakReference host);
        
        
        void CtorBinaryBuffer(void* pMemory, U32, ParentWeakReference host);
        
        void DtorBinaryBuffer(void* pMemory, U32);
        
        void CopyBinaryBuffer(const void* pSrc, void* pDst, ParentWeakReference host);
        
        void MoveBinaryBuffer(void* pSrc, void* pDst, ParentWeakReference host);
        
    }
    
    // Special type internally used to store binary buffers. No serialiser in this type, but just holds a reference to the memory (frees it)
    struct BinaryBuffer
    {
        U8* Buffer = nullptr;
        U32 BufferSize = 0;
    };
    
    struct BlowfishKey // encryption key
    {
        
        U8 BfKey[56];
        U32 BfKeyLength = 0;
        
        inline BlowfishKey()
        {
            memset(BfKey, 0, 56);
        }
        
    };
    
    // Registered game
    struct RegGame {
        
        String Name, ID;
        StreamVersion MetaVersion = MBIN;
        LuaVersion LVersion = LuaVersion::LUA_5_2_3;
        std::map<String, BlowfishKey> PlatformToEncryptionKey;
        BlowfishKey MasterKey; // key used for all platforms
        Bool ModifiedBlowfish = false;
        
    };
    
    // ======================================== PUBLIC META TYPES ========================================
    
    class ClassInstance {
        
        // FRIENDS FOR ACCESSING PRIVATE FUNCTIONALITY
        
        friend class ClassInstanceCollection;
        
        friend class ClassInstanceScriptRef;
        
        friend ClassInstance GetMember(ClassInstance& inst, const String& name);
        
        friend ClassInstance _Impl::_MakeInstance(U32, ClassInstance&);
        
        friend Bool _Impl::_Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite);
        
    public:
        
        // Default move and copy stuff
        ClassInstance(const ClassInstance&) = default;
        ClassInstance(ClassInstance&&) = default;
        ClassInstance& operator=(const ClassInstance&) = default;
        ClassInstance& operator=(ClassInstance&&) = default;
        
        // Default constructor refers to no instance in memory.
        inline ClassInstance() : _InstanceClassID(0), _InstanceMemory(nullptr) {}
        
        inline operator Bool() const {
            return _InstanceClassID && (_InstanceMemory ? true : false); // call bool operator on ptr
        }
        
        // Internal use. Returns the deleter
        inline void (*_GetDeleter())(U8*)
        {
            if(!_InstanceMemory)
                return nullptr;
            auto Dx = std::get_deleter<void(*)(U8*)>(_InstanceMemory);
            return Dx ? *Dx : nullptr;
        }
        
        // Pushes a weak reference to this instance to the lua stack. Pass in the host class (owner) of this object.
        // Weak here means instance is not owned by the script reference. After all C++ instances are destroyed, getting the object returns nil.
        void PushScriptRef(LuaManager& man);
        
        // returns true if this instance has expired because its parent is no longer alive.
        inline Bool Expired()
        {
            return !IsWeakPtrUnbound(_ParentAttachMemory) && _ParentAttachMemory.expired();
        }
        
        // Internal use. Gets the memory pointer
        inline U8* _GetInternal()
        {
            return Expired() ? nullptr : _InstanceMemory.get();
        }
        
        // Gets the class ID
        inline U32 GetClassID() const
        {
            return _InstanceClassID;
        }
        
        // obtains a parent weak reference.
        inline ParentWeakReference ObtainParentRef()
        {
            return _InstanceMemory ? IsWeakPtrUnbound(_ParentAttachMemory) ?
                ParentWeakReference(_InstanceMemory) : _ParentAttachMemory : ParentWeakReference{};
        }
        
        // sets the parent. NOTE: must have no parent present! Nothing happens if no instance.
        inline void SetParentRef(ClassInstance& parent)
        {
            if(!_InstanceMemory || !_InstanceClassID)
                return;
            TTE_ASSERT(IsWeakPtrUnbound(_ParentAttachMemory), "Cannot assign parent: parent is already set");
            _ParentAttachMemory = parent.ObtainParentRef(); // create weak reference
        }
        
        // Returns if this class instance is a top-level instance, ie it is not owned by any other class instance.
        inline Bool IsTopLevel() const
        {
            if(_InstanceClassID == 0)
                return false; // invalid anyway
            Bool unbound = IsWeakPtrUnbound(_ParentAttachMemory);
            if(unbound)
                return true; // yes, no parent
            // is parent == instance, that still is ok, so check
            auto lck = _ParentAttachMemory.lock();
            return lck ? lck.get() == _InstanceMemory.get() : false;
        }
        
    private:
        
        // Use by _MakeInstance. Deleter is defined in the meta.cpp TU.
        inline ClassInstance(U32 storedID, U8* memory, std::function<void(U8*)> _deleter, ParentWeakReference attachTo) :
            _InstanceClassID(storedID), _InstanceMemory(memory, _deleter), _ParentAttachMemory(std::move(attachTo)) {}
        
        // Use by the collection class. takes in the class and memory, but the memory won't be deleted
        inline ClassInstance(U32 storedID, U8* memoryNoDelete, ParentWeakReference attachTo) : _InstanceClassID(storedID),
            _InstanceMemory(memoryNoDelete, &NullDeleter), _ParentAttachMemory(std::move(attachTo)) {}
        
        // Use by the script ref to create an acquired reference from a script object
        inline ClassInstance(U32 storedID, std::shared_ptr<U8> acquired, ParentWeakReference prnt) : _InstanceClassID(storedID),
            _InstanceMemory(std::move(acquired)), _ParentAttachMemory(std::move(prnt)) {}
        
        // after memory and sarray elements, this is stored if we are a top level (ie no parent)
        std::vector<std::shared_ptr<U8>>* _GetInternalChildrenRefs();
        
        U32 _InstanceClassID; // class id
        std::shared_ptr<U8> _InstanceMemory; // memory pointer to instance in memory
        ParentWeakReference _ParentAttachMemory; // weak reference to top level parent this instance is controlled by. NO ACCESS is
        // ever given to the parent. this is such that, for example, in async jobs the parent is kept constant and untouched by each child.
        
    };
    
    // Weak reference to a meta class instance, used internally. Used for letting lua scripts access class instances
    class ClassInstanceScriptRef
    {
        
        U32 ClassID;
        std::weak_ptr<U8> InstanceRef;
        ParentWeakReference ParentWeakRef; // weak reference to parent
        
        // internal version returns ref counted pointer, so acquire increases strong ref #
        inline std::shared_ptr<U8> __GetInternal()
        {
            if(IsWeakPtrUnbound(ParentWeakRef) || !ParentWeakRef.expired())
            {
                Bool exp = InstanceRef.expired();
                return exp ? nullptr : InstanceRef.lock();
            }
            return nullptr;
        }
        
    public:
        
        inline ClassInstanceScriptRef(ClassInstance& inst)
        {
            if(!inst)
                ClassID = 0;
            else
            {
                ClassID = inst.GetClassID();
                InstanceRef = inst._InstanceMemory; // only accessible through parent check
                ParentWeakRef = inst.ObtainParentRef();
            }
        }
        
        ~ClassInstanceScriptRef() = default; // default
        
        // gets the reference, or nullptr if is expired because of parent.
        inline U8* _GetInternal()
        {
            auto p = __GetInternal();
            return p ? p.get() : nullptr;
        }
        
        inline U32 GetClassID()
        {
            return ClassID;
        }
        
        // acquires to a normal class reference, or nullptr if expired
        inline ClassInstance Acquire()
        {
            auto pMemory = __GetInternal();
            return pMemory ? ClassInstance{ClassID, pMemory, ParentWeakRef} : ClassInstance{};
        }
        
    };
    
    // ======================================== PUBLIC META TYPES ========================================
    
    enum ClassInstanceCollectionFlags {
        _COL_KEY_SKIP_CT = 1, // skip key construct (ie pod type)
        _COL_KEY_SKIP_DT = 2, // skip key destruct
        _COL_KEY_SKIP_CP = 4, // skip key copy
        _COL_KEY_SKIP_MV = 8, // skip key move
        _COL_VAL_SKIP_CT = 16, // skip val construct
        _COL_VAL_SKIP_DT = 32, // skip val destruct
        _COL_VAL_SKIP_CP = 64, // skip val copy
        _COL_VAL_SKIP_MV = 128,// skip val move
        _COL_IS_SARRAY = 256, // is a SArray type
    };
    
    // Both dynamic and static arrays, maps and other containers all use this type internally. (DCArray/SArray/Map/Set/Queue/...)
    // This represents a collection of (optionally keyed) class instances stored in an array.
    // This is the internal type, use without underscore version (ref ptr)
    class alignas(8) ClassInstanceCollection {
    public: // ensure we are aligned to 8 bytes because all types have that align or less in the meta system.
        
        // construct with no elements, passing in meta array class. pass in the parent of this collection (the host)
        ClassInstanceCollection(ParentWeakReference host, U32 ArrayTypeIndex);
        
        ~ClassInstanceCollection(); // destructor
        
        // ===== CLASS INFORMATION GETTERS
        
        U32 GetArrayClass(); // gets the array (not the value) meta class ID
        
        U32 GetValueClass(); // gets the array value class ID
        
        U32 GetKeyClass(); // gets the array key class ID. zero means none, if non-zero this is a map.
        
        Bool IsKeyedCollection(); // returns if this is a keyed container, like Map<K,V>
        
        Bool IsStaticArray(); // returns true if this array cannot be changed in size. GetSize will remain constant
        
        // ===============================
        
        U32 GetSize(); // gets the current array size
        
        U32 GetCapacity(); // gets current array capacity
        
        void Reserve(U32 cap); // Ensure capacity is at least the argument. does not call any constructor on indices larger than GetSize().
        
        ClassInstance GetValue(U32 index); // gets the value at the given index
        
        ClassInstance GetKey(U32 index); // gets the key at the given index. must be a keyed collection!
        
        // Index must be less than size. Replaces. If copy is false, then that key or value is moved from the argument instead.
        void SetIndex(U32 index, ClassInstance key, ClassInstance value, Bool bCopyKey, Bool bCopyVal);
        
        // Pushes a new element
        // If a map, key should be non-null.
        // For any collection, value or key can be nullptr meaning a new one is constructed.
        // If key and val are either non-null, you can specify to copy(true) or move(false) using the last two arguments.
        void Push(ClassInstance key, ClassInstance val, Bool bCopyKey, Bool bCopyVal);
        
        void Clear(); // Clears the array to a size zero, clearing memory.
        
        // Pops the element at the given index. If index is bigger than or equal to GetSize(), pops top element.
        Bool Pop(U32 index, ClassInstance& keyOut, ClassInstance& valOut);
        
        // See Pop. Ignores any key, useful for non keyed types
        inline Bool PopValue(U32 index, ClassInstance& valueOut)
        {
            ClassInstance _{};
            return Pop(index, _, valueOut);
        }
        
        // For use in non keyed collections, specifies no key and calls normal push.
        inline void PushValue(ClassInstance val, Bool bCopyVal)
        {
            Push(ClassInstance{}, std::move(val), false, bCopyVal);
        }
    
    private: // copy stuff is private, only to be done by meta sys
        
        ClassInstanceCollection& operator=(const ClassInstanceCollection& rhs) = delete;// copy operators
        ClassInstanceCollection(const ClassInstanceCollection& rhs) = delete;
        
        ClassInstanceCollection& operator=(ClassInstanceCollection&& rhs) = delete;// move operators. remaining still valid for type.
        ClassInstanceCollection(ClassInstanceCollection&& rhs) = delete;
        
        ClassInstanceCollection(const ClassInstanceCollection& rhs, ParentWeakReference); // copy
        ClassInstanceCollection(ClassInstanceCollection&& rhs, ParentWeakReference); // move
        
        friend void _Impl::CtorCol(void*, U32, ParentWeakReference);
        friend void _Impl::DtorCol(void*, U32);
        friend void _Impl::CopyCol(const void*, void*, ParentWeakReference);
        friend void _Impl::MoveCol(void*, void*, ParentWeakReference);
        
        void SetIndexInternal(U32 index, ClassInstance key, ClassInstance value, Bool bCopyKey, Bool bCopyVal);
        
        ClassInstance SubRef(U32 classID, U8* pMemory); // creates ref to memory inside this collection, depends that we are alive.
        
        U32 _Size; // dynamic size of array
        U32 _Cap; // dynamic capacity of array (if SArray, not dynamic, this value is UINT32_MAX)
        
        U32 _ValuSize; // cached instance value size (size of one value element)
        U32 _PairSize; // size of key-value pair with align bytes between them
        
        U32 _ColID; // collection class ID
        U32 _ColFl; // collection flags, see above enum
        
        U8* _Memory; // allocated memory
        
        ParentWeakReference _PrntRef; // parent ref for this collection
        
    };
    
    // INTERNAL USE FOR TOOL CONTEXT ONLY
    
    void Initialise();
    void Shutdown();
    
    void InitGame();
    void RelGame();
    
    // ==================================
    
    // ================================= PUBLIC META API =================================
    
    // Get the class ID of the given type from its information. ClassIDs are ALWAYS internal and do not store to disc. Returns 0 if not found
    U32 FindClass(U64 typeHash, U32 versionNumber);
    
    // Same as find class but finds by version CRC instead of version number
    U32 FindClassByCRC(U64 typeHash, U32 versionCRC);
    
    // Same version using the string instead of the hash of the file name (lower case, use symbol)
    inline U32 FindClass(const String& typeName, U32 versionNumber)
    {
        return FindClass(Symbol(typeName).GetCRC64(), versionNumber);
    }
    
    // Writes a meta stream file to the output stream, from the instance. This can kick off async jobs, so could block while waiting to finish.
    // Pass in the name of the file you are writing, the instance to write to it, the output stream and the version of the meta stream.
    Bool WriteMetaStream(const String& name, ClassInstance instance, DataStreamRef& stream, StreamVersion StreamVersion);
    
    // Reads a meta stream file from the input stream, into return value. This can kick off async jobs, so could block while waiting to finish.
    ClassInstance ReadMetaStream(DataStreamRef& stream);
    
    // ===== CLASS FUNCTIONALITY FOR SINGLE INSTANCES ======

    // Creates an instance of the given class. Thread safe between game switches. If creating a type which belongs inside another parent
    // type (ie a non top-level) type, then pass the parent instance as the second argument.
    ClassInstance CreateInstance(U32 ClassID, ClassInstance host = {});
    
    // Creates an exact copy of the given instance. Thread safe between game switches. See CreateInstance second argument information.
    ClassInstance CopyInstance(ClassInstance instance, ClassInstance host = {});
    
    // Moves the instance argument to a new instance, leaving the old one still alive but with none of its previous data (now in new returned one).
    // Thread safe between game switches. See CreateInstance second argument information.
    ClassInstance MoveInstance(ClassInstance instance, ClassInstance host = {});
    
    // Acquires a reference to the given script object on the stack. After using ClassInstance::PushScriptRef, this can be used on the pushed value
    // Thread safe between game switches.
    ClassInstance AcquireScriptInstance(LuaManager& man, I32 stackIndex);
    
    // Returns true if the given instance's type is a collection, ie you can use CastToCollection
    // Thread safe between game switches.
    inline Bool IsCollection(ClassInstance instance)
    {
        return instance ? (_Impl::_GetClass(instance.GetClassID())->Flags & CLASS_CONTAINER) != 0 : false;
    }
    
    // Returns if the given instance type is typeName, eg 'String'
    // Thread safe between game switches.
    inline Bool Is(const ClassInstance& inst, const String& typeName)
    {
        return inst ? _Impl::_GetClass(inst.GetClassID())->TypeHash == Symbol(typeName) : false;
    }
    
    // Gets a member of a complex type, ie a meta type (type is not intrinsic, can be, but for that prefer GetMember<T>).
    // Thread safe between game switches.
    ClassInstance GetMember(ClassInstance& inst, const String& name);
    
    // Performs the less than operator '<' with the left and right hand side arguments (must be same type).
    Bool PerformLessThan(ClassInstance& lhs, ClassInstance& rhs);
    
    // Performs the equality operator '==' with the left and right hand side arguments (must be same type).
    Bool PerformEquality(ClassInstance& lhs, ClassInstance& rhs);
    
    // Performs the to string operator.
    String PerformToString(ClassInstance& inst);
    
    // Get a reference to the member in the given type instance. T must be intrinsic (String,int,...), as well as U64 (equivalent) for Symbol
    // or U32 (or equivalent) for Flags type, or the BinaryBuffer class defined above
    // Thread safe between game switches.
    template<typename T>
    T& GetMember(ClassInstance& inst, const String& name)
    {
        TTE_ASSERT(inst, "Instance is null");
        Class* pClass = _Impl::_GetClass(inst.GetClassID());
        
        for(auto& member: pClass->Members)
        {
            if(member.Name == name) // Find matching member
            {
                TTE_ASSERT(_Impl::_GetClass(member.ClassID)->Flags & CLASS_INTRINSIC || Is(inst, "class Symbol") || Is(inst, "class Flags")
                           || Is(inst, "Symbol") || Is(inst, "Flags") || Is(inst, "__INTERNAL_BINARY_BUFFER__"),
                           "GetMember<T> can only be used on intrinsic types!");
                return *((T*)(inst._GetInternal() + member.RTOffset)); // Offset in memory, skip header.
            }
        }
        TTE_ASSERT(false, "Member does not exist. HasMember should always be checked first, abort!");
        return *((T*)0); // !! abort.
    }
    
    // Returns if the given instance has a member of the given name
    // Thread safe between game switches.
    inline Bool HasMember(ClassInstance& inst, const String& name)
    {
        TTE_ASSERT(inst, "Instance is null");
        Class* pClass = _Impl::_GetClass(inst.GetClassID());
        
        for(auto& member: pClass->Members)
        {
            if(member.Name == name) // Find matching member
            {
                return true;
            }
        }
        return false;
    }
    
    // If the given instance class is a collection, this returns the modifyable collection for it. DO NOT access this after arrayType is not alive.
    // Thread safe between game switches.
    inline ClassInstanceCollection& CastToCollection(ClassInstance& arrayType){
        TTE_ASSERT(arrayType, "Cannot cast to collection: array type is null");
        TTE_ASSERT(_Impl::_GetClass(arrayType.GetClassID())->Flags & CLASS_CONTAINER, "Cannot cast class to collection: it is not a collection");
        return *(ClassInstanceCollection*)arrayType._GetInternal();
    }
    
    // ====================================================================================
    
}

// FOR SERIALISERS BELOW, CLAZZ CAN BE NULL, as we know the class 100%.

// Serialises an unsigned byte (can be cast to signed)
inline Bool SerialiseU8(Meta::Stream& stream, Meta::ClassInstance&, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    return IsWrite ? stream.Write(const_cast<const U8*>((U8*)pMemory), 1) : stream.Read((U8*)pMemory, 1);
}

// Serialises an unsigned short (can be cast to signed)
inline Bool SerialiseU16(Meta::Stream& stream, Meta::ClassInstance&, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8*>((U8*)pMemory), 2) : stream.Read((U8*)pMemory, 2);
}

// Serialises an unsigned int (can be cast to signed)
inline Bool SerialiseU32(Meta::Stream& stream, Meta::ClassInstance&, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8*>((U8*)pMemory), 4) : stream.Read((U8*)pMemory, 4);
}

// Serialises an unsigned longlong (can be cast to signed)
inline Bool SerialiseU64(Meta::Stream& stream, Meta::ClassInstance&, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8*>((U8*)pMemory), 8) : stream.Read((U8*)pMemory, 8);
}

// Serialises an unsigned int (can be cast to signed) FROM/TO A DATA STREAM not a meta stream (used in header)
inline Bool SerialiseDataU32(DataStreamRef& stream, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    // Endianness checks in the future?
    return IsWrite ? stream->Write(const_cast<const U8*>((U8*)pMemory), 4) : stream->Read((U8*)pMemory, 4);
}

// Serialises an unsigned longlong (can be cast to signed) FROM/TO A DATA STREAM not a meta stream (used in header)
inline Bool SerialiseDataU64(DataStreamRef& stream, Meta::Class* clazz, void* pMemory, Bool IsWrite){
    // Endianness checks in the future?
    return IsWrite ? stream->Write(const_cast<const U8*>((U8*)pMemory), 8) : stream->Read((U8*)pMemory, 8);
}
