#include <Meta/Meta.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Core/Context.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp>
#include <Core/Base64.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <sstream>
#include <utility>
#include <set>
#include <sstream>
#include <algorithm>

extern thread_local Bool _IsMain;

// ===================================================================         META LUA API
// ===================================================================

namespace Meta
{
    
    // access meta.cpp
    extern InternalState State;
    
    namespace L { // Lua functions
        
        // helper
        static BlowfishKey luaToKey(String hexkey)
        {
            BlowfishKey ret{};
            TTE_ASSERT((hexkey.length() & 1) == 0, "Hexadecimal encryption key length is not a multiple of two!");
            
            U32 len = (U32)hexkey.length();
            ret.BfKeyLength = len >> 1;
            TTE_ASSERT(ret.BfKeyLength < 56, "Provided encryption key"
                       " is too large at %d bytes. Maximum 56.", ret.BfKeyLength);
            
            for(U32 i = 0; i < len; i += 2) // convert hex string to max 56 byte buffer
            {
                String byt = hexkey.substr(i,2);
                ret.BfKey[i >> 1] = (U8)strtol(byt.c_str(), nullptr, 0x10);
            }
            return ret;
        }
        
        // assign(gameName, mask, foldername)
        U32 luaAssignFolderExt(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaAssociateFolderExtension can only be called in initialisation on the main thread");
            if(!GetToolContext() || GetToolContext()->GetActiveGame()) // no active game can be set here
            {
                TTE_ASSERT(false, "At MetaAssociateFolderExtension: this can only be called during initialisation of a game");
                return 0;
            }
            TTE_ASSERT(man.GetTop() == 3, "MetaAssociateFolderExtension: invalid number of argumenst, expected 3");
            
            String game = man.ToString(1);
            String folder = man.ToString(3);
            String ext = man.ToString(2);
            TTE_ASSERT(folder.length(), "MetaAssociateFolderExtension: folder name empty")
            
            for(auto& gameit : State.Games)
            {
                if(CompareCaseInsensitive(gameit.ID, game))
                {
                    std::replace(folder.begin(), folder.end(), '\\', '/');
                    if(folder[folder.length() - 1] != '/')
                    {
                        folder += '/';
                    }
                    gameit.FolderAssociates.insert(std::make_pair(std::move(ext), std::move(folder)));
                    return 0;
                }
            }
            
            TTE_ASSERT(false, "When registering %s => %s, the game %s has not been registered", ext.c_str(), folder.c_str(), game.c_str());
            
            return 0;
        }

        U32 luaMetaPushHash(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "Can only be called in initialisation on the main thread");
            if (!GetToolContext() || GetToolContext()->GetActiveGame()) // no active game can be set here
            {
                TTE_ASSERT(false, "This function can only be called during initialisation of a game");
                return 0;
            }
            man.PushLString("HashMap");
            man.GetTable(1);
            if (man.Type(-1) != LuaType::TABLE)
            {
                man.Pop(1);
                man.PushLString("HashMap");
                man.PushTable();
                man.SetTable(1);
                man.PushLString("HashMap");
                man.GetTable(1);
            }
            // game tab, hash str, platform, vendor, hash tab
            String hash = man.ToString(2);
            String plat = man.ToString(3);
            String vend = man.ToString(4);
            U64 hashVal = (U64)std::stoull(hash, nullptr, 16);
            if(hashVal == 0)
            {
                TTE_LOG("WARNING: Hash %s is invalid when pushing game hash", hash.c_str());
            }
            else
            {
                man.PushLString(hash);
                man.PushTable();
                man.PushLString("Platform");
                man.PushLString(plat);
                man.SetTable(-3);
                man.PushLString("Vendor");
                man.PushLString(vend);
                man.SetTable(-3);
                man.SetTable(-3);
            }
            return 0;
        }
        
        U32 luaMetaPushGameCap(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "Can only be called in initialisation on the main thread");
            if(!GetToolContext() || GetToolContext()->GetActiveGame()) // no active game can be set here
            {
                TTE_ASSERT(false, "This function can only be called during initialisation of a game");
                return 0;
            }
            man.PushLString("Caps");
            man.GetTable(1);
            if(man.Type(-1) != LuaType::TABLE)
            {
                man.Pop(1);
                man.PushLString("Caps");
                man.PushTable();
                man.SetTable(1);
                man.PushLString("Caps");
                man.GetTable(1);
            }
            
            // gametab, one, .., N, capstab
            I32 tryIndex = 1;
            for(I32 arg = 2; arg < man.GetTop(); arg++)
            {
                for(;;)
                {
                    man.PushInteger(tryIndex);
                    man.GetTable(-2);
                    LuaType ty = man.Type(-1);
                    man.Pop(1);
                    if(ty == LuaType::NIL)
                        break;
                    tryIndex++;
                }
                man.PushInteger(tryIndex);
                man.PushCopy(arg);
                man.SetTable(-3);
            }
            return 0;
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
            ScriptManager::TableGet(man, "ResourceSetMask");
            if(man.Type(-1) == LuaType::STRING)
                reg.ResourceSetDescMask = ScriptManager::PopString(man);
            else
                man.Pop(1);
            ScriptManager::TableGet(man, "IsArchive2");
            if(ScriptManager::PopBool(man))
                reg.Fl.Add(RegGame::ARCHIVE2);
            ScriptManager::TableGet(man, "ArchiveVersion");
            if(man.Type(-1) == LuaType::TABLE)
            {
                man.PushNil(); // set for each table
                while (man.TableNext(-2) != 0)
                {
                    TTE_ASSERT(man.Type(-2) == LuaType::STRING && man.Type(-1) == LuaType::NUMBER, "Archive version table invalid");

                    man.PushCopy(-2); // push copy of key

                    U32 archive = (U32)man.ToInteger(-2);
                    String platformName = man.ToString(-1); // more like snapshot ID
                    reg.SnapToArchiveVersion[platformName] = archive;

                    man.Pop(2);
                }
                man.Pop(1); // pop last key
            }
            else
            {
                reg.MasterArchiveVersion = ScriptManager::PopUnsignedInteger(man);
            }
            ScriptManager::TableGet(man, "ModifiedEncryption");
            if(man.Type(-1) == LuaType::BOOL)
            {
                if(ScriptManager::PopBool(man))
                    reg.Fl.Add(RegGame::MODIFIED_BLOWFISH);
            } // [Gitbook Documentation](https://telltale-editor.gitbook.io/documentation-for-the-telltale-editor
            else
            {
                man.Pop(1);
            }
            
            ScriptManager::TableGet(man, "Caps");
            if(man.Type(-1) == LuaType::TABLE)
            {
                I32 index = 1;
                for(;;)
                {
                    man.PushInteger(index);
                    man.GetTable(-2);
                    if(man.Type(-1) != LuaType::NUMBER)
                    {
                        man.Pop(1);
                        break;
                    }
                    reg.Caps.Set((GameCapability)man.ToInteger(-1), true);
                    man.Pop(1);
                    index++;
                }
            }
            man.Pop(1);

            ScriptManager::TableGet(man, "HashMap");
            if (man.Type(-1) == LuaType::TABLE)
            {
                ITERATE_TABLE(it, -1)
                {
                    String hash = man.ToString(it.KeyIndex());
                    man.PushLString("Platform");
                    man.GetTable(it.ValueIndex());
                    String plat = ScriptManager::PopString(man);
                    man.PushLString("Vendor");
                    man.GetTable(it.ValueIndex());
                    String vend = ScriptManager::PopString(man);
                    U64 hashVal = (U64)std::stoull(hash, nullptr, 16);
                    reg.ExecutableHash[hashVal] = std::make_pair(plat, vend);
                }
            }
            man.Pop(1);
            
            ScriptManager::TableGet(man, "AllowOodle");
            if(man.Type(-1) == LuaType::BOOL)
            {
                if(ScriptManager::PopBool(man))
                    reg.Fl.Add(RegGame::ENABLE_OODLE);
            }
            else
            {
                man.Pop(1);
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
                    reg.SnapToEncryptionKey[platformName] = luaToKey(key);
                    
                    man.Pop(2);
                }
                man.Pop(1); // pop last key
            }
            else man.Pop(1); // no key given, skip
            
            ScriptManager::TableGet(man, "Platforms");
            if(man.Type(-1) == LuaType::STRING)
            {
                String val = ScriptManager::PopString(man);
                std::stringstream ss(val);
                String token{};
                while (std::getline(ss, token, ';'))
                {
                    TTE_ASSERT(_Impl::_CheckPlatform(token), "The platform %s is not valid", token.c_str());
                    reg.ValidPlatforms.push_back(token);
                }
            }
            else
            {
                TTE_ASSERT(false, "Game table must specify Platform as a string");
                man.Pop(1);
                return 0;
            }
            
            ScriptManager::TableGet(man, "Vendors");
            if(man.Type(-1) == LuaType::STRING)
            {
                String val = StringTrim(ScriptManager::PopString(man));
                if(val.length() == 0)
                {
                    reg.ValidVendors.push_back(""); // default vendor is empty
                }
                else
                {
                    std::stringstream ss(val);
                    String token{};
                    while (std::getline(ss, token, ';'))
                    {
                        if(token.length() == 0)
                        {
                            TTE_ASSERT(false, "At MetaRegisterGame: %s: vendor array invalid", reg.Name.c_str());
                            return 0;
                        }
                        reg.ValidVendors.push_back(token);
                    }
                }
            }
            else
            {
                TTE_ASSERT(false, "At MetaRegisterGame: %s: vendor array not specified",
                           reg.Name.c_str());
                man.Pop(1);
                return 0;
            }

            if(reg.ValidVendors[0].length())
            {
                ScriptManager::TableGet(man, "DefaultVendor");
                if (man.Type(-1) == LuaType::STRING)
                {
                    reg.DefaultVendor = ScriptManager::PopString(man);
                    if(std::find(reg.ValidVendors.begin(), reg.ValidVendors.end(), reg.DefaultVendor) == reg.ValidVendors.end())
                    {
                        TTE_ASSERT(false, "At MetaRegisterGame: %s: the default vendor %s is not a valid vendor specified in the vendors array",
                                   reg.Name.c_str());
                        return 0;
                    }
                }
                else
                {
                    TTE_ASSERT(false, "At MetaRegisterGame: %s: the default vendor must be specified here!", reg.Name.c_str());
                    man.Pop(1);
                    return 0;
                }
            }

            ScriptManager::TableGet(man, "CommonSelector");
            if(man.Type(-1) == LuaType::STRING)
            {
                reg.CommonSelector = ScriptManager::PopString(man);
            }
            else
            {
                reg.CommonSelector = "";
                man.Pop(1); // OK, just use normal version number 0
            }
            
            ScriptManager::TableGet(man, "LuaVersion");
            String v = ScriptManager::PopString(man);
            reg.LVersion = v == "5.0.2" ? LuaVersion::LUA_5_0_2 : v == "5.1.4" ? LuaVersion::LUA_5_1_4 : LuaVersion::LUA_5_2_3;
            
            ScriptManager::TableGet(man, "DefaultMetaVersion");
            v = ScriptManager::PopString(man);
            
            reg.MetaVersion = v == "MSV6" ? MSV6 : v == "MSV5" ? MSV5 : v == "MSV4" ? MSV4 : v == "MCOM" ? MCOM : v == "MTRE" ? MTRE : v == "MBIN" ? MBIN : v == "MBES" ? MBES : (StreamVersion)-1;
            
            TTE_ASSERT(reg.MetaVersion != (StreamVersion)-1, "Invalid meta version");
            
            State.Games.push_back(std::move(reg));
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
        static String GenerateToString(Class*, const void* pIntrin)
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
        
        static String StringToStringOperation(Class*, const void* pStr) // to string operation for string... returns the string
        {
            return *((const String*)pStr);
        }
        
        static String SymbolToStringOperation(Class*,const void* pSym)
        {
            U64 hash = ((const Symbol*)pSym)->GetCRC64();
            String s = SymbolTable::Find(hash); // try find it first
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
            
            REGISTER_INTRINSIC(U32,     "uint32", "kMetaUInt32", 4, &SerialiseU32);
            REGISTER_INTRINSIC(I32,     "int", "kMetaInt", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U32,     "uint", "kMetaUInt", 4, &SerialiseU32); // alias (obvious 32)
            REGISTER_INTRINSIC(I32,     "int32", "kMetaInt32", 4, &SerialiseU32); // alias
            REGISTER_INTRINSIC(U64,     "uint64", "kMetaUInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I64,     "int64", "kMetaInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(U64,     "unsigned __int64", "kMeta__UnsignedInt64", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I32,     "long", "kMetaLong", 4, &SerialiseU32); // alias for int32 on most windows
            REGISTER_INTRINSIC(float,   "float", "kMetaFloat", 4, &SerialiseU32); // serialise it as U32
            REGISTER_INTRINSIC(double,  "double", "kMetaDouble", 8, &SerialiseU64); // serialise as U64
            REGISTER_INTRINSIC(bool,    "bool", "kMetaBool", 1, &_Impl::SerialiseBool);
            REGISTER_INTRINSIC(I8,      "int8", "kMetaInt8", 1, &SerialiseU8);
            REGISTER_INTRINSIC(U8,      "uint8", "kMetaUnsignedInt8", 1, &SerialiseU8);
            REGISTER_INTRINSIC(I16,     "int16", "kMetaInt16", 2, &SerialiseU16);
            REGISTER_INTRINSIC(U16,     "uint16", "kMetaUnsignedInt16", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I8,      "signed char", "kMetaSignedChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(I8,      "char", "kMetaChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(double,  "long double", "kMetaLongDoubler", 8, &SerialiseU64); // alias for double
            REGISTER_INTRINSIC(U8,      "unsigned char", "kMetaUnsignedChar", 1, &SerialiseU8);
            REGISTER_INTRINSIC(U32,     "unsigned int", "kMetaUnsignedInt", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U32,     "unsigned long", "kMetaUnsignedLong", 4, &SerialiseU32);
            REGISTER_INTRINSIC(U16,     "unsigned short", "kMetaUnsignedShort", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I16,     "short", "kMetaShort", 2, &SerialiseU16);
            REGISTER_INTRINSIC(wchar_t, "wchar_t", "kMetaWideChar", 2, &SerialiseU16);
            REGISTER_INTRINSIC(I64,     "long long", "kMetaLongLong", 8, &SerialiseU64);
            REGISTER_INTRINSIC(U64,     "unsigned long long", "kMetaUnsignedLongLong", 8, &SerialiseU64);
            REGISTER_INTRINSIC(I64,     "__int64", "kMeta__Int64", 8, &SerialiseU64); // alias for uint64
            
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
            c.Name = String("class String");
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
            
            // data stream cache internal
            c.Flags = CLASS_INTRINSIC | CLASS_NON_BLOCKED; // keep it intrinsic, so it does not go into meta headers. ensure no block size (invis)
            c.Name = "__INTERNAL_DATASTREAM_CACHE__"; // internal class.
            c.RTSize = (U32)sizeof(DataStreamCache);
            c.Constructor = &_Impl::CtorDSCache;
            c.Destructor = &_Impl::DtorDSCache;
            c.CopyConstruct = &_Impl::CopyDSCache;
            c.MoveConstruct = &_Impl::MoveDSCache;
            AddIntrinsic(man, "kMetaClassInternalDataStreamCache", "__INTERNAL_DATASTREAM_CACHE__", std::move(c));
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
                    
                    // read flag/enum names
                    if(m.Flags & MEMBER_ENUM || m.Flags & MEMBER_FLAG)
                    {
                        ScriptManager::TableGet(man, m.Flags & MEMBER_ENUM ? "EnumInfo" : "FlagInfo");
                        if(man.Type(-1) == LuaType::TABLE)
                        {
                            // iterate over enum / flag info tabl
                            man.PushNil();
                            while(man.TableNext(-2) != 0)
                            {
                                if(man.Type(-1) != LuaType::TABLE)
                                {
                                    continue; // ignore
                                }
                                
                                EnumFlag Descriptor{};
                                
                                ScriptManager::TableGet(man, "Name");
                                Descriptor.Name = ScriptManager::PopString(man);
                                
                                ScriptManager::TableGet(man, "Value");
                                Descriptor.Value = ScriptManager::PopInteger(man);
                                
                                m.Descriptors.push_back(std::move(Descriptor)); // add it
                                
                                man.Pop(1); // next
                                
                            }
                        }
                        man.Pop(1);
                    }
                    
                    ScriptManager::TableGet(man, "Class");
                    if(man.Type(-1) != LuaType::TABLE)
                    {
                        TTE_LOG("ERROR registering type %s: member '%s' class must be a pre-registered table", c.Name.c_str(), m.Name.c_str());
                        man.Pop(3);
                        return false;
                    }
                    else
                    {
                        man.PushLString("__MetaId");
                        man.GetTable(-2);
                        U32 memberType = (U32)ScriptManager::PopInteger(man);
                        if(State.Classes.find(memberType) == State.Classes.end()){
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
        
        // table = regdup(origin, name, versionnumber)
        static U32 luaMetaRegisterDuplicate(LuaManager& man)
        {
            TTE_ASSERT(_IsMain, "MetaRegisterXXX can only be called in snapshot initialisation on the main thread");
            
            if(!GetToolContext() || GetToolContext()->GetActiveGame())
            {
                TTE_ASSERT(false, "At MetaRegisterXXX: this can only be called during initialisation of a game");
                man.PushNil();
                return 1;
            }
            
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaRegisterDuplicate");
            
            U32 n = (U32)man.ToInteger(-1);
            String name = man.ToString(-2);
            
            if(man.Type(1) != LuaType::TABLE)
            {
                TTE_LOG("Duplicate source type is not a table");
                man.PushNil();
                return 1;
            }
            
            man.PushLString("__MetaId");
            man.GetTable(1);
            U32 cls = ScriptManager::PopUnsignedInteger(man);
            
            if(State.Classes.find(cls) == State.Classes.end())
            {
                TTE_LOG("At MetaRegisterDuplicate: previously not registered origin class");
                man.PushNil();
                return 1;
            }
            
            if(FindClass(Symbol(name).GetCRC64(), n, true) != 0)
            {
                TTE_LOG("At MetaRegisterDuplicate: new duplicated class name and version number already exist (%s:%u)", name.c_str(), n);
                man.PushNil();
                return 1;
            }
            
            Class dup = State.Classes[cls];
            dup.VersionNumber = n;
            dup.Name = name;
            dup.TypeHash = 0;
            dup.RTSize = 0;
            dup.VersionCRC = 0; // reset back
            
            U32 id = _Impl::_Register(man, std::move(dup), 1);
            
            man.PushTable(); // return value
            man.PushLString("__MetaId");
            man.PushUnsignedInteger(id);
            man.SetTable(-3);
            
            return 1;
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
            String name = c.Name = ScriptManager::PopString(man);
            ScriptManager::TableGet(man, "Extension");
            if(man.Type(-1) == LuaType::STRING)
                c.Extension = ScriptManager::PopString(man);
            else
                man.Pop(1);
            TTE_ASSERT(c.VersionNumber <= MAX_VERSION_NUMBER, "%s: Version number cannot be larger than %d", c.Name.c_str(), MAX_VERSION_NUMBER);
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
            
            ScriptManager::TableGet(man, "Normaliser");
            if(man.Type(-1) == LuaType::STRING)
                c.NormaliserStringFn = ScriptManager::PopString(man);
            else
                man.Pop(1);
            
            ScriptManager::TableGet(man, "Specialiser");
            if(man.Type(-1) == LuaType::STRING)
                c.SpecialiserStringFn = ScriptManager::PopString(man);
            else
                man.Pop(1);
            
            if(!luaHelperRegisterMembers(c, man))
                return 0;
            
            U32 id = _Impl::_Register(man, std::move(c), man.GetTop());
            
            ScriptManager::TableGet(man, "VersionOverride");
            if(man.Type(-1) == LuaType::STRING)
            {
                U32 crc = 0;
                std::stringstream ss;
                ss << std::hex << ScriptManager::PopString(man);
                ss >> crc;
                Class* pClass = _Impl::_GetClass(id);
                TTE_ASSERT(crc != pClass->VersionCRC, "Version CRCs already match for %s!",name.c_str()); // GOOD
                State._Temp.DeferredWarnings.push_back({pClass->Name, pClass->VersionCRC, crc});
                pClass->VersionCRC = crc;
                
            }
            else
                man.Pop(1);
            
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
            
            InternalState::DeferredRegister def{};
            
            if(man.Type(2) == LuaType::NUMBER) // SArray
                c.ArraySize = man.ToInteger(2);
            else if(man.Type(2) == LuaType::TABLE)
            {
                man.PushLString("__MetaId");
                man.GetTable(2);
                TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "When registering collection, key type table not registered properly");
                c.ArrayKeyClass = (U32)ScriptManager::PopInteger(man);
            }
            else if(man.Type(2) == LuaType::STRING) // defer it
            {
                String val = man.ToString(2);
                auto semi = val.find_last_of(';');
                if(semi != String::npos)
                {
                    def.KeyVersion = (U32)std::stoi(val.substr(semi+1));
                    val = val.substr(0, semi);
                }
                def.KeyType = std::move(val);
            }
            
            
            if(man.Type(3) == LuaType::STRING) // defer it
            {
                String val = man.ToString(3);
                auto semi = val.find_last_of(';');
                if(semi != String::npos)
                {
                    def.ValueVersion = (U32)std::stoi(val.substr(semi+1));
                    val = val.substr(0, semi);
                }
                def.ValueType = std::move(val);
            }
            else if(man.Type(3) == LuaType::TABLE)
            {
                man.PushLString("__MetaId");
                man.GetTable(3);
                TTE_ASSERT(man.Type(-1) == LuaType::NUMBER, "When registering collection, value type table not registered properly");
                c.ArrayValClass = (U32)ScriptManager::PopInteger(man);
            }
            else
            {
                TTE_ASSERT(false, "When registering collection, value type table not registered properly");
            }
            
            Bool bDefer = def.KeyType.length() || def.ValueType.length();
            TTE_ASSERT(!bDefer || c.ArraySize == 0, "When registering collection: cannot forward declare static arrays as their size is required.");
            
            U32 id = _Impl::_Register(man, std::move(c), 1);
            
            man.PushLString("__MetaId");
            man.PushUnsignedInteger(id);
            man.SetTable(1);
            
            if(bDefer)
            {
                def.CollectionClass = id;
                State._Temp.Deferred.push_back(std::move(def));
            }
            
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
            auto it = GetInternalState().Classes.find(ScriptManager::PopInteger(man));
            
            if(it == GetInternalState().Classes.end())
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
            
            auto it = GetInternalState().Classes.find(ScriptManager::PopInteger(man));
            
            if(it == GetInternalState().Classes.end())
                man.PushNil();
            else
            {
                man.PushLString(SymbolToHexString(it->second.TypeHash));
            }
            
            return 1; // return value
        }
        
        static void _HandleClassNotFound(String tn, U32 ver)
        {
            Symbol s = SymbolFromHexString(tn, true);
            if(s.GetCRC64())
            {
                String str = GetRuntimeSymbols().Find(s);
                if(str.length())
                {
                    tn = str;
                }
                TTE_LOG("%s with version index %d does not exist", tn.c_str(), ver);
            }
            else
            {
                TTE_LOG("%s with version index %d does not exist", tn.c_str(), ver);
            }
            TTE_ASSERT(false, "Serialise requires class which does not exist. Please check serialiser");
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
            
            State.VersionCalcFun = man.ToString(-1);
            
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
                man.PushUnsignedInteger(State.Classes[cls].Flags);
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
                _HandleClassNotFound(tn, ver);
                man.PushNil();
                return 1;
            }
            
            auto& members = State.Classes[cls].Members;
            
            for(auto& member: members)
            {
                if(CompareCaseInsensitive(member.Name, mem))
                {
                    man.PushInteger(State.Classes[member.ClassID].Flags);
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
                _HandleClassNotFound(tn, ver);
                man.PushNil();
                man.PushNil();
                return 2;
            }
            
            auto& members = State.Classes[cls].Members;
            
            for(auto& member: members)
            {
                if(CompareCaseInsensitive(member.Name, mem))
                {
                    man.PushLString(State.Classes[member.ClassID].Name);
                    man.PushInteger(State.Classes[member.ClassID].VersionNumber);
                    return 2;
                }
            }
            
            man.PushNil();
            man.PushNil();
            return 2;
        }
        
        // table GetEnumFlags(typename, version index, memberName). returns table of [Name] = value
        static U32 luaMetaGetEnumFlags(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of GetEnumFlags");
            String tn = man.ToString(-3);
            I32 ver = man.ToInteger(-2);
            String mem = man.ToString(-1);
            U32 cls = FindClass(Symbol(tn), (U32)ver);
            
            if(cls == 0)
            {
                _HandleClassNotFound(tn, ver);
                man.PushNil();
                return 1;
            }
            
            man.PushTable();
            
            auto& members = State.Classes[cls].Members;
            
            U32 i = 0;
            for(auto& member: members)
            {
                if(CompareCaseInsensitive(member.Name, mem))
                {
                    if(!(member.Flags & MEMBER_ENUM || member.Flags & MEMBER_FLAG))
                    {
                        TTE_LOG("At GetEnumFlags: member %s is not an enum or flag. It must be marked as one when registered using"
                                " the appropriate flags.", mem.c_str());
                        man.Pop(1);
                        man.PushNil();
                    }
                    else
                    {
                        for(auto& desc: member.Descriptors)
                        {
                            man.PushLString(desc.Name);
                            man.PushInteger(desc.Value);
                            man.SetTable(-3);
                        }
                    }
                    break;
                }
            }
            
            return 1;
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
                _HandleClassNotFound(tn, ver);
                man.PushNil();
                return 1;
            }
            
            man.PushTable();
            
            auto& members = State.Classes[cls].Members;
            
            U32 i = 0;
            for(auto& member: members)
            {
                man.PushInteger(++i);
                man.PushLString(member.Name.c_str());
                man.SetTable(-3);
            }
            
            return 1;
        }
        
        // bool isalive(instance) if parent expired returns false
        static U32 luaMetaInstanceAlive(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaInstanceAlive");
            
            if(man.Type(-1) == LuaType::LIGHT_OPAQUE)
                man.PushBool(AcquireScriptInstance(man, -1));
            else
                man.PushBool(false); // invalid
            
            return 1;
        }
        
        // nil setvalue(instance, value)
        static U32 luaMetaSetClassValue(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaSetClassValue");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            if(inst)
            {
                Symbol type = State.Classes[inst.GetClassID()].TypeHash;
                if(type == Symbol("String") || type == Symbol("class String"))
                {
                    // push string
                    ((String*)inst._GetInternal())->assign(man.ToString(-1));
                }
                else if(type == Symbol("Symbol") || type == Symbol("class Symbol"))
                {
                    *((Symbol*)inst._GetInternal()) = ScriptManager::ToSymbol(man, -1);
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU64)
                {
                    *((U64*)inst._GetInternal()) = SymbolFromHexString(man.ToString(-1)).GetCRC64();
                }
                else if(type == Symbol("float"))
                {
                    *((float*)inst._GetInternal()) = man.ToFloat(-1);
                }
                else if(type == Symbol("double"))
                {
                    *((double*)inst._GetInternal()) = (double)man.ToFloat(-1);
                }
                else if(type == Symbol("bool"))
                {
                    *((bool*)inst._GetInternal()) = man.ToBool(-1);
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU8)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        *((U8*)inst._GetInternal()) = (U8)man.ToInteger(-1);
                    }
                    else
                    {
                        *((I8*)inst._GetInternal()) = (I8)man.ToInteger(-1);
                    }
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU16)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        *((U16*)inst._GetInternal()) = (U16)man.ToInteger(-1);
                    }
                    else
                    {
                        *((I16*)inst._GetInternal()) = (I16)man.ToInteger(-1);
                    }
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU32)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        *((U32*)inst._GetInternal()) = man.ToInteger(-1);
                    }
                    else
                    {
                        *((I32*)inst._GetInternal()) = (I32)man.ToInteger(-1);
                    }
                }
                else
                {
                    TTE_LOG("Warning: trying to assign to non-intrinsic type %s from lua script! Ignoring",
                            State.Classes[inst.GetClassID()].Name.c_str());
                }
            }
            return 0;
        }
        
        // value metagetclassvalue(instance)
        static U32 luaMetaGetClassValue(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClassValue");
            
            ClassInstance inst = AcquireScriptInstance(man, -1);
            if(inst)
            {
                Symbol type = State.Classes[inst.GetClassID()].TypeHash;
                if(type == Symbol("String") || type == Symbol("class String"))
                {
                    // push string
                    man.PushLString(*((String*)inst._GetInternal()));
                }
                else if(type == Symbol("Symbol") || type == Symbol("class Symbol")
                        || State.Classes[inst.GetClassID()].Serialise == &SerialiseU64) // for int64 types (signed and un) push as symbol
                {
                    man.PushLString(SymbolToHexString(*((Symbol*)inst._GetInternal())));
                }
                else if(type == Symbol("float"))
                {
                    man.PushFloat(*((float*)inst._GetInternal()));
                }
                else if(type == Symbol("double"))
                {
                    man.PushFloat((float)*((double*)inst._GetInternal()));
                }
                else if(type == Symbol("bool"))
                {
                    man.PushBool(*((Bool*)inst._GetInternal()));
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU8)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        man.PushUnsignedInteger(*((U8*)inst._GetInternal()));
                    }
                    else
                    {
                        man.PushInteger((I32)*((I8*)inst._GetInternal()));
                    }
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU16)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        man.PushUnsignedInteger(*((U16*)inst._GetInternal()));
                    }
                    else
                    {
                        man.PushInteger((I32)*((I16*)inst._GetInternal()));
                    }
                }
                else if(State.Classes[inst.GetClassID()].Serialise == &SerialiseU32)
                {
                    if(State.Classes[inst.GetClassID()].Name.find('u') != String::npos
                       || State.Classes[inst.GetClassID()].Name.find('U') != String::npos)
                    {
                        man.PushUnsignedInteger(*((U32*)inst._GetInternal()));
                    }
                    else
                    {
                        man.PushInteger(*((I32*)inst._GetInternal()));
                    }
                }
                else man.PushNil();
            }else man.PushNil();
            return 1;
        }
        
        static U32 luaMetaSVIF(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2 || man.GetTop() == 3, "See usage of get serialised version info file name");
            U32 clz = FindClass(man.ToString(1), man.ToInteger(2));
            if(clz)
            {
                man.PushLString(MakeSerialisedVersionInfoFileName(clz, man.GetTop() >= 3 ? man.ToBool(3) : false));
            }
            else
            {
                String clas = man.ToString(1);
                TTE_LOG("Invalid class %s [%d]", clas.c_str(), man.ToInteger(2));
                man.PushNil();
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
                ClassInstance meminst = GetMember(inst, mem, false);
                if(meminst)
                {
                    meminst.PushWeakScriptRef(man, inst.ObtainParentRef());
                }
                else
                    man.PushNil();
            }
            else
            {
                String v = man.ToString(-1);
                TTE_LOG("MetaGetMember(%s) called with invalid instance. Returning nil", v.c_str());
                man.PushNil();
            }
            
            return 1;
        }
        
        // createinstance(typename_string, version_number, attachedName, attach)
        // STRONG reference to attach. attach is another instance, which when destroyed
        // this one will be destroyed (it owns it) - example in a Chore, a ChoreAgent since its not in a DCArray
        static U32 luaMetaCreateInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 4, "Incorrect usage of CreateInstance");
            TTE_ASSERT(man.Type(1) == LuaType::STRING, "Typename is invalid. Likely the class was not found");
            String tn = man.ToString(-4);
            I32 ver = man.ToInteger(-3);
            
            Symbol name = ScriptManager::ToSymbol(man, -2);
            TTE_ASSERT(name.GetCRC64(), "Name must be properly specified in CreateInstance.");
            
            ClassInstance attach = AcquireScriptInstance(man, -1);
            if(!attach)
            {
                TTE_ASSERT(false, "At MetaCreateInstance: attaching instance was null or invalid");
                man.PushNil();
                return 1;
            }
            
            U32 cls = FindClass(SymbolFromHexString(tn), (U32)ver);
            
            if(cls == 0)
            {
                _HandleClassNotFound(tn, ver);
                man.PushNil();
                return 1;
            }
            
            if(!IsAttachable(attach))
            {
                TTE_ASSERT(false, "This class cannot have attached to it");
                man.PushNil();
                return 1;
            }
            
            ClassInstance cinst = CreateInstance(cls, attach, name);
            cinst.PushWeakScriptRef(man, attach.ObtainParentRef());
            
            return 1;
        }
        
        // typename, versionindex = getclass(instance)
        static U32 luaMetaGetClass(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetClass");
            
            ClassInstance inst = AcquireScriptInstance(man, -1);
            
            if(inst)
            {
                man.PushLString(State.Classes[inst.GetClassID()].Name);
                man.PushUnsignedInteger(State.Classes[inst.GetClassID()].VersionNumber);
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
                                                  inst, &State.Classes[inst.GetClassID()], inst._GetInternal(), isWrite, nullptr)); // perform it
            
            return 1;
        }
        
        // bool serialse(stream, instance, is write, debug name)
        static U32 luaMetaSerialise(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 4 || man.GetTop() == 3, "Incorrect usage of MetaSerialise");
            ClassInstance inst = AcquireScriptInstance(man, 2);
            Bool isWrite = man.ToBool(3);
            Stream* stream = ((Stream*)man.ToPointer(1));
            String name = man.GetTop() == 4 ? man.ToString(-1) : ("Embedded " + State.Classes[inst.GetClassID()].Name);
            
            if(stream == nullptr || !inst)
            {
                TTE_ASSERT(false, "At MetaSerialise: stream and/or instance is null");
                man.PushBool(false);
                return 1;
            }
            
            man.PushBool(_Impl::_Serialise(*stream,
                                           inst, &State.Classes[inst.GetClassID()], inst._GetInternal(), isWrite, name.c_str())); // perform it
            
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
        
        // num getchildcount(instance)
        static U32 luaMetaGetChildrenCount(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetChildrenCount");
            
            ClassInstance inst = AcquireScriptInstance(man, -1);
            
            if(!IsAttachable(inst))
            {
                TTE_ASSERT("At MetaGetChildrenNames: class cannot have children (not attachable)");
                man.PushNil();
                return 1;
            }
            
            man.PushInteger((I32)inst._GetInternalChildrenRefs()->size());
            return 1;
        }
        
        // table[symbols] getchildrennames(instance)
        static U32 luaMetaGetChildrenNames(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetChildSymbol");
            ClassInstance inst = AcquireScriptInstance(man, -1);
            if(!inst)
            {
                TTE_ASSERT("At MetaGetChildrenNames: null instance passed in");
                man.PushNil();
                return 1;
            }
            if(!IsAttachable(inst))
            {
                TTE_ASSERT("At MetaGetChildrenNames: class cannot have children (not attachable)");
                man.PushNil();
                return 1;
            }
            man.PushTable();
            I32 i = 0;
            auto refs = inst._GetInternalChildrenRefs();
            for(auto pair = refs->begin(); pair != refs->end(); pair++)
            {
                man.PushInteger(++i);
                ScriptManager::PushSymbol(man, pair->first);
                man.SetTable(-3);
            }
            return 1;
        }
        
        static U32 luaMetaClasses(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 0, "Incorrect usage of MetaGetClasses");
            std::set<String> names{};
            for(const auto& cls: GetInternalState().Classes)
                names.insert(cls.second.Name);
            man.PushTable();
            I32 i = 0;
            for(const auto& cls: names)
            {
                man.PushInteger(++i);
                man.PushLString(cls);
                man.SetTable(-3);
            }
            return 1;
        }
        
        // nil release(instance, name)
        static U32 luaMetaReleaseChild(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaReleaseChild");
            
            ClassInstance inst = AcquireScriptInstance(man, -2);
            
            if(!inst)
            {
                TTE_ASSERT("At MetaReleaseChild: null instance passed in");
                return 0;
            }
            if(!IsAttachable(inst))
            {
                TTE_ASSERT("At MetaReleaseChild: class is not attachable");
                man.PushNil();
                return 1;
            }
            
            Symbol n = ScriptManager::ToSymbol(man, -1);
            
            // remove from map
            auto children = inst._GetInternalChildrenRefs();
            auto it = children->find(n);
            if(it != children->end())
            {
                children->erase(it);
            }
            
            return 0;
        }
        
        // obj getchild(instance, name)
        static U32 luaMetaGetChild(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaGetChild");
            ClassInstance inst = AcquireScriptInstance(man, -2);
            Symbol n = ScriptManager::ToSymbol(man, -1);
            
            if(!inst || !n.GetCRC64())
            {
                TTE_ASSERT("At MetaGetChild: null instance passed in or empty symbol");
                man.PushNil();
                return 1;
            }
            if(!IsAttachable(inst))
            {
                TTE_ASSERT("At MetaGetChild: class cannot have children (not attachable)");
                man.PushNil();
                return 1;
            }
            
            auto it = inst._GetInternalChildrenRefs()->find(n);
            
            if(it == inst._GetInternalChildrenRefs()->end())
                man.PushNil();
            else
            {
                it->second.PushWeakScriptRef(man, inst.ObtainParentRef());
            }
            
            return 1;
        }
        
        static U32 luaMetaDumpVersions(LuaManager& man)
        {
            std::set<String> sortedClassNames{};
            for(auto& clazz: State.Classes)
            {
                std::ostringstream ss{};
                ss << std::left << std::setw(50) << clazz.second.Name;
                ss << " => VCRC: 0x" << std::hex << std::uppercase
                << std::setw(10) << clazz.second.VersionCRC;
                ss << "[" << std::dec << std::nouppercase
                << clazz.second.VersionNumber << "]";
                ss << " LHASH: 0x" << std::hex << std::uppercase << std::setw(0)
                << clazz.second.LegacyHash;
                sortedClassNames.insert(ss.str());
            }
            for(auto& it: sortedClassNames)
                TTE_LOG(it.c_str());
            return 0;
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
        
        // copyinstance(instance, attachName, attach)
        static U32 luaMetaCopyInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaCopyInstance");
            ClassInstance inst = AcquireScriptInstance(man, -3);
            Symbol name = ScriptManager::ToSymbol(man, -2);
            ClassInstance attach = AcquireScriptInstance(man, -1);
            
            if(!inst || !attach || !name.GetCRC64())
            {
                TTE_LOG("Cannot CopyInstance, source, destination or name are either null or not specified");
                man.PushNil();
                return 1;
            }

            ClassInstance cinst = CopyInstance(inst, attach, name);
            cinst.PushWeakScriptRef(man, attach.ObtainParentRef());
            
            return 1;
        }
        
        // move instance(instance, attachName, attach)
        static U32 luaMetaMoveInstance(LuaManager& man)
        {
            TTE_ASSERT(man.GetTop() == 3, "Incorrect usage of MetaMoveInstance");
            ClassInstance inst = AcquireScriptInstance(man, -3);
            Symbol name = ScriptManager::ToSymbol(man, -2);
            ClassInstance attach = AcquireScriptInstance(man, -1);
            
            if(!inst || !attach || !name.GetCRC64())
            {
                TTE_LOG("Cannot MoveInstance, source, destination or name are either null or not specified");
                man.PushNil();
                return 1;
            }
            
            ClassInstance cinst = MoveInstance(inst, attach, name);
            cinst.PushWeakScriptRef(man, attach.ObtainParentRef());
            
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
                    man.PushBool((State.Classes[cls].Flags & CLASS_CONTAINER) != 0);
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
    
    // async stuff isnt implemented BUT BEWARE WAITING FROM A JOB MAYBE DEADLOCK
    static U32 luaMetaStreamAsyncSerialiseEnabled(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamAsyncSerialiseEnabled");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        
        man.PushBool(stream.DebugOutputFile.get() == nullptr); // if no debug file, we can async do stuff
        
        return 1;
    }
    
    static U32 luaMetaStreamReadFloat(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        Float val{};
        ::Meta::ClassInstance e{}; // empty
        SerialiseU32(stream, e, 0, &val, false);
        
        man.PushFloat(val);
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteFloat(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Float val = ScriptManager::PopFloat(man);
        ::Meta::ClassInstance e{}; // empty
        SerialiseU32(stream, e, 0, &val, true);
        
        return 0;
    }
    
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
    
    static U32 luaMetaStreamReadBool(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamRead");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        I8 val{};
        ::Meta::ClassInstance e{}; // empty
        SerialiseU8(stream, e, 0, &val, false);
        
        man.PushBool(val == (U8)0x31);
        
        return 1;
    }
    
    static U32 luaMetaStreamWriteBool(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWrite");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Bool val = ScriptManager::PopBool(man);
        I8 wval = val ? (U8)0x31 : (U8)0x30;
        ::Meta::ClassInstance e{}; // empty
        SerialiseU8(stream, e, 0, &wval, true);
        
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
        SerialiseU8(stream, e, 0, &wval, true);
        
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
    
    static U32 luaMetaStreamWriteZeros(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamZeros");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        U32 val = ScriptManager::PopUnsignedInteger(man);
        DataStreamManager::GetInstance()->WriteZeros(stream.Sect[stream.CurrentSection].Data, val);
        
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
        if(val == Symbol("class Procedural_LookAt_Value"))
        {
            TTE_LOG("");
        }
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
    
    static U32 luaMetaStreamGetFileName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaStreamGetFileName");
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-1));
        man.PushLString(stream.Name.c_str());
        return 1;
    }
    
    // Bffer(stream, buffer_member, size)
    static U32 luaMetaStreamReadBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() >= 2, "Incorrect usage of MetaStreamReadBuffer");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(1));
        I32 _size = man.GetTop() > 2 ? man.ToInteger(3) : -1;
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, 2);
        DataStreamRef& sect = stream.Sect[stream.CurrentSection].Data;
        
        U32 actualSize = _size == -1 ? (U32)(sect->GetSize() - sect->GetPosition()) : (U32)_size;
        
        TTE_ASSERT(bufInst, "Invalid buffer");
        TTE_ASSERT(actualSize >= 0 && actualSize < 0x10000000, "Buffer size invalid (>256MB)"); // increase size limit?
        
        Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufInst._GetInternal());
        
        U8* Buffer = TTE_ALLOC(actualSize, MEMORY_TAG_RUNTIME_BUFFER);
        buf.BufferSize = (U32)actualSize;
        
        TTE_ASSERT(stream.Read(Buffer, (U64)actualSize), "Binary buffer read fail - size is likely too large.");
        
        buf.BufferData = Ptr<U8>(Buffer, [](U8* p){TTE_FREE(p);});
        
        if(stream.DebugOutputFile)
        {
            stream.WriteTabs();
            U32 base64Encoded = MIN(stream.MaxInlinableBuffer, buf.BufferSize);
            String base64 = Base64::Encode(Buffer, base64Encoded);
            stream.DebugOutput << "[BufferData Base64] >> \"" << base64;
            if(base64Encoded != buf.BufferSize)
                stream.DebugOutput << "...\"\n";
            else
                stream.DebugOutput << "\"\n";
        }
        
        return 0;
    }
    
    // CachedreadBffer(stream, buffer_member, size [-1 => read remaining])
    static U32 luaMetaStreamReadCache(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() >= 2, "Incorrect usage of MetaStreamReadCache");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(1));
        I32 size = man.GetTop() > 2 ? man.ToInteger(3) : -1;
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, 2);
        
        TTE_ASSERT(bufInst, "Invalid buffer");
        if(size != -1)
            TTE_ASSERT(size >= 0 && size < 0x10000000, "Buffer size invalid (>256MB)"); // increase size limit?
        
        DataStreamRef& sect = stream.Sect[stream.CurrentSection].Data;
        
        U32 actualSize = size == -1 ? (U32)(sect->GetSize() - sect->GetPosition()) : (U32)size;
        
        Meta::DataStreamCache& buf = *((Meta::DataStreamCache*)bufInst._GetInternal());
        
        buf.Stream = DataStreamManager::GetInstance()->CreateSubStream(sect, sect->GetPosition(), (U64)actualSize);
        
        TTE_ASSERT(sect->GetPosition() + actualSize <= sect->GetSize(), "At read cache, trying to read too many bytes");
        
        U64 endPos = sect->GetPosition() + actualSize; // cache here as output dbg may change it
        
        if(stream.DebugOutputFile)
        {
            stream.WriteTabs();
            U32 base64Encoded = MIN(stream.MaxInlinableBuffer, actualSize);
            
            U8* Temp = TTE_ALLOC(base64Encoded, MEMORY_TAG_TEMPORARY);
            buf.Stream->Read(Temp, base64Encoded);
            buf.Stream->SetPosition(0);
            
            String base64 = Base64::Encode(Temp, base64Encoded);
            stream.DebugOutput << "[CachedBufferData Base64] >> \"" << base64;
            if(base64Encoded != actualSize)
                stream.DebugOutput << "...\"\n";
            else
                stream.DebugOutput << "\"\n";
            
            TTE_FREE(Temp);
        }
        
        sect->SetPosition(endPos); // move pointer
        
        return 0;
    }
    
    // write(stream, member)
    static U32 luaMetaStreamWriteCached(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWriteCached");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid cache");
        
        Meta::DataStreamCache& buf = *((Meta::DataStreamCache*)bufInst._GetInternal());
        
        if(buf.Stream)
        {
            buf.Stream->SetPosition(0);
            DataStreamManager::GetInstance()->Transfer(buf.Stream, stream.Sect[stream.CurrentSection].Data, buf.Stream->GetSize());
            buf.Stream->SetPosition(0);
        }
        
        return 0;
    }
    
    // size(stream, buffer_member)
    static U32 luaMetaStreamCachedSize(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamCachedSize");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid cache");
        
        Meta::DataStreamCache& buf = *((Meta::DataStreamCache*)bufInst._GetInternal());
        
        man.PushUnsignedInteger(buf.Stream ? (U32)buf.Stream->GetSize() : 0);
        
        return 0;
    }
    
    static U32 luaMetaBufferSize(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage of MetaGetBufferSize");
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid buffer");
        
        Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufInst._GetInternal());
        
        man.PushUnsignedInteger(buf.BufferSize);
        
        return 1;
    }
    
    // writebuffer(stream, buffer_member)
    static U32 luaMetaStreamWriteBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of MetaStreamWriteBuffer");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        Meta::ClassInstance bufInst = Meta::AcquireScriptInstance(man, -1);
        
        TTE_ASSERT(bufInst, "Invalid buffer");
        
        Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufInst._GetInternal());
        
        TTE_ASSERT(buf.BufferData && stream.Write(buf.BufferData.get(), (U64)buf.BufferSize), "Binary buffer is null or buffer write fail");
        
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
    
    static U32 luaMetaStreamAdvance(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect use of MetaStreamAdvance");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(-2));
        U32 nBytes = ScriptManager::PopUnsignedInteger(man);
        stream.Sect[stream.CurrentSection].Data->SetPosition(stream.Sect[stream.CurrentSection].Data->GetPosition() + nBytes);
        
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
            
            auto it = ::Meta::State.Classes.find(::Meta::FindClass(tnSymbol.GetCRC64(), i));
            
            if(it != ::Meta::State.Classes.end()) // the class is valid, test header
            {
                
                Bool Found = (it->second.Flags &
                              (::Meta::CLASS_INTRINSIC | ::Meta::CLASS_CONTAINER)) != 0; // if intrinsic its not in the header, so we are ok
                
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
    
    static U32 luaMetaStreamDDSSize(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid usage");
        ScriptManager::TableGet(man, "Format CC");
        String fourCC = ScriptManager::PopString(man);
        man.PushUnsignedInteger(fourCC == "DX10" ? 0x94 : 0x80);
        return 1;
    }
    
    static U32 luaMetaStreamReadDDS(LuaManager& man)
    {
        Meta::ClassInstance _{};
        TTE_ASSERT(man.GetTop() == 1, "Invalid use of read DDS");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(1));
        man.PushTable();
        
        U32 value{};
        U32 unused[32]{};
        SerialiseU32(stream, _, nullptr, &value, false);
        TTE_ASSERT(value == 0x20534444, "Invalid DDS header magic");
        SerialiseU32(stream, _, nullptr, &value, false);
        TTE_ASSERT(value == 0x7C, "Invalid DDS header size");
        
        // flags
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Flags");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // height
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Height");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // width
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Width");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // pitch
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Pitch");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // depth
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Depth");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // mips
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Mip Count");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        stream.Read((U8*)unused, 12 * 4); // 11 reserved + size of format struct
        
        // flags
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Flags");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        // format
        U8 str[5]{0,0,0,0,0};
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format CC");
        memcpy(str, &value, 4);
        man.PushLString(value == 0 ? "" : (CString)str);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Bit Count");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Red Mask");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Green Mask");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Blue Mask");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Format Alpha Mask");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false); // caps.
        man.PushLString("Caps");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        SerialiseU32(stream, _, nullptr, &value, false);
        man.PushLString("Additional Caps");
        man.PushUnsignedInteger(value);
        man.SetTable(2, true);
        
        stream.Read((U8*)unused, 3 * 4); // reserved
        
        if(!memcmp(str, "DX10", 4))
        {
            SerialiseU32(stream, _, nullptr, &value, false);
            man.PushLString("DXGI Format");
            man.PushUnsignedInteger(value);
            man.SetTable(2, true);
            
            SerialiseU32(stream, _, nullptr, &value, false);
            man.PushLString("Resource Dimension");
            man.PushUnsignedInteger(value);
            man.SetTable(2, true);
            
            SerialiseU32(stream, _, nullptr, &value, false);
            man.PushLString("Misc Flags 1");
            man.PushUnsignedInteger(value);
            man.SetTable(2, true);
            
            SerialiseU32(stream, _, nullptr, &value, false);
            man.PushLString("Array Size");
            man.PushUnsignedInteger(value);
            man.SetTable(2, true);
            
            SerialiseU32(stream, _, nullptr, &value, false);
            man.PushLString("Misc Flags 2");
            man.PushUnsignedInteger(value);
            man.SetTable(2, true);
        }
        
        return 1;
    }
    
    // dump buffer(buf mem, offset, size, outfilename)
    static U32 luaMetaStreamDumpBuffer(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 4, "Invalid use of dump buffer");
        Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, 1);
        U32 off = (U32)man.ToInteger(2), size = (U32)man.ToInteger(3);
        String out = man.ToString(4);
        DataStreamRef outStream = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(out));
        if(outStream)
        {
            Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)inst._GetInternal());
            if(off + size >= buf.BufferSize)
            {
                outStream->Write(buf.BufferData.get() + off, size);
            }
            else
            {
                TTE_LOG("Can not dump to %s: offset + size > buffer size.", out.c_str());
            }
        }
        else
        {
            TTE_LOG("Could not open stream dump %s", out.c_str());
        }
        return 0;
    }
    
    static U32 luaMetaStreamWriteDDS(LuaManager& man)
    {
        Meta::ClassInstance _{};
        TTE_ASSERT(man.GetTop() == 2, "Invalid use of write DDS");
        
        Meta::Stream& stream = *((Meta::Stream*)man.ToPointer(1));
        
        U32 value = 0x7C;
        SerialiseU32(stream, _, nullptr, &value, true);
        value = 0x20534444;
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Flags");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Height");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Width");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Pitch");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Depth");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Mip Count");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        DataStreamManager::GetInstance()->WriteZeros(stream.Sect[stream.CurrentSection].Data, 44); // reserved
        value = 0x20;
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Flags");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format CC");
        String ccvalue = ScriptManager::PopString(man);
        if(ccvalue.length() == 0)
            value = 0;
        else if(ccvalue.length() == 4)
        {
            memcpy(&value, ccvalue.c_str(), 4);
        }
        else TTE_ASSERT(false, "Invalid Four CC");
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Bit Count");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Red Mask");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Green Mask");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Blue Mask");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Format Alpha Mask");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Caps");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        ScriptManager::TableGet(man, "Additional Caps");
        value = ScriptManager::PopUnsignedInteger(man);
        SerialiseU32(stream, _, nullptr, &value, true);
        
        DataStreamManager::GetInstance()->WriteZeros(stream.Sect[stream.CurrentSection].Data, 12); // reserved
        
        if(ccvalue == "DX10")
        {
            ScriptManager::TableGet(man, "DXGI Format");
            value = ScriptManager::PopUnsignedInteger(man);
            SerialiseU32(stream, _, nullptr, &value, true);
            
            ScriptManager::TableGet(man, "Resource Dimension");
            value = ScriptManager::PopUnsignedInteger(man);
            SerialiseU32(stream, _, nullptr, &value, true);
            
            ScriptManager::TableGet(man, "Misc Flags 1");
            value = ScriptManager::PopUnsignedInteger(man);
            SerialiseU32(stream, _, nullptr, &value, true);
            
            ScriptManager::TableGet(man, "Array Size");
            value = ScriptManager::PopUnsignedInteger(man);
            SerialiseU32(stream, _, nullptr, &value, true);
            
            ScriptManager::TableGet(man, "Misc Flags 2");
            value = ScriptManager::PopUnsignedInteger(man);
            SerialiseU32(stream, _, nullptr, &value, true);
        }
        
        return 0;
    }
    
}

namespace LuaMisc
{
    
    // string SymbolFind(symbol string)
    static U32 luaSymbolFind(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage for SymbolFind")
        Symbol sym = SymbolFromHexString(man.ToString(-1));
        String str = sym.GetCRC64() == 0 ? "" : SymbolTable::Find(sym);
        man.PushLString(std::move(str));
        return 1;
    }
    
    // nil SymbolRegister(symbol string)
    static U32 luaSymbolReg(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Incorrect usage for SymbolRegister")
        GetRuntimeSymbols().Register(man.ToString(-1));
        return 0;
    }
    
    // bool SymbolCompare(left, right)
    static U32 luaSymbolCmp(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage for SymbolCompare");
        String l = man.ToString(-2);
        String r = man.ToString(-1);
        if(CompareCaseInsensitive(l, r))
        {
            man.PushBool(true);
        }
        else
        {
            Symbol ls = SymbolFromHexString(l);
            Symbol rs = SymbolFromHexString(r);
            ls = ls.GetCRC64() == 0 ? Symbol(l) : ls;
            rs = rs.GetCRC64() == 0 ? Symbol(r) : rs;
            man.PushBool(ls == rs);
        }
        return 1;
    }
    
    static U32 luaSymbol(LuaManager& man)
    {
        man.PushLString(SymbolToHexString(Symbol(man.ToString(-1))));
        return 1;
    }
    
    static U32 luaSymbolClear(LuaManager&)
    {
        GetRuntimeSymbols().Clear();
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

    static U32 luaGetMountPoints(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 0, "Invalid use of TTE_GetMountPoints");
        auto reg = ResourceRegistry::GetBoundRegistry(man);
        man.PushTable();
        if(reg)
        {
            std::vector<String> names{};
            reg->GetResourceLocationNames(names);
            I32 i = 0;
            for(const auto& name: names)
            {
                man.PushInteger(++i);
                man.PushLString(name);
                man.SetTable(-3);
            }
        }
        return 1;
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
            Bool hasReg = ResourceRegistry::GetBoundRegistry(man).unique();
            GameSnapshot snap{};
            snap.ID = man.ToString(-3);
            snap.Platform = man.ToString(-2);
            snap.Vendor = man.ToString(-1);
            Context->Switch(snap);
            if(hasReg)
            {
                Context->CreateResourceRegistry(false); // cached in dependents. keeps alive
            }
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
        Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man);
        if (!reg)
        {
            TTE_LOG("At TTE_OpenMetaStream: no bound resource registry. Cannot be used here");
            man.PushBool(false);
            return 1;
        }
        if(EnsureMain())
        {
            String path = man.ToString(1);
            DataStreamRef r;
            if(man.GetTop() == 1)
            {
                ResourceAddress addr = reg->CreateResolvedAddress(path, false);
                if (!addr)
                {
                    TTE_LOG("At TTE_OpenMetaStream: resource address %s is invalid", path.c_str());
                    man.PushBool(false);
                    return 1;
                }
                r = reg->OpenDataStream(addr.GetLocationName(), addr.GetName());
            }
            else
            {
                // open from archive
                U32 tag = ScriptManager::GetScriptObjectTag(man, 2);
                if(tag == TTARCHIVE1)
                    r = (ScriptManager::GetScriptObject<TTArchive>(man, 2))->Find(path, nullptr); // open stream
                else if(tag == TTARCHIVE2)
                    r = (ScriptManager::GetScriptObject<TTArchive2>(man, 2))->Find(path, nullptr); // open stream
                else
                {
                    TTE_LOG("At TTE_OpenMetaStream: invalid argument(s()");
                    man.PushNil();
                    return 1;
                }
            }
            if(r && r->GetSize() > 0)
            {
                Meta::ClassInstance inst = Meta::ReadMetaStream(path, r);
                if(inst)
                {
                    inst.PushStrongScriptRef(man);
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
        Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man);
        if(!reg)
        {
            TTE_LOG("At TTE_SaveMetaStream: no bound resource registry. Cannot be used here");
            man.PushBool(false);
            return 1;
        }
        if(!Context)
        {
            man.PushBool(false);
            return 1;
        }
        if(EnsureMain())
        {
            String path = man.ToString(-1);
            ResourceAddress addr = reg->CreateResolvedAddress(path, false);
            if(!addr)
            {
                TTE_LOG("At TTE_SaveMetaStream: resource address %s is invalid", path.c_str());
                man.PushBool(false);
                return 1;
            }
            DataStreamRef r = reg->OpenDataStream(addr.GetLocationName(), addr.GetName());
            Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, -1);
            if(inst)
            {
                
                Meta::MetaStreamParams params{}; // in the future we can allow users to compress/encrypt these if they want.
                params.Version = Context->GetActiveGame()->MetaVersion;
                
                if(Meta::WriteMetaStream(path, std::move(inst), r, params))
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
        TTE_ASSERT(Context, "At TTE_GetActiveSnapshot: no context is present. Ensure any modding scripts are run after context initialisation.");
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
        
        man.PushLString("Vendor");
        man.PushLString(Context->GetSnapshot().Vendor);
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
        
        if(!Context->GetActiveGame()->UsesArchive2())
        {
            TTE_ASSERT(false, "At TTE_OpenArchive2: please use TTE_OpenArchive as the current game does not use .TTARCH, but rather .TTARCH2.");
            man.PushNil();
            return 1;
        }
        
        String path = man.ToString(-1);
        DataStreamRef r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
        
        if(r->GetSize() > 0)
        {
            TTArchive2* pArchive = TTE_NEW(TTArchive2, MEMORY_TAG_SCRIPT_OBJECT, Context->GetActiveGame()->GetArchiveVersion(Context->GetSnapshot()));
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
        
        if(Context->GetActiveGame()->UsesArchive2())
        {
            TTE_ASSERT(false, "At TTE_OpenArchive: please use TTE_OpenArchive2 as the current game does not use .TTARCH2, but rather .TTARCH.");
            man.PushNil();
            return 1;
        }
        
        String path = man.ToString(-1);
        DataStreamRef r = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, path));
        
        if(r->GetSize() > 0)
        {
            TTArchive* pArchive = TTE_NEW(TTArchive, MEMORY_TAG_SCRIPT_OBJECT, Context->GetActiveGame()->GetArchiveVersion(Context->GetSnapshot()));
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
    
    static U32 luaLog(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid use TTE_Log. Must have one string argument");
        
        String s = man.ToString(-1);
        TTE_LOG(s.c_str());
        
        return 0;
    }
    
    static U32 luaDumpMemLeaks(LuaManager& man)
    {
        Memory::DumpTrackedMemory();
        return 0;
    }
    
    // files = listfiles(archive)
    static U32 luaArchiveListFiles(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 1, "Invalid use of TTE_ArchiveListFiles");
        // open from archive
        U32 tag = ScriptManager::GetScriptObjectTag(man, -1);
        if(tag == TTARCHIVE1)
        {
            TTArchive* pArchive = ScriptManager::GetScriptObject<TTArchive>(man, -1);
            man.PushTable();
            std::set<String> files{};
            pArchive->GetFiles(files);
            U32 i = 1;
            for(auto& it: files)
            {
                man.PushUnsignedInteger(i++);
                man.PushLString(std::move(it)); // move
                man.SetTable(-3);
            }
        }
        else if(tag == TTARCHIVE2)
        {
            TTArchive2* pArchive = ScriptManager::GetScriptObject<TTArchive2>(man, -1);
            man.PushTable();
            std::set<String> files{};
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
    
    static U32 luaAssert(LuaManager& man)
    {
        if(man.GetTop() >= 1 && man.Type(1) == LuaType::BOOL)
        {
            String reason = man.GetTop() > 1 && man.Type(2) == LuaType::STRING ? man.ToString(2) : "General lua script assertion failed!";
            TTE_ASSERT(man.ToBool(1), reason.c_str());
        }
        return 0;
    }
    
    // bool blowfish(bufferInstance, size, encrypt (true) or decrypt (false)) . size must be less than or equal to buffer size
    static U32 luaBlowfish(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 3, "Invalid use of TTE_Blowfish()");
        
        Meta::ClassInstance buf = Meta::AcquireScriptInstance(man, -3);
        Bool enc = man.ToBool(-1);
        U32 sz = (U32)man.ToInteger(-2);
        
        if(!buf)
        {
            man.PushBool(false);
            return 1;
        }
        
        Meta::BinaryBuffer* pBuffer = (Meta::BinaryBuffer*)buf._GetInternal();
        
        if(!pBuffer)
        {
            man.PushBool(false);
            return 1;
        }
        
        if(sz > pBuffer->BufferSize)
        {
            TTE_LOG("At TTE_Blowfish: buffer size is %d - cannot crypt %d bytes", pBuffer->BufferSize, sz);
            man.PushBool(false);
            return 1;
        }
        
        if(enc)
            Blowfish::GetInstance()->Encrypt(pBuffer->BufferData.get(), sz);
        else
            Blowfish::GetInstance()->Decrypt(pBuffer->BufferData.get(), sz);
        
        man.PushBool(true);
        return 1;
    }
    
    static U32 luaMountArchive(LuaManager& man)
    {
        Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man);
        if(reg)
        {
            reg->MountArchive(man.ToString(1), man.ToString(2));
        }
        else
        {
            TTE_LOG("At TTE_MountArchive: no resource registry found");
        }
        return 0;
    }
    
    static U32 luaMountSystem(LuaManager& man)
    {
        Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man);
        if(reg)
        {
            reg->MountSystem(man.ToString(1), man.ToString(2), man.ToBool(3));
        }
        else
        {
            TTE_LOG("At TTE_MountSystem: no resource registry found");
        }
        return 0;
    }
    
    static U32 luaPrintSets(LuaManager& man)
    {
        Ptr<ResourceRegistry> reg = ResourceRegistry::GetBoundRegistry(man);
        if(reg)
        {
            reg->PrintSets();
        }
        else
        {
            TTE_LOG("At TTE_PrintResourceSets: no resource registry found");
        }
        return 0;
    }
    
    static U32 luaContainerToTable(LuaManager& man)
    {
        if(man.GetTop() != 1)
        {
            TTE_LOG("Expected one argument");
            man.PushNil();
            return 1;
        }
        Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, 1);
        man.PushTable();
        Meta::ClassInstanceCollection& col = CastToCollection(inst);
        Bool bKeyed = col.IsKeyedCollection();
        for(U32 i = 0; i < col.GetSize(); i++)
        {
            // KEY
            if(bKeyed)
            {
                Meta::ClassInstance key = col.GetKey(i);
                if(!CoerceMetaToLua(man, key))
                {
                    TTE_LOG("Could not coerce collection because the key type was not coercable as of this type (type %s).",
                            Meta::GetClass(key.GetClassID()).Name.c_str());
                    man.Pop(1);
                    man.PushNil();
                    return 1;
                }
            }
            else
            {
                man.PushInteger(i+1);
            }
            Meta::ClassInstance value = col.GetValue(i);
            if(!CoerceMetaToLua(man, value))
            {
                TTE_LOG("Could not coerce collection because the value type was not coercable as of this type (type %s).",
                        Meta::GetClass(value.GetClassID()).Name.c_str());
                man.Pop(bKeyed ? 2 : 1);
                man.PushNil();
                return 1;
            }
            man.SetTable(-3);
        }
        return 1;
    }
    
    static U32 luaGetPlatform(LuaManager& man)
    {
        if(man.GetTop() != 0)
        {
            TTE_LOG("At GetPlatform: did not expect arguments");
            return 0;
        }
        man.PushLString(PLATFORM_NAME);
        return 1;
    }
    
    static U32 luaTableToCollection(LuaManager& man)
    {
        if(man.GetTop() != 2)
        {
            TTE_LOG("Expected two arguments");
            return 0;
        }
        Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, 1);
        Meta::ClassInstanceCollection& col = Meta::CastToCollection(inst);
        Bool bKeyed = col.IsKeyedCollection();
        ITERATE_TABLE(it, 2)
        {
            Meta::ClassInstance k{},v{};
            if(bKeyed)
            {
                k = Meta::CreateInstance(col.GetKeyClass());
                if(!Meta::CoerceLuaToMeta(man, it.KeyIndex(), k))
                {
                    TTE_LOG("Could not coerce collection from ;ua because the key type could not be coerced (type %s).",
                            Meta::GetClass(k.GetClassID()).Name.c_str());
                    return 0;
                }
            }
            v = Meta::CreateInstance(col.GetValueClass());
            if(!Meta::CoerceLuaToMeta(man, it.ValueIndex(), v))
            {
                TTE_LOG("Could not coerce collection from Lua because the value type could not be coerced (type %s).",
                        Meta::GetClass(v.GetClassID()).Name.c_str());
                return 0;
            }
            if(bKeyed)
                col.Push(std::move(k), std::move(v), false, false);
            else
                col.PushValue(std::move(v), false);
        }
        return 0;
    }
    
}

static U32 luaMetaIsIsolated(LuaManager& man)
{
    man.PushBool(JobScheduler::IsRunningFromWorker());
    return 1;
}


// ===================================================================         REGISTER OBJECT
// ===================================================================

#define ADD_FN(namespace, name, fun, decl, desc) Col.Functions.push_back({name, &namespace :: fun, decl, desc})

LuaFunctionCollection luaLibraryAPI(Bool bWorker)
{
    LuaFunctionCollection Col{};
    
    // REGISTER TTE API
    
    Col.Functions.push_back({"IsIsolated", &luaMetaIsIsolated, "bool IsIsolated()", "Returns if currently being called from a worker thread."});
    
    if(!bWorker)
    {
        Col.Functions.push_back({"TTE_PrintResourceSets", &TTE::luaMountArchive, "nil TTE_PrintResourceSets()",
            "This function is only available to mod scripts! "
            "Prints to the console all of the resource sets in the resource system, Must have a resource system attached!"
        });
        Col.Functions.push_back({"TTE_MountArchive", &TTE::luaMountArchive, "nil TTE_MountArchive(locationID, physPath)",
            "This function is only available to mod scripts! "
            "Mounts a game data archive from the current game snapshot into the resource system, so that all of the files inside that archive"
            " can be found. This supports .ttarch2/ttarch/iso/pk2. Physical path is the path on your local machine. Location ID should be in the format "
            "<XXX>/. Please note that the archive must be from the current game snapshot! Otherwise it will fail to read due to incorrect encryptino and "
            "expected format."
        });
        Col.Functions.push_back({"TTE_MountSystem", &TTE::luaMountSystem, "nil TTE_MountSystem(locationID, physPath, forceLegacy)",
            "This function is only available to mod scripts! "
            "Mounts the resource system (like creating a concrete directory location) to the given physical path under the name locationID."
            " Location ID should be in the format <XXX>/. Physical path can be absolute or relative to your working directory. "
            "The last argument can be set to true to use the old telltale engine resource system which had no resource set descriptions. "
            "This means that it will recurse all directories and add them all. Set this to true to just easily get all resources quickly for debugging,"
            " so that they are all in resource system ready to be used."
        });
        Col.Functions.push_back({"TTE_Switch", &TTE::luaSwitch, "nil TTE_Switch(gameID, platformName, vendorName)",
            "This function is only available to mod scripts! "
            "Switches the editor context to a new game snapshot. Note that this can only be called from a mod "
            "script at startup or when no files are being read or processed. If it is called at one of those"
            " times, an error is thrown to the log and it does nothing."});
        Col.Functions.push_back({"TTE_OpenMetaStream", &TTE::luaOpenMetaStream, "instance TTE_OpenMetaStream(address_or_file, --[[optional]] archive)",
            "This function is only available to mod scripts! "
            "Opens and reads a meta stream, returning the base class instance that the file contains. "
            "This returns a strong reference to the instance. If one argument is passed in, filename is "
            "resource address in the resource system where it is located. If two are "
            "passed in, the first must be the file name string and the second is the archive (any version) which the file is located in."
        });
        Col.Functions.push_back({"TTE_SaveMetaStream", &TTE::luaSaveMetaStream, "bool TTE_SaveMetaStream(address, instance)",
            "This function is only available to mod scripts! "
            "Writes the given instance to the file located at the given resource address. Returns if it was successfully written."
        });
        Col.Functions.push_back({"TTE_OpenTTArchive", &TTE::luaOpenTTArch,
            "arc TTE_OpenTTArchive(filepath)", "This function is only available to mod scripts! "
            "Opens a TTARCH file which is located at the given file path on your computer (or relative, like all file paths, to the executable)."
            " Returns the archive instance, a strong reference although this is not a class instance. "
            "You can then use this to list files and open files from it. Note that this archive MUST match the version of the currently selected game,"
            "ie the archive must be from the game that the editor context is currently working with."
        });
        Col.Functions.push_back({"TTE_OpenTTArchive2", &TTE::luaOpenTTArch2, "arc TTE_OpenTTArchive2(filepath)", "This function is only available to mod scripts! "
            "Opens a TTARCH2 file which is located at the given file path on your computer (or relative). "
            "Returns the archive instance, a strong reference. This archive, like others, must be from the currently"
            " active game such that the encryption key matches."
        });
        Col.Functions.push_back({"TTE_ArchiveListFiles", &TTE::luaArchiveListFiles, "table TTE_ArchiveListFiles(archive)", "This function is only available to mod scripts! "
            "Returns a table (indices 1 to N) of all the file names in the given archive, which was previously opened with open TTArchive or TTArchive2."
        });
    }

    Col.Functions.push_back({ "TTE_Blowfish", &TTE::luaBlowfish,
    "nil TTE_Blowfish(bufferMetaInstance, size, isEncrypt)",
    "Performs a blowfish encryption on the given buffer class instance (must be kMetaClassInternalBinaryBuffer class). "
    "The second argument must be less than or equal to the size of the buffer, how many bytes are to be encrypted or decrypted. "
    "The last argument is if we want to encrypt (true) or decrypt (false). The key used is the current game key. "
    "Note that blowfish encrypts in blocks of eight, ie the last remaining bytes aren't encrypted. All blowfish is like this, however, so don't worry."
    });
    Col.Functions.push_back({ "TTE_TableToContainer", &TTE::luaTableToCollection, "table TTE_TableToContainer(container)",
    "Converts the container into a lua table and returns it. For keyed container, they are the table keys. For non keyed, they are 0 based indices."
    });
    Col.Functions.push_back({ "TTE_ContainerToTable", &TTE::luaContainerToTable, "table TTE_ContainerToTable(container, table)",
        "Inserts all key-value mappings from the input table into the container. For non keyed containers (eg arrays) keys are ignored and its added at the back."
    });
    
    // part of the TTE but can be called async
    Col.Functions.push_back({"TTE_DumpMemoryLeaks", &TTE::luaDumpMemLeaks, "nil TTE_DumpMemoryLeaks()",
        "Dumps to the logger all memory which has not been freed yet. This will contain a lot of script stuff which won't "
        "matter as the script engine requires memory which s tracked, however can be useful for tracking specific script object allocations."
    });
    Col.Functions.push_back({"TTE_GetActiveSnapshot", &TTE::luaActiveGame, "table TTE_GetActiveSnapshot()",
        "Returns the information about the currently active game. The returned table contains keys string "
        "'Name', string 'ID', and string 'Vendor'"
    });
    Col.Functions.push_back({"TTE_Assert", &TTE::luaAssert, "nil TTE_Assert(bool, errorMessage)", "If the first parameter is false, debug builds "
        "will throw an assert and break in the debugger"
    });
    Col.Functions.push_back({"TTE_Log", &TTE::luaLog, "nil TTE_Log(valueStr)", "Logs to the editor logger, the string argument."});
    Col.Functions.push_back({"TTE_GetPlatform", &TTE::luaGetPlatform, "string TTE_GetPlatform()", "Gets the platform name which the Telltale Editor"
        " is running on. This will return strings 'Windows' or 'MacOS', 'Linux' and etc for others in the future."
    });
    Col.Functions.push_back({"TTE_GetMountPoints", &TTE::luaGetMountPoints, "table TTE_GetMountPoints()", "Returns a N indexed array of all the mount"
        " points in the resource system. These are the resource location names. You can use there to find resources."
    });
    
    // REGISTER META API
    
    if(!bWorker)
    {
        Col.Functions.push_back({"MetaRegisterGame",&Meta::L::luaMetaRegisterGame, "nil MetaRegisterGame(gameInfoTable)",
            "This is used in the Games.lua script to register a game to the Meta system on initialisation. "
            "It takes in a table which must have keys string Name, string ID, bool ModifiedEncryption, string DefaultMetaVersion,"
            "string LuaVersion, int/table ArchiveVersion, string/table Key, bool IsArchive2, table Platforms, table Vendors and pushed"
            " capabilities. Archive version and key are either one static key/version or a table of snapshot ID to it. This is just 'Platform/Vendor'"
            " if there are more than just the default (empty string) vendor. Else its just the platform name. See existing examples."
            " If vendors are specified, then DefaultVendor must be as well as a string. The 'CommonSelector' key is also useful. It should specify a "
            "function *name* which takes in the platform, vendor and common class type and returns the class name and version number of the common class "
            "to create for that snapshot and class type. Optional, defaults to the normal class name and version 0."
            " If specified, return just nil and 0 to use default class name as well. Else do special checks dependent on that snapshot."
        });
        Col.Functions.push_back({"MetaPushGameCapability",&Meta::L::luaMetaPushGameCap, "nil MetaPushGameCapability(gameTable, cap)","Push a game capability to the "
            "game table. Call before registering the game table."
        });
        Col.Functions.push_back({"MetaPushExecutableHash",&Meta::L::luaMetaPushHash, "nil MetaPushExecutableHash(gameTable, hashStr, platform, vendor)",
                                "Push an executable hash which maps to the given platform/vendor pair. This is used for the Editor when user selects "
                                "the mount points of their installation such that we can detect their installation game snapshot."
        });
        Col.Functions.push_back({"MetaAssociateFolderExtension",&Meta::L::luaAssignFolderExt,
            "nil MetaAssociateFolderExtension(gameID, mask, folder)", "This associates file name masks to folders. When extracting "
            "files in the Telltale Editor sometimes the user can select to extract to sub-folders. This function associates masks to "
            "sub-folders which the game engines. For example, a typical mapping is any files matching 'module_*.prop' should go into "
            "'Properties/Primitives/'. You should use forward slashes and end with one, although if not it will be changed for you. "
            "This should only be called during initialisation."
        });
        Col.Functions.push_back({"MetaRegisterIntrinsics", &Meta::L::luaMetaRegisterIntrinsics, "nil MetaRegisterIntrinsics()",
            "This registers intrinsic types required for the meta system for the current game."
            " This should only be called a game meta classes registration script (eg WD100.lua)."
        });
        Col.Functions.push_back({"MetaRegisterClass", &Meta::L::luaMetaRegisterClass, "nil MetaRegisterClass(table)",
            "This registers a class to the meta system, passing in the information table. "
            "This must only be called in the initialisation. Table must contain the Name string, VersionIndex number, "
            "optional flags number, optional serialiser script name and optional members table. See example "
            "scripts from games for more information on its use with examples. You can register multiple classes "
            "with the same name as long as they are not exactly the same, ie the version hashes are different."
        });
        Col.Functions.push_back({"MetaRegisterCollection", &Meta::L::luaRegisterCollection, "nil MetaRegisterCollection(table, keyTypeTable, valueTypeTable)",
            "This registers a collection class to the meta system, only to be used in initialisation. "
            "Pass in the same table information as would be used in the normal register class. The table is the information about the collection class. "
            "The second argument is the key type value table, (previously defined and must have been passed into register class). This can be nil for non keyed "
            "collections, such as most arrays, but must be a previoulsy registered table otherwise (eg for Map classes). If this is a static array (SArray) class, "
            "then this should be an integer being the number of elements. The third argument must always be non nil and a table, which is the previously "
            "registered value type in the collection. There is a special case where key or value type table can be strings. "
            "If you pass them as a string, this means they will be resolved once all classes have been registered. This allows forward declaration of "
            "a class inside a collection before its fully defined. Instead of passing in the type table you pass in the string name of the class it "
            "should be. Optionally you can end the string with a semicolon followed by the version index, eg 'class Hello;1', such that version index "
            "1 is used in this example. By default version index 0 is used."
        }); // used in Meta::Initialise4 in engine
    }
    
    Col.Functions.push_back({"MetaGetClassID", &Meta::L::luaMetaGetId, "number MetaGetClassID(table)",
        "Gets the unique class ID for that table. Must be previously registered using the collection or class register function."});
    Col.Functions.push_back({"MetaGetVersionCRC", &Meta::L::luaMetaGetVersion,"number MetaGetVersionCRC(table)",
        "Gets the version hash for that class table. Must be previously registered using the collection or class register function."
    });
    Col.Functions.push_back({"MetaGetClassHash", &Meta::L::luaMetaGetClassHash, "string MetaGetClassHash(table)",
        "Gets the hexadecimal string hash of the given class table. Must be previously registered using the collection or class register "
        "function. Returned as a string as Lua numbers internally are floating points which cannot represent the 64 bit hash properly."
    });
    Col.Functions.push_back({"MetaFlagQuery", &Meta::L::luaMetaFlagQuery, "bool MetaFlagsQuery(flags, flagToTest)",
        "This queries if the given flagToTest exists in the the first flags argument."
    });
    Col.Functions.push_back({"MetaHashByte", &Meta::L::luaMetaHashByte, "number MetaHashByte(initialHash, byte)",
        "CRC32 hashes the given byte and returns the 32 bit hash. Pass in an optional initial hash (else zero). "
        "The byte is a number, in which the bottom 8 bits are hashed, so make sure its in the range of 0 to 255 inclusive. Used for version hashing."
    });
    Col.Functions.push_back({"MetaHashInt", &Meta::L::luaMetaHashInt, "number MetaHashInt(initialHash, int)",
        "CRC32 hashes the given number and returns the 32 bit hash. Pass in an optional initial hash (else zero). "
        "Only the bottom 32 bits are hashed, ie the value of the number, which must be from 0 to about 4 billion. Used for version hashing."
    });
    Col.Functions.push_back({"MetaHashLong", &Meta::L::luaMetaHashLong, "number MetaHashLong(initialHash, valuestring)",
        "Very similar to MetaHashHexString, however the value string must be of length 16 and the value is endian flipped such that it works for little endian machines."
    });
    Col.Functions.push_back({"MetaHashHexString", &Meta::L::luaMetaHashHexString, "number MetaHashHexString(initialHash, valuestring)",
        "CRC32 hashes the given hexadecimal string buffer and returns the 32 bit hash. Pass in an optional initial hash (else zero). "
        "The string must be a set of adjacent hexadecimal digits (padded), and its length must be a multiple of 2. For example '00' "
        "is valid but '0' is not. '00010203' hashes the buffer with bytes 0, 1, 2 and 3 in that order. Used for version hashing."
    });
    Col.Functions.push_back({"MetaHashString", &Meta::L::luaMetaHashString, "number MetaHashString(initialHash, value)",
        "This hashes the given string, CRC32, with an optional initial hash (else set to 0)."
    });
    Col.Functions.push_back({"MetaSetVersionFn", &Meta::L::luaMetaSetVersionCalc, "nil MetaSetVersionFn(functionName)",
        "Sets the function (pass the name not the function) which generates the version hash for the given class table "
        "(the function should return a number, see hashing functions below, and take in the class table). This should be set first, "
        "then call register intrinsics, then the rest of the classes. See existing game scripts for examples."
    });
    Col.Functions.push_back({"MetaDumpVersions", &Meta::L::luaMetaDumpVersions, "nil MetaDumpVersions()",
        "This function simply dumps all of the currently registered meta types to the log, along with their version CRC hashes and version numbers."
    });
    Col.Functions.push_back({"MetaRegisterDuplicate", &Meta::L::luaMetaRegisterDuplicate, "table MetaRegisterDuplicate(source, typename, versionNumber)",
        "Registers a duplicate of the pre-registered class, source. "
        "The only difference is the new name and version number as specified by the last two arguments. Returns the table of the type."
    });
    
    // RUNTIME META API
    
    ADD_FN(Meta::L, "MetaGetClassFlags", luaMetaGetClassFlags, "number MetaGetClassFlags(typename, versionNumber)", "Returns the flags of a given class.");
    ADD_FN(Meta::L, "MetaGetMemberFlags", luaMetaGetMemberFlags, "number MetaGetMemberFlags(typename, versionNumber, memberName)",
           "Returns the flags of the given member of the given class.");
    ADD_FN(Meta::L, "MetaGetClass", luaMetaGetClass, "typename, versionNumber MetaGetClass(instance)",
           "Returns the class information of a given class instance. For a similar function to find the class "
           "information about a class serialised inside a meta stream, use MetaStreamFindClass.");
    ADD_FN(Meta::L, "MetaGetMemberClass", luaMetaGetMemberClass, "typename, versionNumber MetaGetMemberClass(typename, versionNumber, member)",
           "Returns the class name and version number of the member specified of the class specified.");
    ADD_FN(Meta::L, "MetaGetMember", luaMetaGetMember,"instance MetaGetMember(instance, member)",
           "Obtains a weak reference (instance) to the given member variable in the given class instance in passed."
           " Pass in the name of the member variable as the second argument. ");
    ADD_FN(Meta::L, "MetaGetClasses", luaMetaClasses, "table MetaGetClasses()",
           "Returns a table of all the class names. Returns table indexed 1 to N. Does not take into account version numbers.");
    ADD_FN(Meta::L, "MetaSetClassValue", luaMetaSetClassValue, "nil MetaSetClassValue(instance, value)",
           "Similar to the get class function. Sets the value argument. Ensure that the type matches what you are setting it to, as it won't be casted.");
    ADD_FN(Meta::L, "MetaGetClassValue", luaMetaGetClassValue, "value MetaGetClassValue(instance)",
           "If the instance is an intrinsic type, it is converted to the equivalent Lua type. Strings, booleans, "
           "floats and doubles translate as normal. Integers, unsigned or signed, casted all to the same internal Lua number type, "
           "so this can be used for all integer types. Symbols are converted to symbol strings. Integers of width 64, signed and "
           "unsigned, are always converted to symbol strings as well, as the Lua number type cannot hold them to the correct value. "
           "If it is not one of those types, a normal weak reference is returned as it is a compound, normal class type.");
    ADD_FN(Meta::L, "MetaGetEnumFlags", luaMetaGetEnumFlags, "table MetaGetEnumFlags(typename, versionNumber, member)",
           "Given the member in the given class, this function returns a table of string enum or flag name to enum or flag "
           "integer value. The returned table is not a table of tables like when registering enums or flags, however is a "
           "table where the keys are the enum or flag names and the values are the integer values. Note that this works if "
           "the member is an enum or a flag. Returns nil if the member is not an enum or flag member.");
    ADD_FN(Meta::L, "MetaGetMemberNames", luaMetaGetMemberNames, "table MetaGetMemberNames(typename, versionNumber)",
           "Returns a table of the member variable names in the given class. Indexed from 1 to N.");
    ADD_FN(Meta::L, "MetaCreateInstance", luaMetaCreateInstance, "instance MetaCreateInstance(typename, versionNumber, name, parent)",
           "Creates an instance of the class specified by the first two arguments. The third argument is the name you want to "
           "give to the returned child instance. You can then access this returned instance later by finding the child instance "
           "by this unique name. Pass in a valid instance as the parent instance, which the returned instance's lifetime depends totally on.");
    ADD_FN(Meta::L, "MetaCopyInstance", luaMetaCopyInstance,"instance MetaCopyInstance(instance, name, parent)"
           , "Creates a new instance, copying everything from the 1st argument into the returned instance, "
           "leaving the first one unchanged. Rest of arguments are the same as CreateInstance. Parent and instance cannot be the same.");
    ADD_FN(Meta::L, "MetaMoveInstance", luaMetaMoveInstance, "instane MetaMoveInstance(instance, name, parent)",
           "Creates a new instance, moving everything from the 1st argument into the returned instance, leaving "
           "the first one alive but empty like it was just created. Rest of arguments are the same as CreateInstance. Parent and instance cannot be the same.");
    ADD_FN(Meta::L, "MetaIsCollection", luaMetaIsCollection, "bool MetaIsCollection(instance_Or_typename, --[[optional]] versionNumber)",
           "Returns true if the given instance (1 argument) else is a collection class. If two arguments, pass in the typename and version number to test.");
    ADD_FN(Meta::L, "MetaToString", luaMetaToString, "string MetaToString(instance)",
           "Converts the given instance to a string. If the class of the instance has a defined to string meta operation, "
           "then that is called. All intrinsic types such as integer are converted to string by default. "
           "Higher level typed, is used defined typed, return a list of the member names and the instance values formatted nicely in a string.");
    ADD_FN(Meta::L, "MetaEquals", luaMetaEquals, "bool MetaEquals(instance1, instance2)",
           "Same as less than but returns true only if they are equal using the comparison meta operation.");
    ADD_FN(Meta::L, "MetaLessThan", luaMetaLessThan, "bool MetaLessThan(instanceL, instanceR)",
           "Compares the two instances, using their less than meta operation. For intrinsic types such as floats and integers, "
           "this does the usual. Else this will return false for other types without a defined comparison operation.");
    ADD_FN(Meta::L, "MetaGetSerialisedVersionInfoFileName", luaMetaSVIF, "string MetaGetSerialisedVersionInfoFileName(className, classVersionNumber, --[[optional]] bAltFileName)",
           "Returns the .VERS file name for the given class. Optionally pass in to use the alternative file name (without struct etc), which is generally not used but is "
           "done by Telltale at runtime but not saved on disk.");
    ADD_FN(Meta::L, "MetaGetChild", luaMetaGetChild, "instance MetaGetChild(instance, name)",
           "Returns a weak reference to the child instance associated with the given parent under the given name.");
    ADD_FN(Meta::L, "MetaGetNumChildren", luaMetaGetChildrenCount, "number MetaGetNumChildren(instance)",
           "Returns the number of named children that the given instance holds. "
           "This does not include any instances returned with GetMember, but only MetaCreateInstance.");
    ADD_FN(Meta::L, "MetaGetChildrenNames", luaMetaGetChildrenNames, "table MetaGetChildrenNames(instance)",
           "Returns a table (indexed from 1 to N)  of all of the children names in the given instance. Each value "
           "in the table is a string. Some children names may be hash strings (length 16, upper case hex) if the "
           "symbol could not be resolved. Internally child names are stored as symbols. Try to use global symbols,"
           " and use SymbolCompare to compare, so reduce this risk.");
    ADD_FN(Meta::L, "MetaReleaseChild", luaMetaReleaseChild, "nil MetaReleaseChild(instance, name)",
           "Releases the given child (identified by name) of the given instance. Use this to remove it from the internal map such "
           "that it will get destroyed after (unless a lower level C++ object still obtains a reference to it). Calls to get child after this will return nil.");
    ADD_FN(Meta::L, "MetaInstanceAlive", luaMetaInstanceAlive, "bool MetaInstanceAlive(instance)",
           "Returns true if the given instance of a class is alive. Returns false if the instance is "
           "invalid or the parent of the instance is no longer alive - meaning you cannot access the instance anymore as it does not exist.");
    
    // META STREAM API
    
    ADD_FN(MS, "MetaStreamAsyncSerialiseEnabled", luaMetaStreamAsyncSerialiseEnabled, "bool MetaStreamAsyncSerialiseEnabled(stream)",
           "Returns if asynchronous serialisation is possible for the current serialisation");
    ADD_FN(MS, "MetaStreamReadInt", luaMetaStreamReadInt, "number MetaStreamReadInt(stream)",
           "Reads a signed 4 byte integer from the given meta stream and returns it.");
    ADD_FN(MS, "MetaStreamReadByte", luaMetaStreamReadByte, "number MetaStreamReadByte(stream)",
           "Reads a signed 1 byte integer from the given meta stream and returns it.");
    ADD_FN(MS, "MetaStreamReadBool", luaMetaStreamReadBool, "bool MetaStreamReadBool(stream)", "Reads a boolean from the meta stream.");
    ADD_FN(MS, "MetaStreamReadShort", luaMetaStreamReadShort, "number MetaStreamReadShort(stream)",
           "Reads a signed 2 byte integer from the given meta stream and returns it.");
    ADD_FN(MS, "MetaStreamReadString", luaMetaStreamReadString, "string MetaStreamReadString(stream)",
           "Reads a string, reading 4 bytes as the length and then that many bytes as the ASCII string data.");
    ADD_FN(MS, "MetaStreamReadSymbol", luaMetaStreamReadSymbol, "string MetaStreamReadSymbol(stream)",
           "Reads an unsigned 64 bit integer as a symbol, returning it as a hexadecimal string of length 16.");
    ADD_FN(MS, "MetaStreamAdvance", luaMetaStreamAdvance, "nil MetaStreamAdvance(stream, nBytes)",
           "Advances by the given number of bytes in the current meta stream. Used in read mode to skip over data which we don't need or know the correct value already.");
    ADD_FN(MS, "MetaStreamWriteZeros", luaMetaStreamWriteZeros, "nil MetaStreamWriteZeros(stream, N)", "Writes zeros to the meta stream in the current section.");
    ADD_FN(MS, "MetaStreamWriteInt", luaMetaStreamWriteInt, "nil MetaStreamWriteInt(stream, value)",
           "Writes the argument to the stream argument as a 4 byte signed integer.");
    ADD_FN(MS, "MetaStreamWriteFloat", luaMetaStreamWriteFloat, "nil MetaStreamWriteFloat(stream, value)",
           "Writes the float argument to the stream argument.");
    ADD_FN(MS, "MetaStreamReadFloat", luaMetaStreamReadFloat, "float MetaStreamReadFloat(stream, value)",
           "Reads a float from the meta stream");
    ADD_FN(MS, "MetaStreamWriteByte", luaMetaStreamWriteByte, "nil MetaStreamWriteByte(stream, value)",
           "Writes the argument to the stream argument as a 1 byte signed integer.");
    ADD_FN(MS, "MetaStreamWriteShort", luaMetaStreamWriteShort, "nil MetaStreamWriteShort(stream, value)",
           "Writes the argument to the stream argument as a 2 byte signed integer.");
    ADD_FN(MS, "MetaStreamGetFileName", luaMetaStreamGetFileName, "str MetaStreamGetFileName(stream)",
           "Returns the name of the currently processing file. Pass in the meta stream.");
    ADD_FN(MS, "MetaStreamWriteString", luaMetaStreamWriteString,
           "nil MetaStreamWriteString(stream, value)", "Writes the length of the stream as a 4 byte signed integer followed by the string ASCII data.");
    ADD_FN(MS, "MetaStreamWriteBool", luaMetaStreamWriteBool, "nil MetaStreamWriteBool(stream, value)", "Writes a boolean to the meta stream given.");
    ADD_FN(MS, "MetaStreamWriteSymbol", luaMetaStreamWriteSymbol, "nil MetaStreamWriteSymbol(stream, value)",
           "Writes the symbol argument. If it is a 16 byte hex string hash with <>, the value is represents is written, else the string"
           " is hashed on the lower case version. Note that SymbolXXX can be used to create and manage symbols.");
    ADD_FN(MS, "MetaStreamSetMainSection", luaMetaStreamSetMainSection, "nil MetaStreamSetMainSection(stream)",
           "Sets the current section to write to in stream as the main one, this is the default one and is by default where all "
           "writes go to. When calling any of the SetXXXSection functions, note that block sizes are restricted to their section that "
           "they are called in. If you call start block in the main section and then an end block call in the async section, "
           "it will error as the main block is waiting for to call end block, not the async one.");
    ADD_FN(MS, "MetaStreamSetAsyncSection", luaMetaStreamSetAsyncSection, "nil MetaStreamSetAsyncSection(stream)",
           "Sets the current section in the stream as the async section. This is where buffers in meshes and textures "
           "normally store their data. Do not use this in games which don't use meta stream versions of MSV5 or 6.");
    ADD_FN(MS, "MetaStreamSetDebugSection", luaMetaStreamSetDebugSection,"nil MetaStreamSetDebugSection(stream)",
           "Sets the current section in the stream as the debug section. This is where buffers in meshes and textures normally "
           "store their data. Do not use this in games which don't use meta stream versions of MSV5 or 6.");
    ADD_FN(MS, "MetaStreamBeginBlock", luaMetaStreamBeginBlock, "nil MetaStreamBeginBlock(stream, isWrite)",
           "Starts a block size in the stream, in the current section. Pass in if you are writing or reading to the stream currently. You must have a call "
           "to end block some time after this call to write the size as the difference between the offsets in the file when begin and end offset are called.");
    ADD_FN(MS, "MetaStreamEndBlock", luaMetaStreamEndBlock, "nil MetaStreamEndBlock(stream, isWrite)",
           "Ends a block in the stream in the current section. BeginBlock must have previously been called in the "
           "current section in the meta stream. Pass in if you are reading or writing to the meta stream currently.");
    ADD_FN(MS, "MetaGetBufferSize", luaMetaBufferSize, "number MetaGetBufferSize(bufferInstance)",
           "Returns the size of the buffer instance (kMetaClassInternalBinaryBuffer class). "
           "This size is set when you read a buffer from a meta stream - the argument is saved along side the buffer internal.");
    ADD_FN(MS, "MetaStreamReadBuffer", luaMetaStreamReadBuffer, "nil MetaStreamReadBuffer(stream, bufferInstance, size)",
           "Reads size bytes from the stream into the buffer instance. The buffer instance must be of the class kMetaClassInternalBinaryBuffer, "
           "so must be a member variable of another class. This type is never serialised and is used so you can hold a memory block in "
           "which it is freed after the class instance is destroyed. You can declare member variables with this type and then use it to store the buffer.");
    ADD_FN(MS, "MetaStreamWriteBuffer", luaMetaStreamWriteBuffer, "nil MetaStreamWriteBuffer(stream, bufferInstance)",
           "See above for information about bufferInstance. Does not write the size. Writes the buffer in its entirety, size bytes.");
    ADD_FN(MS, "MetaStreamReadCache", luaMetaStreamReadCache, "nil MetaStreamReadCache(stream, cacheInstance, --[[optional]] size)",
           "Reads size bytes from the stream into the buffer instance. The buffer instance must be of the class kMetaClassInternalDataStreamCache, "
           "so must be a member variable of another class. This type is very similar to the binary buffer type above, however it stores a cache "
           "of the range of bytes from the current position in the stream to the current position plus the size parameter. What this means is that "
           "even if the file is finished reading from it will stay open as long as the instance is alive. This is useful for large data blocks which "
           "we wouldn't want in memory such as archives of files. By default the size is -1 and doesn't need to be passed in, suggesting that all remaining bytes"
           " should be cached. After this call the meta stream will have effectively read all these bytes so the next read bytes will be after this.");
    ADD_FN(MS, "MetaStreamWriteCached", luaMetaStreamWriteCached, "nil MetaStreamWriteCached(stream, cacheInstance)",
           "See above for information about cacheInstance. Does not write the size. Writes the cached bytes in its entirety to the stream.");
    ADD_FN(MS, "MetaStreamWriteDDS", luaMetaStreamWriteDDS, "nil MetaStreamWriteDDS(stream, table)",
           "Writes a DDS file header with the given information. It must include the same information as returned by the read version. See MetaStreamReadDDS for information about the table values.");
    ADD_FN(MS, "MetaStreamDumpBuffer", luaMetaStreamDumpBuffer, "nil MetaStreamDumpBuffer(binaryBuffer, offset, size, outFileName)",
           "Dumps the binary buffer out to a file name. Pass in the offset in the buffer and number of bytes to dump.");
    ADD_FN(MS, "MetaStreamGetDDSHeaderSize", luaMetaStreamDDSSize, "number MetaStreamGetDDSHeaderSize(table)",
           "Gets the number of bytes that would be written or have been read from the given DDS header table that "
           "either you contruct or was returned in the read function above. This is the number of bytes read or would be written.");
    ADD_FN(MS, "MetaStreamReadDDS", luaMetaStreamReadDDS, "table MetaStreamReadDDS(stream)",
           "Reads a DDS file header returning a table with its information. See Gitbook documentation in the Meta System page for the table values.");
    ADD_FN(MS, "MetaGetCachedSize", luaMetaStreamCachedSize, "number MetaGetCachedSize(cacheInstance)",
           "Returns the size of the cached data stream instance (kMetaClassInternalDataStreamCache class). "
           "This size is set when you read a buffer from a meta stream - its held internally.");
    ADD_FN(MS, "MetaStreamFindClass", luaMetaStreamFindClass,"typename, versionNumber MetaStreamFindClass(stream, typeSymbol)",
           "Finds the class in the meta system associated with the given type symbol for the given meta stream. The symbol "
           "is either a symbol (16 byte hex hash string with <>) or a string which will be hashed. Examples of use are passing in a symbol "
           "read from the meta stream when the meta stream contains a class determined by its symbol also stored in the meta "
           "stream. Returns the class type name and version number associated. The class is found by searching the meta stream header for "
           "the type name symbol, if it is found then the returned class is one with the same version hash as in the header of the meta stream. "
           "If it is not found in the header, then it may still be valid as intrinsic types are container types are not in headers. In those cases, "
           "the type with version number zero is returned - as those types don't have different versions and if they do they are never used.");
    ADD_FN(Meta::L, "MetaSerialiseDefault", luaMetaSerialiseDefault, "bool MetaSerialiseDefault(stream, instance, isWrite)",
           "Default serialises the given class instance to/from the given stream in the given serialisation mode. "
           "See documentation for more information about this function. Returns if success or failed."); // serialise related
    ADD_FN(Meta::L, "MetaSerialise", luaMetaSerialise, "bool MetaSerialise(stream, instance, isWrite, --[[optional]] debugName)",
           "Serialises the given instance to/from the given stream in the given serialisation mode. "
           "This will decide whether to call the custom serialisation, if there is one, or else to call the default serialiser. "
           "Use this to serialise any class instance to/from the given stream, as opposed to the default serialiser which "
           "should be called in a custom serialiser if needed."
           " Optionally specify a debug name to put into any debug outputs when reading.");
    
    // MISC
    
    ADD_FN(LuaMisc, "SymbolTableFind", luaSymbolFind, "string SymbolTableFind(symbolString)",
           "Searches the global symbol table for the given symbol hash (argument must be a symbol). "
           "If it is found, the string version is returned, else an empty string.");
    ADD_FN(LuaMisc, "SymbolTableRegister", luaSymbolReg, "nil SymbolTableRegister(string)",
           "Registers a string such that a call to find after with the symbol whose hash is equal to the arguments hash, will return the argument.");
    ADD_FN(LuaMisc, "SymbolTableClear", luaSymbolClear, "nil SymbolTableClear()",
           "Clears the global symbol table. Do not use this function unless there is an explicit reason to do so: you will loose all string versions of hashes.");
    ADD_FN(LuaMisc, "SymbolCompare", luaSymbolCmp,"bool SymbolCompare(left, right)",
           "Compares if the symbol hash of left and right are equal to each other. Either or both of them can be an un-hashed string, "
           "or any of them can be a hashed string (ie a 16 byte hex hash string with <>).");
    ADD_FN(LuaMisc, "SymbolCreate", luaSymbol, "symbol SymbolCreate(string)",
           "Creates a symbol, by hashing the string argument in lower case. If the string argument "
           "is itself a string 16 byte hex hash with <>, it will still be hashed so be careful.");
    
    // Global LUA flag constants for use in the library init scripts
    
    PUSH_GLOBAL_I(Col, "kMetaClassNonBlocked", Meta::CLASS_NON_BLOCKED, "Class is not blocked");
    PUSH_GLOBAL_I(Col, "kMetaClassIntrinsic", Meta::CLASS_INTRINSIC, "Class is intrinsic");
    PUSH_GLOBAL_I(Col, "kMetaClassAllowAsync", Meta::CLASS_ALLOW_ASYNC, "Can be serialised asynchronously");
    PUSH_GLOBAL_I(Col, "kMetaClassContainer", Meta::CLASS_CONTAINER, "Container flag");
    PUSH_GLOBAL_I(Col, "kMetaClassAbstract", Meta::CLASS_ABSTRACT, "Class is abstract");
    PUSH_GLOBAL_I(Col, "kMetaClassAttachable", Meta::CLASS_ATTACHABLE, "Class is attachable");
    PUSH_GLOBAL_I(Col, "kMetaClassProxy", Meta::CLASS_PROXY, "Proxy class disables member blocking");
    PUSH_GLOBAL_I(Col, "kMetaClassEnumWrapper", Meta::CLASS_ENUM_WRAPPER, "Is an enum wrapper class. Should have one member (normally mVal), as integer value.");
    
    PUSH_GLOBAL_I(Col, "kMetaMemberEnum", Meta::MEMBER_ENUM, "Member is an enum");
    PUSH_GLOBAL_I(Col, "kMetaMemberFlag", Meta::MEMBER_FLAG, "Member is a flag");
    PUSH_GLOBAL_I(Col, "kMetaMemberBaseClass", Meta::MEMBER_BASE, "Member is a base class");
    PUSH_GLOBAL_I(Col, "kMetaMemberMemoryDisable", Meta::MEMBER_MEMORY_DISABLE, "Member is excluded from memory");
    PUSH_GLOBAL_I(Col, "kMetaMemberSerialiseDisable", Meta::MEMBER_SERIALISE_DISABLE, "Member is excluded from disk");
    PUSH_GLOBAL_I(Col, "kMetaMemberVersionDisable", Meta::MEMBER_VERSION_HASH_DISABLE, "Excluded from version hash");
    
    PUSH_GLOBAL_DESC(Col, "kMetaUInt32", "Intrinsic unsigned 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaInt", "Intrinsic signed 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUInt", "Alias for unsigned 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaInt32", "Alias for signed 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUInt64", "Intrinsic unsigned 64-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaInt64", "Intrinsic signed 64-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMeta__UnsignedInt64", "Alias for unsigned 64-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaLong", "Alias for signed 32-bit integer (Windows)");
    PUSH_GLOBAL_DESC(Col, "kMetaFloat", "Intrinsic 32-bit floating point");
    PUSH_GLOBAL_DESC(Col, "kMetaDouble", "Intrinsic 64-bit floating point");
    PUSH_GLOBAL_DESC(Col, "kMetaBool", "Intrinsic boolean type");
    PUSH_GLOBAL_DESC(Col, "kMetaInt8", "Intrinsic signed 8-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedInt8", "Intrinsic unsigned 8-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaInt16", "Intrinsic signed 16-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedInt16", "Intrinsic unsigned 16-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaSignedChar", "Alias for signed 8-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaChar", "Alias for signed 8-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaLongDoubler", "Alias for 64-bit float (double)");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedChar", "Alias for unsigned 8-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedInt", "Alias for unsigned 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedLong", "Alias for unsigned 32-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedShort", "Alias for unsigned 16-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaShort", "Alias for signed 16-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaWideChar", "Wide character (typically UTF-16)");
    PUSH_GLOBAL_DESC(Col, "kMetaLongLong", "Alias for signed 64-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMetaUnsignedLongLong", "Alias for unsigned 64-bit integer");
    PUSH_GLOBAL_DESC(Col, "kMeta__Int64", "Alias for signed 64-bit integer");
    
    PUSH_GLOBAL_DESC(Col, "kMetaSymbol", "Intrinsic Symbol type");
    PUSH_GLOBAL_DESC(Col, "kMetaClassSymbol", "Intrinsic Symbol type with class prefix"); // with class prefix
    
    PUSH_GLOBAL_DESC(Col, "kMetaString", "Intrinsic String type");
    PUSH_GLOBAL_DESC(Col, "kMetaClassString", "Intrinsic String type with class prefix"); // with class prefix
    
    PUSH_GLOBAL_DESC(Col, "kMetaClassInternalBinaryBuffer", "Internal binary buffer class"); // internal class
    PUSH_GLOBAL_DESC(Col, "kMetaClassInternalDataStreamCache", "Internal data stream cache class"); // internal class

    
    // CAPS
    
    const GameCapDesc* pDesc = GameCapDescs;
    while(pDesc->Cap != GameCapability::NONE)
    {
        PUSH_GLOBAL_I(Col, pDesc->Name, (U32)pDesc->Cap, "Game capabilities");
        pDesc++;
    }
    
    return Col;
}
