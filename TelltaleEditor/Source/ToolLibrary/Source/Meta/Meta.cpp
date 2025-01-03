#include <Meta/Meta.hpp>
#include <Core/Context.hpp>
#include <Scripting/LuaManager.hpp>
#include <Core/Symbol.hpp>
#include <sstream>
#include <map>

thread_local Bool _IsMain = false; // Used in meta.

// ===================================================================         TOOL CONTEXT
// ===================================================================

static ToolContext* GlobalContext = nullptr;

ToolContext* CreateToolContext()
{

    if(GlobalContext == nullptr){
        _IsMain = true; // CreateToolContext sets this current thread as the main thread
        GlobalContext = (ToolContext*)TTE_ALLOC(sizeof(ToolContext), MEMORY_TAG_TOOL_CONTEXT); // alloc raw and construct, as its private.
        new (GlobalContext) ToolContext();
    }else{
        TTE_ASSERT(false, "Trying to create more than one ToolContext. Only one instance is allowed per process.");
    }
    
    return GlobalContext;
}

ToolContext* GetToolContext()
{
    return GlobalContext;
}

void DestroyToolContext()
{
    TTE_ASSERT(_IsMain, "Must only be called from main thread");
    
    if(GlobalContext)
        TTE_FREE(GlobalContext);
    
    GlobalContext = nullptr;
    
}

// ===================================================================         META
// ===================================================================

extern LuaFunctionCollection luaMetaGameEngine(); // defined totally in MetaLuaAPI.cpp

namespace Meta {
    
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
    
    static std::vector<RegGame> Games{};
    static std::map<U32, Class> Classes{};
    static U32 GameIndex{}; // Current game index
    static String VersionCalcFun{}; // lua function which calculates version crc for a type.
    
    // ===================================================================         IMPL
    // ===================================================================
    
    namespace _Impl {
        
        U32 _GenerateClassID(U64 typeHash, U32 versionCRC)
        {
            return CRC32((U8*)&typeHash, 8, versionCRC); // needs to be unique. hash the type hash with initial crc being the version crc.
        }
        
        // register class argument to meta system
        U32 _Register(LuaManager& man, Class&& cls, I32 stackIndex)
        {
            TTE_ASSERT(_IsMain, "Must only be called from main thread"); // modifications to Classes should only happen at beginning on main.
            
            cls.TypeHash = CRC64LowerCase((const U8*)cls.Name.c_str(), (U32)cls.Name.length());
            U32 crc = _DoLuaVersionCRC(man, stackIndex);
            if(crc == 0)
                return 0; // errored
            U32 id = _GenerateClassID(cls.TypeHash, crc);
            
            if(Classes.find(id) != Classes.end())
            {
                TTE_LOG("Duplicate type detected for '%s': version CRCs are the same for the given class!", cls.Name.c_str());
                return 0;
            }
            
            if(!(cls.Flags & CLASS_INTRINSIC))
            { // if not intrinsic, calc its size and align {
                if(cls.Members.size() == 0)
                {
                    cls.RTSize = 1; // default size 1 byte
                }
                else
                {
                    cls.RTSize = 0;
                    for(auto& member: cls.Members)
                    {
                        if(!(member.Flags & MEMBER_GHOST)) // ignore ghost members in the size
                        {
                            // if member type is >= 8 bytes, ensure 8 byte aligned.
                            if(Classes[member.ClassID].RTSize >= 8)
                                cls.RTSize += TTE_PADDING(cls.RTSize, 8);
                            else if(Classes[member.ClassID].RTSize >= 4) // else 4
                                cls.RTSize += TTE_PADDING(cls.RTSize, 4);
                            else if(Classes[member.ClassID].RTSize >= 2) // else 2
                                cls.RTSize += TTE_PADDING(cls.RTSize, 2);
                            
                            member.RTOffset = cls.RTSize;
                            cls.RTSize += Classes[member.ClassID].RTSize;
                        }
                    }
                }
            }
            else
            {
                // intrinsic
                if(cls.RTSize == 0)
                    TTE_LOG("Intrinsic type size has not been specified");
            }
            
            cls.VersionCRC = crc;
            Classes[id] = std::move(cls);
            return id;
        }
        
        // Meta serialise the given class and instance (pMemory).
        Bool _DoSerialise(Stream& stream, Class* clazz, void* pMemory, Bool IsWrite)
        {
            if(!pMemory || clazz == nullptr)
            {
                TTE_ASSERT(false, "Cannot serialise, type or class passed in is null");
                return false;
            }
            
            Bool bBlocked = (clazz->Flags & CLASS_NON_BLOCKED) == 0;
            
            if((clazz->Flags & CLASS_INTRINSIC) != 0) // add to version header if needed
            {
                if(IsWrite)
                {
                    // ensure no duplicate type hashes
                    Bool ok = true;
                    for(auto id: stream.VersionInf)
                    {
                        if(Classes[id].TypeHash == clazz->TypeHash)
                        {
                            ok = false;
                            break; // already in the version info, we are ok so skip adding it.
                        }
                    }
                    if(ok)
                        stream.VersionInf.push_back(clazz->ClassID); // for version info header, if not intrinsic.
                }
                else
                {
                    // read mode. check the version CRCs match !
                    for(auto id: stream.VersionInf)
                    {
                        // If we have the same type and it has a different version hash, we cannot continue. Format is not known.
                        if(Classes[id].TypeHash == clazz->TypeHash && Classes[id].VersionCRC != clazz->VersionCRC)
                        {
                            TTE_ASSERT(false, "Cannot serialise meta stream class: version CRC mismatch");
                            return false;
                        }
                    }
                }
            }
            
            // START OF BLOCK
            if(bBlocked)
            {
                if(IsWrite) // in write store position in stream
                {
                    stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition());
                    U32 zero{};
                    SerialiseU32(stream, nullptr, &zero, true); // write zero for block size now. will come back after.
                }
                else
                {
                    // read. store size + current offset to check after
                    U32 size{};
                    SerialiseU32(stream, nullptr, &size, true); // read block size (4 is added to it).
                    stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition() + size - 4);
                }
            }
            
            StreamSection initialSection = stream.CurrentSection; // ensure after the section is the same
            
            // BLOCK DATA
            if(clazz->Serialise) // has serialiser.
            {
                clazz->Serialise(stream, clazz, pMemory, IsWrite);
            }
            else if(clazz->SerialiseScriptFunction.length() > 0)
            {
                // call lua serialiser
                TTE_ASSERT(false, "IMPLEMENT LUA SERIALISER"); // args, read write functions? etc...
            }
            else
            {
                // serialise each member
                for(auto& member: clazz->Members)
                {
                    if(!_DoSerialise(stream, &Classes[member.ClassID], (U8*)pMemory + member.RTOffset, IsWrite))
                        return false;
                }
            }
            
            stream.CurrentSection = initialSection; // reset section
            
            // END OF BLOCK
            if(bBlocked)
            {
                if(IsWrite) // in write store position in stream
                {
                    U64 pos = stream.Sect[stream.CurrentSection].Blocks.back();
                    stream.Sect[stream.CurrentSection].Blocks.pop_back();
                    
                    U32 blockSize = (U32)(stream.Sect[stream.CurrentSection].Data->GetPosition() - pos);
                    U64 currentPos = stream.Sect[stream.CurrentSection].Data->GetPosition(); // cache current position
                    
                    stream.Sect[stream.CurrentSection].Data->SetPosition(pos); // go back to block offset
                    SerialiseU32(stream, nullptr, &blockSize, true); // write it
                    stream.Sect[stream.CurrentSection].Data->SetPosition(currentPos); // go back to normal. done
                }
                else
                {
                    U64 pos = stream.Sect[stream.CurrentSection].Blocks.back();
                    stream.Sect[stream.CurrentSection].Blocks.pop_back();
                    if(pos != stream.Sect[stream.CurrentSection].Data->GetPosition())
                    {
                        TTE_ASSERT(false, "Block size mismatch when reading class %s", clazz->Name.c_str());
                        return false;
                    }
                }
            }
            
            return true;
        }
        
        // calculate the crc32 version (important!) for the given class.
        U32 _DoLuaVersionCRC(LuaManager& man, I32 classTableStackIndex)
        {
            if(VersionCalcFun.length() == 0)
            {
                TTE_ASSERT(false, "No version hash calculation function has been set. Please set it before registering any classes.");
                return 0;
            }
            ScriptManager::GetGlobal(man, VersionCalcFun, true);
            if(man.Type(-1) != LuaType::FUNCTION)
            {
                TTE_ASSERT(false, "No version hash calculation function has been set. Please set it before registering any classes.");
                return 0;
            }
            else
            {
                man.PushCopy(classTableStackIndex); // by ref, ok
                man.CallFunction(1, 1);
                return ScriptManager::PopUnsignedInteger(man);
            }
        }
        
        // get class by crc (Use map)
        Class* _GetClass(U32 i) {
            return &Classes[i];
        }
    
        ClassInstance _MakeInstance(U32 ClassID)
        {
            auto it = Classes.find(ClassID);
            if(it == Classes.end()){
                TTE_LOG("Cannot create class instance for class index %X; not found in registry", ClassID);
                return ClassInstance{};
            }
            
            // allocate
            U8* pMemory = TTE_ALLOC(it->second.RTSize, MEMORY_TAG_META_TYPE);
            
            return ClassInstance{ClassID, pMemory, [=](U8* pMem){ // use a c++11 lambda so we get a closure.
                
                // this is a closure, and the only captured value (lua upvalue equivalent) is ClassID, as its not stored in the memory block.
                // this is the only time we use lambdas here, as they work quite well for this case, as otherwise there is no other way to
                // have a reference to ClassID unless its also stored in the memory block, which would not work in collections.
                
                _Impl::_DoDestruct(&Classes[ClassID], pMem); // call destructor
                
                // free memory
                TTE_FREE((U8*)pMem); // return with deleter
                
            }};
        }
        
        // perform constructor.
        void _DoConstruct(Class* pClass, U8* pInstanceMemory)
         {
            // If the whole class has a constructor, call it (eg String)
            if(pClass->Constructor)
            {
                pClass->Constructor(pInstanceMemory, pClass->ClassID);
            }
            else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit constructor, but is intrinsic, memset.
            {
                memset(pInstanceMemory, 0, pClass->RTSize); // set to zero (eg int types)
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoConstruct(&Classes[member.ClassID], pInstanceMemory + member.RTOffset); // again
                }
            }
        }
        
        // perform destructor
        void _DoDestruct(Class* pClass, U8* pInstanceMemory)
        {
            // If the whole class has a destructor, call it (eg String)
            if(pClass->Destructor)
            {
                pClass->Destructor(pInstanceMemory, pClass->ClassID);
            }
            else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit destructor, but is intrinsic.
            {
                memset(pInstanceMemory, 0, pClass->RTSize); // set to zero on destroys (eg int types)
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoDestruct(&Classes[member.ClassID], pInstanceMemory + member.RTOffset); // again
                }
            }
        }
        
        // perform copy
        void _DoCopyConstruct(Class* pClass, U8* pMemory, const U8* pSrc)
        {
            // If the whole class has a copy constructor, call it (eg String)
            if(pClass->CopyConstruct)
            {
                pClass->CopyConstruct(pSrc, pMemory);
            }else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit copy constructor, but is intrinsic, memcpy.
            {
                memcpy(pMemory, pSrc, pClass->RTSize);
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoCopyConstruct(&Classes[member.ClassID], pMemory + member.RTOffset, pSrc + member.RTOffset); // again
                }
            }
        }
        
        // perform move
        void _DoMoveConstruct(Class* pClass, U8* pMemory, U8* pSrc)
        {
            // If the whole class has a move constructor, call it (eg String)
            if(pClass->MoveConstruct)
            {
                pClass->MoveConstruct(pSrc, pMemory);
            }else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit move constructor, but is intrinsic, memcpy.
            {
                memcpy(pMemory, pSrc, pClass->RTSize);
                memset(pSrc, 0, pClass->RTSize); // move requires setting to zero after for non-trivial (POD) intrinsics
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoMoveConstruct(&Classes[member.ClassID], pMemory + member.RTOffset, pSrc + member.RTOffset); // again
                }
            }
        }
        
        // ===================================================================         INTRINSIC DEFAULTS
        // ===================================================================
        
        static void CtorCol(void* pMemory, U32 Array)
        {
            new (pMemory) ClassInstanceCollection(Array);
        }
        
        static void DtorCol(void* pMemory, U32)
        {
            ((ClassInstanceCollection*)pMemory)->~ClassInstanceCollection();
        }
        
        static void CopyCol(const void* pSrc, void* pDst)
        {
            new (pDst) ClassInstanceCollection(*((const ClassInstanceCollection*)pSrc));
        }
        
        static void MoveCol(void* pSrc, void* pDst)
        {
            new (pDst) ClassInstanceCollection(std::move(*((ClassInstanceCollection*)pSrc)));
        }
        
        static void CtorString(void* pMemory, U32)
        {
            new (pMemory) String();
        }
        
        static void DtorString(void* pMemory, U32)
        {
            ((String*)(pMemory))->~String();
        }
        
        static void CopyString(const void* pSrc, void* pDst)
        {
            new (pDst) String(*((const String*)pSrc)); // force call copy constructor
        }
        
        static void MoveString(void* pSrc, void* pDst)
        {
            new (pDst) String(std::move(*((String*)pSrc))); // force call move constructor
        }
        
        // string default serialiser. class shouldnt be used, can be null as used in other serialisers where class is obvious.
        static Bool SerialiseString(Stream& stream, Class*, void* pMemory, Bool IsWrite)
        {
            String& str = *((String*)pMemory);
            if(IsWrite)
            {
                U32 len = (U32)str.length();
                if(!SerialiseU32(stream, nullptr, &len, true)) // write length
                    return false;
                if(len && !SerialiseU32(stream, nullptr, (void*)str.c_str(), true)) // write ASCII
                    return false;
            }
            else
            {
                U32 len = 0;
                if(!SerialiseU32(stream, nullptr, &len, false)) // read length
                    return false;
                
                if(len > 10000) // large case
                {
                    TTE_LOG("Trying to read string with length > 10000. Assuming this is an error - failing.");
                    TTE_ASSERT(false, "String capacity too large");
                    return false;
                }
                
                if(len == 0) // empty case
                {
                    str = "";
                    return true;
                }
                
                str = String((size_t)len, ' '); // reserve string buffer
                if(len && !SerialiseU32(stream, nullptr, (void*)str.c_str(), false)) // read ASCII
                    return false;
            }
            return true;
        }
        
        // serialise symbol. if MBIN we store the string version of it.
        static Bool SerialiseSymbol(Stream& stream, Class* clazz, void* pMemory, Bool IsWrite)
        {
            Symbol& sym = *((Symbol*)pMemory);
            if(IsWrite)
            {
                if(stream.Version == MBIN)
                {
                    String value = RuntimeSymbols.Find(sym); // find the string for it
                    if(value.length() == 0 && sym.GetCRC64() != 0)
                    {
                        TTE_ASSERT(false, "Could not resolve symbol which should have been already loaded (runtime symbols cleared?)");
                        return false; // we cannot serialise as we need the symbol
                    }
                    
                    U32 len = (U32)value.length(); // write length then string
                    SerialiseU32(stream, 0, &len, true); // length
                    if(!stream.Write((const U8*)value.c_str(), (U64)len))
                        return false;
                }
                else
                {
                    // write U64
                    U64 hash = sym.GetCRC64();
                    SerialiseU64(stream, nullptr, &hash, true);
                }
            }
            else
            {
                if(stream.Version == MBIN)
                {
                    // read string
                    String symbol{};
                    if(!SerialiseString(stream, nullptr, &symbol, false))
                        return false; // string too large, corrupt.
                    
                    RuntimeSymbols.Register(symbol); // register it as for remapping back to a string
                    
                    sym = Symbol(std::move(symbol)); // assign symbol
                }
                else
                {
                    // read U64
                    U64 hash{};
                    SerialiseU64(stream, 0, &hash, false);
                    sym = hash;
                }
            }
            return true;
        }
        
        // booleans need to be serialised as ascii 0 or 1, but in memory we store as 0 or 1 numbers
        static Bool SerialiseBool(Stream& stream, Class* clazz, void* pMemory, Bool IsWrite)
        {
            if(IsWrite)
            {
                U8 val = (*((U8*)pMemory)) ? 0x31 : 0x30;
                return stream.Write(&val, 1);
            }
            else
            {
                U8 val = 0;
                Bool ret = stream.Read(&val, 1);
                *((U8*)pMemory) = val == 0x31;
                return ret;
            }
        }
        
        // serialise all telltale collections. DCArray, SArray, Map, Set, ...
        static Bool SerialiseCollection(Stream& stream, Class* clazz, void* pMemory, Bool IsWrite)
        {
            ClassInstanceCollection* pCollection = (ClassInstanceCollection*)pMemory;
            Bool bKeyed = pCollection->IsKeyedCollection();
            
            U32 keyClass = pCollection->GetKeyClass();
            U32 valClass = pCollection->GetValueClass();
            
            if(IsWrite)
            {
                U32 size = (U32)pCollection->GetSize();
                SerialiseU32(stream, nullptr, &size, true);
                for(U32 i = 0; i < size; i++)
                {
                    ClassInstance instance{};
                    
                    if(bKeyed)
                    {
                        // write key
                        instance = pCollection->GetKey(i);
                        if(!_DoSerialise(stream, &Classes[keyClass], instance._GetInternal(), true))
                            return false;
                        
                    }
                    
                    // write value
                    instance = pCollection->GetValue(i);
                    if(!_DoSerialise(stream, &Classes[valClass], instance._GetInternal(), true))
                        return false;
                    
                }
            }
            else
            {
                pCollection->Clear(); // ensure empty
                U32 size{};
                SerialiseU32(stream, nullptr, &size, true);
                if(size > 0x10000)
                {
                    TTE_ASSERT(false, "When serialising collection class: read size too large. Assuming corrupt: %d", size);
                    return false;
                }
                pCollection->Reserve(size); // only one allocation
                
                for(U32 i = 0; i < size; i++)
                {
                    ClassInstance kinstance{};
                    ClassInstance vinstance{};
                    
                    if(bKeyed)
                    {
                        kinstance = CreateInstance(keyClass); // serialise in key
                        if(!_Impl::_DoSerialise(stream, &Classes[keyClass], kinstance._GetInternal(), false))
                            return false;
                    }
                    
                    vinstance = CreateInstance(valClass); // serialise in value
                    if(!_Impl::_DoSerialise(stream, &Classes[valClass], vinstance._GetInternal(), false))
                        return false;
                    
                    // MOVE each into the collection
                    pCollection->Push(kinstance, vinstance, false, false);
                    
                }
            }
            return true;
        }
        
    }
    
    // ===================================================================          META LUA API
    // ===================================================================
    
    namespace L { // Lua functions
         
        // helper
        static BlowfishKey luaToKey(String hexkey)
        {
            BlowfishKey ret{};
            TTE_ASSERT((hexkey.length() & 1) == 0, "Hexadecimal encryption key length is not a multiple of two!");
            
            U32 len = (U32)hexkey.length();
            ret.BfKeyLength = len >> 1;
            TTE_ASSERT(ret.BfKeyLength, "Provided encryption key"
                       " is too large at %d bytes. Maximum 56.", ret.BfKeyLength);
            
            for(U32 i = 0; i < len; i += 2) // convert hex string to max 56 byte buffer
            {
                String byt = hexkey.substr(i,2);
                ret.BfKey[i >> 1] = (U8)strtol(byt.c_str(), nullptr, 0x10);
            }
            return ret;
        }
        
        // MetaRegisterGame(gameInfoTable)
        U32 luaMetaRegisterGame(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaRegisterGame can only be called in initialisation on the main thread");
            RegGame reg{};
            
            // pop stuff into reg game struct
            ScriptManager::TableGet(man, "Name");
            reg.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "ID");
            reg.ID = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "ModifiedEncryption");
            if(man.Type(-1) == LuaType::BOOL)
                reg.ModifiedBlowfish = ScriptManager::PopBool(man);
            else
            {
                man.Pop(1);
                reg.ModifiedBlowfish = false; // not modified default
            }
            
            ScriptManager::TableGet(man, "Key");
            if(man.Type(-1) == LuaType::STRING)
            {
                String hexkey = ScriptManager::PopString(man); // set master key
                reg.MasterKey = luaToKey(std::move(hexkey));
            }
            else if(man.Type(-1) == LuaType::TABLE)
            {
                man.PushNil(); // set for each table
                while(man.TableNext(-2) != 0)
                {
                    TTE_ASSERT(man.Type(-1) == LuaType::STRING && man.Type(-2) == LuaType::STRING, "Encryption key table invalid");
                    
                    man.PushCopy(-2); // push copy of key
                    
                    String key = man.ToString(-2);
                    String platformName = man.ToString(-1);
                    reg.PlatformToEncryptionKey[platformName] = luaToKey(key); // set platform key
                    
                    man.Pop(2);
                }
                man.Pop(1); // pop last key
            }
            else man.Pop(1); // no key given, skip
            
            ScriptManager::TableGet(man, "LuaVersion");
            String v = ScriptManager::PopString(man);
            reg.LVersion = v == "5.0.2" ? LuaVersion::LUA_5_0_2 : v == "5.1.4" ? LuaVersion::LUA_5_1_4 : LuaVersion::LUA_5_2_3;
            
            ScriptManager::TableGet(man, "DefaultMetaVersion");
            v = ScriptManager::PopString(man);
            
            reg.MetaVersion = v == "MSV6" ? MSV6 : v == "MSV5" ? MSV5 : v == "MSV4" ? MSV4 : v == "MCOM" ? MCOM : v == "MTRE" ? MTRE : v == "MBIN" ? MBIN : v == "MBES" ? MBES : (StreamVersion)-1;
            
            TTE_ASSERT(reg.MetaVersion != (StreamVersion)-1, "Invalid meta version");
            
            Games.push_back(std::move(reg));
            return 0;
        }
        
        // pushes constant to lua globals for intrinsic types
        static void AddIntrinsic(LuaManager& man, CString kName, String typeName, Class&& c) {
            
            man.PushTable();
            
            man.PushLString("Name");
            man.PushLString(typeName);
            man.SetTable(-3);
            
            man.PushLString("Flags");
            man.PushUnsignedInteger(c.Flags);
            man.SetTable(-3);
            
            U32 idx = _Impl::_Register(man, std::move(c), man.GetTop()); // intrinsic class table is top of stack
            
            man.PushLString("__MetaId");
            man.PushUnsignedInteger(idx);
            man.SetTable(-3);
    
            ScriptManager::SetGlobal(man, kName, true);
        }
        
        // MetaRegisterIntrinsics()
        U32 luaMetaRegisterIntrinsics(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaRegisterIntrinsics can only be called in snapshot initialisation on the main thread");
            Class c{};
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "uint32";
            c.RTSize = 4;
            c.Serialise = &SerialiseU16;
            AddIntrinsic(man, "kMetaUnsignedInt32", "uint32", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "int32";
            c.RTSize = 4;
            c.Serialise = &SerialiseU32;
            AddIntrinsic(man, "kMetaInt32", "int32", std::move(c));
            
            // alias for int32, int, but it is used as well. ik, its weird. (we need these for version matches)
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "int";
            c.RTSize = 4;
            c.Serialise = &SerialiseU32;
            AddIntrinsic(man, "kMetaInt", "int", std::move(c));
            
            // also for uint
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "uint";
            c.RTSize = 4;
            c.Serialise = &SerialiseU32;
            AddIntrinsic(man, "kMetaUnsignedInt", "uint", std::move(c));
            
            // and ive seen this for unsigned __int64
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "unsigned __int64";
            c.RTSize = 8;
            c.Serialise = &SerialiseU64;
            AddIntrinsic(man, "kMeta__UnsignedInt64", "unsigned __int64", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "long";
            c.RTSize = 4;
            c.Serialise = &SerialiseU32;
            AddIntrinsic(man, "kMetaLong", "long", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "float";
            c.RTSize = 4;
            c.Serialise = &SerialiseU32;
            AddIntrinsic(man, "kMetaFloat", "float", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "double";
            c.RTSize = 8;
            c.Serialise = &SerialiseU64;
            AddIntrinsic(man, "kMetaDouble", "double", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "bool";
            c.RTSize = 1;
            c.Serialise = &_Impl::SerialiseBool;
            AddIntrinsic(man, "kMetaBool", "bool", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "int8";
            c.RTSize = 1;
            c.Serialise = &SerialiseU8;
            AddIntrinsic(man, "kMetaInt8", "int8", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "uint8";
            c.RTSize = 1;
            c.Serialise = &SerialiseU8;
            AddIntrinsic(man, "kMetaUnsignedInt8", "uint8", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "int16";
            c.RTSize = 2;
            c.Serialise = &SerialiseU16;
            AddIntrinsic(man, "kMetaInt16", "int16", std::move(c));
            
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED;
            c.Name = "uint16";
            c.RTSize = 2;
            c.Serialise = &SerialiseU16;
            AddIntrinsic(man, "kMetaUnsignedInt32", "uint16", std::move(c));
            
            c.Flags = CLASS_NON_BLOCKED; // not intrinsic! it is stored in meta headers.
            c.Name = "Symbol";
            c.RTSize = sizeof(Symbol); // should definitely be 8!
            c.Serialise = &_Impl::SerialiseSymbol;
            AddIntrinsic(man, "kMetaSymbol", "Symbol", std::move(c));
            
            c.Flags = CLASS_INTRINSIC;
            c.Name = "String";
            c.RTSize = (U32)sizeof(String);
            c.Constructor = &_Impl::CtorString;
            c.Destructor = &_Impl::DtorString;
            c.Serialise = &_Impl::SerialiseString;
            c.CopyConstruct = &_Impl::CopyString;
            c.MoveConstruct = &_Impl::MoveString;
            AddIntrinsic(man, "kMetaString", "String", std::move(c));
            memset(&c, 0, sizeof(Class));
            
            return 0;
        }
        
        // helper function used twice. top of stack must be type table
        static Bool luaHelperRegisterMembers(Class& c, LuaManager& man)
        {
            ScriptManager::TableGet(man, "Members");
            if(man.Type(-1) == LuaType::TABLE){
                
                man.PushNil();
                while(man.TableNext(-2) != 0)
                {
                    
                    if(man.Type(-1) != LuaType::TABLE){
                        TTE_LOG("ERROR registering type %s: element in members array is not a table", c.Name.c_str());
                        man.Pop(3);
                        return false;
                    }
                    
                    ScriptManager::TableGet(man, "Name"); // member name
                    Member m{};
                    m.Name = ScriptManager::PopString(man);
                    
                    ScriptManager::TableGet(man, "Flags"); // pop flags
                    if(man.Type(-1) == LuaType::NUMBER)
                        m.Flags = ScriptManager::PopInteger(man);
                    else
                    {
                        man.Pop(1); // pop nil likely
                        // set flags to 0 if not set
                        man.PushLString("Flags");
                        man.PushInteger(0);
                        man.SetTable(-3);
                    }
                    
                    ScriptManager::TableGet(man, "Class");
                    if(man.Type(-1) != LuaType::TABLE)
                    {
                        TTE_LOG("ERROR registering type %s: member '%s' class is not the table", c.Name.c_str(), m.Name.c_str());
                        man.Pop(3);
                        return false;
                    }
                    else
                    {
                        man.PushLString("__MetaId");
                        man.GetTable(-2);
                        U32 memberType = (U32)ScriptManager::PopInteger(man);
                        if(Classes.find(memberType) == Classes.end()){
                            // must be registered in order used, cannot use a class as a member type before type is declared.
                            TTE_LOG("ERROR registering type %s: member '%s' class has not "
                                    "been registered or was not found.", c.Name.c_str(), m.Name.c_str());
                            man.Pop(3);
                            return false;
                        }
                        m.ClassID = memberType;
                    }
                    
                    c.Members.push_back(std::move(m)); // push new member
                    
                    man.Pop(2); // pop value and class table
                }
                
            }
            man.Pop(1); // pop members table/nil
            return true;
        }
        
        // MetaRegisterClass(classInfoTable)
        static U32 luaMetaRegisterClass(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaRegisterClass can only be called in snapshot initialisation on the main thread");
            
            Class c{};
            ScriptManager::TableGet(man, "Name");
            c.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "Flags");
            if(man.Type(-1) == LuaType::NUMBER)
                c.Flags = ScriptManager::PopInteger(man);
            else
            {
                man.Pop(1); // pop nil likely
                // set flags to 0 if not set
                man.PushLString("Flags");
                man.PushInteger(0);
                man.SetTable(-3);
            }
            ScriptManager::TableGet(man, "Serialiser");
            if(man.Type(-1) == LuaType::STRING)
                c.SerialiseScriptFunction = ScriptManager::PopString(man);
            else
                man.Pop(1);
            
            if(!luaHelperRegisterMembers(c, man))
                return 0;
            
            U32 id = _Impl::_Register(man, std::move(c), man.GetTop());
            
            man.PushLString("__MetaId");
            man.PushUnsignedInteger(id);
            man.SetTable(-3);
            
            man.Pop(1); // pop table
            
            return 0;
        }
        
        // MetaRegisterCollection(typeInfoTable, keyTypeTable OR SArray<T,N> N value, valueTypeTable)
        // type info table needs only Name, optional (likely needed) Members, and optional extra flags.
        // MEMBERS MUST ALL BE GHOST MEMBERS. they are skipped in all constructors/destructors/copies/moves/serialises.
        static U32 luaRegisterCollection(LuaManager& man)
        {
            Class c{};
            TTE_ASSERT(man.GetTop() == 3, "Incorrect call");
            TTE_ASSERT(man.Type(3) == LuaType::TABLE, "At MetaRegisterCollection, the value class argument (3) was not a table.");
            TTE_ASSERT(man.Type(1) == LuaType::TABLE, "At MetaRegisterCollection, the collection class argument (1) was not a table.");
            
            c.Flags = CLASS_CONTAINER;
            c.RTSize = (U32)sizeof(ClassInstanceCollection);
            c.Constructor = &_Impl::CtorCol;
            c.Destructor = &_Impl::DtorCol;
            c.CopyConstruct = &_Impl::CopyCol;
            c.MoveConstruct = &_Impl::MoveCol;
            c.Serialise = &_Impl::SerialiseCollection;
            
            man.PushCopy(1);
            
            ScriptManager::TableGet(man, "Name");
            c.Name = ScriptManager::PopString(man);
            
            ScriptManager::TableGet(man, "Flags");
            if(man.Type(-1) == LuaType::NUMBER)
                c.Flags |= ScriptManager::PopInteger(man);
            else
            {
                man.Pop(1); // pop nil likely
                // set flags to 0 if not set
                man.PushLString("Flags");
                man.PushInteger(0);
                man.SetTable(-3);
            }
            
            if(!luaHelperRegisterMembers(c, man))
                return 0; // collections may still have members albeit not serialised (ghosts)
            
            for(auto& member: c.Members) // must all be ghosts! collection class has no members available through meta API.
                TTE_ASSERT(member.Flags & MEMBER_GHOST, "When registering collection, all members must be ghosts: %s", member.Name.c_str());
            
            man.Pop(1); // pop copy of first arg
            
            if(man.Type(2) == LuaType::NUMBER) // SArray
                c.ArraySize = man.ToInteger(2);
            else if(man.Type(2) == LuaType::TABLE)
            {
                man.PushLString("__MetaId");
                man.GetTable(2);
                TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "When registering collection, key type table not registered properly");
                c.ArrayKeyClass = (U32)ScriptManager::PopInteger(man);
            }
            
            man.PushLString("__MetaId");
            man.GetTable(3);
            TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "When registering collection, value type table not registered properly");
            c.ArrayValClass = (U32)ScriptManager::PopInteger(man);
            
            U32 id = _Impl::_Register(man, std::move(c), 1);
            
            man.PushLString("__MetaId");
            man.PushUnsignedInteger(id);
            man.SetTable(1);
            
            return 0;
        }
        
        // most of the functions below are related to allowing lua to calculate version crcs.
        
        // MetaGetClassID(table)
        static U32 luaMetaGetId(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClassID");
            ScriptManager::TableGet(man, "__MetaId");
            return 1; // return value
        }
        
        // number MetaGetVersionCRC(table).
        static U32 luaMetaGetVersion(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClassID");
            ScriptManager::TableGet(man, "__MetaId");
            auto it = Classes.find(ScriptManager::PopInteger(man));
            
            if(it == Classes.end())
                man.PushNil();
            else
                man.PushUnsignedInteger(it->second.VersionCRC);
            
            return 1; // return value
        }
        
        // string MetaGetClassHash(table) (hex hash). returns the STRING HEX HASH VERSION.
        static U32 luaMetaGetClassHash(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClassHash");
            ScriptManager::TableGet(man, "__MetaId");

            auto it = Classes.find(ScriptManager::PopInteger(man));
            
            if(it == Classes.end())
                man.PushNil();
            else
            {
                U64 hash = it->second.TypeHash;
                std::stringstream stream;
                stream << std::hex << std::uppercase << hash;
                man.PushLString(stream.str());
            }
            
            return 1; // return value
        }
        
        // bool MetaFlagQuery(flags, flag_To_Test) (although, order doesn't matter
        static U32 luaMetaFlagQuery(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2,  "Incorrect usage of MetaFlagQuery");
            TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "Incorrect usage of MetaFlagQuery (arg)");
            TTE_ASSERT(man.Type(-2) == LuaType::NUMBER, "Incorrect usage of MetaFlagQuery (arg)");
            
            man.PushBool(((U32)man.ToInteger(-2) & (U32)man.ToInteger(-1)) != 0);
            
            return 1;
        }
        
        // number MetaHashByte(number initial_hash, number byte) : lower 8 bits hashed.
        static U32 luaMetaHashByte(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            U8 by = (U8)(((U32)man.ToInteger(-1)) & 0xFF);
            man.PushUnsignedInteger(CRC32(&by, 1, init));
            
            return 1;
        }
        
        // number MetaHashInt(number initial_hash, number val) : lower 32 bits hashed.
        static U32 luaMetaHashInt(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            U32 by = (U32)man.ToInteger(-1);
            man.PushUnsignedInteger(CRC32((const U8*)&by, 4, init));
            
            return 1;
        }
        
        // number MetaHashHexString(number initial_hash, string hexhash) : converts to U64 and hashes
        static U32 luaMetaHashHexString(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            String str = man.ToString(-1);
            U64 val = strtoull(str.c_str(), NULL, 16);
            
            man.PushUnsignedInteger(CRC32((const U8*)&val, 8, init));
            
            return 1;
        }
        
        // number MetaHashString(number initial_hash, string hash)
        static U32 luaMetaHashString(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            String str = man.ToString(-1);
            
            man.PushUnsignedInteger(CRC32((const U8*)str.c_str(), (U32)str.length(), init));
            
            return 1;
        }
        
        // MetaSetVersionFn(string fn)
        static U32 luaMetaSetVersionCalc(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of hash function");
            
            VersionCalcFun = man.ToString(-1);
            
            return 0;
        }
        
        // Generates new registerable meta library lua API
        static LuaFunctionCollection luaMeta()
        {
            LuaFunctionCollection Col{};
            Col.Functions.push_back({"MetaRegisterGame",&luaMetaRegisterGame});
            Col.Functions.push_back({"MetaRegisterIntrinsics", &luaMetaRegisterIntrinsics});
            Col.Functions.push_back({"MetaRegisterClass", &luaMetaRegisterClass});
            Col.Functions.push_back({"MetaRegisterCollection", &luaRegisterCollection}); // used in Meta::Initialise4 in engine
            Col.Functions.push_back({"MetaGetClassID", &luaMetaGetId});
            Col.Functions.push_back({"MetaGetVersionCRC", &luaMetaGetVersion});
            Col.Functions.push_back({"MetaGetClassHash", &luaMetaGetClassHash});
            Col.Functions.push_back({"MetaFlagQuery", &luaMetaFlagQuery});
            Col.Functions.push_back({"MetaHashByte", &luaMetaHashByte});
            Col.Functions.push_back({"MetaHashInt", &luaMetaHashInt});
            Col.Functions.push_back({"MetaHashHexString", &luaMetaHashHexString});
            Col.Functions.push_back({"MetaHashString", &luaMetaHashString});
            Col.Functions.push_back({"MetaSetVersionFn", &luaMetaSetVersionCalc});
            return Col;
        }
        
    }
    
    // garbage collector function for script refs to class instances
    U32 luaClassInstanceGC(LuaManager& man)
    {
        ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.ToPointer(-1);
        pRef->~ClassInstanceScriptRef(); // release strong ref if needed
        return 0;
    }
    
    void ClassInstance::PushScriptRef(LuaManager& man, Bool Strong)
    {
        if(_InstanceMemory)
        {
            ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.CreateUserData((U32)sizeof(ClassInstanceScriptRef));
            new (pRef) ClassInstanceScriptRef(*this, Strong); // construct it (previous just allocates in the lua mem)

            man.PushTable();
            
            man.PushLString("__gc");
            man.PushFn(&luaClassInstanceGC);
            man.SetTable(-3); // set gc method
            
            man.PushLString("__MetaId"); // also include the class id in the metatable so we can check it
            man.PushUnsignedInteger(_InstanceClassID);
            man.SetTable(-3);
            
            man.SetMetaTable(-2);
            
        }
        else
        {
            man.PushNil(); // no object
        }
    }
    
    // ===================================================================         CORE META SYSTEM
    // ===================================================================
    
    // get pointer from stack, then call get
    ClassInstance AcquireScriptInstance(LuaManager& man, I32 stackIndex)
    {
        if(man.Type(stackIndex) != LuaType::FULL_OPAQUE)
            return {};
        
        ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.ToPointer(stackIndex);
        
        if(pRef == nullptr || pRef->GetClassID() == 0)
            return {};
        
        return ClassInstance(pRef->GetClassID(), pRef->_GetInternal());
    }
    
    ClassInstance CreateInstance(U32 ClassID)
    {
        ClassInstance instance = _Impl::_MakeInstance(ClassID);
        
        _Impl::_DoConstruct(&Classes[ClassID], instance._GetInternal());
        
        return instance;
    }

    ClassInstance CopyInstance(ClassInstance instance)
    {
        if(!instance)
            return ClassInstance{};
        
        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID());
        
        _Impl::_DoCopyConstruct(&Classes[instance.GetClassID()], instanceNew._GetInternal(), instance._GetInternal());
        
        return instanceNew;
    }
    
    ClassInstance MoveInstance(ClassInstance instance)
    {
        if(!instance)
            return ClassInstance{};
        
        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID());
        
        _Impl::_DoMoveConstruct(&Classes[instance.GetClassID()], instanceNew._GetInternal(), instance._GetInternal());
        
        return instanceNew;
    }
    
    U32 FindClassID(U64 typeHash, U32 versionCRC)
    {
        U32 id = _Impl::_GenerateClassID(typeHash, versionCRC);
        return Classes.find(id) == Classes.end() ? 0 : id; // ensure it exists before we return it.
    }
    
    // initialise the snapshot classes
    void InitGame()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        GameSnapshot snap = GetToolContext()->GetSnapshot();
        
        // Ensure game exists
        Bool Found = false;
        RegGame* pActiveGame = nullptr;
        for(auto& game: Games)
        {
            if(game.ID == snap.ID)
            {
                Found = true;
                pActiveGame = &game;
                break;
            }
        }
        TTE_ASSERT(Found, "Game with ID '%s' does not exist or was not registered, cannot initialise with game snapashot.", snap.ID.c_str());
        if(!Found)
        {
            RelGame();
            return;
        }
        
        // Run RegisterAll and register all classes in the current snapshot
        
        ScriptManager::GetGlobal(GetToolContext()->GetLibraryLVM(), "RegisterAll", true);
        
        if(GetToolContext()->GetLibraryLVM().Type(-1) != LuaType::FUNCTION)
        {
            GetToolContext()->GetLibraryLVM().Pop(1);
            TTE_ASSERT(false,"Cannot switch to new snapshot. Classes registration script is invalid");
            RelGame();
            return;
        }
        else
        {
            GetToolContext()->GetLibraryLVM().PushLString(snap.ID);
            GetToolContext()->GetLibraryLVM().PushLString(snap.Platform);
            GetToolContext()->GetLibraryLVM().PushLString(snap.Vendor);
            ScriptManager::Execute(GetToolContext()->GetLibraryLVM(), 3, 1);
            if(!ScriptManager::PopBool(GetToolContext()->GetLibraryLVM()))
            {
                TTE_LOG("RegisterAll failed!");
            }
            else // success, so init blowfish encryption for the game
            {
                BlowfishKey key = pActiveGame->MasterKey; // set to master key
                
                // if there is a specific platform encryption key, do that here
                auto it = pActiveGame->PlatformToEncryptionKey.find(snap.Platform);
                if(it != pActiveGame->PlatformToEncryptionKey.end())
                    key = it->second;
                
                if(key.BfKeyLength == 1 && key.BfKey[0] == 0)
                {
                    TTE_ASSERT(false, "Game encryption key for %s/%s has not been registered. Please contact us with the executable of "
                               "the game for this platform so we can get the encryption key!", snap.ID.c_str(), snap.Platform.c_str());
                    // encryption key '00' means we dont know it yet.
                    RelGame();
                    return;
                }
                
                Blowfish::Initialise(pActiveGame->ModifiedBlowfish, key.BfKey, key.BfKeyLength);
            }
        }
        
        TTE_LOG("Meta fully initialised with snapshot of %s: registered %d classes", snap.ID.c_str(), (U32)Classes.size());
    }
    
    void RelGame()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        Classes.clear();
        VersionCalcFun = "";
    }
    
    void Initialise()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        
        ScriptManager::RegisterCollection(GetToolContext()->GetLibraryLVM(), L::luaMeta()); // Register meta scripting API
        ScriptManager::RegisterCollection(GetToolContext()->GetLibraryLVM(), luaMetaGameEngine()); // Register low level API to match engine
        
        // Global LUA flag constants for use in the library init scripts
        
        GetToolContext()->GetLibraryLVM().PushInteger(CLASS_NON_BLOCKED);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaClassNonBlocked", true); // class is not blocked
        GetToolContext()->GetLibraryLVM().PushInteger(CLASS_INTRINSIC);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaClassIntrinsic", true); // class is intrinsic
        GetToolContext()->GetLibraryLVM().PushInteger(CLASS_ALLOW_ASYNC);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaClassAllowAsync", true); // class can be serialised async faster
        GetToolContext()->GetLibraryLVM().PushInteger(CLASS_CONTAINER);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaClassContainer", true); // container flag
        GetToolContext()->GetLibraryLVM().PushInteger(CLASS_ABSTRACT);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaClassAbstract", true); // abstract
        
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_ENUM);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberEnum", true); // member is an enum
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_FLAG);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberFlag", true); // member is a flag
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_BASE);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberBaseClass", true); // member is a base class
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_GHOST);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberGhost", true); // member not in memory (no Class needed)f
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_SKIP);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberSerialiseDisable", true); // member not on disk
        
        // Setup games script
        
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(),
                               GetToolContext()->LoadLibraryStringResource("Games.lua"), "Games.lua"); // Initialise games
        
        // Setup classes script
        
        String src = GetToolContext()->LoadLibraryStringResource("Classes.lua");
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(), src, "Classes.lua");
        src.clear();
        
        TTE_LOG("Meta partially initialised with %d games", (U32)Games.size());
    }
    
    void Shutdown()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        
        Games.clear();
        
        TTE_LOG("Meta shutdown");
    }
    
    Bool WriteMetaStream(const String& name, ClassInstance instance, DataStreamRef& stream, StreamVersion StreamVersion)
    {
        
        // SETUP LOCALS
        
        if(name.empty() || !stream || !instance)
        {
            TTE_ASSERT(false, "Invalid arguments passed into WriteMetaStream");
            return false;
        }
        
        Stream metaStream{};
        U8 Buffer[32]{};
        String magic =  StreamVersion == MSV6 ? "6VSM" :
                        StreamVersion == MSV5 ? "5VSM" :
                       // StreamVersion == MSV4 ? "4VSM" :
                       // StreamVersion == MCOM ? "MOCM" :
                        StreamVersion == MTRE ? "ERTM" :
                       // StreamVersion == MBES ? "SEBM" :
                        StreamVersion == MBIN ? "NIBM" : "X";
        
        if(magic == "X")
        {
            TTE_LOG("Invalid meta stream version");
            return false;
        }
        
        // 1. SETUP META STREAM
        
        metaStream.Version = StreamVersion;
        metaStream.CurrentSection = STREAM_SECTION_MAIN;
        metaStream.Sect[STREAM_SECTION_MAIN].Data = DataStreamManager::GetInstance()->CreatePrivateCache(name, 0x8000); // 32 KB
        if(StreamVersion == MSV5 || StreamVersion  == MSV6)
        {
            // set main and async as memory streams
            metaStream.Sect[STREAM_SECTION_ASYNC].Data = DataStreamManager::GetInstance()->CreatePrivateCache(name, 0x100000);// 1 MB (mesh/tex)
            metaStream.Sect[STREAM_SECTION_DEBUG].Data = DataStreamManager::GetInstance()->CreatePrivateCache(name, 0x100); // 256 B (rare)
        }
        else
        {
            // set them as null streams. stop any writes in older versions.
            metaStream.Sect[STREAM_SECTION_ASYNC].Data = DataStreamManager::GetInstance()->CreateNullStream();
            metaStream.Sect[STREAM_SECTION_DEBUG].Data = DataStreamManager::GetInstance()->CreateNullStream();
        }
        
        // 2. SERIALISE INSTANCE TO THE STREAM
        
        if(!_Impl::_DoSerialise(metaStream, &Classes[instance.GetClassID()], instance._GetInternal(), true))
        {
            TTE_LOG("Cannot write meta stream: serialisation failed");
            return false;
        }
         
        if(StreamVersion != MSV5 && StreamVersion != MSV6 &&
                     metaStream.Sect[STREAM_SECTION_ASYNC].Data->GetSize() > 0 && metaStream.Sect[STREAM_SECTION_DEBUG].Data->GetSize() > 0)
        {
            TTE_ASSERT(false, "Cannot write serialised meta stream: data written to sections invalid for the given stream version");
            // async and debug sections do not exist in anything less than MSV5
            return false;
        }
        
        // 3. WRITE HEADER
        
        SerialiseDataU32(stream, nullptr, const_cast<char*>(magic.c_str()), true); // magic
        
        if(StreamVersion == MSV6 || StreamVersion == MSV5) // TODO optional compressed sections (set top bit of size to 1)
        {
            U32 size = (U32)metaStream.Sect[STREAM_SECTION_MAIN].Data->GetSize(); // size MUST fit into a U31 (yes a 1). otherwise file too large.
            TTE_ASSERT((U64)(size & 0x7FFF'FFFF) ==
                       metaStream.Sect[STREAM_SECTION_MAIN].Data->GetSize(), "Cannot serialise out meta stream: main section too large");
            SerialiseDataU32(stream, nullptr, &size, true);
            
            size = (U32)metaStream.Sect[STREAM_SECTION_ASYNC].Data->GetSize();
            TTE_ASSERT((U64)(size & 0x7FFF'FFFF) ==
                       metaStream.Sect[STREAM_SECTION_ASYNC].Data->GetSize(), "Cannot serialise out meta stream: async section too large");
            SerialiseDataU32(stream, nullptr, &size, true);
            
            size = (U32)metaStream.Sect[STREAM_SECTION_DEBUG].Data->GetSize();
            TTE_ASSERT((U64)(size & 0x7FFF'FFFF) ==
                       metaStream.Sect[STREAM_SECTION_DEBUG].Data->GetSize(), "Cannot serialise out meta stream: debug section too large");
            SerialiseDataU32(stream, nullptr, &size, true);
        }
        
        U32 numVersionInfo = (U32)metaStream.VersionInf.size();
        SerialiseDataU32(stream, nullptr, &numVersionInfo, true); // write version info count
        
        for(U32 i = 0; i < numVersionInfo; i++)
        {
            if(StreamVersion == MBIN)
            {
                U32 len = (U32)Classes[metaStream.VersionInf[i]].Name.length();
                SerialiseDataU32(stream, nullptr, &len, true); // write type name length
                
                // write type name string
                if(!stream->Write((const U8*)(Classes[metaStream.VersionInf[i]].Name.c_str()), len))
                {
                    TTE_ASSERT(false, "Cannot serialise out meta stream: type name could not be written in header");
                    return false;
                }
            }
            else
            {
                SerialiseDataU64(stream, nullptr, &Classes[metaStream.VersionInf[i]].TypeHash, true); // write type hash
            }
            
            SerialiseDataU32(stream, nullptr, &Classes[metaStream.VersionInf[i]].VersionCRC, true); // write version crc
            
        }
        
        // 4. WRITE MAIN SECTION
        
        metaStream.Sect[STREAM_SECTION_MAIN].Data->SetPosition(0);
        DataStreamManager::GetInstance()->Transfer(metaStream.Sect[STREAM_SECTION_MAIN].Data, stream,
                                                   metaStream.Sect[STREAM_SECTION_MAIN].Data->GetSize());
        
        // 5. WRITE ASYNC AND DEBUG SECTIONS IF NEEDED
        
        if(StreamVersion == MSV5 || StreamVersion == MSV6)
        {
            metaStream.Sect[STREAM_SECTION_ASYNC].Data->SetPosition(0);
            DataStreamManager::GetInstance()->Transfer(metaStream.Sect[STREAM_SECTION_ASYNC].Data, stream,
                                                       metaStream.Sect[STREAM_SECTION_ASYNC].Data->GetSize());
            
            metaStream.Sect[STREAM_SECTION_DEBUG].Data->SetPosition(0);
            DataStreamManager::GetInstance()->Transfer(metaStream.Sect[STREAM_SECTION_DEBUG].Data, stream,
                                                       metaStream.Sect[STREAM_SECTION_DEBUG].Data->GetSize());
        }
        
        // Finished!
        
        return true;
    }
    
    // Reads meta stream file format
    ClassInstance ReadMetaStream(DataStreamRef& stream)
    {
        Stream metaStream{};
        U8 Buffer[256]{};
        U32 mainSectionSize{}, asyncSectionSize{}, debugSectionSize{};
        
        // read magic string, add null terminator
        if(!stream->Read(Buffer, 4))
            return {};
        Buffer[4] = 0;
        
        String magic = (const char*)Buffer;
        metaStream.Version =    magic == "6VSM" ? MSV6 :
                                magic == "5VSM" ? MSV5 :
                                magic == "4VSM" ? MSV4 :
                                magic == "MOCM" ? MCOM :
                                magic == "ERTM" ? MTRE :
                                magic == "SEBM" ? MBES :
                                magic == "NIBM" ? MBIN : (StreamVersion)-1;
        
        // ENCRYPTED MAGICS. DEAL WITH AND DELEGATE ENCRYPTION TO DataStreamLegacyEncrypted
        if(metaStream.Version == (StreamVersion)-1 || metaStream.Version == MBES) // other encrypted formats have weird non ascii magics.
        {
            U32 z = 0, raw = 0, bf = 0;
            if(metaStream.Version == MBES)
            {
                z = 64; // block size
                raw = 100; // every 100 blocks, it is left as is
                bf = 64; // every 64 blocks, we encrypt with blowfish
                // any other blocks are bit inverted. see legacy encrypted stream
            }
            else if(!memcmp(Buffer, "\x64\x17\x4A\xFB", 4) || !memcmp(Buffer, "\x91\x40\x79\xEB", 4) || !memcmp(Buffer, "\xFB\xDE\xAF\x64", 4))
            { // other weird magics
                z = 128;
                raw = 80;
                bf = 32;
            }
            else if(!memcmp(Buffer, "\xAA\xDE\xAF\x64", 4))
            { // other weird magic
                z = 256;
                raw = 24;
                bf = 8;
            }
            else // anything else we haven't seen. not a meta stream!
            {
                TTE_ASSERT(false, "File is not a meta stream!");
                return {};
            }
            
            metaStream.Version = MBIN; // set as internal MBIN. set input stream as legacy encrypted.
            
            // override stream such that we read the decrypted bytes without any worry
            DataStreamRef decryptingStream = DataStreamManager::GetInstance()->
                    CreateLegacyEncryptedStream(stream, stream->GetPosition(), (U16)z, (U8)raw, (U8)bf);
            
            stream = decryptingStream; // override stream. decryptingStream has access to stream, so it will delete when it gets deleted.
        }
        
        // these versions need looking into. they exist, but very very rare (if ever used)
        if(metaStream.Version == MSV4 || metaStream.Version == MCOM)
        {
            TTE_LOG("Unsupported meta stream version %d", metaStream.Version);
            return {};
        }
        
        if(metaStream.Version == MSV5 || metaStream.Version == MSV6) // latest versions
        {
            SerialiseDataU32(stream, nullptr, &mainSectionSize, false);
            SerialiseDataU32(stream, nullptr, &asyncSectionSize, false);
            SerialiseDataU32(stream, nullptr, &debugSectionSize, false);
        }
        
        U32 numVersionInfo{};
        SerialiseDataU32(stream, nullptr, &numVersionInfo, false);
        
        // Sanity check
        if(numVersionInfo > (U32)Classes.size())
        {
            TTE_ASSERT(false, "Input data stream does not follow the meta stream format, or file is corrupt");
            return {};
        }
        
        // Read version info header
        for(U32 i = 0; i < numVersionInfo; i++)
        {
            U64 typeHash{}; U32 versionCRC{};
            
            // read type hash (or string if very old).
            if(metaStream.Version == MBIN)
            {
                U32 len{};
                
                SerialiseDataU32(stream, nullptr, &len, false);
                
                if(len > 256) // should never occur.
                {
                    TTE_ASSERT(false, "Cannot serialise in meta stream: a serialised class name is too long");
                    return {};
                }
                
                if(!stream->Read(Buffer, len))
                {
                    TTE_LOG("Cannot serialise in meta stream: unexpected EOF");
                    return {};
                }
                
                typeHash = CRC64LowerCase(Buffer, len);
            }
            else
            {
                SerialiseDataU64(stream, 0, &typeHash, false);
            }
            
            // read type version CRC
            SerialiseDataU32(stream, 0, &versionCRC, false);
            
            // try and find it in the registered classes. if it is not found, the version CRC likely mismatches. cannot load, as
            // format is not 100% gaurunteed to match.
            U32 ClassID = _Impl::_GenerateClassID(typeHash, versionCRC);
            
            if(Classes.find(ClassID) == Classes.end())
            {
                TTE_LOG("Cannot serialise in meta stream: a serialised class does not match any runtime class version."
                        " Type hash 0x%llX and version %X", typeHash, versionCRC);
                return {};
            }
            
            metaStream.VersionInf.push_back(ClassID);
            
        }
        
        // .VERS files contain a Class serialised to a file. They can be loaded separately in the future. Cannot detect a type from it.
        if(numVersionInfo == 0)
        {
            TTE_LOG("Cannot serialise in meta stream: no version information. Is this a .VERS file? If so, it cannot be loaded this way.");
            return {};
        }
        
        if((mainSectionSize & 0x80000000u) || (asyncSectionSize & 0x80000000u) || (debugSectionSize & 0x80000000u))
        {
            TTE_ASSERT(false, "Containerised (compressed) meta streams are not supported at the moment");
            return {}; // TODO add container support wrappers
        }
        
        if(metaStream.Version == MSV5 || metaStream.Version == MSV6) // create separate sub streams for each section
        {
            metaStream.Sect[STREAM_SECTION_MAIN].Data = DataStreamManager::GetInstance()->
                        CreateSubStream(stream, stream->GetPosition(), (U64)mainSectionSize);
            metaStream.Sect[STREAM_SECTION_ASYNC].Data = DataStreamManager::GetInstance()->
                        CreateSubStream(stream, stream->GetPosition() + (U64)mainSectionSize, (U64)asyncSectionSize);
            metaStream.Sect[STREAM_SECTION_DEBUG].Data = DataStreamManager::GetInstance()->
                        CreateSubStream(stream, stream->GetPosition() + (U64)mainSectionSize + (U64)asyncSectionSize, (U64)debugSectionSize);
        }
        else
        {
            metaStream.Sect[STREAM_SECTION_MAIN].Data = DataStreamManager::GetInstance()->
                        CreateSubStream(stream, stream->GetPosition(), stream->GetSize() - stream->GetPosition()); // remaining bytes
            
            // async and debug don't exist in old games. ignore them here
            metaStream.Sect[STREAM_SECTION_ASYNC].Data = DataStreamManager::GetInstance()->CreateNullStream();
            metaStream.Sect[STREAM_SECTION_DEBUG].Data = DataStreamManager::GetInstance()->CreateNullStream();
        }
        
        // Finally, detect the type and then serialise and return it.
        U32 ClassID = metaStream.VersionInf.front(); // first version info entry is the type.
        
        ClassInstance instance = CreateInstance(ClassID);
        
        if(!instance)
        {
            TTE_LOG("Could not create instance for class stored in meta stream");
            return instance;
        }
    
        // perform actual serialisation of the primary class stored.
        if(!_Impl::_DoSerialise(metaStream, &Classes[ClassID], instance._GetInternal(), false))
        {
            TTE_ASSERT(false, "Could not seralise meta stream primary class");
            return {};
        }
        
        return instance; // OK
    }
    
    // ===================================================================         COLLECTIONS
    // ===================================================================
    
    // constructor for arrays of meta class instances. init flags, and internals
    ClassInstanceCollection::ClassInstanceCollection(U32 type) : _Size(0), _Cap(0), _ValuSize(0), _Memory(nullptr), _PairSize(0), _ColFl(0)
    {
        auto clazz = Classes.find(type);
        TTE_ASSERT(clazz != Classes.end(), "Invalid class ID passed into collection initialiser"); // class must be valid
        TTE_ASSERT(clazz->second.Flags & CLASS_CONTAINER, "Class ID passed into collection is not a container class"); // not a collection
        TTE_ASSERT(Classes[clazz->second.ArrayValClass].RTSize > 0 && (clazz->second.ArrayKeyClass == 0
                        || Classes[clazz->second.ArrayKeyClass].RTSize > 0), "Collection key or value class has no size");
        
        _ColID = type;
        _ValuSize = Classes[clazz->second.ArrayValClass].RTSize;
        
        // flags init
        if(Classes[clazz->second.ArrayValClass].Constructor == nullptr && Classes[clazz->second.ArrayValClass].Flags & CLASS_INTRINSIC)
            _ColFl |= _COL_VAL_SKIP_CT;
        if(Classes[clazz->second.ArrayValClass].Destructor == nullptr && Classes[clazz->second.ArrayValClass].Flags & CLASS_INTRINSIC)
            _ColFl |= _COL_VAL_SKIP_DT;
        if(Classes[clazz->second.ArrayValClass].CopyConstruct == nullptr  && Classes[clazz->second.ArrayValClass].Flags & CLASS_INTRINSIC)
            _ColFl |= _COL_VAL_SKIP_CP;
        if(Classes[clazz->second.ArrayValClass].MoveConstruct == nullptr  && Classes[clazz->second.ArrayValClass].Flags & CLASS_INTRINSIC)
            _ColFl |= _COL_VAL_SKIP_MV;
        
        
        // if we are an SArray, we are not dynamic so allocate fixed size
        if(clazz->second.ArraySize > 0)
        {
            Reserve(clazz->second.ArraySize);
            _Size = clazz->second.ArraySize;
            _Cap = UINT32_MAX; // signal we are an sarray.
        }
        
        if(clazz->second.ArrayKeyClass) // if this is a Map type
        {
            // we will need padding bytes if the key type is small (lower than 8 bytes) and the value type is big (8 bytes or larger)
            Class* pKey = _Impl::_GetClass(clazz->second.ArrayKeyClass);
            Class* pVal = &clazz->second;
            
            // key flags
            if(pKey->Constructor == nullptr && pKey->Flags & CLASS_INTRINSIC)
                _ColFl |= _COL_KEY_SKIP_CT;
            if(pKey->Destructor == nullptr && pKey->Flags & CLASS_INTRINSIC)
                _ColFl |= _COL_KEY_SKIP_DT;
            if(pKey->CopyConstruct == nullptr && pKey->Flags & CLASS_INTRINSIC)
                _ColFl |= _COL_KEY_SKIP_CP;
            if(pKey->MoveConstruct == nullptr && pKey->Flags & CLASS_INTRINSIC)
                _ColFl |= _COL_KEY_SKIP_MV;
            
            
            if(pKey->RTSize < 8) // we need alignment bytes between key and value in memory. max align is 8 !!
            {
                if(pVal->RTSize < 8) // both are small types, intrinsic types.
                {
                    // we could use __builtin_clz to detect bit indices, we know its only used for values 1 to 7, so macro can be used
                    // use _TO_ALIGN since align isnt stored in classes. sizes 7-4 have align 4, 3-2 align 2 and 1 align 1, as best approx.
                    
#define _TO_ALIGN(val) (val & 0b100) ? 4 : (val & 0b010) ? 2 : 1
                    
                    _PairSize = pKey->RTSize + TTE_PADDING(pKey->RTSize, _TO_ALIGN(pVal->RTSize)) + pVal->RTSize;
                    
#undef _TO_ALIGN
                    
                }else _PairSize = pKey->RTSize + TTE_PADDING(pKey->RTSize, 8) + _ValuSize; // pair size plus in between padding bytes for value
            }
            else _PairSize = pKey->RTSize + pVal->RTSize; // max align is 8, so no padding needed.
            
        }
        else // this is *not* a map type. always skip key construction
        {
            _ColFl |= (_COL_KEY_SKIP_CP | _COL_KEY_SKIP_CT | _COL_KEY_SKIP_DT | _COL_KEY_SKIP_MV); // skip all key stuff
            _PairSize = _ValuSize; // same size, non keyed container
        }
    }
    
    ClassInstanceCollection::~ClassInstanceCollection()
    {
        Clear();
        _ColFl = _PairSize = _ValuSize = _ColID = 0;
    }
    
    ClassInstanceCollection& ClassInstanceCollection::operator=(ClassInstanceCollection&& rhs)
    {
        TTE_ASSERT(this != &rhs, "Invalid move operation"); // cannot move to self
        Clear();
        // move rhs into this. everything in this is POD, use memcpy.
        memcpy(this, &rhs, sizeof(ClassInstanceCollection));
         // ensure rhs has no refs to the memory block
        rhs._Cap = rhs._Size = 0;
        rhs._Memory = nullptr;
        return *this;
    }
    
    ClassInstanceCollection::ClassInstanceCollection(ClassInstanceCollection&& rhs)
    {
        TTE_ASSERT(this != &rhs, "Invalid move operation"); // cannot move to self
        // move construct.
        memcpy(this, &rhs, sizeof(ClassInstanceCollection));
        rhs._Cap = rhs._Size = 0;
        rhs._Memory = nullptr;
    }
    
    ClassInstanceCollection::ClassInstanceCollection(const ClassInstanceCollection& rhs)
    {
        // copy construct. set this to zero memory, then call operator= copy as its a big function (ensuring Clear succeeds)
        memset(this, 0, sizeof(ClassInstanceCollection));
        
        this->operator=(rhs);
    }
    
    ClassInstanceCollection& ClassInstanceCollection::operator=(const ClassInstanceCollection& rhs)
    {
        if(this == &rhs) return *this; // sanity
        
        // clear then copy from rhs
        Clear();
        
        // memcpy everything, then change pMemory to new buffer with copied elements
        memcpy(this, &rhs, sizeof(ClassInstanceCollection));
        
        // set these to zero, for reserve function
        _Cap = _Size = 0;
        _Memory = nullptr;
        
        if(rhs._Cap > 0)
        {
            Reserve(rhs._Cap);
            _Size = rhs._Size; // same size after this call
            
            // check if we we can skip copy constructor with memcpy
            Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_CP) != 0;
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_CP) != 0;
            
            if(bSkipKey && bSkipVal) // fastest, skip both by a memcpy
                memcpy(_Memory, rhs._Memory, _PairSize * _Size); // copy *size* of rhs, not capacity
            else
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    const U8* pSrcMem = rhs._Memory + (i * _PairSize);
                    U8* pDstMem = _Memory + (i * _PairSize);
                    
                    // construct each element
                    
                    if(!bSkipKey)
                    {
                        // copy construct the key
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayKeyClass && Classes[Classes[_ColID].ArrayKeyClass].CopyConstruct,
                                   "Meta collection class: no key copy constructor found");
                        _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem);
                    }
                    else
                    {
                        // memcpy the key (technically, also the padding bytes. these should be zero anyway)
                        memcpy(pDstMem, pSrcMem, _PairSize - _ValuSize);
                    }
                    
                    // offset for value from key value pair memory pointer is = _PairSize - _ValuSize
                    U32 offset = _PairSize - _ValuSize;
                    
                    if(!bSkipVal)
                    {
                        // copy construct the value
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].CopyConstruct,
                                   "Meta collection class: no value copy constructor found");
                        _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset);
                    }
                    else
                    {
                        // memcpy the value.
                        memcpy(pDstMem + offset, pSrcMem + offset, _ValuSize);
                    }
                    
                }
            }
        }
        else
        {
            // no capacity, so no memory allocated
        }
        
        return *this;
    }
    
    U32 ClassInstanceCollection::GetSize()
    {
        return _Size;
    }
    
    U32 ClassInstanceCollection::GetCapacity()
    {
        return _Cap;
    }
    
    Bool ClassInstanceCollection::IsKeyedCollection()
    {
        return _ValuSize != _PairSize;
    }
    
    U32 ClassInstanceCollection::GetArrayClass()
    {
        return _ColID;
    }
    
    U32 ClassInstanceCollection::GetValueClass()
    {
        return Classes[_ColID].ArrayValClass;
    }
    
    U32 ClassInstanceCollection::GetKeyClass()
    {
        return Classes[_ColID].ArrayKeyClass;
    }
    
    void ClassInstanceCollection::Reserve(U32 cap)
    {
        // ensure the backend memory array has size at least cap.
        
        // sanity
        if(_Cap >= cap)
            return;
        
        U32 newCapacity{};
        TTE_ROUND_UPOW2_U32(newCapacity, cap); // round to upper power of 2 for capacity.
        U8* pNewMemory = TTE_ALLOC(_PairSize * newCapacity, MEMORY_TAG_META_COLLECTION);
        
        if(_Size > 0) // is previous size is zero no moving needs to be done
        {
            
            // check if we we can skip moveconstructor with memcpy
            Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_MV) != 0;
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_MV) != 0;
            
            if(bSkipKey && bSkipVal) // skip both, one memcpy
                memcpy(pNewMemory, _Memory, _Size * _PairSize);
            else
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pSrcMem = _Memory + (i * _PairSize);
                    U8* pDstMem = pNewMemory + (i * _PairSize);
                    
                    // construct each element
                    
                    if(!bSkipKey)
                    {
                        // move construct the key
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayKeyClass && Classes[Classes[_ColID].ArrayKeyClass].MoveConstruct,
                                   "Meta collection class: no key move constructor found");
                        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem);
                    }
                    else
                    {
                        // memcpy the key (technically, also the padding bytes. these should be zero anyway)
                        memcpy(pDstMem, pSrcMem, _PairSize - _ValuSize);
                    }
                    
                    // offset for value from key value pair memory pointer is = _PairSize - _ValuSize
                    U32 offset = _PairSize - _ValuSize;
                    
                    if(!bSkipVal)
                    {
                        // move construct the value
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].MoveConstruct,
                                   "Meta collection class: no value move constructor found");
                        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset);
                    }
                    else
                    {
                        // memcpy the value.
                        memcpy(pDstMem + offset, pSrcMem + offset, _ValuSize);
                    }
                    
                }
            }
        }
        
        // memset the remaining to zeros
        memset(pNewMemory + (_Size * _PairSize), 0, (newCapacity - _Size) * _PairSize);
        
        U32 size = _Size;
        Clear(); // call clear now, call destructors and deallocates.
        
        // set new data
        _Size = size;
        _Cap = newCapacity;
        _Memory = pNewMemory;
    }
    
    void ClassInstanceCollection::Clear()
    {
        if(_Memory != nullptr)
        {
            // check if we we can skip destructor
            Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_DT) != 0;
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_DT) != 0;
        
            if(!bSkipKey || !bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pMem = _Memory + (i * _PairSize);
                    
                    if(!bSkipKey) // if we need to destruct the key
                    {
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayKeyClass && Classes[Classes[_ColID].ArrayKeyClass].Destructor,
                                   "Meta collection class: no key destructor found");
                        _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayKeyClass], pMem);
                    }
                    
                    if(!bSkipVal) // if we need to destruct the value
                    {
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].Destructor,
                                   "Meta collection class: no value destructor found");
                        _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayValClass], pMem);
                    }
                    
                }
            }
            TTE_FREE(_Memory);
        }
        _Memory = nullptr;
        _Size = _Cap = 0;
    }
    
    void ClassInstanceCollection::Push(ClassInstance k, ClassInstance v, Bool copyk, Bool copyv)
    {
        Reserve(_Size + 1); // Ensure size
        
        U8* pDstMemory = _Memory + (_Size * _PairSize); // get destination memory pointer, at size.
        
        // 1. key construction
        if(IsKeyedCollection())
        {
            if(!k)
            {
                // construct new key without copy or move
                _Impl::_DoConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory);
            }
            else
            {
                if(copyk) // copy or move from value
                    _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory, k._GetInternal());
                else
                    _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory, k._GetInternal());
            }
        }
        
        // 2. val construction
        pDstMemory += (_PairSize - _ValuSize); // move pointer to start of value memory (skip key and padding)
        if(!v)
        {
            _Impl::_DoConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory);
        }
        else
        {
            if(copyv) // copy or move from value
                _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal());
            else
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal());
        }
        
        // update size
        _Size++;
    }
    
    Bool ClassInstanceCollection::Pop(U32 index, ClassInstance& keyOut, ClassInstance& valOut)
    {
        if(_Size == 0)
            return false; // if empty, ignore.
        
        // if >= size, set to top
        if(index >= _Size)
            index = _Size - 1;
        
        // Move construct both into new value, also call destructors
        if(IsKeyedCollection())
        {
            keyOut = _Impl::_MakeInstance(Classes[_ColID].ArrayKeyClass);
            _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], keyOut._GetInternal(), GetKey(index)._GetInternal());
            _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayKeyClass], GetKey(index)._GetInternal());
        }
        valOut = _Impl::_MakeInstance(Classes[_ColID].ArrayValClass);
        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], valOut._GetInternal(), GetValue(index)._GetInternal());
        _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayValClass], GetValue(index)._GetInternal());
        
        // check if we we can skip move constructor with memcpy
        Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_MV) != 0;
        Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_MV) != 0;
        
        // Now we want to move any elements above it, down in memory by one space.
        for(U32 i = index + 1; i < _Size; i++)
        {
            // move construct FROM i TO i - 1
            
            U8* pSrcMem = _Memory + (i * _PairSize);
            U8* pDstMem = _Memory + ((i - 1) * _PairSize);
            
            // construct each element
            
            if(!bSkipKey)
            {
                // move construct the key
                TTE_ASSERT(_ColID && Classes[_ColID].ArrayKeyClass && Classes[Classes[_ColID].ArrayKeyClass].MoveConstruct,
                           "Meta collection class: no key move constructor found");
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem);
            }
            else
            {
                // memcpy the key (technically, also the padding bytes. these should be zero anyway)
                memcpy(pDstMem, pSrcMem, _PairSize - _ValuSize);
            }
            
            // offset for value from key value pair memory pointer is = _PairSize - _ValuSize
            U32 offset = _PairSize - _ValuSize;
            
            if(!bSkipVal)
            {
                // move construct the value
                TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].MoveConstruct,
                           "Meta collection class: no value move constructor found");
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset);
            }
            else
            {
                // memcpy the value.
                memcpy(pDstMem + offset, pSrcMem + offset, _ValuSize);
            }
        }
        
        // reduce size
        _Size--;
        
        return true;
    }
    
    ClassInstance ClassInstanceCollection::GetKey(U32 i)
    {
        TTE_ASSERT(IsKeyedCollection(), "GetKey cannot be called on a collection class which is not a keyed collection");
        TTE_ASSERT(i < _Size, "Trying to access element outside array bounds");
        
        return ClassInstance(Classes[_ColID].ArrayKeyClass, _Memory + (i * _PairSize));
    }
    
    ClassInstance ClassInstanceCollection::GetValue(U32 i)
    {
        TTE_ASSERT(i < _Size, "Trying to access element outside array bounds");
        
        return ClassInstance(Classes[_ColID].ArrayValClass, _Memory + (++i * _PairSize) - _ValuSize);
    }
    
}
