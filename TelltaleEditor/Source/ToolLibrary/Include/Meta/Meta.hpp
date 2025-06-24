#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>
#include <Resource/DataStream.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Core/BitSet.hpp>
#include <Core/Math.hpp>
#include <Core/GameCaps.hpp>

#include <climits>
#include <vector>
#include <set>
#include <sstream>
#include <functional>
#include <map>

// ======================================== PRE DECLARATIONS ========================================

class PropertySet;
class FunctionBase;
enum class CommonClass;

// ======================================= META UTILITY TYPES =======================================

template<typename T>
struct TRect
{
    T top, right, bottom, left;
};

template<typename T>
struct TRange
{
    T min, max;
};

// Telltale helper class used a lot! Exclusive OR: either a chore or animation handle
struct AnimOrChore
{
    String HandleAnim;
    String HandleChore;
};

template class TRect<Float>;
template class TRange<Float>;
template class TRange<U32>;
using Rect = TRect<I32>;

// maximum number of versions of a given typename (eg int) with different version CRCs allowed (normally theres only 1 so any more is not likely)
#define MAX_VERSION_NUMBER 10

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
    
    constexpr CString StreamSectionName[]
    {
        "Main",
        "Async",
        "Debug"
    };
    
    constexpr CString PlatformNames[]
    {
        "PC",
        "MacOS",
        "PS2",
        "PS3",
        "PS4",
        "XBOne",
        "XB360",
        "Linux",
        "NX", // Switch
        "WiiU",
        "iPhone",
        "Android",
        "Vita"
    };
    
    // Binary stream versions
    enum StreamVersion
    {
        MBIN = 0, // Meta BINary
        MBES = 1, // Meta Binary Encrypted Stream. This is not a valid version to write with, encrypt archives if needed.
        MTRE = 2, // Meta Type References
        MCOM = 3, // Meta Compressed
        MSV4 = 4, // Meta Stream Version 4 (unused)
        MSV5 = 5, // 5
        MSV6 = 6, // 6
        BMS3 = 7, // Legacy CSI3/PS2. Binary Meta Stream 3
        EMS3 = 8, // Legacy CSI3/PS2. Haven't seen any files use it. Encrypted Meta Stream 3
        UNSPECIFIED = 9,
    };
    
    // A binary meta stream. Used internally.
    struct Stream
    {
        
        struct Section
        {
            DataStreamRef Data; // Section binary data stream
            Bool Compressed; // If section is compressed
            std::vector<U64> Blocks; // For blocks. In read, stores the sizes, in write stores the block offset initial.
        };
        
        String Name; // file name
        StreamVersion Version;
        Section Sect[STREAM_SECTION_COUNT];
        std::vector<U32> VersionInf; // vector of class IDs
        
        // optional debug of reads. when reading if set outputs JSON-like
        // IF THESE ARE PRESENT, NO ASYNC STUFF!
        DataStreamRef DebugOutputFile;
        std::stringstream DebugOutput;
        U32 TabDepth = 0; // debug tab depth
        U32 MaxInlinableBuffer = UINT32_MAX;
        
        StreamSection CurrentSection = STREAM_SECTION_MAIN;
        
        inline Bool Write(const U8* Buffer, U64 BufferLength)
        {
            return Sect[CurrentSection].Data->Write(Buffer, BufferLength);
        }
        
        inline Bool Read(U8* Buffer, U64 BufferLength)
        {
            return Sect[CurrentSection].Data->Read(Buffer, BufferLength);
        }
        
        inline void WriteTabs()
        {
            for(U32 i = 0; i < TabDepth; i++)
                DebugOutput << "    ";
        }
        
    };
    
    // ======================================== INTERNAL META TYPES ========================================
    
    enum MetaMemberFlag
    {
        MEMBER_MEMORY_DISABLE = 1, // this member is not actually in memory but just exists for the version hashing
        MEMBER_ENUM = 2, // this member is an enum
        MEMBER_FLAG = 4, // this member is a flags bitfield
        MEMBER_BASE = 8, // this member is a base class
        MEMBER_SERIALISE_DISABLE = 16, // skip serialisation for this member. this also excludes this member from version hash
        MEMBER_VERSION_HASH_DISABLE = 32, // dont include in any version hashes, although is still in memory and serialised
    };
    
    enum MetaClassFlag
    {
        CLASS_INTRINSIC = 1, // this class is an intrinsic type so is not added to meta class header
        CLASS_ALLOW_ASYNC = 2, // allow this class to be serialised asynchronously, ie its a very large class.
        CLASS_CONTAINER = 4, // this class is a container type (map/set/array/etc)
        CLASS_ABSTRACT = 8, // this class is abstract (Baseclass_ prefix for ex.) so instances of it cannot be created.
        CLASS_NON_BLOCKED = 16, // this class is not blocked in serialisation
        CLASS_ATTACHABLE = 32, // can have children attached to it, used in PropertySet and other big complex types
        CLASS_PROXY = 64, // proxy type which is used for telltale game errors. disables all block sizes in members of this type.
        CLASS_ENUM_WRAPPER = 128, // enum class wrapper. has one member integer (normally mVal)
        _CLASS_PROP = 256, // Internal flag denoting this class is the property set class (undergoes specific treatment in resource API)
    };
    
    // Enum / flag descriptor for a member in a class
    struct EnumFlag
    {
        
        String Name; // Name of this flag / enum
        I32 Value; // enum / flag value
        
    };
    
    // A type member
    struct Member {
        
        String Name;
        U32 Flags;
        U32 ClassID; // member type
        
        std::vector<EnumFlag> Descriptors{};
        
        U32 RTOffset = 0; // runtime offset in memory
        
    };
    
    class ClassInstance;
    struct RegGame;
    
    // Weak reference to the parent. Deep into member trees, these point to the top level class, eg: array of materials , top level is D3DMesh
    using ParentWeakReference = WeakPtr<U8>;
    
    // A type class. This as well as Member are used internally. Refer to classes using the index (U32 - internal version CRC).
    // Refer to members by name string
    struct Class
    {
        
        // MEMBERS REQUIRED.
        String Name = "";
        U32 Flags = 0;
        
        // CREATED INTERNALLY.
        U64 TypeHash = 0;
        U64 ToolTypeHash = 0; // without specifiers.
        
        String Extension; // file extension if this class is a full file
        
        U32 ClassID;
        U32 VersionCRC;
        U32 VersionNumber; // similar to version crc, but a number (normally 0, 1 etc). Associate a ID for multiple verisons of the same typename.
        U32 LegacyHash; // (CSI3 PS2) hash. simple loop of bit shifting
        
        // USE FOR CONTAINER TYPES
        U32 ArraySize = 0; // if this is an SArray<T,N>, this is N. Value only set of SArrays
        U32 ArrayKeyClass = 0; // if this is a collection, this is the key value class
        U32 ArrayValClass = 0; // if this is a collection, this ia the value class
        
        // RUNTIME DATA, INTERNAL.
        U32 RTSize = 0; // runtime internal size of the class, automatically generated unless intrinsic.
        U32 RTPaddingChildren = 0, RTPaddingCallbacks = 0; // to be able to reverse the padding
        U32 RTSizeRaw; // size without any children array or prop stuff
        
        // C++ IMPL FOR INTRINSICS/CONTAINERS/NON-POD
        void (*Constructor)(void* pMemory, U32 ClassID, ParentWeakReference host) = nullptr;
        void (*Destructor)(void* pMemory, U32 ClassID) = nullptr;
        void (*CopyConstruct)(const void* pSrc, void* pDst, ParentWeakReference host) = nullptr;
        void (*MoveConstruct)(void* pSrc, void* pDst, ParentWeakReference host) = nullptr;
        
        // OTHER OPERATIONS (MAINLY FOR INTRINSICS)
        Bool (*LessThan)(const void* pLHS, const void* pRHS) = nullptr; // less than operator on two instances
        Bool (*Equals)(const void* pLHS, const void* pRHS) = nullptr; // compare two instances
        String (*ToString)(Class* pClass, const void* pMemory) = nullptr; // converts to string
        
        // serialiser (needed for intrinsics/containers) function. iswrite if write, else reading.
        Bool (*Serialise)(Stream& stream, ClassInstance& host, Class* clazz, void* pInstance, Bool IsWrite) = nullptr;
        String SerialiseScriptFn = ""; // custom serialise overrider, function name in scripts
        
        String NormaliserStringFn = ""; // normalisation function
        String SpecialiserStringFn = ""; // specialiser function
        
        // MEMBERS ARRAY
        std::vector<Member> Members;
        
    };
    
    // compilsed serialisation script
    struct CompiledScript
    {
        U8* Binary = nullptr;
        U32 Size = 0;
    };
    
    // Internal implementation
    namespace _Impl {
        
        String _EnumFlagMemberToString(Member& member, const void* pVal);
        
        String _EnumFlagToString(Class* pClass, const void* pVal);
        
        Bool _CheckPlatformForGame(RegGame&, const String& platform);
        
        Bool _CheckVendorForGame(RegGame&, const String& platform);
        
        Bool _CheckPlatform(const String& platform);
        
        U32 _PerformLegacyClassHash(const String& name);
        
        Class* _GetClass(U32 i); // gets class ptr from index. MUST exist.
        
        U32 _ClassChildrenArrayOff(Class& clazz); // internal children refs vector offset
        
        U32 _ClassPropertyCallbacksArrayOff(Class& clazz);
        
        U32 _Register(LuaManager& man, Class&& c, I32 classTableStackIndex); // register new class and calc CRCs
        
        U32 _DoLuaVersionCRC(LuaManager& man, I32 classTableStackIndex); // calculate version crc32

        void _PushCompiledScript(std::map<Symbol, CompiledScript>& scriptMap, const String& fn);

        Bool _CollectCompiledScriptFunctionMT(CompiledScript& out, const String& fn);

        U32 _ResolveCommonClassID(const String& extension);

        void _FreeCompiledScriptMT(CompiledScript& script); // main only

        U32 _ResolveCommonClassIDSafe(CommonClass clz); // will use common selector. thread safe
        
        // internally construct type into memory. concrete is actual ref if its a top level so members have it as its parent
        void _DoConstruct(Class* pClass, U8* pMemory, ParentWeakReference host, ParentWeakReference& concrete, Bool bTopLevel);
        
        void _DoDestruct(Class* pClass, U8* pMemory, Bool bTopLevel); // internally call destruct
        
        // internally copy type
        void _DoCopyConstruct(Class* pClass, U8* pDst, const U8* pSrc, ParentWeakReference host, ParentWeakReference& concrete, Bool bSrcTopLevel, Bool bTopLevel);
        
        // internally move type
        void _DoMoveConstruct(Class* pClass, U8* pDst, U8* pSrc, ParentWeakReference host, ParentWeakReference& concrete, Bool bSrcTopLevel, Bool bTopLevel);
        
        String _PerformToString(U8* pMemory, Class* pClass);
        
        // for c++ controlled: host is empty. if script object: host MUST be a reference to a higher level parent.
        // if host argument is attachable, name can be specified and it will be appended to the child list for host
        // allocates but does not construct anything in the memory
        ClassInstance _MakeInstance(U32 ClassID, ClassInstance& host, Symbol name, U8* pAlloc = nullptr, U32 allocSize = 0);
        
        // some serialisers have 'host' argument: top level class object
        Bool _Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite, CString memberName, Member* hostMember = nullptr);
        // serialises a given type to the stream
        
        Bool _DefaultSerialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite, CString memberName, Member* hostMember = nullptr);
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
        
        void CtorDSCache(void* pMemory, U32 Array, ParentWeakReference host);
        
        void DtorDSCache(void* pMemory, U32);
        
        void CopyDSCache(const void* pSrc, void* pDst, ParentWeakReference host);
        
        void MoveDSCache(void* pSrc, void* pDst, ParentWeakReference host);
        
    }
    
    // Special type internally used to store binary buffers. No serialiser in this type, but just holds a reference to the memory (frees it)
    struct BinaryBuffer
    {
        Ptr<U8> BufferData;
        U32 BufferSize = 0;
    };
    
    // Special type internal used to store reference to a data stream. This is used mainly for large cached files which we dont want to load into memory which are used in meta streams.
    struct DataStreamCache
    {
        DataStreamRef Stream;
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
        
        enum GameFlags
        {
            MODIFIED_BLOWFISH = 1, // new games use a modified blowfish version
            ARCHIVE2 = 2,  // if the game uses .ttarch2 instead of .ttarch
            ENABLE_OODLE = 4, // some games are not shipped with oodle, so keep this safe. If they are, this is true.
            LEGACY_HASHING = 8, // legacy hashing for select old games
        };
        
        struct FolderAssociateComparator
        {
            
            inline Bool operator()(const String& maskLHS, const String& maskRHS) const
            {
                size_t cLeft = std::count(maskLHS.begin(), maskLHS.end(), '/');
                size_t cRight = std::count(maskRHS.begin(), maskRHS.end(), '/');
                
                if (cLeft != cRight)
                    return cLeft < cRight;
                
                if (maskLHS.length() != maskRHS.length())
                    return maskLHS.length() > maskRHS.length();
                
                size_t wildcardsLHS = std::count(maskLHS.begin(), maskLHS.end(), '*') + std::count(maskLHS.begin(), maskLHS.end(), '?');
                size_t wildcardsRHS = std::count(maskRHS.begin(), maskRHS.end(), '*') + std::count(maskRHS.begin(), maskRHS.end(), '?');
                
                return wildcardsLHS < wildcardsRHS;
            }
            
        };
        
        String Name, ID, ResourceSetDescMask;
        StreamVersion MetaVersion = MBIN;
        LuaVersion LVersion = LuaVersion::LUA_5_2_3;
        std::multimap<String, String, FolderAssociateComparator> FolderAssociates; // mask to folder name, eg '*.dlg' into Dialogs/, and 'module_*.prop' into Properties/Primitives/, '*.prop' => Properties/
        std::vector<String> ValidPlatforms; // game platforms
        String DefaultVendor; // if more than 0 vendors, this specifies the default one to map from an empty string to.
        std::vector<String> ValidVendors; // if non zero it must be specified. eg 'DevBuild' for early dev releases etc. see script. empty string always allowed as well
        Flags Fl; // flags
        String CommonSelector;
        std::map<U64, std::pair<String, String>> ExecutableHash; // exe hash => platform + vendor pair

        // These are snapshot => XX mappings. snapshot is not the game ID, but the 'Platform/Vendor' or just 'Platform' (substitute)

        BlowfishKey MasterKey; // key used for all platforms
        U32 MasterArchiveVersion = 0;  // archive version for old ttarch. for new ttarch2, this is the TTAX (X value) so 2,3 or 4.
        std::map<String, U32> SnapToArchiveVersion;
        std::map<String, BlowfishKey> SnapToEncryptionKey;

        GameCapabilitiesBitSet Caps;
        
        inline Bool UsesArchive2() const
        {
            return Fl.Test(ARCHIVE2);
        }

        U32 GetArchiveVersion(GameSnapshot snapshot) const;

        BlowfishKey GetEncryptionKey(GameSnapshot snapshot) const;
        
    };
    
    using ClassChildMap = std::map<Symbol, ClassInstance>;
    
    // ======================================== PUBLIC META TYPES ========================================
    
    class ClassInstance {
        
        // FRIENDS FOR ACCESSING PRIVATE FUNCTIONALITY
        
        friend class ::PropertySet;
        
        friend class ClassInstanceCollection;
        
        friend class ClassInstanceScriptRef;
        
        friend ClassInstance GetMember(ClassInstance& inst, const String& name, Bool bInsist);
        
        friend ClassInstance _Impl::_MakeInstance(U32, ClassInstance&, Symbol, U8*, U32);
        
        friend Bool _Impl::_Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite, CString member, Member*);
        
        friend ClassInstance CreateInstance(U32 ClassID, ClassInstance host, Symbol n);
        
        friend ClassInstance CreateTemporaryInstance(U8* Alloc, U32 AlZ, U32 ClassID);
        
        friend ClassInstance CopyInstance(ClassInstance instance, ClassInstance host, Symbol n);
        
        friend ClassInstance MoveInstance(ClassInstance instance, ClassInstance host, Symbol n);
        
        friend void _Impl::_DoCopyConstruct(Class* pClass, U8* pDst, const U8* pSrc, ParentWeakReference host,
                                     ParentWeakReference& concrete, Bool bSrcTopLevel, Bool bTopLevel);
        
        friend void _Impl::_DoMoveConstruct(Class* pClass, U8* pDst, U8* pSrc, ParentWeakReference host, ParentWeakReference& concrete,
                                     Bool bSrcTopLevel, Bool bTopLevel);
        
    public:
        
        // Default move and copy stuff
        ClassInstance(const ClassInstance&) = default;
        ClassInstance(ClassInstance&&) = default;
        ClassInstance& operator=(const ClassInstance&) = default;
        ClassInstance& operator=(ClassInstance&&) = default;
        
        // Default constructor refers to no instance in memory.
        inline ClassInstance() : _InstanceClassID(0), _InstanceMemory(nullptr) {}
        
        // returns if the instance is valid, if the parent (if has one) is valid
        inline operator Bool() const
        {
            return Expired() ? false : _InstanceClassID && (_InstanceMemory ? true : false); // call bool operator on ptr
        }
        
        inline Bool operator==(const ClassInstance& rhs) const
        {
            return _GetInternal() == rhs._GetInternal();
        }
        
        inline Bool operator!=(const ClassInstance& rhs) const
        {
            return _GetInternal() != rhs._GetInternal();
        }
        
        // Internal use. Returns the deleter
        inline void (*_GetDeleter())(U8*)
        {
            if(!_InstanceMemory)
                return nullptr;
            auto Dx = std::get_deleter<void(*)(U8*)>(_InstanceMemory);
            return Dx ? *Dx : nullptr;
        }
        
        // The lua object will keep it alive. Only use this for high level calls.
        void PushStrongScriptRef(LuaManager& man);
        
        // Pushes a reference to this instance to the lua stack
        // Weak here means instance is not owned by the script reference. After all C++ instances are destroyed, getting the object returns nil.
        // Pass in the higher level object which will keep this alive. Keep alive can be empty, meaning it will be equal to this.
        void PushWeakScriptRef(LuaManager& man, ParentWeakReference keepAlive);
        
        // returns true if this instance has expired because its parent is no longer alive.
        inline Bool Expired() const
        {
            return !IsWeakPtrUnbound(_ParentAttachMemory) && _ParentAttachMemory.expired();
        }
        
        // Internal use. Gets the memory pointer
        inline U8* _GetInternal()
        {
            return Expired() ? nullptr : _InstanceMemory.get();
        }
        
        inline const U8* _GetInternal() const
        {
            return Expired() ? nullptr : _InstanceMemory.get();
        }
        
        // Gets the class ID
        inline U32 GetClassID() const
        {
            return _InstanceClassID;
        }
        
        // obtains a parent weak reference. if we have a parent, gets the top level, else returns this as its top level.
        inline ParentWeakReference ObtainParentRef()
        {
            return _InstanceMemory ? IsWeakPtrUnbound(_ParentAttachMemory) ?
            ParentWeakReference{_InstanceMemory} : _ParentAttachMemory : ParentWeakReference{};
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
        
        // after memory and sarray elements in top level parent
        ClassChildMap* _GetInternalChildrenRefs();
        
    private:
        
        void* _GetInternalPropertySetData(); // used by prop to store callback array. not per key. key doesnt have to exist yet.
        
        // Use by _MakeInstance. Deleter is defined in the meta.cpp TU.
        inline ClassInstance(U32 storedID, U8* memory, std::function<void(U8*)> _deleter, ParentWeakReference attachTo) :
        _InstanceClassID(storedID), _InstanceMemory(memory, _deleter), _ParentAttachMemory(std::move(attachTo)) {}
        
        // Use by the collection class. takes in the class and memory, but the memory won't be deleted
        inline ClassInstance(U32 storedID, U8* memoryNoDelete, ParentWeakReference attachTo) : _InstanceClassID(storedID),
        _InstanceMemory(memoryNoDelete, &NullDeleter), _ParentAttachMemory(std::move(attachTo)) {}
        
        // Use by the script ref to create an acquired reference from a script object
        inline ClassInstance(U32 storedID, Ptr<U8> acquired, ParentWeakReference prnt) : _InstanceClassID(storedID),
        _InstanceMemory(std::move(acquired)), _ParentAttachMemory(std::move(prnt)) {}
        
        U32 _InstanceClassID; // class id
        Ptr<U8> _InstanceMemory; // memory pointer to instance in memory
        ParentWeakReference _ParentAttachMemory; // weak reference to top level parent this instance is controlled by. NO ACCESS is
        // ever given to the parent. this is such that, for example, in async jobs the parent is kept constant and untouched by each child.
        
    };
    
    // Maximum transience value for below. States that the collection has been destroyed.
#define COLLECTION_TRANSIENCE_MAX 0xFFFF'FFFF
    
    // script reference juncture, used in transient references, a point in time when it was acquired.
    struct TransientJuncture
    {
        U32 JunctureValue; // value when created
        Ptr<std::atomic<U32>> CurrentValue; // current changing value
        U8* Value = nullptr; // value we cached
    };
    
    /**
     There are four types of script class instance references:
     - Strong: held alive as long as the parent C++ instance (shared ptr) is alive
     - Weak: held alive until the parent instance gets destroyed, at which calls to retrieve it after this will return nil.
     - Transient: from a collection, accessing an element in a collection. when the collection is operated on (eg pushed, popped, cleared), memory changes, so could invalidate. causes nil return after.
     - Persistent: persistent and will always return the class instance. used when passing into lua script functions internally where the lifetime is known, eg in serialisers.
     */
    class ClassInstanceScriptRef
    {
        
        U32 ClassID;
        U8* ConcreteInstanceRef; // actual instance pointer. only accessibe when the parent weak ref is alive
        ParentWeakReference ParentWeakRef; // weak reference to parent
        Ptr<U8> StrongRef; // for strong lua managed references
        TransientJuncture Juncture; // if transient
        
        // internal version returns ref counted pointer, so acquire increases strong ref #
        inline U8* __GetInternal()
        {
            if(IsTransient())
            {
                // Check transience state.
                if(Juncture.CurrentValue->load() > Juncture.JunctureValue)
                {
                    // someone has used a ContainerGetElement/Emplace etc to get an element but the collection changed
                    // and they pushed / poppped / clear etc, now the memory pointer is invalid. only access return values
                    // temporarily
                    TTE_ASSERT(false, "Script reference has expired: temporary collection access object still has references");
                    return nullptr;
                }
                return Juncture.Value; // dont delete it
            }
            if(IsWeakPtrUnbound(ParentWeakRef) || !ParentWeakRef.expired())
            {
                Bool exp = ParentWeakRef.expired();
                return exp ? nullptr : ConcreteInstanceRef;
            }
            return nullptr;
        }
        
    public:
        
        // INTERNAL: Create transient
        inline ClassInstanceScriptRef(U32 classID, TransientJuncture&& junc, ParentWeakReference&& p) :
        Juncture(std::move(junc)), ClassID(classID), ParentWeakRef(std::move(p)) {}
        
        // INTERNAL: Create strong ref / persistent
        inline ClassInstanceScriptRef(ClassInstance& inst) : ClassID(inst.GetClassID())
        {
            StrongRef = inst._InstanceMemory;
        }
        
        // INTERNAL: Create weak ref
        inline ClassInstanceScriptRef(ClassInstance& inst, ParentWeakReference keepAlive)
        {
            if(!inst)
                ClassID = 0;
            else
            {
                ClassID = inst.GetClassID();
                ConcreteInstanceRef = inst._InstanceMemory.get(); // only accessible through parent check
                ParentWeakRef = std::move(keepAlive);
            }
        }
        
        ~ClassInstanceScriptRef() = default; // default
        
        // Returns if this is a transient reference
        inline Bool IsTransient()
        {
            return Juncture.Value != nullptr;
        }
        
        inline U32 GetClassID()
        {
            return ClassID;
        }
        
        // acquires to a normal class reference, or nullptr if expired
        inline ClassInstance Acquire()
        {
            if(IsTransient())
                return ClassInstance(ClassID, __GetInternal(), ParentWeakRef);
            if(StrongRef)
                return ClassInstance(ClassID, StrongRef, {}); // no parent required as *this* holds a strong reference
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
    
    using CollectionComparatorLess = Bool(void* user, Meta::ClassInstance lhs, Meta::ClassInstance rhs);
    
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
        
        // Sorts the container. User data is passed to each comparator call.
        // THIS WILL COMPARE KEYS IF IT IS A KEYED COLLECTION. ELSE WILL COMPARE VALUES!
        void Sort(void* user, CollectionComparatorLess* pComparator);
        
        // Index must be less than size. Replaces. If copy is false, then that key or value is moved from the argument instead.
        void SetIndex(U32 index, ClassInstance key, ClassInstance value, Bool bCopyKey, Bool bCopyVal);
        
        // Inserts into at a specific index, shifting elements above up if needed. If bigger than size, pushes back like normal.
        void Insert(ClassInstance key, ClassInstance value, I32 index, Bool bCopyKey, Bool bCopyVal);
        
        // Pushes a new element
        // If a map, key should be non-null.
        // For any collection, value or key can be nullptr meaning a new one is constructed.
        // If key and val are either non-null, you can specify to copy(true) or move(false) using the last two arguments.
        void Push(ClassInstance key, ClassInstance val, Bool bCopyKey, Bool bCopyVal);
        
        void Clear(); // Clears the array to a size zero, clearing memory.
        
        // Pops the element at the given index. If index is bigger than or equal to GetSize(), pops top element. Warning: after
        // completion if key or val out are assigned, they are not owned by anything (ie are top-level)!
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
        
        // Pushes an object in this collection to the lua stack. If this collection transient state changes, the lua object
        // should not be accessed and any accesses will result in a transient test fault.
        // Set pushKey to true to push the key instead if this is a keyed collection
        // Pass in the parent, as it still required, it may be high level.
        void PushTransientScriptRef(LuaManager& L, U32 index, Bool bPushKey, ParentWeakReference parent);
        
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
        
        // state is updated
        void AdvanceTransienceFenceInternal();
        
        U32 _Size; // dynamic size of array
        U32 _Cap; // dynamic capacity of array (if SArray, not dynamic, this value is UINT32_MAX)
        
        U32 _ValuSize; // cached instance value size (size of one value element)
        U32 _PairSize; // size of key-value pair with align bytes between them
        
        U32 _ColID; // collection class ID
        U32 _ColFl; // collection flags, see above enum
        
        U8* _Memory; // allocated memory
        
        ParentWeakReference _PrntRef; // parent ref for this collection
        
        Ptr<std::atomic<U32>> _TransienceFence;
        
    };
    
    // INTERNAL USE FOR TOOL CONTEXT ONLY
    
    void Initialise();
    void Shutdown();
    
    void InitGame();
    void RelGame();
    
    // ==================================
    
    // ================================= PUBLIC META API =================================
    
    // Get the class ID of the given type from its information. ClassIDs are ALWAYS internal and do not store to disc. Returns 0 if not found.
    // Pass in an exact match optionally last (default false).
    // This means that if no class for 'class PropertySet' is found, an attempt to find without 'class '/etc erased 'PropertySet' is not done
    U32 FindClass(U64 typeHash, U32 versionNumber, Bool bExactMatch = false);
    
    // Same as find class but finds by version CRC instead of version number
    U32 FindClassByCRC(U64 typeHash, U32 versionCRC);
    
    // Find a class by its extension eg 'scene' for .scene files
    U32 FindClassByExtension(const String& ext, U32 versionNumber);
    
    // Same version using the string instead of the hash of the file name. INCLUDE 'CLASS ' etc. It will try again if not found with no class or other specifiers. (unless you want exact match..)
    // See FindClass with type hash for the last argument default
    inline U32 FindClass(const String& typeName, U32 versionNumber, Bool bExactMatch = false)
    {
        return FindClass(Symbol(typeName).GetCRC64(), versionNumber, bExactMatch);
    }
    
    // Optional parameters
    struct MetaStreamParams
    {
        
        StreamVersion Version = StreamVersion::UNSPECIFIED;
        
        // Optionally compress each meta section
        Compression::Type Compression[STREAM_SECTION_COUNT] = {Compression::Type::END_LIBRARY, Compression::Type::END_LIBRARY, Compression::Type::END_LIBRARY};
        
        // Optionally encrypt each meta section
        Bool Encryption[STREAM_SECTION_COUNT] = {false, false, false}; // if one is true, it is compressed (if not defined, then Zlib used)
        
    };
    
    // Writes a meta stream file to the output stream, from the instance. This can kick off async jobs, so could block while waiting to finish.
    // Pass in the name of the file you are writing, the instance to write to it, the output stream and the version of the meta stream.
    Bool WriteMetaStream(const String& name, ClassInstance instance, DataStreamRef& stream, MetaStreamParams params);
    
    /**
     Reads a meta stream file from the input stream, into return value instance.
     An optional debug stream can be used when debugging, such that all reads are written output as a string file.
     Any debug stream present causes any async stuff to not complete.
     Set last argument to optional maximum number of bytes to inline into the debug output stream when reading binary buffers. Any longer ones are truncated.
     */
    ClassInstance ReadMetaStream(const String& fileName, DataStreamRef& stream, DataStreamRef debugOutputStream = {}, U32 debugInlinableBufferSizeCap = 128);
    
    // Some older game files are encrypted with MBES headers and similar. This function takes any of those and returns a decrypting stream which
    // will come out as a normal MBIN file
    DataStreamRef MapDecryptingStream(DataStreamRef& stream);
    
    // ===== CLASS FUNCTIONALITY FOR SINGLE INSTANCES ======
    
    // Creates an instance of the given class. Thread safe between game switches. If creating a type which belongs inside another parent
    // type (ie a non top-level) type, then pass the parent instance as the second argument along with the name to give to that child
    // class. This is needed as child classes are all named. If you just want it to hold a reference, generate a random symbol, or a known
    // one that is only set once, as if it already exists the previous one will be replaced in the underlying map.
    // Don't use host as a property set, ie dont attach to them!
    ClassInstance CreateInstance(U32 ClassID, ClassInstance host = {}, Symbol name = {});
    
    // See CreateInstance. Creates a temporary instance in the buffer.
    // The instance must not be passed around! Only use it when you know its temporary to pass values around, such that its destructed before Buffer is out of scope.
    // BUFFER SIZE! Ensure it is at least 32 bytes larger than the expected size of the class (If not do big on the stack, not too big). As an extra checking block
    // is inserted after it in memory to make sure you don't pass it around, as well as top level child class array just in case.
    ClassInstance CreateTemporaryInstance(U8* Buffer, U32 BufferSize, U32 ClassID);
    
    // Creates an exact copy of the given instance. Thread safe between game switches. See CreateInstance second/third argument information.
    // Don't use host as a property set, ie dont attach to them!
    ClassInstance CopyInstance(ClassInstance instance, ClassInstance host = {}, Symbol name = {});
    
    // Moves the instance argument to a new instance, leaving the old one still alive but with none of its previous data (now in new returned one).
    // Thread safe between game switches. See CreateInstance second/third argument information.
    // Don't use host as a property set, ie dont attach to them!
    ClassInstance MoveInstance(ClassInstance instance, ClassInstance host = {}, Symbol name = {});
    
    // Acquires a reference to the given script object on the stack. After using ClassInstance::PushScriptRef, this can be used on the pushed value
    // Thread safe between game switches.
    ClassInstance AcquireScriptInstance(LuaManager& man, I32 stackIndex);
    
    // Returns if the given instance can have instances attached to it, using it passed into Create/Copy/Move Instance.
    Bool IsAttachable(ClassInstance& instance);
    
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
    // Thread safe between game switches. If last argument is true, will error if not found
    ClassInstance GetMember(ClassInstance& inst, const String& name, Bool bInsist);
    
    // Performs the less than operator '<' with the left and right hand side arguments (must be same type).
    Bool PerformLessThan(ClassInstance& lhs, ClassInstance& rhs);
    
    // Performs the equality operator '==' with the left and right hand side arguments (must be same type).
    Bool PerformEquality(ClassInstance& lhs, ClassInstance& rhs);
    
    // Returns the .VERS file name for the given class. Set the second argument to true to always use the tool type name (see MakeTypeName, no 'class ' etc)
    String MakeSerialisedVersionInfoFileName(U32 cls, Bool bAltName = false);
    
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
                           || Is(inst, "Symbol") || Is(inst, "Flags") || Is(inst, "__INTERNAL_BINARY_BUFFER__") || Is(inst, "__INTERNAL_DATASTREAM_CACHE__"),
                           "GetMember<T> can only be used on intrinsic types!");
                return *((T*)(inst._GetInternal() + member.RTOffset)); // Offset in memory, skip header.
            }
        }
        TTE_ASSERT(false, "Member %s::%s does not exist! Abort!!", pClass->Name.c_str(), name.c_str());
        return *((T*)FileNull()); // !! abort.
    }
    
    inline Bool IsSymbolClass(const Class& clazz)
    {
        return CompareCaseInsensitive(clazz.Name, "class Symbol") || CompareCaseInsensitive(clazz.Name, "Symbol");
    }
    
    inline Bool IsStringClass(const Class& clazz)
    {
        return CompareCaseInsensitive(clazz.Name, "class String") || CompareCaseInsensitive(clazz.Name, "String");
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
    
    const Class& GetClass(U32 id);
    
    // If the given instance class is a collection, this returns the modifyable collection for it. DO NOT access this after arrayType is not alive.
    // Thread safe between game switches.
    inline ClassInstanceCollection& CastToCollection(ClassInstance& arrayType)
    {
        TTE_ASSERT(arrayType, "Cannot cast to collection: array type is null");
        TTE_ASSERT(_Impl::_GetClass(arrayType.GetClassID())->Flags & CLASS_CONTAINER, "Cannot cast class to collection: it is not a collection");
        return *(ClassInstanceCollection*)arrayType._GetInternal();
    }
    
    // ====================================================================================
    
    struct InternalState
    {
        std::vector<RegGame> Games{};
        std::map<U32, Class> Classes{};
        std::map<Symbol, CompiledScript> Serialisers{}; // map of serialiser name => compiled script binary
        std::map<Symbol, CompiledScript> Normalisers{};
        std::map<Symbol, CompiledScript> Specialisers{};
        CompiledScript Collector{};
        I32 GameIndex = -1;
        String VersionCalcFun{}; // lua function which calculates version crc for a type.
        
        struct DeferredRegister
        {
            String KeyType, ValueType;
            U32 KeyVersion, ValueVersion;
            U32 CollectionClass;
        };
        
        struct DeferredWarning
        {
            String Class;
            U32 Calculated;
            U32 Overriden;
        };
        
        struct
        {
            
            std::vector<DeferredRegister> Deferred;
            std::vector<DeferredWarning> DeferredWarnings;
            
        } _Temp; // temporary internal deferred collection registr
        
        inline const RegGame& GetActiveGame() const
        {
            TTE_ASSERT(GameIndex != -1, "No game set!");
            return Games[GameIndex];
        }
        
    };
    
    // Gets the internal state. Its important that this is constant as it should NOT change between game switches.
    const InternalState& GetInternalState();
    
    namespace _Impl
    {
        template<typename T>
        struct _Coersion;
    }
    
    /**
     Extracts into out the contents of the class. This is useful for intrinsic types which never change in the engine such as Vector2, Transform or Quaternions.
     The type must be implemented in this header, which is likely is. If not, make a PR please.
     */
    template<typename T> void ExtractCoercableInstance(T& out, Meta::ClassInstance& inst)
    {
        _Impl::_Coersion<T>::Extract(out, inst);
    }
    
    /**
     Imports T 'in' into the class instance inst. This is useful for intrinsic types which never change in the engine such as Vector2, Transform or Quaternions.
     The type must be implemented in this header, which is likely is. If not, make a PR please.
     */
    template<typename T> void ImportCoercableInstance(const T& in, Meta::ClassInstance& inst)
    {
        _Impl::_Coersion<T>::Import(in, inst);
    }
    
    /**
     Puts the lua value on the stack into T.
     */
    template<typename T> void ExtractCoercableLuaValue(T& out, LuaManager& man, I32 stackIndex)
    {
        _Impl::_Coersion<T>::ExtractLua(out, man, stackIndex);
    }
    
    /**
     Pushes a lua value with the same value as T onto the stack.
     */
    template<typename T> void ImportCoercableLuaValue(const T& in, LuaManager& man)
    {
        _Impl::_Coersion<T>::ImportLua(in, man);
    }
    
    /**
     Pushes a lua value with the same value as the class instance.
     Specify optionally to push meta instances (for prop and collections) as transients (if coercing from a collection value for example) specify the collection it is inside of (as well as its index in it, still pass in inst for parent ref)
     */
    Bool CoerceMetaToLua(LuaManager& man, ClassInstance& inst, ClassInstanceCollection* pOwningCollection = nullptr, I32 collectionIndex = -1);
    
    /**
     Puts into class instance inst the value on the stack at stack index.
     */
    Bool CoerceLuaToMeta(LuaManager& man, I32 stackIndex, ClassInstance& inst);
    
    /**
     Pushes onto the stack the C++ type erased pObj which has associated meta type class.
     */
    Bool CoerceTypeErasedToLua(LuaManager& man, void* pObj, U32 clz);
    
    /**
     Gets the common instance allocator for the given class. The class must be a common class or it will return nullptr. Example classes, Mesh, Chore, Texture or Skeleton etc.
     */
    CommonClassAllocator* GetCommonAllocator(U32 clz);
    
}

// Register current class (T) to coersion. 
#define REGISTER_MY_COERSION(_Tp) (void)Meta::_Impl::_CoersionRegistrar<_Tp>::_ForceInstantiate();

namespace InstanceTransformation
{
    
    Bool PerformNormaliseAsync(Ptr<Handleable> pCommonInstanceOut, Meta::ClassInstance inInstance, LuaManager& lvm);
    
    Bool PerformSpecialiseAsync(Ptr<Handleable> pCommonInstance, Meta::ClassInstance outInstance, LuaManager& lvm);
    
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

#include <Meta/MetaCoersion.inl>
