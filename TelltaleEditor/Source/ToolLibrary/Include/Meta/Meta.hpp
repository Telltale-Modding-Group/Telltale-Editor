#pragma once

#include <Core/Config.hpp>
#include <Resource/DataStream.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Scripting/LuaManager.hpp>
#include <set>
#include <vector>

// Meta system is all inside this namespace. This is a reflection system initialised by the lua scripts.
// ClassID is the combined hash of the type name and version crc of that type.
namespace Meta
{

// ======================================== META BINARY TYPES ========================================

// Sections of binary stream
enum StreamSection
{
    STREAM_SECTION_MAIN = 0,
    STREAM_SECTION_ASYNC = 1,
    STREAM_SECTION_DEBUG = 2,
    STREAM_SECTION_COUNT = 3,
};

// Binary stream versions
enum StreamVersion
{
    MBIN = 0, // Meta BINary
    MBES = 1, // Meta Binary Encrypted Stream. This is not a valid version to write with, encrypt archives if needed.
    MTRE = 2, // Meta Type References
    MCOM = 3, // Meta Compressed
    MSV4 = 4, // Meta Stream Version 4 (unused)
    MSV5 = 5, // Meta Stream Version 5
    MSV6 = 6, // Meta Stream Version 6
};

// A binary meta stream. Used internally.
struct Stream
{

    struct Section
    {
        DataStreamRef Data;      // Section binary data stream
        Bool Compressed;         // If section is compressed
        std::vector<U64> Blocks; // For blocks. In read, stores the sizes, in write stores the block offset initial.
    };

    StreamVersion Version;
    Section Sect[STREAM_SECTION_COUNT];
    std::vector<U32> VersionInf; // vector of class IDs

    StreamSection CurrentSection = STREAM_SECTION_MAIN;

    inline Bool Write(const U8 *Buffer, U64 BufferLength) { return Sect[CurrentSection].Data->Write(Buffer, BufferLength); }

    inline Bool Read(U8 *Buffer, U64 BufferLength) { return Sect[CurrentSection].Data->Read(Buffer, BufferLength); }
};

// ======================================== INTERNAL META TYPES ========================================

enum MetaMemberFlag
{
    MEMBER_GHOST = 1, // this member is not actually in memory but just exists for the version hashing
    MEMBER_ENUM = 2,  // this member is an enum
    MEMBER_FLAG = 4,  // this member is a flags bitfield
    MEMBER_BASE = 8,  // this member is a base class
    MEMBER_SKIP = 16, // skip serialisation for this member
};

enum MetaClassFlag
{
    CLASS_INTRINSIC = 1,    // this class is an intrinsic type so is not added to meta class header
    CLASS_ALLOW_ASYNC = 2,  // allow this class to be serialised asynchronously, ie its a very large class.
    CLASS_CONTAINER = 4,    // this class is a container type (map/set/array/etc)
    CLASS_ABSTRACT = 8,     // this class is abstract (Baseclass_ prefix for ex.) so instances of it cannot be created.
    CLASS_NON_BLOCKED = 16, // this class is not blocked in serialisation
};

// A type member
struct Member
{

    String Name;
    U32 Flags;
    U32 ClassID; // member type

    U32 RTOffset = 0; // runtime offset in memory
};

// A type class. This as well as Member are used internally. Refer to classes using the index (U32 - internal version CRC).
// Refer to members by name string
struct Class
{

    // MEMBERS REQUIRED.
    String Name = "";
    U32 Flags = 0;

    // CREATED INTERNALLY.
    U64 TypeHash = 0;

    union
    {
        U32 ClassID;
        U32 VersionCRC; // synonymous
    };

    // USE FOR CONTAINER TYPES
    U32 ArraySize = 0;     // if this is an SArray<T,N>, this is N. Value only set of SArrays
    U32 ArrayKeyClass = 0; // if this is a collection, this is the key value class
    U32 ArrayValClass = 0; // if this is a collection, this ia the value class

    // RUNTIME DATA, INTERNAL.
    U32 RTSize = 0; // runtime internal size of the class, automatically generated unless intrinsic.

    // C++ IMPL FOR INTRINSICS/CONTAINERS/NON-POD
    void (*Constructor)(void *pMemory, U32 ClassID) = nullptr;
    void (*Destructor)(void *pMemory, U32 ClassID) = nullptr;
    void (*CopyConstruct)(const void *pSrc, void *pDst) = nullptr;
    void (*MoveConstruct)(void *pSrc, void *pDst) = nullptr;

    // serialiser (needed for intrinsics/containers) function. iswrite if write, else reading.
    Bool (*Serialise)(Stream &stream, Class *clazz, void *pInstance, Bool IsWrite) = nullptr;
    String SerialiseScriptFunction = ""; // custom serialise overrider, function name in lua script.

    // MEMBERS ARRAY
    std::vector<Member> Members;
};

class ClassInstance;

// Internal implementation
namespace _Impl
{

Class *_GetClass(U32 i); // gets class ptr from index. MUST exist.

U32 _Register(LuaManager &man, Class &&c, I32 classTableStackIndex); // register new class and calc CRCs

U32 _DoLuaVersionCRC(LuaManager &man, I32 classTableStackIndex); // calculate version crc32

void _DoConstruct(Class *pClass, U8 *pMemory); // internally construct type into memory

void _DoDestruct(Class *pClass, U8 *pMemory); // internally call destruct

void _DoCopyConstruct(Class *pClass, U8 *pDst, const U8 *pSrc); // internally copy type

void _DoMoveConstruct(Class *pClass, U8 *pDst, U8 *pSrc); // internally move type

ClassInstance _MakeInstance(U32 ClassID); // allocates but does not construct anything in the memory

Bool _DoSerialise(Stream &stream, Class *clazz, void *pMemory, Bool IsWrite); // serialises a given type to the stream

} // namespace _Impl

// ======================================== PUBLIC META TYPES ========================================

class ClassInstance
{

    friend class ClassInstanceCollection;

    friend class ClassInstanceScriptRef;

    friend ClassInstance AcquireScriptInstance(LuaManager &man, I32 stackIndex);

    friend ClassInstance _Impl::_MakeInstance(U32);

  public:
    // Default move and copy stuff
    ClassInstance(const ClassInstance &) = default;
    ClassInstance(ClassInstance &&) = default;
    ClassInstance &operator=(const ClassInstance &) = default;
    ClassInstance &operator=(ClassInstance &&) = default;

    // Default constructor refers to no instance in memory.
    inline ClassInstance() : _InstanceClassID(0), _InstanceMemory(nullptr) {}

    inline operator Bool() const
    {
        return _InstanceClassID && (_InstanceMemory ? true : false); // call bool operator on ptr
    }

    // Internal use. Returns the deleter
    inline void (*_GetDeleter())(U8 *)
    {
        if (!_InstanceMemory)
            return nullptr;
        auto Dx = std::get_deleter<void (*)(U8 *)>(_InstanceMemory);
        return Dx ? *Dx : nullptr;
    }

    // Pushes a weak reference to this instance to the lua stack.
    // Strong: means instance is deleted only when all C++ instance AND script instance objects are destroyed/garbage collected.
    // Weak:   means instance is not owned by the script reference. If all C++ instances are destroyed, getting the object returns nil.
    void PushScriptRef(LuaManager &man, Bool IsStrong);

    // Internal use. Gets the memory pointer (const)
    inline const U8 *_GetInternal() const { return _InstanceMemory.get(); }

    // Internal use. Gets the memory pointer
    inline U8 *_GetInternal() { return _InstanceMemory.get(); }

    // Gets the class ID
    inline U32 GetClassID() const { return _InstanceClassID; }

  private:
    // Use by _MakeInstance. Deleted is defined in the meta.cpp TU.
    inline ClassInstance(U32 storedID, U8 *memory, std::function<void(U8 *)> _deleter) : _InstanceClassID(storedID), _InstanceMemory(memory, _deleter)
    {
    }

    // Use by the collection class. takes in the class and memory, but the memory won't be deleted
    inline ClassInstance(U32 storedID, U8 *memoryNoDelete) : _InstanceClassID(storedID), _InstanceMemory(memoryNoDelete, &NullDeleter) {}

    U32 _InstanceClassID;                // class id
    std::shared_ptr<U8> _InstanceMemory; // memory pointer to instance in memory
};

// Weak or strong reference to a meta class instance, used internally. Used for letting lua scripts access class instances
class ClassInstanceScriptRef
{

    U32 ClassID;
    std::weak_ptr<U8> WeakRef;
    std::shared_ptr<U8> StrongRef;

  public:
    inline ClassInstanceScriptRef(ClassInstance &inst, Bool Strong) : StrongRef(), WeakRef()
    {
        if (!inst)
            ClassID = 0;
        else
        {
            ClassID = inst.GetClassID();
            if (Strong)
                StrongRef = inst._InstanceMemory;
            else
                WeakRef = inst._InstanceMemory;
        }
    }

    ~ClassInstanceScriptRef() = default; // default

    inline U8 *_GetInternal()
    {
        if (StrongRef)
            return StrongRef.get();
        auto val = WeakRef.lock();
        return val ? val.get() : nullptr;
    }

    inline U32 GetClassID() { return ClassID; }
};

// ======================================== PUBLIC META TYPES ========================================

enum ClassInstanceCollectionFlags
{
    _COL_KEY_SKIP_CT = 1,   // skip key construct (ie pod type)
    _COL_KEY_SKIP_DT = 2,   // skip key destruct
    _COL_KEY_SKIP_CP = 4,   // skip key copy
    _COL_KEY_SKIP_MV = 8,   // skip key move
    _COL_VAL_SKIP_CT = 16,  // skip val construct
    _COL_VAL_SKIP_DT = 32,  // skip val destruct
    _COL_VAL_SKIP_CP = 64,  // skip val copy
    _COL_VAL_SKIP_MV = 128, // skip val move
};

// Both dynamic and static arrays, maps and other containers all use this type internally. (DCArray/SArray/Map/Set/Queue/...)
// This represents a collection of (optionally keyed) class instances stored in an array.
// This is the internal type, use without underscore version (ref ptr)
class alignas(8) ClassInstanceCollection
{
  public:                                        // ensure we are aligned to 8 bytes because all types have that align or less in the meta system.
    ClassInstanceCollection(U32 ArrayTypeIndex); // construct with no elements, passing in meta array class

    ~ClassInstanceCollection(); // destructor

    ClassInstanceCollection &operator=(const ClassInstanceCollection &rhs); // copy operators
    ClassInstanceCollection(const ClassInstanceCollection &rhs);

    ClassInstanceCollection &operator=(ClassInstanceCollection &&rhs); // move operators. remaining still valid for type.
    ClassInstanceCollection(ClassInstanceCollection &&rhs);

    // ===== CLASS INFORMATION GETTERS

    U32 GetArrayClass(); // gets the array (not the value) meta class ID

    U32 GetValueClass(); // gets the array value class ID

    U32 GetKeyClass(); // gets the array key class ID. zero means none, if non-zero this is a map.

    Bool IsKeyedCollection(); // returns if this is a keyed container, like Map<K,V>

    // ===============================

    U32 GetSize(); // gets the current array size

    U32 GetCapacity(); // gets current array capacity

    void Reserve(U32 cap); // Ensure capacity is at least the argument. does not call any constructor on indices larger than GetSize().

    ClassInstance GetValue(U32 index); // gets the value at the given index

    ClassInstance GetKey(U32 index); // gets the key at the given index. must be a keyed collection!

    // Pushes a new element
    // If a map, key must be non-null.
    // For any collection, value or key can be nullptr meaning a new one is constructed.
    // If key and val are either non-null, you can specify to copy(true) or move(false) using the last two arguments.
    void Push(ClassInstance key, ClassInstance val, Bool bCopyKey, Bool bCopyVal);

    void Clear(); // Clears the array to a size zero, clearing memory.

    // Pops the element at the given index. If index is bigger than or equal to GetSize(), pops top element.
    Bool Pop(U32 index, ClassInstance &keyOut, ClassInstance &valOut);

    // See Pop. Ignores any key, useful for non keyed types
    inline Bool PopValue(U32 index, ClassInstance &valueOut)
    {
        ClassInstance _{};
        return Pop(index, _, valueOut);
    }

    // For use in non keyed collections, specifies no key and calls normal push.
    inline void PushValue(ClassInstance val, Bool bCopyVal) { Push(ClassInstance{}, std::move(val), false, bCopyVal); }

  private:
    U32 _Size; // dynamic size of array
    U32 _Cap;  // dynamic capacity of array (if SArray, not dynamic, this value is UINT32_MAX)

    U32 _ValuSize; // cached instance value size (size of one value element)
    U32 _PairSize; // size of key-value pair with align bytes between them

    U32 _ColID; // collection class ID
    U32 _ColFl; // collection flags, see above enum

    U8 *_Memory; // allocated memory
};

// INTERNAL USE FOR TOOL CONTEXT ONLY

void Initialise();
void Shutdown();

void InitGame();
void RelGame();

// ==================================

// ================================= PUBLIC META API =================================

// Get the class ID of the given type from its information. ClassIDs are ALWAYS internal and do not store to disc. Returns 0 if not found
U32 FindClassID(U64 typeHash, U32 versionCRC);

// Same version using the string instead of the hash of the file name (lower case, use symbol)
inline U32 FindClassID(const String &typeName, U32 versionCRC) { return FindClassID(Symbol(typeName).GetCRC64(), versionCRC); }

// Writes a meta stream file to the output stream, from the instance. This can kick off async jobs, so could block while waiting to finish.
// Pass in the name of the file you are writing, the instance to write to it, the output stream and the version of the meta stream.
Bool WriteMetaStream(const String &name, ClassInstance instance, DataStreamRef &stream, StreamVersion StreamVersion);

// Reads a meta stream file from the input stream, into return value. This can kick off async jobs, so could block while waiting to finish.
ClassInstance ReadMetaStream(DataStreamRef &stream);

// ===== CLASS FUNCTIONALITY FOR SINGLE INSTANCES ======

// Creates an instance of the given class
ClassInstance CreateInstance(U32 ClassID);

// Creates an exact copy of the given instance
ClassInstance CopyInstance(ClassInstance instance);

// Moves the instance argument to a new instance, leaving the old one still alive but with none of its previous data (now in new returned one).
ClassInstance MoveInstance(ClassInstance instance);

// Acquires a reference to the given script object on the stack.
ClassInstance AcquireScriptInstance(LuaManager &man, I32 stackIndex);

// Returns true if the given instance's type is a collection, ie you can use CastToCollection
inline Bool IsCollection(ClassInstance instance)
{
    return instance ? (_Impl::_GetClass(instance.GetClassID())->Flags & CLASS_CONTAINER) != 0 : false;
}

// Returns if the given instance type is typeName, eg 'String'
inline Bool Is(const ClassInstance &inst, const String &typeName)
{
    return inst ? _Impl::_GetClass(inst.GetClassID())->TypeHash == Symbol(typeName) : false;
}

// Gets a member of a complex type, ie a meta type (type is not intrinsic, can be, but for that prefer GetMember<T>).
ClassInstance GetMember(ClassInstance &inst, const String &name);

// Get a reference to the member in the given type instance. T must be intrinsic (String,int,...), as well as U64 (equivalent) for Symbol
// or U32 (or equivalent) for Flags type
template <typename T> T &GetMember(ClassInstance &inst, const String &name)
{
    TTE_ASSERT(inst, "Instance is null");
    Class *pClass = _Impl::_GetClass(inst.GetClassID());

    for (auto &member : pClass->Members)
    {
        if (member.Name == name) // Find matching member
        {
            TTE_ASSERT(_Impl::_GetClass(member.ClassID)->Flags & CLASS_INTRINSIC || Is(inst, "Symbol") || Is(inst, "Flags"),
                       "GetMember<T> can only be used on intrinsic types or Symbol or Flags types!");
            return *((T *)(inst._GetInternal() + member.RTOffset)); // Offset in memory, skip header.
        }
    }
    TTE_ASSERT(false, "Member does not exist. HasMember should always be checked first, abort!");
    return *((T *)0); // !! abort.
}

// Returns if the given instance has a member of the given name
inline Bool HasMember(ClassInstance &inst, const String &name)
{
    TTE_ASSERT(inst, "Instance is null");
    Class *pClass = _Impl::_GetClass(inst.GetClassID());

    for (auto &member : pClass->Members)
    {
        if (member.Name == name) // Find matching member
        {
            return true;
        }
    }
    return false;
}

// If the given instance class is a collection, this returns the modifyable collection for it. DO NOT access this after arrayType is not alive.
inline ClassInstanceCollection &CastToCollection(ClassInstance &arrayType)
{
    TTE_ASSERT(arrayType, "Cannot cast to collection: array type is null");
    TTE_ASSERT(_Impl::_GetClass(arrayType.GetClassID())->Flags & CLASS_CONTAINER, "Cannot cast class to collection: it is not a collection");
    return *(ClassInstanceCollection *)arrayType._GetInternal();
}

// ====================================================================================

} // namespace Meta

// FOR SERIALISERS BELOW, CLAZZ CAN BE NULL, as we know the class 100%.

// Serialises an unsigned byte (can be cast to signed)
inline Bool SerialiseU8(Meta::Stream &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    return IsWrite ? stream.Write(const_cast<const U8 *>((U8 *)pMemory), 1) : stream.Read((U8 *)pMemory, 1);
}

// Serialises an unsigned short (can be cast to signed)
inline Bool SerialiseU16(Meta::Stream &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8 *>((U8 *)pMemory), 2) : stream.Read((U8 *)pMemory, 2);
}

// Serialises an unsigned int (can be cast to signed)
inline Bool SerialiseU32(Meta::Stream &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8 *>((U8 *)pMemory), 4) : stream.Read((U8 *)pMemory, 4);
}

// Serialises an unsigned longlong (can be cast to signed)
inline Bool SerialiseU64(Meta::Stream &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    // Endianness checks in the future?
    return IsWrite ? stream.Write(const_cast<const U8 *>((U8 *)pMemory), 8) : stream.Read((U8 *)pMemory, 8);
}

// Serialises an unsigned int (can be cast to signed) FROM/TO A DATA STREAM not a meta stream (used in header)
inline Bool SerialiseDataU32(DataStreamRef &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    // Endianness checks in the future?
    return IsWrite ? stream->Write(const_cast<const U8 *>((U8 *)pMemory), 4) : stream->Read((U8 *)pMemory, 4);
}

// Serialises an unsigned longlong (can be cast to signed) FROM/TO A DATA STREAM not a meta stream (used in header)
inline Bool SerialiseDataU64(DataStreamRef &stream, Meta::Class *clazz, void *pMemory, Bool IsWrite)
{
    // Endianness checks in the future?
    return IsWrite ? stream->Write(const_cast<const U8 *>((U8 *)pMemory), 8) : stream->Read((U8 *)pMemory, 8);
}
