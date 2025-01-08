#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Core/Context.hpp>

#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp>

#include <sstream>
#include <utility>

extern thread_local bool _IsMain;

// ===================================================================         META LUA API
// ===================================================================

namespace Meta
{
 
    // access meta.cpp
    extern std::map<U32, Class> Classes;
    extern std::vector<RegGame> Games;
    extern String VersionCalcFun;
    
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
            if(!GetToolContext() || GetToolContext()->GetActiveGame()) // no active game can be set here
            {
                TTE_ASSERT(false, "At MetaRegisterGame: this can only be called during initialisation of a game");
                return 0;
            }
            RegGame reg{};
            
            // pop stuff into reg game struct
            ScriptManager::TableGet(man, "Name");
            reg.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "ID");
            reg.ID = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "IsArchive2");
            reg.UsesArchive2 = ScriptManager::PopBool(man);
            ScriptManager::TableGet(man, "ArchiveVersion");
            reg.ArchiveVersion = ScriptManager::PopUnsignedInteger(man);
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
            man.SetTop(0);
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
        
        // TO STRING, EQUALS, AND LESS THAN OPERATORS FOR INTRINSICS.
        
        template <typename _Type>
        static String GenerateToString(const void* pIntrin)
        {
            std::ostringstream ss{};
            ss << *((const _Type*)pIntrin);
            return ss.str();
        }
        
        template<typename _Type>
        static Bool GenerateEquals(const void* l, const void* r)
        {
            return *((const _Type*)l) == *((const _Type*)r);
        }
        
        template<typename _Type>
        static Bool GenerateLT(const void* l, const void* r) // less than operator, for T
        {
            return *((const _Type*)l) < *((const _Type*)r);
        }
        
        static String StringToStringOperation(const void* pStr) // to string operation for string... returns the string
        {
            return *((const String*)pStr);
        }
        
        static String SymbolToStringOperation(const void* pSym)
        {
            U64 hash = ((const Symbol*)pSym)->GetCRC64();
            String s = RuntimeSymbols.Find(hash); // try find it first
            if(s.length() > 0)
                return s;
            return "Symbol<" + SymbolToHexString(hash) + ">";
        }

        // MetaRegisterIntrinsics()
        U32 luaMetaRegisterIntrinsics(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaRegisterIntrinsics can only be called in snapshot initialisation on the main thread");
            if(!GetToolContext() || GetToolContext()->GetActiveGame())
            {
                TTE_ASSERT(false, "At MetaRegisterIntrinsics: this can only be called during initialisation of a game");
                return 0;
            }
            Class c{};
            
            // helper macro
#define REGISTER_INTRINSIC(cxx_type, name_string, script_constant_string, size_bytes, serialiser) \
c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED; \
c.Name = name_string; \
c.ToString = &GenerateToString<cxx_type>;\
c.RTSize = size_bytes; \
c.LessThan = &GenerateLT<cxx_type>; \
c.Equals = &GenerateEquals<cxx_type>; \
c.Serialise = serialiser; \
AddIntrinsic(man, script_constant_string, name_string, std::move(c));
            
            // I define all possible type names used in all games here as it doesn't matter how many there are. (different compiler ones etc)
            
            REGISTER_INTRINSIC(U32, "uint32", "kMetaUnsignedInt32", 4, &SerialiseU32);
            REGISTER_INTRINSIC(I32,"int", "kMetaInt", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U32,"uint", "kMetaUnsignedInt", 4, &SerialiseU32); // alias (obvious 32)
            REGISTER_INTRINSIC(I32,"int32", "kMetaInt32", 4, &SerialiseU32); // alias
            REGISTER_INTRINSIC(U64,"uint64", "kMetaUnsignedInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I64,"int64", "kMetaInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(U64,"unsigned __int64", "kMeta__UnsignedInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I32,"long", "kMetaLong", 4, &SerialiseU32); // alias for int32 on most windows
            REGISTER_INTRINSIC(float,"float", "kMetaFloat", 4, &SerialiseU32); // serialise it as U32
            REGISTER_INTRINSIC(double,"double", "kMetaDouble", 8, &SerialiseU64); // serialise as U64
            REGISTER_INTRINSIC(bool,"bool", "kMetaBool", 1, &_Impl::SerialiseBool);
            REGISTER_INTRINSIC(I8,"int8", "kMetaInt8", 1, &SerialiseU8);
            REGISTER_INTRINSIC(U8,"uint8", "kMetaUnsignedInt8", 1, &SerialiseU8);
            REGISTER_INTRINSIC(I16,"int16", "kMetaInt16", 2, &SerialiseU16);
            REGISTER_INTRINSIC(U16,"uint16", "kMetaUnsignedInt16", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I8,"signed char", "kMetaSignedChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(I8,"char", "kMetaChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(double,"long double", "kMetaLongDoubler", 8, &SerialiseU64); // alias for double
            REGISTER_INTRINSIC(U8,"unsigned char", "kMetaUnsignedChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(U32,"unsigned int", "kMetaUnsignedInt", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U32,"unsigned long", "kMetaUnsignedLong", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U16,"unsigned short", "kMetaUnsignedShort", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I16,"short", "kMetaShort", 2, &SerialiseU16);
            REGISTER_INTRINSIC(wchar_t,"wchar_t", "kMetaWideChar", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I64,"long long", "kMetaLongLong", 8, &SerialiseU64);
            REGISTER_INTRINSIC(U64,"unsigned long long", "kMetaUnsignedLongLong", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I64,"__int64", "kMeta__Int64", 8, &SerialiseU64); // alias for uint64
            
#undef REGISTER_INTRINSIC
            
            c.Flags = CLASS_NON_BLOCKED; // not intrinsic! it is stored in ALL games meta headers.
            c.Name = "Symbol";
            c.RTSize = sizeof(Symbol); // should definitely be 8!
            c.ToString = &SymbolToStringOperation;
            c.Serialise = &_Impl::SerialiseSymbol;
            AddIntrinsic(man, "kMetaSymbol", "Symbol", std::move(c));
            
            // older games have class before it
            c.Flags = CLASS_NON_BLOCKED; // not intrinsic! it is stored in ALL games meta headers.
            c.Name = "class Symbol";
            c.RTSize = sizeof(Symbol); // should definitely be 8!
            c.ToString = &SymbolToStringOperation;
            c.Serialise = &_Impl::SerialiseSymbol;
            AddIntrinsic(man, "kMetaClassSymbol", "class Symbol", std::move(c));
            
            c.Flags = CLASS_INTRINSIC;
            c.Name = "String";
            c.RTSize = (U32)sizeof(String);
            c.Constructor = &_Impl::CtorString;
            c.Destructor = &_Impl::DtorString;
            c.Serialise = &_Impl::SerialiseString;
            c.CopyConstruct = &_Impl::CopyString;
            c.MoveConstruct = &_Impl::MoveString;
            c.ToString = &StringToStringOperation;
            AddIntrinsic(man, "kMetaString", "String", std::move(c));
            memset(&c, 0, sizeof(Class));
            
            // register string again with 'class', some older games use that
            c.Flags = CLASS_INTRINSIC;
            c.Name = "class String";
            c.RTSize = (U32)sizeof(String);
            c.Constructor = &_Impl::CtorString;
            c.Destructor = &_Impl::DtorString;
            c.Serialise = &_Impl::SerialiseString;
            c.CopyConstruct = &_Impl::CopyString;
            c.MoveConstruct = &_Impl::MoveString;
            c.ToString = &StringToStringOperation;
            AddIntrinsic(man, "kMetaClassString", "class String", std::move(c));
            memset(&c, 0, sizeof(Class));
            
            // binary buffer internal type. does not have a serialiser! use this to store a ref counted memory buffer
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED; // keep it intrinsic, so it does not go into meta headers. ensure no block size (invis)
            c.Name = "__INTERNAL_BINARY_BUFFER__"; // internal class.
            c.RTSize = (U32)sizeof(BinaryBuffer);
            c.Constructor = &_Impl::CtorBinaryBuffer;
            c.Destructor = &_Impl::DtorBinaryBuffer;
            c.CopyConstruct = &_Impl::CopyBinaryBuffer;
            c.MoveConstruct = &_Impl::MoveBinaryBuffer;
            AddIntrinsic(man, "kMetaClassInternalBinaryBuffer", "__INTERNAL_BINARY_BUFFER__", std::move(c));
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
            
            if(!GetToolContext() || GetToolContext()->GetActiveGame())
            {
                TTE_ASSERT(false, "At MetaRegisterClass: this can only be called during initialisation of a game");
                return 0;
            }
            
            Class c{};
            ScriptManager::TableGet(man, "VersionIndex");
            c.VersionNumber = ScriptManager::PopInteger(man);
            ScriptManager::TableGet(man, "Name");
            c.Name = ScriptManager::PopString(man);
            TTE_ASSERT(c.VersionNumber <= MAX_VERSION_NUMBER, "%s: Version number cannot be larger than %s", c.Name.c_str(), MAX_VERSION_NUMBER);
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
                c.SerialiseScriptFn = ScriptManager::PopString(man);
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
            
            if(!GetToolContext() || GetToolContext()->GetActiveGame())
            {
                TTE_ASSERT(false, "At MetaRegisterCollection: this can only be called during initialisation of a game");
                return 0;
            }
            
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
            
            ScriptManager::TableGet(man, "VersionIndex");
            c.VersionNumber = ScriptManager::PopInteger(man);
            TTE_ASSERT(c.VersionNumber <= MAX_VERSION_NUMBER, "%s: Version number cannot be larger than %s", c.Name.c_str(), MAX_VERSION_NUMBER);
            
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
                TTE_ASSERT(member.Flags & MEMBER_MEMORY_DISABLE, "When registering collection,"
                           " member has associated memory which is not allowed: %s", member.Name.c_str());
            
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
                man.PushLString(SymbolToHexString(it->second.TypeHash));
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
        
        static U32 luaMetaHashLong(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            String str = man.ToString(-1);
            U64 value = strtoull(str.c_str(), NULL, 16);
            
            // flip
            value = ((value & 0x00000000000000FF) << 56) |
            ((value & 0x000000000000FF00) << 40) |
            ((value & 0x0000000000FF0000) << 24) |
            ((value & 0x00000000FF000000) << 8)  |
            ((value & 0x000000FF00000000) >> 8)  |
            ((value & 0x0000FF0000000000) >> 24) |
            ((value & 0x00FF000000000000) >> 40) |
            ((value & 0xFF00000000000000) >> 56);
            
            man.PushUnsignedInteger(CRC32((const U8*)&value, 8, init));
            
            return 1;
        }
        
        // number MetaHashHexString(number initial_hash, string hexhash) : converts to buf and hashes
        static U32 luaMetaHashHexString(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of hash function");
            
            U32 init = (U32)man.ToInteger(-2);
            String str = man.ToString(-1);
            
            std::vector<U8> buffer;
            buffer.resize(str.length() >> 1);
            for (size_t i = 0; i < str.length(); i += 2) {
                std::string byte_string = str.substr(i, 2);
                buffer.push_back(static_cast<uint8_t>(std::stoi(byte_string, nullptr, 16)));
            }
            
            man.PushUnsignedInteger(CRC32((const U8*)buffer.data(), (U32)buffer.size(), init));
            
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
        
        // flags = MetaGetClassFlags(typename, versionindex)
        static U32 luaMetaGetClassFlags(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of GetClassFlags");
            String tn = man.ToString(-2);
            U32 v = (U32)man.ToInteger(-1);
            U32 cls = FindClass(Symbol(tn), v);
            if(cls)
                man.PushUnsignedInteger(Classes[cls].Flags);
            else
                man.PushNil(); // error
            return 1;
        }
        
        // flags = MetaGetMemberFlags(typename, versioindex, member)
        static U32 luaMetaGetMemberFlags(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of GetMemberFlags");
            
            String tn = man.ToString(-3);
            I32 ver = man.ToInteger(-2);
            U32 cls = FindClass(Symbol(tn), (U32)ver);
            String mem = man.ToString(-1);
            
            if(cls == 0)
            {
                TTE_LOG("%s with version index %s does not exist", tn.c_str(), ver);
                man.PushNil();
                return 1;
            }
            
            auto& members = Classes[cls].Members;
            
            for(auto& member: members)
            {
                if(CompareCaseInsensitive(member.Name, mem))
                {
                    man.PushInteger(Classes[member.ClassID].Flags);
                    return 2;
                }
            }
            
            man.PushNil();
            return 1;
        }
        
        // typename, versionindex = MetaGetMemberClass(typename, versionindex, member)
        static U32 luaMetaGetMemberClass(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of GetMemberClass");
            
            String tn = man.ToString(-3);
            I32 ver = man.ToInteger(-2);
            U32 cls = FindClass(Symbol(tn), (U32)ver);
            String mem = man.ToString(-1);
        
            if(cls == 0)
            {
                TTE_LOG("%s with version index %s does not exist", tn.c_str(), ver);
                man.PushNil();
                man.PushNil();
                return 2;
            }
            
            auto& members = Classes[cls].Members;
            
            for(auto& member: members)
            {
                if(CompareCaseInsensitive(member.Name, mem))
                {
                    man.PushLString(Classes[member.ClassID].Name);
                    man.PushInteger(Classes[member.ClassID].VersionNumber);
                    return 2;
                }
            }
            
            man.PushNil();
            man.PushNil();
            return 2;
        }
        
        // table GetMemberNames(typename, version index). returns table of members
        static U32 luaMetaGetMemberNames(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of GetMemberNames");
            String tn = man.ToString(-2);
            I32 ver = man.ToInteger(-1);
            U32 cls = FindClass(Symbol(tn), (U32)ver);
            
            if(cls == 0)
            {
                TTE_LOG("%s with version index %s does not exist", tn.c_str(), ver);
                man.PushNil();
                return 1;
            }
            
            man.PushTable();
            
            auto& members = Classes[cls].Members;
            
            U32 i = 0;
            for(auto& member: members)
            {
                man.PushInteger(i++);
                man.PushLString(member.Name.c_str());
                man.SetTable(-3);
            }
            
            return 1;
        }
        
        // instance = MetaGetMember(instance, name). attains a weak reference.
        static U32 luaMetaGetMember(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaGetMember");
            
            ClassInstance inst = AcquireScriptInstance(man, -2);
            
            if(inst)
            {
                String mem = man.ToString(-1);
                inst = GetMember(inst, mem);
                if(inst)
                    inst.PushScriptRef(man);
                else
                    man.PushNil();
            }
            else
            {
                TTE_LOG("MetGetMember called with invalid instance. Returning nil");
                man.PushNil();
            }
            
            return 1;
        }
        
        // createinstance(typename_string, version_number, attach) STRONG reference to attach. attach is another instance, which when destroyed
        // this one will be destroyed (it owns it) - example in a Chore, a ChoreAgent since its not in a DCArray
        static U32 luaMetaCreateInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of CreateInstance");
            String tn = man.ToString(-3);
            I32 ver = man.ToInteger(-2);
            
            ClassInstance attach = AcquireScriptInstance(man, -1);
            if(!attach)
            {
                TTE_ASSERT("At MetaCreateInstance: attaching instance was null or invalid");
                man.PushNil();
                return 1;
            }
            
            U32 cls = FindClass(Symbol(tn), (U32)ver);
            
            if(cls == 0)
            {
                TTE_LOG("%s with version index %s does not exist", tn.c_str(), ver);
                man.PushNil();
                return 1;
            }
            
            ClassInstance cinst = CreateInstance(cls, attach);
            cinst.PushScriptRef(man);
            
            return 1;
        }
        
        // typename, versionindex = getclass(instance)
        static U32 luaMetaGetClass(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClass");
            
            ClassInstance inst = AcquireScriptInstance(man, -1);
            
            if(inst)
            {
                man.PushLString(Classes[inst.GetClassID()].Name);
                man.PushUnsignedInteger(Classes[inst.GetClassID()].VersionNumber);
            }
            else
            {
                man.PushNil();
                man.PushNil();
            }
            
            return 2;
        }
        
        // bool serialisedef(stream, isntance, is write)
        static U32 luaMetaSerialiseDefault(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaSerialiseDefault");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            Bool isWrite = man.ToBool(-1);
            Stream* stream = ((Stream*)man.ToPointer(-3));
            
            if(stream == nullptr || !inst)
            {
                TTE_ASSERT(false, "At MetaSerialiseDefault: stream and/or instance is null");
                man.PushBool(false);
                return 1;
            }
             
            man.PushBool(_Impl::_DefaultSerialise(*stream,
                            inst, &Classes[inst.GetClassID()], inst._GetInternal(), isWrite)); // perform it
            
            return 1;
        }
        
        // bool serialse(stream, instance, is write)
        static U32 luaMetaSerialise(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaSerialise");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            Bool isWrite = man.ToBool(-1);
            Stream* stream = ((Stream*)man.ToPointer(-3));
            
            if(stream == nullptr || !inst)
            {
                TTE_ASSERT(false, "At MetaSerialise: stream and/or instance is null");
                man.PushBool(false);
                return 1;
            }
            
            man.PushBool(_Impl::_Serialise(*stream,
                            inst, &Classes[inst.GetClassID()], inst._GetInternal(), isWrite)); // perform it
            
            return 1;
        }
        
        // string f(inst)
        static U32 luaMetaToString(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaToString");
            ClassInstance inst = AcquireScriptInstance(man, -1);
            man.PushLString(PerformToString(inst));
            
            return 1;
        }
        
        // bool f(inst, inst)
        static U32 luaMetaLessThan(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaLessThan");
            ClassInstance inst = AcquireScriptInstance(man, -1);
            ClassInstance inst2 = AcquireScriptInstance(man, -2);
            man.PushBool(PerformLessThan(inst2, inst));
            
            return 1;
        }
        
        // bool f(inst, inst)
        static U32 luaMetaEquals(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaEquals");
            ClassInstance inst = AcquireScriptInstance(man, -1);
            ClassInstance inst2 = AcquireScriptInstance(man, -2);
            man.PushBool(PerformEquality(inst2, inst));
            
            return 1;
        }
        
        // copyinstance(instance, attach)
        static U32 luaMetaCopyInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaCopyInstance");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            ClassInstance attach = AcquireScriptInstance(man, -1);
            
            if(!inst || !attach)
            {
                TTE_LOG("Cannot CopyInstance, source or destination instance are null");
                man.PushNil();
                return 1;
            }
            
            ClassInstance cinst = CopyInstance(inst, attach);
            cinst.PushScriptRef(man);
            
            return 1;
        }
        
        // move instance(instance, attach)
        static U32 luaMetaMoveInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaMoveInstance");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            ClassInstance attach = AcquireScriptInstance(man, -1);
            
            if(!inst || !attach)
            {
                TTE_LOG("Cannot MoveInstance, source or destination instance are null");
                man.PushNil();
                return 1;
            }
            
            ClassInstance cinst = MoveInstance(inst, attach);
            cinst.PushScriptRef(man);
            
            return 1;
        }
        
        // iscollection(typename, version index) OR iscollection(instance)
        static U32 luaMetaIsCollection(LuaManager& man)
        {
            if(man.GetTop() == 1)
            {
                ClassInstance inst = AcquireScriptInstance(man, -1);
                man.PushBool(inst ? IsCollection(inst) : false);
            }
            else if(man.GetTop() == 2)
            {
                String tn = man.ToString(-2);
                U32 v = (U32)man.ToInteger(-1);
                U32 cls = FindClass(Symbol(tn), v);
                if(cls)
                    man.PushBool((Classes[cls].Flags & CLASS_CONTAINER) != 0);
                else
                    man.PushBool(false);
            }
            else
            {
                TTE_ASSERT(false, "Incorrect usage of IsCollection");
                man.PushBool(false);
            }
            return 1;
        }
        
    }
    
}

// ===================================================================         META STREAM LUA API
// ===================================================================

namespace MS
{
    
    // read signed integer from stream.
    static U32 luaMetaStreamReadInt(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        I32 val{};
        ::Meta::ClassInstance e{}; // empty
        SerialiseU32(stream, e, 0, &val, false);
        
        man.PushInteger(val);
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteInt(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        I32 val = ScriptManager::PopInteger(man);
        ::Meta::ClassInstance e{}; // empty
        SerialiseU32(stream, e, 0, &val, true);
        
        return 0;
    }
    
    static U32 luaMetaStreamReadByte(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        I8 val{};
        ::Meta::ClassInstance e{}; // empty
        SerialiseU8(stream, e, 0, &val, false);
        
        man.PushInteger((I32)val);
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteByte(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        I32 val = ScriptManager::PopInteger(man);
        I8 wval = val & 0xFF;
        ::Meta::ClassInstance e{}; // empty
        SerialiseU8(stream, e, 0, &val, true);
        
        return 0;
    }
    
    static U32 luaMetaStreamReadShort(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        ::Meta::ClassInstance e{}; // empty
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        I16 val{};
        SerialiseU16(stream, e, 0, &val, false);
        
        man.PushInteger(val);
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteShort(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        I32 val = ScriptManager::PopInteger(man);
        I16 wval = val & 0xFFFF;
        ::Meta::ClassInstance e{}; // empty
        SerialiseU8(stream, e, 0, &val, true);
        
        return 0;
    }
    
    // read string from stream
    static U32 luaMetaStreamReadString(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        String val{};
        ::Meta::ClassInstance e{}; // empty
        ::Meta::_Impl::SerialiseString(stream, e, 0, &val, false);
        
        man.PushLString(std::move(val));
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteString(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        String val = ScriptManager::PopString(man);
        ::Meta::ClassInstance e{}; // empty
        ::Meta::_Impl::SerialiseString(stream, e, 0, &val, true);
        
        return 0;
    }
    
    // read symbol from stream
    static U32 luaMetaStreamReadSymbol(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        Symbol val{};
        ::Meta::ClassInstance e{}; // empty
        ::Meta::_Impl::SerialiseSymbol(stream, e, 0, &val, false);
        man.PushLString(SymbolToHexString(val));
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteSymbol(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        ::Meta::ClassInstance e{}; // empty
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Symbol val = SymbolFromHexString(ScriptManager::PopString(man));
        ::Meta::_Impl::SerialiseSymbol(stream, e, 0, &val, true);
        
        return 0;
    }
    
    // readbuffer(stream, buffer_member, size)
    static U32 luaMetaStreamReadBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaStreamReadBuffer");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-3));
        I32 size = man.ToInteger(-2);
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid buffer");
        TTE_ASSERT(size >= 0 && size < 0x100000, "Buffer size invalid"); // increase size limit?
        
        Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufInst._GetInternal());
        
        if(buf.Buffer)
            TTE_FREE(buf.Buffer);
        buf.Buffer = TTE_ALLOC(size, MEMORY_TAG_RUNTIME_BUFFER);
        buf.BufferSize = (U32)size;
        
        TTE_ASSERT(stream.Read(buf.Buffer, (U64)size), "Binary buffer read fail");
        
        return 0;
    }
    
    // writebuffer(stream, buffer_member)
    static U32 luaMetaStreamWriteBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWriteBuffer");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid buffer");

        Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufInst._GetInternal());
        
        TTE_ASSERT(buf.Buffer && stream.Write(buf.Buffer, (U64)buf.BufferSize), "Binary buffer is null or buffer write fail");
        
        return 0;
    }
    
    // void f(stream)
    static U32 luaMetaStreamSetAsyncSection(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect use of SetAsyncSection");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        
        stream.CurrentSection = Meta::STREAM_SECTION_ASYNC;
        
        return 0;
    }
    
    // void f(stream)
    static U32 luaMetaStreamSetMainSection(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect use of SetMainSection");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        
        stream.CurrentSection = Meta::STREAM_SECTION_MAIN;
        
        return 0;
    }
    
    // void f(stream)
    static U32 luaMetaStreamSetDebugSection(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect use of SetDebugSection");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        
        stream.CurrentSection = Meta::STREAM_SECTION_DEBUG;
        
        return 0;
    }
    
    // nil beginblock(stream, iswrite)
    static U32 luaMetaStreamBeginBlock(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect use of BeginBlock");
        ::Meta::ClassInstance e{}; // empty
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Bool IsWrite = man.ToBool(-1);
        
        if(IsWrite) // in write store position in stream
        {
            stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition());
            U32 zero{};
            SerialiseU32(stream, e, nullptr, &zero, true); // write zero for block size now. will come back after.
        }
        else
        {
            // read. store size + current offset to check after
            U32 size{};
            SerialiseU32(stream, e, nullptr, &size, false); // read block size (4 is added to it).
            stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition() + size - 4);
        }
        
        return 0;
    }
    
    // nil endblock(stream, iswrite)
    static U32 luaMetaStreamEndBlock(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect use of EndBlock");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Bool IsWrite = man.ToBool(-1);
        ::Meta::ClassInstance e{}; // empty
        
        if(IsWrite) // in write store position in stream
        {
            U64 pos = stream.Sect[stream.CurrentSection].Blocks.back();
            stream.Sect[stream.CurrentSection].Blocks.pop_back();
            
            U32 blockSize = (U32)(stream.Sect[stream.CurrentSection].Data->GetPosition() - pos);
            U64 currentPos = stream.Sect[stream.CurrentSection].Data->GetPosition(); // cache current position
            
            stream.Sect[stream.CurrentSection].Data->SetPosition(pos); // go back to block offset
            SerialiseU32(stream, e, nullptr, &blockSize, true); // write it
            stream.Sect[stream.CurrentSection].Data->SetPosition(currentPos); // go back to normal. done
        }
        else
        {
            U64 pos = stream.Sect[stream.CurrentSection].Blocks.back();
            stream.Sect[stream.CurrentSection].Blocks.pop_back();
            if(pos != stream.Sect[stream.CurrentSection].Data->GetPosition())
            {
                TTE_ASSERT(false, "Block size mismatch after EndBlock() call from serialisation script");
                return false;
            }
        }
        
        return 0;
    }
    
    // typename, versionindex = Metastreamfindclass(metastream, type_symbol/type_name). uses version info header. dont use if writing!
    // if writing, use normal MetaGetClass. use this for version checking, to get the correct one according to the version header.
    static U32 luaMetaStreamFindClass(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect use of MSFindClass");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        String str = man.ToString(-1);
        
        Symbol tnSymbol = SymbolFromHexString(str);
        
        if(tnSymbol.GetCRC64() == 0) // not a hash, so hash it
        {
            tnSymbol = Symbol(str);
        }
        
        for(U32 i = 0; i < MAX_VERSION_NUMBER; i++) // try all valid version indices
        {
            
            auto it = ::Meta::Classes.find(::Meta::FindClass(tnSymbol.GetCRC64(), i));
            
            if(it != ::Meta::Classes.end()) // the class is valid, test header
            {
                
                Bool Found = (it->second.Flags & ::Meta::CLASS_INTRINSIC) != 0; // if intrinsic its not in the header, so we are ok
                
                if(!Found) // try and find it in the meta stream header
                {
                    for(auto& ver: stream.VersionInf)
                    {
                        if(ver == it->second.ClassID)
                        {
                            Found = true; // found matching version
                            break;
                        }
                    }
                }
                
                // If success
                if(Found)
                {
                    man.PushLString(it->second.Name);
                    man.PushInteger(i);
                    return 2;
                }
            }
        }
        
        // not found
        man.PushNil();
        man.PushInteger(0);
        return 2;
    }
    
}

namespace LuaMisc
{
    
    // string SymbolFind(symbol string)
    static U32 luaSymbolFind(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage for SymbolFind")
        Symbol sym = SymbolFromHexString(man.ToString(-1));
        man.PushLString(RuntimeSymbols.Find(sym));
        return 1;
    }
    
    // nil SymbolRegister(symbol string)
    static U32 luaSymbolReg(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage for SymbolRegister")
        RuntimeSymbols.Register(man.ToString(-1));
        return 0;
    }
    
    static U32 luaSymbolClear(LuaManager&)
    {
        RuntimeSymbols.Clear();
        return 0;
    }
    
}

// ===================================================================         TTE API
// ===================================================================

namespace TTE
{
    
    static Bool EnsureMain()
    {
        if(!IsCallingFromMain())
        {
            TTE_ASSERT(false, "This function cannot be called here"); // 90% this might be running on main, but it shouldn't.
            // these functions should only be run in mod scripts, not any script which could be async (ie in meta serialisation etc)
            return false;
        }
        return true;
    }
    
    static U32 luaSwitch(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Invalid use of TTE_Switch");
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_Switch: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context)
            return 0;
        if(Context->GetLockDepth() != 0)
        {
            TTE_ASSERT(false, "At TTE_Switch: lock depth is non zero. This function cannot be called at this time.");
            return 0;
        }
        if(EnsureMain())
        {
            GameSnapshot snap{};
            snap.ID = man.ToString(-3);
            snap.Platform = man.ToString(-2);
            snap.Vendor = man.ToString(-1);
            Context->Switch(snap);
        }
        return 0;
    }
    
    // instance = openms(filepath, optional arc). this instance is a strong reference, in which the lua GC, when called, will finally destroy it.
    static U32 luaOpenMetaStream(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1 || man.GetTop() == 2, "Invalid use of TTE_OpenMetaStream");
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_OpenMetaStream: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context)
        {
            man.PushNil();
            return 1;
        }
        if(EnsureMain())
        {
            String path = man.ToString(1);
            DataStreamRef r;
            if(man.GetTop() == 1)
                r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
            else
            {
                // open from archive
                U32 tag = ScriptManager::GetScriptObjectTag(man, 2);
                if(tag == TTARCHIVE1)
                    r = ((TTArchive*)man.ToPointer(2))->Find(path); // open stream
                else
                {
                    TTE_LOG("At TTE_OpenMetaStream: invalid argument(s()");
                    man.PushNil();
                    return 1;
                }
            }
            if(r && r->GetSize() > 0)
            {
                Meta::ClassInstance inst = Meta::ReadMetaStream(r);
                if(inst)
                {
                    inst.PushScriptRef(man, true); // set to strong, as otherwise when 'inst' destructor is called, we will exit.`
                }
                else
                {
                    TTE_LOG("Failed to read meta stream for file %s", path.c_str());
                    man.PushNil();
                }
            }
            else
            {
                TTE_LOG("Failed to open meta stream for file %s: did not exist (size 0) or could not open", path.c_str());
                man.PushNil();
            }
        }
        return 1;
    }
    
    // bool = savems(filepath, instance)
    static U32 luaSaveMetaStream(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Invalid use of TTE_SaveMetaStream");
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_SaveMetaStream: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context)
        {
            man.PushBool(false);
            return 1;
        }
        if(EnsureMain())
        {
            String path = man.ToString(-1);
            DataStreamRef r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
            Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, -1);
            if(inst)
            {
                if(Meta::WriteMetaStream(path, std::move(inst), r, Context->GetActiveGame()->MetaVersion))
                    man.PushBool(true);
                else
                {
                    TTE_LOG("Cannot save meta stream to %s: write failed", path.c_str());
                    man.PushBool(false);
                }
            }
            else
            {
                TTE_LOG("Cannot save meta stream to %s: instance is not valid or null", path.c_str());
                man.PushBool(false);
            }
        }
        return 1;
    }
    
    static U32 luaActiveGame(LuaManager& man)
    {
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_ActiveGame: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context || !Context->GetActiveGame())
        {
            man.PushNil();
            return 1;
        }
        const Meta::RegGame* pGame = Context->GetActiveGame();
        
        man.PushTable();
        
        man.PushLString("Name");
        man.PushLString(pGame->Name);
        man.SetTable(-3);
        
        man.PushLString("ID");
        man.PushLString(pGame->ID);
        man.SetTable(-3);
        
        man.PushLString("ArchiveVersion");
        man.PushUnsignedInteger(pGame->ArchiveVersion);
        man.SetTable(-3);
        
        man.PushLString("IsArchive2");
        man.PushBool(pGame->UsesArchive2);
        man.SetTable(-3);
        
        return 1;
    }
    
    // arc = openarchive2(path)
    static U32 luaOpenTTArch2(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid use of TTE_OpenTTArchive2");
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_OpenTTArchive2: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context)
        {
            man.PushNil();
            return 1;
        }
        
        if(!Context->GetActiveGame()->UsesArchive2)
        {
            TTE_ASSERT(false, "At TTE_OpenArchive2: please use TTE_OpenArchive as the current game does not use .TTARCH, but rather .TTARCH2.");
            man.PushNil();
            return 1;
        }
        
        String path = man.ToString(-1);
        DataStreamRef r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
        
        if(r->GetSize() > 0)
        {
            TTArchive2* pArchive = TTE_NEW(TTArchive2, MEMORY_TAG_SCRIPT_OBJECT, Context->GetActiveGame()->ArchiveVersion);
            TTE_LOG("Loading telltale archive 2 %s...", path.c_str());
            if(!pArchive->SerialiseIn(r))
            {
                TTE_LOG("Cannot open archive %s: read failed (archive format invalid)", path.c_str());
                man.PushNil();
                TTE_DEL(pArchive);
            }else ScriptManager::PushScriptOwned(man, pArchive, TTARCHIVE2); // gc will call del
        }
        else
        {
            TTE_LOG("Cannot open archive %s: not found or could not open", path.c_str());
            man.PushNil();
        }
        
        return 1;
    }
    
    // arc = openarchive(path)
    static U32 luaOpenTTArch(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid use of TTE_OpenTTArchive");
        ToolContext* Context = ::GetToolContext();
        TTE_ASSERT(Context, "At TTE_OpenTTArchive: no context is present. Ensure any modding scripts are run after context initialisation.");
        if(!Context)
        {
            man.PushNil();
            return 1;
        }
        
        if(Context->GetActiveGame()->UsesArchive2)
        {
            TTE_ASSERT(false, "At TTE_OpenArchive: please use TTE_OpenArchive2 as the current game does not use .TTARCH2, but rather .TTARCH.");
            man.PushNil();
            return 1;
        }
    
        String path = man.ToString(-1);
        DataStreamRef r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
        
        if(r->GetSize() > 0)
        {
            TTArchive* pArchive = TTE_NEW(TTArchive, MEMORY_TAG_SCRIPT_OBJECT, Context->GetActiveGame()->ArchiveVersion);
            TTE_LOG("Loading telltale archive %s...", path.c_str());
            if(!pArchive->SerialiseIn(r))
            {
                TTE_LOG("Cannot open archive %s: read failed (archive format invalid)", path.c_str());
                man.PushNil();
                TTE_DEL(pArchive);
            }else ScriptManager::PushScriptOwned(man, pArchive, TTARCHIVE1); // gc will call del
        }
        else
        {
            TTE_LOG("Cannot open archive %s: not found or could not open", path.c_str());
            man.PushNil();
        }
        
        return 1;
    }
    
    // files = listfiles(archive)
    static U32 luaArchiveListFiles(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid use of TTE_ArchiveListFiles");
        // open from archive
        U32 tag = ScriptManager::GetScriptObjectTag(man, -1);
        if(tag == TTARCHIVE1)
        {
            TTArchive* pArchive = (TTArchive*)man.ToPointer(-1);
            man.PushTable();
            std::vector<String> files{};
            pArchive->GetFiles(files);
            U32 i = 1;
            for(auto& it: files)
            {
                man.PushUnsignedInteger(i++);
                man.PushLString(std::move(it)); // move
                man.SetTable(-3);
            }
        }else if(tag == TTARCHIVE2)
        {
            TTArchive2* pArchive = (TTArchive2*)man.ToPointer(-1);
            man.PushTable();
            std::vector<String> files{};
            pArchive->GetFiles(files);
            U32 i = 1;
            for(auto& it: files)
            {
                man.PushUnsignedInteger(i++);
                man.PushLString(std::move(it)); // move
                man.SetTable(-3);
            }
        }
        else
        {
            TTE_LOG("At TTE_ArchiveListFiles: invalid argument(s()");
            man.PushNil();
            return 1;
        }
        return 1;
    }
    
}


// ===================================================================         REGISTER OBJECT
// ===================================================================

#define ADD_FN(namespace, name, fun) Col.Functions.push_back({name, &namespace :: fun})

LuaFunctionCollection luaLibraryAPI()
{
    LuaFunctionCollection Col{};
    
    // REGISTER TTE API
    
    Col.Functions.push_back({"TTE_Switch", &TTE::luaSwitch});
    Col.Functions.push_back({"TTE_OpenMetaStream", &TTE::luaOpenMetaStream});
    Col.Functions.push_back({"TTE_SaveMetaStream", &TTE::luaSaveMetaStream});
    Col.Functions.push_back({"TTE_GetActiveGame", &TTE::luaActiveGame});
    Col.Functions.push_back({"TTE_OpenTTArchive", &TTE::luaOpenTTArch});
    Col.Functions.push_back({"TTE_OpenTTArchive2", &TTE::luaOpenTTArch2});
    Col.Functions.push_back({"TTE_ArchiveListFiles", &TTE::luaArchiveListFiles});
    
    // REGISTER META API
    
    Col.Functions.push_back({"MetaRegisterGame",&Meta::L::luaMetaRegisterGame});
    Col.Functions.push_back({"MetaRegisterIntrinsics", &Meta::L::luaMetaRegisterIntrinsics});
    Col.Functions.push_back({"MetaRegisterClass", &Meta::L::luaMetaRegisterClass});
    Col.Functions.push_back({"MetaRegisterCollection", &Meta::L::luaRegisterCollection}); // used in Meta::Initialise4 in engine
    Col.Functions.push_back({"MetaGetClassID", &Meta::L::luaMetaGetId});
    Col.Functions.push_back({"MetaGetVersionCRC", &Meta::L::luaMetaGetVersion});
    Col.Functions.push_back({"MetaGetClassHash", &Meta::L::luaMetaGetClassHash});
    Col.Functions.push_back({"MetaFlagQuery", &Meta::L::luaMetaFlagQuery});
    Col.Functions.push_back({"MetaHashByte", &Meta::L::luaMetaHashByte});
    Col.Functions.push_back({"MetaHashInt", &Meta::L::luaMetaHashInt});
    Col.Functions.push_back({"MetaHashLong", &Meta::L::luaMetaHashLong});
    Col.Functions.push_back({"MetaHashHexString", &Meta::L::luaMetaHashHexString});
    Col.Functions.push_back({"MetaHashString", &Meta::L::luaMetaHashString});
    Col.Functions.push_back({"MetaSetVersionFn", &Meta::L::luaMetaSetVersionCalc});
    
    // RUNTIME META API
    
    ADD_FN(Meta::L, "MetaGetClassFlags", luaMetaGetClassFlags);
    ADD_FN(Meta::L, "MetaGetMemberFlags", luaMetaGetMemberFlags);
    ADD_FN(Meta::L, "MetaGetClass", luaMetaGetClass);
    ADD_FN(Meta::L, "MetaGetMemberClass", luaMetaGetMemberClass);
    ADD_FN(Meta::L, "MetaGetMember", luaMetaGetMember);
    ADD_FN(Meta::L, "MetaGetMemberNames", luaMetaGetMemberNames);
    ADD_FN(Meta::L, "MetaCreateInstance", luaMetaCreateInstance);
    ADD_FN(Meta::L, "MetaCopyInstance", luaMetaCopyInstance);
    ADD_FN(Meta::L, "MetaMoveInstance", luaMetaMoveInstance);
    ADD_FN(Meta::L, "MetaIsCollection", luaMetaIsCollection);
    ADD_FN(Meta::L, "MetaToString", luaMetaToString);
    ADD_FN(Meta::L, "MetaEquals", luaMetaEquals);
    ADD_FN(Meta::L, "MetaLessThan", luaMetaLessThan);
    
    // META STREAM API
    
    ADD_FN(MS, "MetaStreamReadInt", luaMetaStreamReadInt);
    ADD_FN(MS, "MetaStreamReadByte", luaMetaStreamReadByte);
    ADD_FN(MS, "MetaStreamReadShort", luaMetaStreamReadShort);
    ADD_FN(MS, "MetaStreamReadString", luaMetaStreamReadString);
    ADD_FN(MS, "MetaStreamReadSymbol", luaMetaStreamReadSymbol);
    ADD_FN(MS, "MetaStreamWriteInt", luaMetaStreamWriteInt);
    ADD_FN(MS, "MetaStreamWriteByte", luaMetaStreamWriteByte);
    ADD_FN(MS, "MetaStreamWriteShort", luaMetaStreamWriteShort);
    ADD_FN(MS, "MetaStreamWriteString", luaMetaStreamWriteString);
    ADD_FN(MS, "MetaStreamWriteSymbol", luaMetaStreamWriteSymbol);
    ADD_FN(MS, "MetaStreamSetMainSection", luaMetaStreamSetMainSection);
    ADD_FN(MS, "MetaStreamSetAsyncSection", luaMetaStreamSetAsyncSection);
    ADD_FN(MS, "MetaStreamSetDebugSection", luaMetaStreamSetDebugSection);
    ADD_FN(MS, "MetaStreamBeginBlock", luaMetaStreamBeginBlock);
    ADD_FN(MS, "MetaStreamEndBlock", luaMetaStreamEndBlock);
    ADD_FN(MS, "MetaStreamFindClass", luaMetaStreamFindClass);
    ADD_FN(Meta::L, "MetaSerialiseDefault", luaMetaSerialiseDefault); // serialise related
    ADD_FN(Meta::L, "MetaSerialise", luaMetaSerialise);
    
    // MISC
    
    ADD_FN(LuaMisc, "SymbolTableFind", luaSymbolFind);
    ADD_FN(LuaMisc, "SymbolTableRegister", luaSymbolReg);
    ADD_FN(LuaMisc, "SymbolTableClear", luaSymbolClear);
    
    return Col;
}
