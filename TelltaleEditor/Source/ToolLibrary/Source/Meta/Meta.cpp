#define _META_COERSION_IMPL
#include <Meta/Meta.hpp>
#include <Core/Context.hpp>
#include <Scripting/LuaManager.hpp>
#include <Core/Symbol.hpp>
#include <Core/Callbacks.hpp>
#include <Resource/PropertySet.hpp>

#include <sstream>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ===================================================================         META
// ===================================================================

namespace Meta {


    U32 RegGame::GetArchiveVersion(GameSnapshot snapshot) const
    {
        String snap = snapshot.Platform;
        if(snapshot.Vendor.length())
            snap = snap + "/" + snapshot.Vendor;
        auto it = SnapToArchiveVersion.find(snap);
        if(it == SnapToArchiveVersion.end())
        {
            snap = snapshot.Platform;
            it = SnapToArchiveVersion.find(snap);
            return it == SnapToArchiveVersion.end() ? MasterArchiveVersion : it->second;
        }
        return it->second;
    }

    BlowfishKey RegGame::GetEncryptionKey(GameSnapshot snapshot) const
    {
        String snap = snapshot.Platform;
        if (snapshot.Vendor.length())
            snap = snap + "/" + snapshot.Vendor;
        auto it = SnapToEncryptionKey.find(snap);
        if (it == SnapToEncryptionKey.end())
        {
            snap = snapshot.Platform;
            it = SnapToEncryptionKey.find(snap);
            return it == SnapToEncryptionKey.end() ? MasterKey : it->second;
        }
        return it->second;
    }
    
    InternalState State{};
    
    const InternalState& GetInternalState()
    {
        return State;
    }
    
    // ===================================================================         IMPL
    // ===================================================================
    
    namespace _Impl {
        
        Bool _CheckPlatformForGame(RegGame& game, const String& platform)
        {
            for(auto& p: game.ValidPlatforms)
                if(CompareCaseInsensitive(p, platform))
                    return true;
            return false;
        }
        
        Bool _CheckVendorForGame(RegGame& game, const String& vendor)
        {
            if(game.ValidVendors.size() == 0)
                return true; // most have only 1 branch/vendor
            for(auto& p: game.ValidVendors)
                if(CompareCaseInsensitive(p, vendor))
                    return true;
            return false;
        }
        
        Bool _CheckPlatform(const String& name)
        {
            constexpr U32 platforms = sizeof(PlatformNames) / sizeof(CString);
            for(U32 i = 0; i < platforms; i++)
            {
                if(CompareCaseInsensitive(name, PlatformNames[i]))
                    return true;
            }
            return false;
        }
        
        U32 _GenerateClassID(U64 typeHash, U32 versionNumber)
        {
            return CRC32((U8*)&typeHash, 8, versionNumber ^ 0xF0F0F0F0);
            // needs to be unique. hash the type hash with initial crc being the version number inverted every 4 bits
        }

        Bool _CollectCompiledScriptFunctionMT(CompiledScript& compiledScript, const String& fn)
        {
            ScriptManager::GetGlobal(GetToolContext()->GetLibraryLVM(), fn, true);

            if (GetToolContext()->GetLibraryLVM().Type(-1) != LuaType::FUNCTION)
            {
                GetToolContext()->GetLibraryLVM().Pop(1);
                TTE_LOG("The script function '%s' does not exist as a function.", fn.c_str());
                return false;
            }

            // function is on the top of the stack, compile it.
            DataStreamRef localWriter = DataStreamManager::GetInstance()->CreatePrivateCache(fn);
            if (!GetToolContext()->GetLibraryLVM().Compile(localWriter.get()) || localWriter->GetSize() <= 0)
            {
                GetToolContext()->GetLibraryLVM().Pop(1); // pop the func
                TTE_LOG("Cannot register class script function %s"
                           ": compile failed or empty", fn.c_str());
                return false;
            }
            GetToolContext()->GetLibraryLVM().Pop(1); // pop the func

            localWriter->SetPosition(0); // seek to beginning, then read all bytes.
            U8* compiledBytes = TTE_ALLOC(localWriter->GetSize(), MEMORY_TAG_SCRIPTING);

            if (!localWriter->Read(compiledBytes, localWriter->GetSize()))
            {
                TTE_LOG("Could not read bytes from compiled serialiser script stream");
                TTE_FREE(compiledBytes);
                return false;
            }

            compiledScript.Binary = compiledBytes;
            compiledScript.Size = (U32)localWriter->GetSize();
            return true;
        }

        U32 _ResolveCommonClassIDSafe(CommonClass clz)
        {
            String className = CommonClassInfo::GetCommonClassInfo(clz).ClassName;
            if(State.Games[State.GameIndex].CommonSelector.empty())
            {
                return FindClass(className, 0, false); // default
            }
            const CompiledScript& collector = State.Collector;
            if(collector.Binary == nullptr)
            {
                return FindClass(className, 0, false); // default
            }
            const String& collectorName = State.GetActiveGame().CommonSelector;
            LuaManager& LVM = GetThreadLVM();
            ScriptManager::GetGlobal(LVM, collectorName, true);
            if (LVM.Type(-1) != LuaType::FUNCTION)
            {
                LVM.Pop(1);
                if (!LVM.LoadChunk(collectorName, collector.Binary, collector.Size, LoadChunkMode::BINARY))
                {
                    TTE_ASSERT(false, "Cannot select class version for %s: common selector failed to load", collectorName.c_str());
                    return 0; // FAIL
                }
            }
            GameSnapshot snapshot = GetToolContext()->GetSnapshot();
            LVM.PushLString(snapshot.Platform);
            LVM.PushLString(snapshot.Vendor);
            LVM.PushInteger((I32)clz);
            LVM.CallFunction(3, 1, true);
            if(LVM.Type(1) != LuaType::NUMBER)
            {
                TTE_ASSERT(false, "Cannot select class version at %s: the function failed or did not return version number!", collectorName.c_str());
                return 0; // FAIL
            }
            U32 num = (U32)ScriptManager::PopUnsignedInteger(LVM);
            if(num > MAX_VERSION_NUMBER)
            {
                TTE_ASSERT(false, "Cannot select class version at %s: version number %u is too large!", collectorName.c_str(), num);
                return 0; // FAIL
            }
            return FindClass(className, num, false);
        }

        void _FreeCompiledScriptMT(CompiledScript& script)
        {
            if(script.Binary)
            {
                TTE_FREE(script.Binary);
                script.Binary = nullptr;
                script.Size = 0;
            }
        }
        
        void _PushCompiledScript(std::map<Symbol, CompiledScript>& scriptMap, const String& fn)
        {
            if(fn.length() > 0)
            {
                // register serialise and compile it
                if(scriptMap.find(fn) == scriptMap.end())
                {
                    // compile and add it                
                    CompiledScript compiledScript{};
                    if(_CollectCompiledScriptFunctionMT(compiledScript, fn))
                    {
                        scriptMap[fn] = compiledScript; // save it
                    }        
                }
            }
        }
        
        void _CalculateCollectionSizes(Class* pKey, Class* pVal, U32& _ValuSize, U32& _PairSize, U32& _ColFl)
        {
            
            // flags init
            if(pVal->Constructor == nullptr && (pVal->Flags & CLASS_INTRINSIC) && !(pVal->Flags & CLASS_ATTACHABLE) && pVal->Members.size() == 0)
                _ColFl |= _COL_VAL_SKIP_CT;
            if(pVal->Destructor == nullptr && (pVal->Flags & CLASS_INTRINSIC) && !(pVal->Flags & CLASS_ATTACHABLE) && pVal->Members.size() == 0)
                _ColFl |= _COL_VAL_SKIP_DT;
            if(pVal->CopyConstruct == nullptr && (pVal->Flags & CLASS_INTRINSIC) && !(pVal->Flags & CLASS_ATTACHABLE) && pVal->Members.size() == 0)
                _ColFl |= _COL_VAL_SKIP_CP;
            if(pVal->MoveConstruct == nullptr && (pVal->Flags & CLASS_INTRINSIC) && !(pVal->Flags & CLASS_ATTACHABLE) && pVal->Members.size() == 0)
                _ColFl |= _COL_VAL_SKIP_MV;
            
            _ValuSize = pVal->RTSize;
            
            if(pKey) // if this is a Map type
            {
                // we will need padding bytes if the key type is small (lower than 8 bytes) and the value type is big (8 bytes or larger)
                
                // key flags
                if(pKey->Constructor == nullptr && (pKey->Flags & CLASS_INTRINSIC) && !(pKey->Flags & CLASS_ATTACHABLE) && pKey->Members.size() == 0)
                    _ColFl |= _COL_KEY_SKIP_CT;
                if(pKey->Destructor == nullptr && (pKey->Flags & CLASS_INTRINSIC) && !(pKey->Flags & CLASS_ATTACHABLE) && pKey->Members.size() == 0)
                    _ColFl |= _COL_KEY_SKIP_DT;
                if(pKey->CopyConstruct == nullptr && (pKey->Flags & CLASS_INTRINSIC) && !(pKey->Flags & CLASS_ATTACHABLE) && pKey->Members.size() == 0)
                    _ColFl |= _COL_KEY_SKIP_CP;
                if(pKey->MoveConstruct == nullptr && (pKey->Flags & CLASS_INTRINSIC) && !(pKey->Flags & CLASS_ATTACHABLE) && pKey->Members.size() == 0)
                    _ColFl |= _COL_KEY_SKIP_MV;
                
                
                if(pKey->RTSize < 8) // we need alignment bytes between key and value in memory. max align is 8 !!
                {
                    if(pVal->RTSize < 8) // both are small types, intrinsic types.
                    {
                        // we could use __builtin_clz to detect bit indices, we know its only used for values 1 to 7, so macro can be used
                        // use _TO_ALIGN since align isnt stored in State.Classes. sizes 7-4 have align 4, 3-2 align 2 and 1 align 1, as best approx.
                        
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
        
        // register class argument to meta system
        U32 _Register(LuaManager& man, Class&& cls, I32 stackIndex)
        {
            TTE_ASSERT(IsCallingFromMain(), "Must only be called from main thread"); // modifications to State.Classes should only happen at beginning on main.
            
            cls.TypeHash = CRC64LowerCase((const U8*)cls.Name.c_str(), (U32)cls.Name.length());
            String tool = MakeTypeName(cls.Name);
            Bool bIsProp = CompareCaseInsensitive(tool, "PropertySet");
            cls.ToolTypeHash = CRC64LowerCase((const U8*)tool.c_str(), (U32)tool.length());
            GetRuntimeSymbols().Register(cls.Name);
            U32 crc = _DoLuaVersionCRC(man, stackIndex);
            if(crc == 0)
                return 0; // errored
            U32 id = _GenerateClassID(cls.TypeHash, cls.VersionNumber);
            
            // chcek any duplicate version number
            if(State.Classes.find(id) != State.Classes.end())
            {
                TTE_LOG("Duplicate type detected for '%s': multiple same version indices "
                        "for the given class! Was a class registered twice?", cls.Name.c_str());
                return 0;
            }
            
            _PushCompiledScript(State.Serialisers, cls.SerialiseScriptFn);
            _PushCompiledScript(State.Normalisers, cls.NormaliserStringFn);
            _PushCompiledScript(State.Specialisers, cls.SpecialiserStringFn);
            
            if(cls.RTSize == 0)
            { // if not a c++ defined intrinsic, calc its size
                if(cls.Members.size() == 0)
                {
                    cls.RTSize = 1; // default size 1 byte
                }
                else
                {
                    cls.RTSize = 0;
                    for(auto& member: cls.Members)
                    {
                        if(!(member.Flags & MEMBER_MEMORY_DISABLE)) // ignore MEMORY_DISABLE members in the size
                        {
                            // if member type is >= 8 bytes, ensure 8 byte aligned.
                            if(State.Classes[member.ClassID].RTSize >= 8)
                                cls.RTSize += TTE_PADDING(cls.RTSize, 8);
                            else if(State.Classes[member.ClassID].RTSize >= 4) // else 4
                                cls.RTSize += TTE_PADDING(cls.RTSize, 4);
                            else if(State.Classes[member.ClassID].RTSize >= 2) // else 2
                                cls.RTSize += TTE_PADDING(cls.RTSize, 2);
                            
                            member.RTOffset = cls.RTSize;
                            cls.RTSize += State.Classes[member.ClassID].RTSize;
                        }
                    }
                }
                cls.RTSizeRaw = cls.RTSize;
                if(cls.Flags & CLASS_ATTACHABLE)
                {
                    cls.RTPaddingChildren = TTE_PADDING(cls.RTSize, 8);
                    cls.RTSize += cls.RTPaddingChildren;
                    cls.RTSize += sizeof(ClassChildMap);
                }
            }
            
            if(bIsProp)
            {
                cls.Flags |= _CLASS_PROP;
                cls.Flags |= CLASS_ATTACHABLE; // ensure
                cls.RTPaddingCallbacks = TTE_PADDING(cls.RTSize, 8);
                cls.RTSize += cls.RTPaddingCallbacks;
                cls.RTSize += sizeof(PropertySet::InternalData);
            }
            
            if(cls.Flags & CLASS_ENUM_WRAPPER)
            {
                cls.ToString = &_EnumFlagToString;
            }
            
            if((cls.Flags & CLASS_CONTAINER) && cls.ArraySize > 0)
            {
                Class* pVal = &State.Classes[cls.ArrayValClass];
                Class* pKey = cls.ArrayKeyClass == 0 ? nullptr : &State.Classes[cls.ArrayKeyClass];
                U32 _fl, _v, p;
                _CalculateCollectionSizes(pKey, pVal, _v, p, _fl);
                cls.RTSize += p * cls.ArraySize;
            }
            
            if(cls.RTSize > 8)
                cls.RTSize += TTE_PADDING(cls.RTSize, 8);
            
            if(!cls.RTSizeRaw)
                cls.RTSizeRaw = cls.RTSize;
            
            cls.LegacyHash = _PerformLegacyClassHash(cls.Name);
            
            cls.VersionCRC = crc;
            cls.ClassID = id;
            State.Classes[id] = std::move(cls);
            return id;
        }
        
        String _EnumFlagMemberToString(Member& member, const void* pVal)
        {
            U32 value = *((const U32*)(pVal));
            String val{};
            if(member.Flags & MEMBER_ENUM)
            {
                for(auto& desc: member.Descriptors)
                {
                    if(desc.Value == value)
                    {
                        if(val.length())
                        {
                            val += ";";
                            val += desc.Name;
                        }
                        else val = desc.Name;
                    }
                }
            }
            else
            {
                for(auto& desc: member.Descriptors)
                {
                    if((U32)desc.Value & value)
                    {
                        if(val.length())
                        {
                            val += "| ";
                            val += desc.Name;
                        }
                        else val = desc.Name;
                    }
                }
            }
            return val;
        }
        
        String _EnumFlagToString(Class* pClass, const void* pVal)
        {
            for(auto& mem: pClass->Members)
            {
                if((mem.Flags & MEMBER_BASE) == 0) // EnumBase class
                {
                    return _EnumFlagMemberToString(mem, pVal);
                }
            }
            return "<ENUM_WRAPPER_CLASS_NO_MEMBER_FOUND>";
        }
        
        Bool _Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite, CString member, Member* hostMember)
        {
            if(!pMemory || clazz == nullptr)
            {
                TTE_ASSERT(false, "Cannot serialise, type or class passed in is null");
                return false;
            }
            
            if(((clazz->Flags & CLASS_INTRINSIC) == 0) && ((clazz->Flags & CLASS_CONTAINER) == 0)) // add to version header if needed
            {
                if(IsWrite)
                {
                    // ensure no duplicate type hashes
                    Bool ok = true;
                    for(auto id: stream.VersionInf)
                    {
                        if(State.Classes[id].TypeHash == clazz->TypeHash)
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
                        if(State.Classes[id].TypeHash == clazz->TypeHash && State.Classes[id].VersionCRC != clazz->VersionCRC)
                        {
                            TTE_ASSERT(false, "Cannot serialise meta stream class: version CRC mismatch");
                            return false;
                        }
                    }
                }
            }
            
            StreamSection initialSection = stream.CurrentSection; // ensure after the section is the same
            
            Bool result;
            
            if(clazz->Serialise) // has serialiser.
            {
                if(clazz->Flags & CLASS_CONTAINER)
                {
                    stream.WriteTabs();
                    stream.DebugOutput << clazz->Name << " " << member << ":\n";
                    stream.WriteTabs();
                    stream.DebugOutput << "{\n";
                    stream.TabDepth++;
                }
                result = clazz->Serialise(stream, host, clazz, pMemory, IsWrite);
                if(clazz->Flags & CLASS_CONTAINER)
                {
                    stream.TabDepth--;
                    stream.WriteTabs();
                    stream.DebugOutput << "}\n";
                }else
                {
                    String val{};
                    if(hostMember && (hostMember->Flags & (MEMBER_FLAG | MEMBER_ENUM)))
                        val = _EnumFlagMemberToString(*hostMember, pMemory);
                    else
                        val = _PerformToString((U8*)pMemory, clazz);
                    stream.WriteTabs();
                    stream.DebugOutput << clazz->Name << " " << member << ": " << val.c_str() << "\n";
                }
            }
            else if(clazz->SerialiseScriptFn.length() > 0) // RUN LUA SERIALISER FUNCTION
            {
                
                LuaManager& man = GetThreadLVM();
                
                if(stream.DebugOutputFile)
                {
                    stream.WriteTabs();
                    if(member)
                    {
                        stream.DebugOutput << ":Lua Serialiser for " << member << ": " << clazz->SerialiseScriptFn << "\n";
                    }
                    else
                    {
                        stream.DebugOutput << ":Lua Serialiser: " << clazz->SerialiseScriptFn << "\n";
                    }
                }
                
                auto it = State.Serialisers.find(clazz->SerialiseScriptFn);
                if(it == State.Serialisers.end())
                {
                    TTE_ASSERT(false, "Cannot serialise type %s: serialiser function failed to initilaise", clazz->Name.c_str());
                    return false; // FAIL
                }
                
                ScriptManager::GetGlobal(man, clazz->SerialiseScriptFn, true);
                if(man.Type(-1) != LuaType::FUNCTION)
                {
                    man.Pop(1);
                    if(!man.LoadChunk(clazz->SerialiseScriptFn, it->second.Binary, it->second.Size, LoadChunkMode::BINARY))
                    {
                        TTE_ASSERT(false, "Cannot serialise type %s: compiled serialiser function failed to load", clazz->Name.c_str());
                        return false; // FAIL
                    }
                }
                // else its now on the stack
                
                // arguments: meta stream, instance, is write
                
                man.PushOpaque(&stream); // push stream
                
                // if we are serialising the host class, pass that. else put a tmp class with host as parent and pMemory
                ClassInstance tmp;
                U8* acq = host._GetInternal();
                
                if(acq == pMemory) // if host is the same
                {
                    tmp = host;
                    tmp._InstanceClassID = clazz->ClassID; // may have same memory but be a different member at offset 0.
                }
                else
                {
                    tmp = ClassInstance{clazz->ClassID, (U8*)pMemory, host.ObtainParentRef()};
                }
                
                tmp.PushWeakScriptRef(man, {}); // push instance
                
                man.PushBool(IsWrite); // push is write
                
                man.CallFunction(3, 1, true); // call, locked.
                
                result = ScriptManager::PopBool(man); // check result
                
                if(!result)
                {
                    TTE_LOG("Cannot serialise type %s: script serialisation function returned false", clazz->Name.c_str());
                    return false; // FAIL
                }
                
            }
            else
            {
                result = _DefaultSerialise(stream, host, clazz, pMemory, IsWrite, member, hostMember);
            }
            
            stream.CurrentSection = initialSection;
            
            return result;
        }
        
        // Meta serialise all the members (default)
        Bool _DefaultSerialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite, CString mem, Member* hostMember)
        {
            if(!pMemory || clazz == nullptr)
            {
                TTE_ASSERT(false, "Cannot serialise, type or class passed in is null");
                return false;
            }
            
            StreamSection initialSection = stream.CurrentSection; // ensure after the section is the same
            
            if(stream.DebugOutputFile)
            {
                mem = mem ? mem : "";
                if(clazz->Members.size() > 0)
                {
                    stream.WriteTabs();
                    stream.DebugOutput << clazz->Name << " " << mem << ":\n";
                    stream.WriteTabs();
                    stream.DebugOutput << "{\n";
                    stream.TabDepth++;
                }
            }
            
            Bool bProxy = (clazz->Flags & CLASS_PROXY) != 0;
            
            // serialise each member
            for(auto& member: clazz->Members)
            {
                if(member.Flags & MEMBER_SERIALISE_DISABLE)
                    continue; // skip
                
                Bool bBlocked = !bProxy && ((State.Classes[member.ClassID].Flags & CLASS_NON_BLOCKED) == 0);
                
                // START OF BLOCK
                if(bBlocked)
                {
                    if(IsWrite) // in write store position in stream
                    {
                        stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition());
                        U32 zero{};
                        SerialiseU32(stream, host, nullptr, &zero, true); // write zero for block size now. will come back after.
                    }
                    else
                    {
                        // read. store size + current offset to check after
                        U32 size{};
                        SerialiseU32(stream, host, nullptr, &size, false); // read block size (4 is added to it).
                        stream.Sect[stream.CurrentSection].Blocks.push_back(stream.Sect[stream.CurrentSection].Data->GetPosition() + size - 4);
                    }
                }
                
                // BLOCK DATA
                if(!_Serialise(stream, host, &State.Classes[member.ClassID], (U8*)pMemory + member.RTOffset, IsWrite, member.Name.c_str(), &member))
                    return false; // serialise it
                
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
                        SerialiseU32(stream, host, nullptr, &blockSize, true); // write it
                        stream.Sect[stream.CurrentSection].Data->SetPosition(currentPos); // go back to normal. done
                    }
                    else
                    {
                        U64 fpos = stream.Sect[stream.CurrentSection].Blocks.back();
                        stream.Sect[stream.CurrentSection].Blocks.pop_back();
                        U64 pos = stream.Sect[stream.CurrentSection].Data->GetPosition();
                        if(pos != fpos)
                        {
                            TTE_ASSERT(false, "Block size mismatch in %s when reading %s: at member %s %s => "
                                       "current section position != expected section position: 0x%llX != 0x%llX [%s] [diff 0x%llX].",
                                       stream.Name.c_str(), clazz->Name.c_str(), State.Classes[member.ClassID].Name.c_str(),
                                       member.Name.c_str(), pos, fpos, pos > fpos ? "too many bytes read" : "not enough bytes read",
                                       fpos > pos ? fpos - pos : pos - fpos);
                            return false;
                        }
                    }
                }
                
            }
            
            if(stream.DebugOutputFile && clazz->Members.size() > 0)
            {
                stream.TabDepth--;
                stream.WriteTabs();
                stream.DebugOutput << "}\n";
            }
            
            stream.CurrentSection = initialSection; // reset section
            
            return true;
        }

        U32 _ResolveCommonClassID(const String& extension)
        {
            CommonClassInfo info = CommonClassInfo::GetCommonClassInfoByExtension(extension);
            if (info.Class == CommonClass::NONE)
                return 0; // use def
            return Meta::_Impl::_ResolveCommonClassIDSafe(info.Class);
        }
        
        String _PerformToString(U8* pMemory, Class* pClass)
        {
            if(pClass->ToString != nullptr) // we have a function to do it, else use members
            {
                return pClass->ToString(pClass, pMemory);
            }
            if(pClass->Members.size() == 0)
                return pClass->Name; // return the name of the class - it has no runtime data to get
            String result = "[";
            for(auto& member: pClass->Members)
            {
                result += " ";
                result += member.Name;
                result += " = ";
                result += _PerformToString(pMemory + member.RTOffset, &State.Classes[member.ClassID]);
                result += ", ";
            }
            result[result.length() - 2] = ' ';
            result[result.length() - 1] = ']';
            return result;
        }
        
        U32 _PerformLegacyClassHash(const String& name)
        {
            U32 ind = 0;
            U32 l = (U32)name.length();
            U32 hash = 0;
            for(U32 i = 0; i < l; i++)
            {
                U32 shift = 8*ind;
                ind=(ind+1) & 3;
                hash = hash ^ ((U8)name[i] << shift);
            }
            return hash;
        }
        
        // calculate the crc32 version (important!) for the given class.
        U32 _DoLuaVersionCRC(LuaManager& man, I32 classTableStackIndex)
        {
            if(State.VersionCalcFun.length() == 0)
            {
                TTE_ASSERT(false, "No version hash calculation function has been set. Please set it before registering any State.Classes.");
                return 0;
            }
            ScriptManager::GetGlobal(man, State.VersionCalcFun, true);
            if(man.Type(-1) != LuaType::FUNCTION)
            {
                TTE_ASSERT(false, "No version hash calculation function has been set. Please set it before registering any State.Classes.");
                return 0;
            }
            else
            {
                man.PushCopy(classTableStackIndex); // by ref, ok
                man.CallFunction(1, 1, true);
                return ScriptManager::PopUnsignedInteger(man);
            }
        }
        
        // get class by crc (Use map)
        Class* _GetClass(U32 i)
        {
            return &State.Classes[i];
        }
        
        U32 _ClassChildrenArrayOff(Class& clazz)
        {
            U32 offset = clazz.RTSize;
            
            if(clazz.Flags & _CLASS_PROP) // remove prop array from size (stored at end)
            {
                offset -= sizeof(PropertySet::InternalData);
                offset -= clazz.RTPaddingCallbacks;
            }
            
            if(clazz.Flags & CLASS_ATTACHABLE)
            {
                // remove children from size
                TTE_ASSERT(offset > sizeof(ClassChildMap), "Child map does not exist. Internal error");
                offset -= sizeof(ClassChildMap);
                // keep padding
            } // else RTSize doesnt include it anywhere its just top level so after.
            else
            {
                offset += TTE_PADDING(offset, 8);
            }
            
            return offset;
        }
        
        U32 _ClassPropertyCallbacksArrayOff(Class& clazz)
        {
            TTE_ASSERT(clazz.Flags & _CLASS_PROP, "Class must be prop");
            U32 offset = clazz.RTSize;
            offset -= sizeof(PropertySet::InternalData);
            // keep align
            return offset;
        }

        // MEMORY LAYOUT:
        // <CLASS MEMBERS / MAIN MEMORY>
        // <CLASS CHILD MAP PADDING> <= IF ATTACHABLE FROM HERE ON OR TOP LEVEL.
        // <CLASS CHILD MAP>
        // <KEY CALLBACKS PADDING> <= IF PROP FROM HERE ON
        // <KEY CALLBACKS VECTOR>
        U32 _ClassRuntimeSize(Class& clazz, Bool forceChildArray)
        {
            U32 sz = clazz.RTSize;
            
            if(forceChildArray && ((clazz.Flags & CLASS_ATTACHABLE) == 0)) // if not already attachable but its top level, add child array
            {
                // top level
                sz += TTE_PADDING(sz, 8);
                sz += sizeof(ClassChildMap); // child ref array. if attachable its included in the size already so can be used in collections etc
                // no key callbacks array as prop is attachable.
            }

            return sz;
        }
        
        ClassInstance _MakeInstance(U32 ClassID, ClassInstance& topLevelParent, Symbol name, U8* Alloc, U32 AlZ)
        {
            auto it = State.Classes.find(ClassID);
            if(it == State.Classes.end()){
                TTE_LOG("Cannot create class instance for class index %X; not found in registry", ClassID);
                return ClassInstance{};
            }
            
            // obtain the parent reference
            ParentWeakReference host = topLevelParent.ObtainParentRef();
            
            // check if the parent is valid (ie if this new instance will be a top level class)
            const Bool upvalue_TopLevel = topLevelParent._InstanceClassID == 0;
            const Bool parentHasChildMap = upvalue_TopLevel ? false : ((State.Classes[topLevelParent.GetClassID()].Flags & CLASS_ATTACHABLE) != 0);
            const U32 upvalue_ClassID = ClassID;
            
            // calculate the size
            U32 sz = _ClassRuntimeSize(it->second, upvalue_TopLevel);
            
            // assert top level has not expired
            if(!upvalue_TopLevel)
            {
                if((IsWeakPtrUnbound(host) || host.expired()))
                {
                    TTE_ASSERT(false, "Cannot make instance of %s: the parent weak reference has previously expired (it no longer exists)"
                               , State.Classes[ClassID].Name.c_str());
                    return ClassInstance{};
                }
            }
            
            Bool bNeedsFree = !(Alloc && (sz + 8 <= AlZ));
            U8* const upvalue_TempBlockCheck = bNeedsFree ? nullptr : &Alloc[sz];
            if(upvalue_TempBlockCheck)
                memcpy(upvalue_TempBlockCheck, "I<3Lucas", 8);
            
            // allocate
            U8* pMemory = bNeedsFree ? TTE_ALLOC(sz, MEMORY_TAG_META_TYPE) : Alloc;
            
            ClassInstance inst = ClassInstance{ClassID, pMemory, [=](U8* pMem){ // use a c++11 lambda so we get a closure.
                
                // this is a closure, and the only captured values (lua upvalue equivalent) are a few, as its not stored in the memory block.
                // this is the only time we use lambdas here, as they work quite well for this case, as otherwise there is no other way to
                // have a reference to these unless its also stored in the memory block, which would not work in collections.
                
                _Impl::_DoDestruct(&State.Classes[upvalue_ClassID], pMem, upvalue_TopLevel); // call destructor
                
                // free memory
                if(upvalue_TempBlockCheck)
                {
                    TTE_ASSERT(memcmp(upvalue_TempBlockCheck, "I<3Lucas", 8) == 0, "Corrupted memory block when testing temporary instance validity."
                               " Was a class instance using CreateTemporaryInstance() kept alive outside of the stack frame it was created in?");
                }
                else
                    TTE_FREE((U8*)pMem); // free memory
                
            }, host};
            
            if(parentHasChildMap && name.GetCRC64() != 0) // insert inst into parent children array
            {
                TTE_ASSERT(name.GetCRC64() != 0, "When constructing non-top level class: name key must be specified");
                auto& vec = *topLevelParent._GetInternalChildrenRefs();
                vec[name] = inst; // ref counted
            }
            
            return inst;
        }
        
        // perform constructor.
        void _DoConstruct(Class* pClass, U8* pInstanceMemory, ParentWeakReference host, ParentWeakReference& concrete, Bool bTopLevel)
        {
            ParentWeakReference _{};
            if(pClass->Flags & CLASS_ATTACHABLE || bTopLevel)
            {
                // construct child map
                U8* mapMem = pInstanceMemory + _ClassChildrenArrayOff(*pClass);
                new(mapMem) ClassChildMap();
            }
            if(pClass->Flags & _CLASS_PROP)
            {
                U8* cbMem = pInstanceMemory + _ClassPropertyCallbacksArrayOff(*pClass);
                new(cbMem) PropertySet::InternalData();
            }
            // If the whole class has a constructor, call it (eg String)
            if(pClass->Constructor)
            {
                pClass->Constructor(pInstanceMemory, pClass->ClassID, host);
            }
            else if(pClass->Members.size() == 0 && !(pClass->Flags & CLASS_ATTACHABLE))
                // the class has no explicit constructor, but is intrinsic, memset.
            {
                memset(pInstanceMemory, 0, pClass->RTSize); // set to zero (eg int types)
            }
            else // else its just a normal higher level class, so do each member
            {
                if((IsWeakPtrUnbound(host) || host.expired()) && !IsWeakPtrUnbound(concrete) && !concrete.expired())
                    host = concrete;
                for(auto& member: pClass->Members)
                {
                    _DoConstruct(&State.Classes[member.ClassID], pInstanceMemory + member.RTOffset, host, _, false); // again
                }
            }
        }
        
        // perform destructor
        void _DoDestruct(Class* pClass, U8* pInstanceMemory, Bool bTopLevel)
        {
            if(pClass->Flags & CLASS_ATTACHABLE || bTopLevel)
            {
                // destruct child map
                U8* mapMem = pInstanceMemory + _ClassChildrenArrayOff(*pClass);
                ((ClassChildMap*)mapMem)->~ClassChildMap();
            }
            if(pClass->Flags & _CLASS_PROP)
            {
                U8* cbMem = pInstanceMemory + _ClassPropertyCallbacksArrayOff(*pClass);
                ((PropertySet::InternalData*)cbMem)->~InternalData();
            }
            // If the whole class has a destructor, call it (eg String)
            if(pClass->Destructor)
            {
                pClass->Destructor(pInstanceMemory, pClass->ClassID);
            }
            else if(pClass->Members.size() == 0 && !(pClass->Flags & CLASS_ATTACHABLE))
            {
                memset(pInstanceMemory, 0, pClass->RTSize); // set to zero on destroys (eg int types)
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoDestruct(&State.Classes[member.ClassID], pInstanceMemory + member.RTOffset, false); // again
                }
            }
        }
        
        // perform copy
        void _DoCopyConstruct(Class* pClass, U8* pMemory, const U8* pSrc, ParentWeakReference host, ParentWeakReference& concrete, Bool bSrcTopLevel, Bool bTopLevel)
        {
            ParentWeakReference _{};
            if(pClass->Flags & _CLASS_PROP)
            {
                const PropertySet::InternalData* srcMem = (const PropertySet::InternalData*)(pSrc + _ClassPropertyCallbacksArrayOff(*pClass));
                PropertySet::InternalData* dstMem = (PropertySet::InternalData*)(pMemory + _ClassPropertyCallbacksArrayOff(*pClass));
                new(dstMem) PropertySet::InternalData(*srcMem); // shallow copy, ok just callbacks.
            }
            // If the whole class has a copy constructor, call it (eg String)
            if(pClass->CopyConstruct)
            {
                pClass->CopyConstruct(pSrc, pMemory, host);
            }
            else if(pClass->Members.size() == 0 && !(pClass->Flags & CLASS_ATTACHABLE)) // symbols isnt intrinsic but if no copy construct or members, memcpy.
            {
                memcpy(pMemory, pSrc, pClass->RTSize);
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    if((IsWeakPtrUnbound(host) || host.expired()) && !IsWeakPtrUnbound(concrete) && !concrete.expired())
                        host = concrete;
                    _DoCopyConstruct(&State.Classes[member.ClassID], pMemory + member.RTOffset,
                                     pSrc + member.RTOffset, host, _, false, false); // again
                }
            }
            if(pClass->Flags & CLASS_ATTACHABLE)
            {
                // Do this LAST here because copy instance requires full type
                // copy child map
                const ClassChildMap* srcMap = (const ClassChildMap*)(pSrc + _ClassChildrenArrayOff(*pClass));
                ClassChildMap* dstMap = (ClassChildMap*)(pMemory + _ClassChildrenArrayOff(*pClass));
                new (dstMap) ClassChildMap();
                // Copy children (deep). User should be careful if editing props in sub-arrays and the array is modified.
                for(const auto& src: *srcMap)
                {
                    CopyInstance(src.second, ClassInstance(pClass->ClassID, pMemory, {}), src.first);
                }
            }
            else if(bTopLevel)
            {
                ClassChildMap* dstMap = (ClassChildMap*)(pMemory + _ClassChildrenArrayOff(*pClass));
                new (dstMap) ClassChildMap(); // ignore source
            }
        }
        
        // perform move
        void _DoMoveConstruct(Class* pClass, U8* pMemory, U8* pSrc, ParentWeakReference host, ParentWeakReference& concrete, Bool bSrcTopLevel, Bool bTopLevel)
        {
            ParentWeakReference _{};
            if(pClass->Flags & _CLASS_PROP)
            {
                PropertySet::InternalData* srcMem = (PropertySet::InternalData*)(pSrc + _ClassPropertyCallbacksArrayOff(*pClass));
                PropertySet::InternalData* dstMem = (PropertySet::InternalData*)(pMemory + _ClassPropertyCallbacksArrayOff(*pClass));
                new(dstMem) PropertySet::InternalData(std::move(*srcMem));
            }
            // If the whole class has a move constructor, call it (eg String)
            if(pClass->MoveConstruct)
            {
                pClass->MoveConstruct(pSrc, pMemory, host);
            }
            else if(pClass->Members.size() == 0 && !(pClass->Flags & CLASS_ATTACHABLE))
            {
                memcpy(pMemory, pSrc, pClass->RTSize); // just in case, we wont clear to zeros.
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    if((IsWeakPtrUnbound(host) || host.expired()) && !IsWeakPtrUnbound(concrete) && !concrete.expired())
                        host = concrete;
                    _DoMoveConstruct(&State.Classes[member.ClassID], pMemory + member.RTOffset,
                                     pSrc + member.RTOffset, host, _, false, false); // again
                }
            }
            if(pClass->Flags & CLASS_ATTACHABLE)
            {
                // move child map. cant just move the map. child parent refs need new parent to be this.
                ClassChildMap* srcMap = (ClassChildMap*)(pSrc + _ClassChildrenArrayOff(*pClass));
                ClassChildMap* dstMap = (ClassChildMap*)(pMemory + _ClassChildrenArrayOff(*pClass));
                new(dstMap) ClassChildMap();
                // Move children (deep)
                for(const auto& src: *srcMap)
                {
                    MoveInstance(src.second, ClassInstance(pClass->ClassID, pMemory, {}), src.first);
                }
            }
            else if(bTopLevel)
            {
                ClassChildMap* dstMap = (ClassChildMap*)(pMemory + _ClassChildrenArrayOff(*pClass));
                new (dstMap) ClassChildMap(); // ignore source
            }
        }
        
        // ===================================================================         INTRINSIC DEFAULTS
        // ===================================================================
        
        void CtorCol(void* pMemory, U32 Array, ParentWeakReference host)
        {
            new (pMemory) ClassInstanceCollection(host, Array);
        }
        
        void DtorCol(void* pMemory, U32)
        {
            ((ClassInstanceCollection*)pMemory)->~ClassInstanceCollection();
        }
        
        void CopyCol(const void* pSrc, void* pDst, ParentWeakReference host)
        {
            new (pDst) ClassInstanceCollection(*((const ClassInstanceCollection*)pSrc), host);
        }
        
        void MoveCol(void* pSrc, void* pDst, ParentWeakReference host)
        {
            new (pDst) ClassInstanceCollection(std::move(*((ClassInstanceCollection*)pSrc)), host);
        }
        
        void CtorString(void* pMemory, U32, ParentWeakReference host)
        {
            new (pMemory) String();
        }
        
        void DtorString(void* pMemory, U32)
        {
            ((String*)(pMemory))->~String();
        }
        
        void CopyString(const void* pSrc, void* pDst, ParentWeakReference host)
        {
            new (pDst) String(*((const String*)pSrc)); // force call copy constructor
        }
        
        void MoveString(void* pSrc, void* pDst, ParentWeakReference host)
        {
            new (pDst) String(std::move(*((String*)pSrc))); // force call move constructor
        }
        
        void CtorDSCache(void* pMemory, U32 Array, ParentWeakReference host)
        {
            new (pMemory) DataStreamCache();
        }
        
        void DtorDSCache(void* pMemory, U32)
        {
            ((DataStreamCache*)pMemory)->~DataStreamCache();
        }
        
        void CopyDSCache(const void* pSrc, void* pDst, ParentWeakReference host)
        {
            // copy ptr lightly, no deep copy. its a cache, if a deep copy is wanted then it should be in memory and a Binary Buffer would be used
            *((DataStreamCache*)pDst) = *((const DataStreamCache*)pSrc);
        }
        
        void MoveDSCache(void* pSrc, void* pDst, ParentWeakReference host)
        {
            *((DataStreamCache*)pDst) = std::move(*((DataStreamCache*)pSrc));
        }
        
        void CtorBinaryBuffer(void* pMemory, U32, ParentWeakReference)
        {
            new (pMemory) BinaryBuffer();
        }
        
        void DtorBinaryBuffer(void* pMemory, U32)
        {
            ((BinaryBuffer*)pMemory)->~BinaryBuffer();
        }
        
        void CopyBinaryBuffer(const void* pSrc, void* pDst,ParentWeakReference host) // copy CONSTRUCT.
        {
            const BinaryBuffer* pSrcBuffer = (const BinaryBuffer*)pSrc;
            BinaryBuffer* pDstBuffer = (BinaryBuffer*)pDst;
            
            new(pDst) BinaryBuffer();
            
            pDstBuffer->BufferSize = pSrcBuffer->BufferSize;
            if(pSrcBuffer->BufferData) // if we have a buffer, DEEP copy the memory
            {
                U8* Buffer = TTE_ALLOC(pDstBuffer->BufferSize, MEMORY_TAG_RUNTIME_BUFFER);
                memcpy(Buffer, pSrcBuffer->BufferData.get(), pSrcBuffer->BufferSize);
                pDstBuffer->BufferData = Ptr<U8>(Buffer, [](U8* p){TTE_FREE(p);});
            }
        }
        
        void MoveBinaryBuffer(void* pSrc, void* pDst, ParentWeakReference host)
        {
            BinaryBuffer* pSrcBuffer = (BinaryBuffer*)pSrc;
            BinaryBuffer* pDstBuffer = (BinaryBuffer*)pDst;
            
            memcpy(pDst, pSrcBuffer, sizeof(BinaryBuffer)); // move all
            memset(pSrc, 0, sizeof(BinaryBuffer)); // reset previous
        }
        
        // string default serialiser. class shouldnt be used, can be null as used in other serialisers where class is obvious.
        Bool SerialiseString(Stream& stream, ClassInstance& host, Class*, void* pMemory, Bool IsWrite)
        {
            String& str = *((String*)pMemory);
            if(IsWrite)
            {
                U32 len = (U32)str.length();
                if(!SerialiseU32(stream, host, nullptr, &len, true)) // write length
                    return false;
                if(len && !stream.Write((const U8*)str.c_str(), (U64)len)) // write ASCII
                    return false;
            }
            else
            {
                U32 len = 0;
                if(!SerialiseU32(stream, host, nullptr, &len, false)) // read length
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
                if(len && !stream.Read((U8*)str.c_str(), (U64)len)) // read ASCII
                    return false;
            }
            return true;
        }
        
        // serialise symbol. if MBIN we store the string version of it.
        Bool SerialiseSymbol(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite)
        {
            Symbol& sym = *((Symbol*)pMemory);
            if(IsWrite)
            {
                if(stream.Version == MBIN)
                {
                    String value = SymbolTable::Find(sym); // find the string for it
                    if(value.length() == 0 && sym.GetCRC64() != 0)
                    {
                        TTE_ASSERT(false, "Could not resolve symbol which should have been already loaded (runtime symbols cleared?)");
                        return false; // we cannot serialise as we need the symbol
                    }
                    
                    U32 len = (U32)value.length(); // write length then string
                    SerialiseU32(stream, host, nullptr, &len, true); // length
                    if(!stream.Write((const U8*)value.c_str(), (U64)len))
                        return false;
                }
                else
                {
                    // write U64
                    U64 hash = sym.GetCRC64();
                    SerialiseU64(stream, host, nullptr, &hash, true);
                }
            }
            else
            {
                if(stream.Version == MBIN)
                {
                    // read string
                    String symbol{};
                    if(!SerialiseString(stream, host, nullptr, &symbol, false))
                        return false; // string too large, corrupt.
                    
                    GetRuntimeSymbols().Register(symbol); // register it as for remapping back to a string
                    
                    sym = Symbol(std::move(symbol)); // assign symbol
                }
                else
                {
                    // read U64
                    U64 hash{};
                    SerialiseU64(stream, host, nullptr, &hash, false);
                    sym = hash;
                }
            }
            return true;
        }
        
        // booleans need to be serialised as ascii 0 or 1, but in memory we store as 0 or 1 numbers
        Bool SerialiseBool(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite)
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
        Bool SerialiseCollection(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite)
        {
            ClassInstanceCollection* pCollection = (ClassInstanceCollection*)pMemory;
            Bool bKeyed = pCollection->IsKeyedCollection();
            Bool bStatic = pCollection->IsStaticArray();
            
            U32 keyClass = pCollection->GetKeyClass();
            U32 valClass = pCollection->GetValueClass();
            
            TTE_ASSERT(clazz->ArrayKeyClass == keyClass && valClass == clazz->ArrayValClass, "Corrupt collection! Memory overlap?");
            
            U32 size = (U32)pCollection->GetSize();
            
            if(bStatic)
            {
                for(U32 i = 0; i < size; i++)
                {
                    ClassInstance instance = pCollection->GetValue(i);
                    String kname = "[Value ";
                    kname = kname + std::to_string(i) + "]";
                    if(!_Serialise(stream, host, &State.Classes[valClass], instance._GetInternal(), IsWrite, kname.c_str()))
                        return false;
                }
                return true;
            }
            else SerialiseU32(stream, host, nullptr, &size, IsWrite);
            
            if(IsWrite)
            {
                for(U32 i = 0; i < size; i++)
                {
                    ClassInstance instance{};
                    
                    if(bKeyed)
                    {
                        String kname = "[Key ";
                        kname = kname + std::to_string(i) + "]";
                        // write key
                        instance = pCollection->GetKey(i);
                        if(!_Serialise(stream, host, &State.Classes[keyClass], instance._GetInternal(), true, kname.c_str()))
                            return false;
                        
                    }
                    
                    // write value
                    String kname = "[Value ";
                    kname = kname + std::to_string(i) + "]";
                    instance = pCollection->GetValue(i);
                    if(!_Serialise(stream, host, &State.Classes[valClass], instance._GetInternal(), true, kname.c_str()))
                        return false;
                    
                }
            }
            else
            {
                pCollection->Clear(); // ensure empty
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
                    
                    pCollection->Push(ClassInstance{}, ClassInstance{}, false, false); // push empty values
                    
                    if(bKeyed)
                    {
                        String kname = "[Key ";
                        kname = kname + std::to_string(i) + "]";
                        kinstance = pCollection->GetKey(pCollection->GetSize() - 1);
                        if(!_Impl::_Serialise(stream, host, &State.Classes[keyClass], kinstance._GetInternal(), false, kname.c_str()))
                            return false;
                    }
                    
                    String kname = "[Value ";
                    kname = kname + std::to_string(i) + "]";
                    vinstance = pCollection->GetValue(pCollection->GetSize() - 1);
                    if(!_Impl::_Serialise(stream, host, &State.Classes[valClass], vinstance._GetInternal(), false, kname.c_str()))
                        return false;
                    
                }
            }
            return true;
        }
        
    }
    
    // ===================================================================          META LUA API
    // ===================================================================
    
    void* ClassInstance::_GetInternalPropertySetData()
    {
        Class& clz = State.Classes.find(_InstanceClassID)->second;
        TTE_ASSERT(clz.Flags & _CLASS_PROP, "This can only be used on PropertySet");
        return _GetInternal() + _Impl::_ClassPropertyCallbacksArrayOff(clz);
    }
    
    // garbage collector function for script refs to class instances
    U32 luaClassInstanceGC(LuaManager& man)
    {
        ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.ToPointer(-1);
        pRef->~ClassInstanceScriptRef(); // release weak ref (it will have a weak pointer slot, may owns stuff).
        return 0;
    }
    
    ClassChildMap* ClassInstance::_GetInternalChildrenRefs()
    {
        TTE_ASSERT(IsAttachable(*this), "Cannot get internal children references: class is not attachable");
        U32 skipBytes = _Impl::_ClassChildrenArrayOff(State.Classes[_InstanceClassID]);
        return ((ClassChildMap*)((U8*)_InstanceMemory.get() + skipBytes));
    }
    
    static void DoPushScriptRef(LuaManager& man, ClassInstanceScriptRef* pRef, U32 classID)
    {
        man.PushTable(); // meta table
        
        man.PushLString("__gc");
        man.PushFn(&luaClassInstanceGC);
        man.SetTable(-3); // set gc method
        
        // other metatable operations?
        
        man.PushLString("__MetaId"); // also include the class id in the metatable so we can check it
        man.PushUnsignedInteger(classID);
        man.SetTable(-3);
        
        man.SetMetaTable(-2);
    }
    
    void ClassInstanceCollection::PushTransientScriptRef(LuaManager &L, U32 index, Bool bPushKey, ParentWeakReference prnt)
    {
        if(bPushKey)
            TTE_ASSERT(IsKeyedCollection(), "Cannot push a keyed transient script reference as the collection is not keyed");
        
        ClassInstance obj = bPushKey ? GetKey(index) : GetValue(index);
        U32 id = bPushKey ? GetKeyClass() : GetValueClass();
        
        if(!obj)
            L.PushNil();
        else
        {
            TransientJuncture j{};
            j.JunctureValue = _TransienceFence->load();
            j.CurrentValue = _TransienceFence;
            j.Value = obj._InstanceMemory.get();
            
            ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)L.CreateUserData((U32)sizeof(ClassInstanceScriptRef));
            new (pRef) ClassInstanceScriptRef(id, std::move(j), std::move(prnt));
            
            DoPushScriptRef(L, pRef, id);
            
        }
    }
    
    void ClassInstance::PushStrongScriptRef(LuaManager& man)
    {
        if(_InstanceMemory)
        {
            ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.CreateUserData((U32)sizeof(ClassInstanceScriptRef));
            new (pRef) ClassInstanceScriptRef(*this); // construct it (previous just allocates in the lua mem)
            
            DoPushScriptRef(man, pRef, _InstanceClassID);
            
        }
        else
        {
            man.PushNil(); // no object
        }
    }
    
    void ClassInstance::PushWeakScriptRef(LuaManager& man, ParentWeakReference keepalive)
    {
        if(IsWeakPtrUnbound(keepalive))
            keepalive = ObtainParentRef();
        if(_InstanceMemory)
        {
            ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.CreateUserData((U32)sizeof(ClassInstanceScriptRef));
            new (pRef) ClassInstanceScriptRef(*this, keepalive); // construct it (previous just allocates in the lua mem)
            
            
            DoPushScriptRef(man, pRef, _InstanceClassID);
            
        }
        else
        {
            man.PushNil(); // no object
        }
    }
    
    // ===================================================================         CORE META SYSTEM
    // ===================================================================
    
    Bool IsAttachable(ClassInstance& instance)
    {
        return instance.IsTopLevel() || (instance.GetClassID() != 0 && ((State.Classes[instance.GetClassID()].Flags & CLASS_ATTACHABLE) != 0));
    }
    
    // get pointer from stack, then call get
    ClassInstance AcquireScriptInstance(LuaManager& man, I32 stackIndex)
    {
        if(man.Type(stackIndex) != LuaType::FULL_OPAQUE)
        {
            return {};
        }
        
        ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.ToPointer(stackIndex);
        
        if(pRef == nullptr || pRef->GetClassID() == 0)
            return {};
         
        return pRef->Acquire();
    }
    
    ClassInstance CreateTemporaryInstance(U8* Alloc, U32 AlZ, U32 ClassID)
    {
        ClassInstance _{}; Symbol __{};
        ClassInstance instance = _Impl::_MakeInstance(ClassID, _, __, Alloc, AlZ);
        ParentWeakReference asParent{instance._InstanceMemory};
        
        _Impl::_DoConstruct(&State.Classes[ClassID], instance._GetInternal(), {}, asParent, true);
        
        return instance;
    }
    
    ClassInstance CreateInstance(U32 ClassID, ClassInstance host, Symbol n)
    {
        ClassInstance instance = _Impl::_MakeInstance(ClassID, host, n);
        ParentWeakReference asParent{instance._InstanceMemory};
        Bool bTopLevel = instance.IsTopLevel();
        
        _Impl::_DoConstruct(&State.Classes[ClassID], instance._GetInternal(), host.ObtainParentRef(), asParent, bTopLevel);
        
        return instance;
    }
    
    ClassInstance CopyInstance(ClassInstance instance, ClassInstance host, Symbol n)
    {
        if(!instance)
            return ClassInstance{};
        
        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID(), host, n);
        ParentWeakReference asParent{instanceNew._InstanceMemory};
        
        _Impl::_DoCopyConstruct(&State.Classes[instance.GetClassID()], instanceNew._GetInternal(),
                                instance._GetInternal(), host.ObtainParentRef(), asParent, instance.IsTopLevel(), instanceNew.IsTopLevel());
        
        return instanceNew;
    }
    
    ClassInstance MoveInstance(ClassInstance instance, ClassInstance host, Symbol n)
    {
        if(!instance)
            return ClassInstance{};
        
        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID(), host, n);
        ParentWeakReference asParent{instanceNew._InstanceMemory};
        
        _Impl::_DoMoveConstruct(&State.Classes[instance.GetClassID()], instanceNew._GetInternal(),
                                instance._GetInternal(), host.ObtainParentRef(), asParent, instance.IsTopLevel(), instanceNew.IsTopLevel());
        
        return instanceNew;
    }
    
    ClassInstance GetMember(ClassInstance& inst, const String& name, Bool bi)
    {
        if(!inst)
            return {}; // edge case
        const Class& clazz = State.Classes[inst.GetClassID()];
        for(auto& mem: clazz.Members)
        {
            if(CompareCaseInsensitive(name, mem.Name))
            {
                return ClassInstance(mem.ClassID, Ptr<U8>(inst._InstanceMemory, // same control block, different pointer.
                                                          inst._GetInternal() + mem.RTOffset), inst.ObtainParentRef());
            }
        }
        if(bi)
            TTE_ASSERT(false, "Member %s::%s does not exist!", clazz.Name.c_str(), name.c_str());
        return {}; // not found
    }
    
    Bool PerformLessThan(ClassInstance& lhs, ClassInstance& rhs)
    {
        if(!lhs || !rhs)
            return false;
        if(lhs.GetClassID() != rhs.GetClassID())
            return false;
        auto op = State.Classes[lhs.GetClassID()].LessThan;
        return op ? op(lhs._GetInternal(), rhs._GetInternal()) : false;
    }
    
    Bool PerformEquality(ClassInstance& lhs, ClassInstance& rhs)
    {
        if(!lhs || !rhs)
            return false;
        if(lhs.GetClassID() != rhs.GetClassID())
            return false;
        auto op = State.Classes[lhs.GetClassID()].Equals;
        return op ? op(lhs._GetInternal(), rhs._GetInternal()) : false;
    }
    
    String PerformToString(ClassInstance& inst)
    {
        return inst ? _Impl::_PerformToString(inst._GetInternal(), &State.Classes[inst.GetClassID()]) : "";
    }
    
    // Performs the equality operator '==' with the left and right hand side arguments (must be same type).
    Bool PerformEquality(ClassInstance& lhs, ClassInstance& rhs);
    
    // Performs the to string operator.
    String PerformToString(ClassInstance& inst);
    
    U32 FindClassByCRC(U64 typeHash, U32 versionCRC)
    {
        // version number should only be between 0 and  MAX_VERSION_NUMBER
        for(U32 i = 0; i <= MAX_VERSION_NUMBER; i++)
        {
            U32 id = FindClass(typeHash, i);
            if(id && State.Classes[id].VersionCRC == versionCRC)
                return id;
        }
        return 0; // not found
    }
    
    U32 FindClassByExtension(const String& ext, U32 versionNumber)
    {
        for(auto& clz: State.Classes)
        {
            if(clz.second.VersionNumber == versionNumber && CompareCaseInsensitive(clz.second.Extension, ext))
                return clz.first;
        }
        return 0;
    }
    
    U32 FindClass(U64 typeHash, U32 versionNumber, Bool bExact)
    {
        U32 id = _Impl::_GenerateClassID(typeHash, versionNumber);
        Bool bFound = State.Classes.find(id) != State.Classes.end();
        if(!bFound)
        {
            for(auto& clz: State.Classes)
            {
                if(clz.second.ToolTypeHash == typeHash && clz.second.VersionNumber == versionNumber)
                    return clz.first;
            }
        }
        return bFound ? id : 0;
    }
    
    // initialise the snapshot State.Classes
    void InitGame()
    {
        TTE_ASSERT(IsCallingFromMain(), "Must only be called from main thread");
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        GameSnapshot& snap = GetToolContext()->_Snapshot;
        
        // Ensure game exists
        Bool Found = false;
        U32 gameIdx = 0;
        U32 i=0;
        for(auto& game: State.Games)
        {
            if(game.ID == snap.ID)
            {
                if(snap.Vendor.empty())
                    snap.Vendor = game.DefaultVendor;
                Found = true;
                gameIdx = i;
                if(!_Impl::_CheckPlatformForGame(game, snap.Platform))
                {
                    TTE_ASSERT(false, "The platform '%s' is not (or currently) supported for the game %s!", snap.Platform.c_str(), game.Name.c_str());
                    return;
                }
                if(!_Impl::_CheckVendorForGame(game, snap.Vendor))
                {
                    TTE_ASSERT(false, "The vendor '%s' is not (or currently) supported for the game %s!", snap.Vendor.c_str(), game.Name.c_str());
                    return;
                }
                break;
            }
            i++;
        }
        TTE_ASSERT(Found, "Game with ID '%s' does not exist or was not registered, cannot initialise with game snapshot.", snap.ID.c_str());
        if(!Found)
        {
            RelGame();
            return;
        }
        
        // Run RegisterAll and register all State.Classes in the current snapshot
        
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
            ScriptManager::Execute(GetToolContext()->GetLibraryLVM(), 3, 1, true);
            if(!ScriptManager::PopBool(GetToolContext()->GetLibraryLVM()))
            {
                TTE_LOG("Failed to register game meta class descriptions!");
            }
            else // success, so init blowfish encryption for the game and other stuff
            {
                
                // Process deferred
                for(auto& def: State._Temp.Deferred)
                {
                    auto it = State.Classes.find(def.CollectionClass);
                    if(it != State.Classes.end())
                    {
                        Class& c = it->second;
                        if(def.KeyType.length())
                        {
                            U32 clazz = FindClass(def.KeyType, def.KeyVersion);
                            TTE_ASSERT(clazz != 0, "When registering deferred collection, the key type '%s' (%d) does not exist", def.KeyType.c_str(), def.KeyVersion);
                            c.ArrayKeyClass = clazz;
                        }
                        if(def.ValueType.length())
                        {
                            U32 clazz = FindClass(def.ValueType, def.ValueVersion);
                            TTE_ASSERT(clazz != 0, "When registering deferred collection, the value type '%s' (%d) does not exist", def.ValueType.c_str(), def.ValueVersion);
                            c.ArrayValClass = clazz;
                        }
                    }
                    else
                    {
                        TTE_ASSERT(false, "When registering deferred collection, the class could not be found. INTERNAL ERROR");
                    }
                }
                State._Temp.Deferred.clear();
                for(auto& warn: State._Temp.DeferredWarnings)
                {
                    TTE_LOG("WARNING: Class %s has mismatched version CRC [TODO]. Calculated 0x%X != 0x%X", warn.Class.c_str(), warn.Calculated, warn.Overriden);
                }
                State._Temp.DeferredWarnings.clear();
                
                BlowfishKey key = State.Games[gameIdx].GetEncryptionKey(snap);
                
                if(key.BfKeyLength == 1 && key.BfKey[0] == 0)
                {
                    TTE_ASSERT(false, "Game encryption key for %s/%s has not been registered. Please contact us with the executable of "
                               "the game for this platform so we can find the encryption key!", snap.ID.c_str(), snap.Platform.c_str());
                    // encryption key '00' means we dont know it yet.
                    RelGame();
                    return;
                }

                const String& collector = State.Games[gameIdx].CommonSelector;
                if(!collector.empty() && !_Impl::_CollectCompiledScriptFunctionMT(State.Collector, collector))
                {
                    TTE_ASSERT(false, "The common class collector function for %s could not be found or compiled: %s! Switch failed.",
                               State.Games[gameIdx].Name.c_str(), collector.c_str());
                    RelGame();
                    return;
                }
                
                Blowfish::Initialise(State.Games[gameIdx].Fl.Test(RegGame::MODIFIED_BLOWFISH), key.BfKey, key.BfKeyLength);
            }
        }
        
        TTE_LOG("Meta fully initialised with snapshot of %s: registered %d classes\n\n", snap.ID.c_str(), (U32)State.Classes.size());
        State.GameIndex = (I32)gameIdx;
    }
    
    void RelGame()
    {
        TTE_ASSERT(IsCallingFromMain(), "Must only be called from main thread");
        
        // clear compiled memory
        for(auto& script: State.Serialisers)
            _Impl::_FreeCompiledScriptMT(script.second);
        State.Serialisers.clear();
        for(auto& script: State.Normalisers)
            _Impl::_FreeCompiledScriptMT(script.second);
        State.Normalisers.clear();
        for(auto& script: State.Specialisers)
            _Impl::_FreeCompiledScriptMT(script.second);
        State.Specialisers.clear();
        _Impl::_FreeCompiledScriptMT(State.Collector);
        
        State.Classes.clear();
        State.VersionCalcFun = "";
        State.GameIndex = -1;
    }
    
    void Initialise()
    {
        TTE_ASSERT(IsCallingFromMain(), "Must only be called from main thread");
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        
        InjectFullLuaAPI(GetToolContext()->GetLibraryLVM(), false);
        
        // Setup games script
        
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(),
                               GetToolContext()->LoadLibraryStringResource("Scripts/Games.lua"), "Games.lua", false); // Initialise games
        
        // Setup State.Classes script
        
        String src = GetToolContext()->LoadLibraryStringResource("Scripts/Classes.lua");
        ScriptManager::RunText(GetToolContext()->GetLibraryLVM(), src, "Classes.lua", false);
        src.clear();
        
        TTE_LOG("Meta partially initialised with %d games", (U32)State.Games.size());
    }
    
    void Shutdown()
    {
        TTE_ASSERT(IsCallingFromMain(), "Must only be called from main thread");
        
        State.Games.clear();
    }
    
    Bool WriteMetaStream(const String& name, ClassInstance instance, DataStreamRef& stream, MetaStreamParams params)
    {
        
        // SETUP LOCALS
        
        if(name.empty() || !stream || !instance)
        {
            TTE_ASSERT(false, "Invalid arguments passed into WriteMetaStream");
            return false;
        }
        
        if(params.Version == StreamVersion::UNSPECIFIED)
            params.Version = GetInternalState().GetActiveGame().MetaVersion;
        
        Stream metaStream{};
        U8 Buffer[32]{};
        String magic =  params.Version == MSV6 ? "6VSM" :
        params.Version == MSV5 ? "5VSM" :
        params.Version == BMS3 ? "BMS3" :
        // StreamVersion == MSV4 ? "4VSM" :
        // StreamVersion == MCOM ? "MOCM" :
        params.Version == MTRE ? "ERTM" :
        // StreamVersion == MBES ? "SEBM" :
        params.Version == MBIN ? "NIBM" : "X";
        
        if(magic == "X")
        {
            TTE_LOG("The given meta stream version is not supported at the moment to be written with.");
            return false;
        }
        
        // 1. SETUP META STREAM
        
        metaStream.Version = params.Version;
        metaStream.CurrentSection = STREAM_SECTION_MAIN;
        metaStream.Name = name;
        metaStream.Sect[STREAM_SECTION_MAIN].Data = DataStreamManager::GetInstance()->CreatePrivateCache(name, 0x8000); // 32 KB
        if(params.Version == MSV5 || params.Version == MSV6)
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
        
        if(!_Impl::_Serialise(metaStream, instance, &State.Classes[instance.GetClassID()], instance._GetInternal(), true, nullptr))
        {
            TTE_LOG("Cannot write meta stream: serialisation failed");
            return false;
        }
        
        if(params.Version != MSV5 && params.Version != MSV6 &&
           metaStream.Sect[STREAM_SECTION_ASYNC].Data->GetSize() > 0 && metaStream.Sect[STREAM_SECTION_DEBUG].Data->GetSize() > 0)
        {
            TTE_ASSERT(false, "Cannot write serialised meta stream: data written to sections invalid for the given stream version");
            // async and debug sections do not exist in anything less than MSV5
            return false;
        }
        
        // 3. WRITE HEADER
        
        SerialiseDataU32(stream, nullptr, const_cast<char*>(magic.c_str()), true); // magic
        
        if(params.Version == MSV6 || params.Version == MSV5) // TODO optional compressed sections (set top bit of size to 1 ?)
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
        else if(params.Version == BMS3)
        {
            // Im not sure what this value is about. I know if 0x2000000 is set then we it stores the version header but its always set. theres also 0x1000000. Idk
            U32 value = 0x02'01'00'00;
            SerialiseDataU32(stream, nullptr, &value, true);
        }
        
        U32 numVersionInfo = (U32)metaStream.VersionInf.size();
        SerialiseDataU32(stream, nullptr, &numVersionInfo, true); // write version info count
        
        for(U32 i = 0; i < numVersionInfo; i++)
        {
            if(params.Version == MBIN)
            {
                U32 len = (U32)State.Classes[metaStream.VersionInf[i]].Name.length();
                SerialiseDataU32(stream, nullptr, &len, true); // write type name length
                
                // write type name string
                if(!stream->Write((const U8*)(State.Classes[metaStream.VersionInf[i]].Name.c_str()), len))
                {
                    TTE_ASSERT(false, "Cannot serialise out meta stream: type name could not be written in header");
                    return false;
                }
            }
            else if(params.Version == BMS3)
            {
                // Write legacy hash
                U32 hash = State.Classes[metaStream.VersionInf[i]].LegacyHash;
                SerialiseDataU32(stream, nullptr, &hash, true);
            }
            else
            {
                SerialiseDataU64(stream, nullptr, &State.Classes[metaStream.VersionInf[i]].TypeHash, true); // write type hash
            }
            
            SerialiseDataU32(stream, nullptr, &State.Classes[metaStream.VersionInf[i]].VersionCRC, true); // write version crc
            
        }
        
        // 4. WRITE MAIN SECTION
        
        metaStream.Sect[STREAM_SECTION_MAIN].Data->SetPosition(0);
        DataStreamManager::GetInstance()->Transfer(metaStream.Sect[STREAM_SECTION_MAIN].Data, stream,
                                                   metaStream.Sect[STREAM_SECTION_MAIN].Data->GetSize());
        
        // 5. WRITE ASYNC AND DEBUG SECTIONS IF NEEDED
        
        if(params.Version == MSV5 || params.Version == MSV6)
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
    
    String MakeSerialisedVersionInfoFileName(U32 cls, Bool bAltName)
    {
        U8 Buf[128]{};
        auto it = cls ? GetInternalState().Classes.find(cls) : GetInternalState().Classes.end();
        if(it != GetInternalState().Classes.end())
        {
            String name = bAltName ? MakeTypeName(it->second.Name) : it->second.Name;
            StringReplace(name, "class ", "");
            StringReplace(name, ":", "_");
            StringReplace(name, "?", "_");
            StringReplace(name, "<", "_");
            StringReplace(name, " ", "_");
            StringReplace(name, ">", "_");
            StringReplace(name, "*", "_");
            StringReplace(name, "\"", "_");
            StringReplace(name, ",", ""); // forgotten about, no underscore.
            String crc = StringFromInteger((I64)it->second.VersionCRC, 36, false);
            sprintf((char*)Buf, "%s(%s).vers", name.c_str(), crc.c_str());
            return String((CString)Buf);
        }
        else
        {
            return "";
        }
    }
    
    static DataStreamRef MapDecryptingStream0(DataStreamRef& stream, U8* Buffer) // doesnt include the new 'MBIN' header
    {
        U32 z = 0, raw = 0, bf = 0;
        if(!memcmp(Buffer, "SEBM", 4))
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
        else // anything else
        {
            return {};
        }
        
        // override stream such that we read the decrypted bytes without any worry
        DataStreamRef decryptingStream = DataStreamManager::GetInstance()->
        CreateLegacyEncryptedStream(stream, stream->GetPosition(), (U16)z, (U8)raw, (U8)bf);
        
        return decryptingStream;
    }
    
    DataStreamRef MapDecryptingStream(DataStreamRef& stream)
    {
        U8 Buffer[4]{};
        stream->Read(Buffer, 4);
        DataStreamRef decrypting = MapDecryptingStream0(stream, Buffer);
        if(!decrypting)
        {
            stream->SetPosition(0); // reset stream, MSV6 etc
            return stream;
        }
        auto seq = DataStreamManager::GetInstance()->CreateSequentialStream("Legacy Encrypted MS Sequential");
        DataStreamRef header = DataStreamManager::GetInstance()->CreateBufferStream({}, 4, 0, 0);
        header->Write((const U8*)"NIBM", 4); // NIBM header replacement
        header->SetPosition(0);
        seq->PushStream(header);
        seq->PushStream(decrypting);
        return seq;
    }
    
    // Reads meta stream file format
    ClassInstance ReadMetaStream(const String& fn, DataStreamRef& _stream, DataStreamRef dbg, U32 _max)
    {
        DataStreamRef stream = _stream;
        if (stream->GetSize() > 0x200 && stream->GetSize() < 0x800000)
        {
            stream = DataStreamManager::GetInstance()->CreateCachedStream(_stream);
        }
        Stream metaStream{};
        metaStream.Name = fn;
        metaStream.DebugOutputFile = std::move(dbg);
        metaStream.MaxInlinableBuffer = _max;
        U8 Buffer[256]{};
        U32 mainSectionSize{}, asyncSectionSize{}, debugSectionSize{};
        
        // read magic string, add null terminator
        stream->SetPosition(0);
        if(!stream->Read(Buffer, 4))
            return {};
        Buffer[4] = 0;
        
        String magic = (const char*)Buffer;
        metaStream.Version =    magic == "6VSM" ? MSV6 :
        magic == "BMS3" ? BMS3 :
        magic == "EMS3" ? EMS3 :
        magic == "5VSM" ? MSV5 :
        magic == "4VSM" ? MSV4 :
        magic == "MOCM" ? MCOM :
        magic == "ERTM" ? MTRE :
        magic == "SEBM" ? MBES :
        magic == "NIBM" ? MBIN : (StreamVersion)-1;
        
        // ENCRYPTED MAGICS. DEAL WITH AND DELEGATE ENCRYPTION TO DataStreamLegacyEncrypted
        if(metaStream.Version == (StreamVersion)-1 || metaStream.Version == MBES) // other encrypted formats have weird non ascii magics.
        {
            // override stream. decryptingStream has access to stream, so it will delete when it gets deleted.
            stream = MapDecryptingStream0(stream, Buffer);
            TTE_ASSERT(stream, "File %s is not a meta stream!", fn.c_str());
            if(stream)
                metaStream.Version = MBIN;
            else
                return {};
        }
        
        // these versions need looking into. they exist, but very very rare (if ever used)
        if(metaStream.Version == MSV4 || metaStream.Version == MCOM || metaStream.Version == EMS3)
        {
            TTE_LOG("Unsupported meta stream version %d. Contact us immidiately with this file! File: %s", metaStream.Version, fn.c_str());
            return {};
        }
        
        // if the top bit in the meta stream size is set, then this signifies that that meta stream section is wrapped in a container,
        // which allows for compressed and encrypted meta streams.
        Bool Wrapped[3] = {false, false, false};
        
        if(metaStream.Version == MSV5 || metaStream.Version == MSV6) // latest versions
        {
            SerialiseDataU32(stream, nullptr, &mainSectionSize, false);
            SerialiseDataU32(stream, nullptr, &asyncSectionSize, false);
            SerialiseDataU32(stream, nullptr, &debugSectionSize, false);
            
            // check top bits. if set, notify they are wrapped and then wrap them, removing the top bit (the rest is the
            if((mainSectionSize >> 31) != 0)
            {
                Wrapped[STREAM_SECTION_MAIN] = true;
                asyncSectionSize &= 0x7F'FF'FF'FF;
            }
            if((asyncSectionSize >> 31) != 0)
            {
                Wrapped[STREAM_SECTION_ASYNC] = true;
                asyncSectionSize &= 0x7F'FF'FF'FF;
            }
            if((asyncSectionSize >> 31) != 0)
            {
                Wrapped[STREAM_SECTION_DEBUG] = true;
                asyncSectionSize &= 0x7F'FF'FF'FF;
            }
        }
        else if(metaStream.Version == BMS3)
        {
            // I have no idea on what the 0x10000 value means. 0x2000000 means we have version info. if not!? we will have to require it then because we use that to detect the instance stored.
            U32 unk{};
            SerialiseDataU32(stream, nullptr, &unk, false);
            if(unk != 0x02010000)
            {
                TTE_ASSERT(false, "ERROR: The file %s has an unknown flags field. Please let us know about this file.", fn.c_str());
                return {};
            }
        }
        
        U32 numVersionInfo{};
        SerialiseDataU32(stream, nullptr, &numVersionInfo, false);
        
        // Sanity check
        if(numVersionInfo > (U32)State.Classes.size())
        {
            TTE_ASSERT(false, "Input data stream does not follow the meta stream format, or file is corrupt (%s)", fn.c_str());
            return {};
        }
        
        // Read version info header
        for(U32 i = 0; i < numVersionInfo; i++)
        {
            U64 typeHash{}; U32 versionCRC{};
            
            if(metaStream.Version == BMS3) // Legacy CSI3/PS2
            {
                U32 legacyTN{};
                SerialiseDataU32(stream, nullptr, &legacyTN, false);
                U32 versionCRC{};
                SerialiseDataU32(stream, nullptr, &versionCRC, false);
                
                U32 clazzID = 0;
                for(auto& clazz: State.Classes)
                {
                    if(clazz.second.LegacyHash == legacyTN && versionCRC == clazz.second.VersionCRC)
                    {
                        clazzID = clazz.first;
                        break;
                    }
                }
                if(clazzID == 0)
                {
                    TTE_LOG("Cannot serialise in meta stream %s: a serialised class does not match any runtime class version."
                            " Legacy type hash 0x%X and version CRC 0x%X. Most likely it exists, and its calculated version"
                            " hash was not correct. Check the class, possibly with the compiled executable.", fn.c_str(), legacyTN, versionCRC);
                    return {};
                }
                metaStream.VersionInf.push_back(clazzID);
                continue;
            }
            
            // read type hash (or string if very old).
            if(metaStream.Version == MBIN)
            {
                U32 len{};
                
                SerialiseDataU32(stream, nullptr, &len, false);
                
                if(len > 256) // should never occur.
                {
                    TTE_ASSERT(false, "Cannot serialise in meta stream %s: a serialised class name is too long", fn.c_str());
                    return {};
                }
                
                if(!stream->Read(Buffer, len))
                {
                    TTE_LOG("Cannot serialise in meta stream %s: unexpected EOF", fn.c_str());
                    return {};
                }
                
                Buffer[len] = 0;
                String str((CString)Buffer);
                GetRuntimeSymbols().Register(str);
                typeHash = CRC64LowerCase(Buffer, len);
            }
            else
            {
                SerialiseDataU64(stream, 0, &typeHash, false);
            }
            
            // read type version CRC
            SerialiseDataU32(stream, 0, &versionCRC, false);
            
            // try and find it in the registered State.Classes. if it is not found, the version CRC likely mismatches. cannot load, as
            // format is not 100% gaurunteed to match.
            U32 ClassID = FindClassByCRC(typeHash, versionCRC);
            
            if(State.Classes.find(ClassID) == State.Classes.end())
            {
                // COMMON ERROR! The calculated version hash was wrong, or the class does not exist yet.
                String resolved = SymbolTable::Find(typeHash);
                if(resolved.length())
                {
                    TTE_LOG("Cannot serialise in meta stream %s: a serialised class does not match any runtime class version."
                            " Class '%s' and version %X. Most likely it exists, and its calculated version"
                            " hash was not correct. Check the class, possibly with the compiled executable.", fn.c_str(), resolved.c_str(), versionCRC);
                }
                else
                {
                    TTE_LOG("Cannot serialise in meta stream %s: a serialised class does not match any runtime class version."
                            " Type hash 0x%llX and version %X. Most likely it exists, and its calculated version"
                            " hash was not correct. Check the class, possibly with the compiled executable.", fn.c_str(), typeHash, versionCRC);
                }
                return {};
            }
            
            metaStream.VersionInf.push_back(ClassID);
            
        }
        
        // .VERS files contain a Class serialised to a file. They can be loaded separately in the future. Cannot detect a type from it.
        if(numVersionInfo == 0)
        {
            TTE_LOG("Cannot serialise in meta stream %s: no version information. Is this a .VERS file? If so, it cannot be loaded this way.", fn.c_str());
            return {};
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
            TTE_LOG("Could not create instance for class stored in meta stream %s", fn.c_str());
            return instance;
        }
        
        // now check if we need to add unwrappers
        if(Wrapped[STREAM_SECTION_MAIN])
        {
            metaStream.Sect[STREAM_SECTION_MAIN].Data =
            DataStreamManager::GetInstance()->CreateContainerStream(metaStream.Sect[STREAM_SECTION_MAIN].Data);
            // ensure its OK
            if(!std::static_pointer_cast<DataStreamContainer>(metaStream.Sect[STREAM_SECTION_MAIN].Data)->IsValid())
            {
                TTE_LOG("When opening meta stream %s: main section is wrapped but invalid", fn.c_str());
                return {};
            }
        }
        if(Wrapped[STREAM_SECTION_DEBUG])
        {
            metaStream.Sect[STREAM_SECTION_DEBUG].Data =
            DataStreamManager::GetInstance()->CreateContainerStream(metaStream.Sect[STREAM_SECTION_DEBUG].Data);
            if(!std::static_pointer_cast<DataStreamContainer>(metaStream.Sect[STREAM_SECTION_DEBUG].Data)->IsValid())
            {
                TTE_LOG("When opening meta stream %s: debug section is wrapped but invalid", fn.c_str());
                return {};
            }
        }
        if(Wrapped[STREAM_SECTION_ASYNC])
        {
            metaStream.Sect[STREAM_SECTION_ASYNC].Data =
            DataStreamManager::GetInstance()->CreateContainerStream(metaStream.Sect[STREAM_SECTION_ASYNC].Data);
            if(!std::static_pointer_cast<DataStreamContainer>(metaStream.Sect[STREAM_SECTION_ASYNC].Data)->IsValid())
            {
                TTE_LOG("When opening meta stream %s: async section is wrapped but invalid", fn.c_str());
                return {};
            }
        }
        
        // write debug header
        if(metaStream.DebugOutputFile)
        {
            metaStream.DebugOutput << "Meta Stream [" << magic << "] debug output from the Telltale Editor v" TTE_VERSION "\n";
            metaStream.DebugOutput << "\n_metaVersionInfo:\n{\n";
            for(auto& version: metaStream.VersionInf)
            {
                Class& clazz = State.Classes[version];
                metaStream.DebugOutput << "-\t" << clazz.Name.c_str() << ": Version 0x" <<
                std::hex << std::uppercase << clazz.VersionCRC << "[" << clazz.VersionNumber << "]\n";
            }
            metaStream.DebugOutput << "}\n\n";
        }
        
        // perform actual serialisation of the primary class stored.
        if(!_Impl::_Serialise(metaStream, instance, &State.Classes[ClassID], instance._GetInternal(), false, nullptr))
        {
            TTE_ASSERT(false, "Could not seralise meta stream primary class for %s", fn.c_str());
            instance = {};
        }
        
        if(instance)
        {
            for(U32 i = 0; i < STREAM_SECTION_COUNT; i++)
            {
                U64 diff = (U64)(metaStream.Sect[i].Data->GetSize()-metaStream.Sect[i].Data->GetPosition());
                if(diff != 0)
                {
                    TTE_ASSERT(false, "At section %s: not all bytes were read from stream! Remaining: 0x%X bytes in %s",
                               StreamSectionName[i], (U32)diff, fn.c_str());
                    instance = {};
                }
            }
        }
        
        if(metaStream.DebugOutputFile)
        {
            if(!instance)
            {
                metaStream.DebugOutput << "\n>> ERRORED! Could not read remaining bytes... (see console)";
            }
            // Move output string file
            String val = metaStream.DebugOutput.str();
            metaStream.DebugOutputFile->Write((const U8*)val.c_str(), val.length());
            metaStream.DebugOutput.clear();
            metaStream.DebugOutputFile.reset(); // flush
        }
        
        return instance; // OK / FAIL
    }
    
    const Class& GetClass(U32 id)
    {
        auto it = GetInternalState().Classes.find(id);
        TTE_ASSERT(it != GetInternalState().Classes.end(), "Could not find class by id");
        return it->second;
    }
    
    
    
    // ===================================================================         COLLECTIONS
    // ===================================================================
    
    // constructor for arrays of meta class instances. init flags, and internals
    ClassInstanceCollection::ClassInstanceCollection(ParentWeakReference prnt, U32 type) : _Size(0), _Cap(0), _ValuSize(0), _Memory(nullptr), _PairSize(0), _ColFl(0), _PrntRef(std::move(prnt))
    {
        auto clazz = State.Classes.find(type);
        TTE_ASSERT(clazz != State.Classes.end(), "Invalid class ID passed into collection initialiser"); // class must be valid
        TTE_ASSERT(clazz->second.Flags & CLASS_CONTAINER, "Class ID passed into collection is not a container class"); // not a collection
        TTE_ASSERT(State.Classes[clazz->second.ArrayValClass].RTSize > 0 && (clazz->second.ArrayKeyClass == 0
                                                                             || State.Classes[clazz->second.ArrayKeyClass].RTSize > 0), "Collection key or value class has no size");
        
        _ColID = type;
        
        // if we are an SArray
        if(clazz->second.ArraySize > 0)
        {
            _ColFl |= _COL_IS_SARRAY;
        }
        
        Class* pKey = clazz->second.ArrayKeyClass == 0 ? nullptr : _Impl::_GetClass(clazz->second.ArrayKeyClass);
        Class* pVal = _Impl::_GetClass(clazz->second.ArrayValClass);
        
        _Impl::_CalculateCollectionSizes(pKey, pVal, _ValuSize, _PairSize, _ColFl);
        
        if(_ColFl & _COL_IS_SARRAY)
        {
            _Cap = UINT32_MAX; // signal we are an sarray.
            _PairSize = _ValuSize;
            _Size = clazz->second.ArraySize;
            _Memory = (U8*)this + sizeof(ClassInstanceCollection); // memory is stored after this
            // construct elements
            // check if we we can skip destructor
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_CT) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    ParentWeakReference _{};
                    U8* pMem = _Memory + (i * _ValuSize);
                    
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Constructor,
                    //           "Meta collection class: no value constructor found");
                    _Impl::_DoConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem, _PrntRef, _, false);
                    
                }
            }
            else
            {
                memset((U8*)this + sizeof(ClassInstanceCollection), 0,  _ValuSize * _Size);
            }
        }
        
        // setup transience block
        _TransienceFence = TTE_NEW_PTR(std::atomic<U32>, MEMORY_TAG_TRANSIENT_FENCE, 0);
    }
    
    void ClassInstanceCollection::AdvanceTransienceFenceInternal()
    {
        if(_TransienceFence)
            (*_TransienceFence)++;
    }
    
    ClassInstanceCollection::~ClassInstanceCollection()
    {
        auto fence = std::move(_TransienceFence);
        if(fence.use_count() > 1)
            fence->store(COLLECTION_TRANSIENCE_MAX);
        Clear();
        if(_ColFl & _COL_IS_SARRAY) // now we can destruct our elements (treat them as members)
        {
            // check if we we can skip destructor
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_DT) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pMem = _Memory + (i * _PairSize);
                    
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Destructor,
                    //           "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem, false);
                    
                }
            }
        }
        _ColFl = _PairSize = _ValuSize = _ColID = 0;
        _Memory = nullptr;
    }
    
    ClassInstanceCollection::ClassInstanceCollection(ClassInstanceCollection&& rhs, ParentWeakReference host)
    {
        _Size = rhs._Size;
        _Cap = rhs._Cap;
        _ColID = rhs._ColID;
        _ColFl = rhs._ColFl;
        _ValuSize = rhs._ValuSize;
        _PairSize = rhs._PairSize;
        _PrntRef = host;
        
        // reset RHS stuff to defaults (its still now a valid collection, just size 0)
        rhs._Size = rhs._ColFl & _COL_IS_SARRAY ? State.Classes[_ColID].ArraySize : 0;
        rhs._Cap = rhs._ColFl & _COL_IS_SARRAY ? UINT32_MAX : 0;
        
        if(rhs._ColFl & _COL_IS_SARRAY)
        {
            _Memory = (U8*)this + sizeof(ClassInstanceCollection);
            rhs._Memory = (U8*)&rhs + sizeof(ClassInstanceCollection);
            
            // check if we we can skip destructor
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_DT) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pMem = _Memory + (i * _PairSize);
                    
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Destructor,
                    //           "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem, false);
                    
                }
            }
            // check if we we can skip move ctor
            bSkipVal = (_ColFl & _COL_VAL_SKIP_MV) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                ParentWeakReference _{};
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pDstMem = _Memory + (i * _PairSize);
                    U8* pSrcMem = rhs._Memory + (i * _PairSize);
                    
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].MoveConstruct,
                    //         "Meta collection class: no value move constructor found");
                    
                    _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMem, pSrcMem, host, _, false, false);
                    
                }
            }
            else
                memset(_Memory, 0, _PairSize * _Size);
            
        }
        else
        { // move memory, dynamic array. (very quick!)
            _Memory = rhs._Memory;
            rhs._Memory = nullptr;
        }
        
        // move transience fence
        _TransienceFence = std::move(rhs._TransienceFence);
        rhs._TransienceFence = TTE_NEW_PTR(std::atomic<U32>, MEMORY_TAG_TRANSIENT_FENCE, 0); // rhs still is valid give it a new slot
    }
    
    ClassInstanceCollection::ClassInstanceCollection(const ClassInstanceCollection& rhs, ParentWeakReference host)
    {
        // copy construct. call operator= copy as its a big function (ensuring Clear succeeds)
        _PrntRef = host;
        _ValuSize = rhs._ValuSize;
        _PairSize = rhs._PairSize;
        _ColFl = rhs._ColFl;
        _ColID = rhs._ColID;
        _Size = rhs._Size;
        _Cap = rhs._Cap;
        
        // setup transience block
        _TransienceFence = _TransienceFence = TTE_NEW_PTR(std::atomic<U32>, MEMORY_TAG_TRANSIENT_FENCE, 0);
        // rhs transience fence stays the fence, no update
        
        if(_ColFl & _COL_IS_SARRAY)
        {
            _Memory = (U8*)this + sizeof(ClassInstanceCollection);
            
            // call destructors, then copy constructs
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_DT) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pMem = _Memory + (i * _PairSize);
                    
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Destructor,
                    //           "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem, false);
                    
                }
            }
            
            // call copy constructors
            bSkipVal = (_ColFl & _COL_VAL_SKIP_CP) != 0;
            
            if(bSkipVal) // fastest
                memcpy(_Memory, rhs._Memory, _PairSize * _Size);
            else
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    ParentWeakReference _{};
                    const U8* pSrcMem = rhs._Memory + (i * _PairSize);
                    U8* pDstMem = _Memory + (i * _PairSize);
                    
                    // construct each element
                    // offset for value from key value pair memory pointer is = _PairSize - _ValuSize
                    U32 offset = _PairSize - _ValuSize;
                    
                    // copy construct the value
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].CopyConstruct,
                    //           "Meta collection class: no value copy constructor found");
                    _Impl::_DoCopyConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, host, _, false, false);
                    
                }
            }
            
        }
        else
        {
            _Memory = nullptr;
            if(rhs._Cap > 0)
            {
                _Cap = _Size = 0;
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
                            ParentWeakReference _{};
                            // copy construct the key
                            //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayKeyClass && State.Classes[State.Classes[_ColID].ArrayKeyClass].CopyConstruct,
                            //    	    "Meta collection class: no key copy constructor found");
                            _Impl::_DoCopyConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, host, _, false, false);
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
                            ParentWeakReference _{};
                            // copy construct the value
                            // TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].CopyConstruct,
                            //            "Meta collection class: no value copy constructor found");
                            _Impl::_DoCopyConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, host, _, false, false);
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
        }
    }
    
    U32 ClassInstanceCollection::GetSize()
    {
        return _Size;
    }
    
    U32 ClassInstanceCollection::GetCapacity()
    {
        return _Cap == UINT32_MAX ? _Size : _Cap; // SArray capacity is max
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
        return State.Classes[_ColID].ArrayValClass;
    }
    
    Bool ClassInstanceCollection::IsStaticArray()
    {
        return (_ColFl & _COL_IS_SARRAY) != 0;
    }
    
    U32 ClassInstanceCollection::GetKeyClass()
    {
        return State.Classes[_ColID].ArrayKeyClass;
    }
    
    void ClassInstanceCollection::Reserve(U32 cap)
    {
        AdvanceTransienceFenceInternal(); // new memory potentially, any references to objects from array are invalid
        // ensure the backend memory array has size at least cap.
        
        // sanity
        if(_Cap >= cap) // sarray cap is unint32 max, so ok
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
                        ParentWeakReference _{};
                        // move construct the key
                        //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayKeyClass && State.Classes[State.Classes[_ColID].ArrayKeyClass].MoveConstruct,
                        //           "Meta collection class: no key move constructor found");
                        _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, _PrntRef, _, false, false);
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
                        ParentWeakReference _{};
                        // move construct the value
                        // TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].MoveConstruct,
                        //            "Meta collection class: no value move constructor found");
                        _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass],
                                                pDstMem + offset, pSrcMem + offset, _PrntRef, _, false, false);
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
        AdvanceTransienceFenceInternal(); // any references to objects from array are invalid
        if((_ColFl & _COL_IS_SARRAY) != 0)
            return; // do nothing on clear sarray
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
                        //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayKeyClass && State.Classes[State.Classes[_ColID].ArrayKeyClass].Destructor,
                        //           "Meta collection class: no key destructor found");
                        _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pMem, false);
                    }
                    
                    if(!bSkipVal) // if we need to destruct the value
                    {
                        //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Destructor,
                        //           "Meta collection class: no value destructor found");
                        _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem + _PairSize - _ValuSize, false);
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
        AdvanceTransienceFenceInternal(); // any references to objects from array are invalid
        if(_ColFl & _COL_IS_SARRAY)
        {
            TTE_ASSERT(false, "Cannot push to statically sized array");
            return;
        }
        
        Reserve(_Size + 1); // Ensure size
        
        // update size
        _Size++;
        
        SetIndexInternal(_Size - 1, k, v, copyk, copyv);
    }
    
    void ClassInstanceCollection::Insert(ClassInstance key, ClassInstance value, I32 index, Bool ck, Bool cv)
    {
        if(index >= _Size)
            Push(std::move(key), std::move(value), ck, cv);
        else
        {
            AdvanceTransienceFenceInternal();
            Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_MV) != 0;
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_MV) != 0;
            Bool bSkipKeyDT = (_ColFl & _COL_KEY_SKIP_DT) != 0;
            Bool bSkipValDT = (_ColFl & _COL_VAL_SKIP_DT) != 0;
            Bool bKeyed = IsKeyedCollection();
            Bool bFullPOD = (!bKeyed || bSkipKey) && bSkipVal;
            Reserve(_Size + 1);
            if (bFullPOD)
            {
                U8* dst = _Memory + (_PairSize * (index + 1));
                U8* src = _Memory + (_PairSize * index);
                U32 count = (_Size - index) * _PairSize;
                memmove(dst, src, count);
            }
            else
            {
                for(I32 i = (I32)_Size - 1; i >= index; i--)
                {
                    U8* currentKey = _Memory + (_PairSize * i);
                    U8* currentValue = currentKey + _PairSize - _ValuSize;
                    U8* intoKey = _Memory + (_PairSize * (i+1));
                    U8* intoValue = intoKey + _PairSize - _ValuSize;
                    if(bKeyed)
                    {
                        if(bSkipKey)
                            memcpy(intoKey, currentKey, _PairSize - _ValuSize);
                        else
                            _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], intoKey, currentKey, _PrntRef, _PrntRef, false, false);
                        if(!bSkipKeyDT)
                            _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], currentKey, false);
                    }
                    if(bSkipVal)
                        memcpy(intoValue, currentValue, _ValuSize);
                    else
                        _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], intoValue, currentValue, _PrntRef, _PrntRef, false, false);
                    if(!bSkipValDT)
                        _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], currentValue, false);
                }
            }
            SetIndexInternal(index, std::move(key), std::move(value), ck, cv);
            _Size++; // bump size
        }
    }
    
    void ClassInstanceCollection::SetIndex(U32 i, ClassInstance k, ClassInstance v, Bool copyk, Bool copyv)
    {
        AdvanceTransienceFenceInternal();
        if(i < GetSize())
        {
            // destruct previous
            
            // check if we we can skip destructor
            Bool bSkipKey = (_ColFl & _COL_KEY_SKIP_DT) != 0;
            Bool bSkipVal = (_ColFl & _COL_VAL_SKIP_DT) != 0;
            
            if(!bSkipKey || !bSkipVal)
            {
                U8* pMem = _Memory + (i * _PairSize);
                
                if(!bSkipKey) // if we need to destruct the key
                {
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayKeyClass && State.Classes[State.Classes[_ColID].ArrayKeyClass].Destructor,
                    //           "Meta collection class: no key destructor found");
                    _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pMem, false);
                }
                
                if(!bSkipVal) // if we need to destruct the value
                {
                    //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].Destructor,
                    //           "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pMem, false);
                }
            }
            
            SetIndexInternal(i, k, v, copyk, copyv); // set it
        }
    }
    
    void ClassInstanceCollection::SetIndexInternal(U32 i, ClassInstance k, ClassInstance v, Bool copyk, Bool copyv)
    {
        ParentWeakReference _{};
        
        U8* pDstMemory = _Memory + (i * _PairSize); // get destination memory pointer, at size.
        
        // 1. key construction
        if(IsKeyedCollection())
        {
            if(!k)
            {
                ClassInstance me{_ColID, (U8*)this, _PrntRef};
                // construct new key without copy or move
                _Impl::_DoConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pDstMemory, _PrntRef, _, false);
            }
            else
            {
                if(copyk) // copy or move from value
                    _Impl::_DoCopyConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass],
                                            pDstMemory, k._GetInternal(), _PrntRef, _, k.IsTopLevel(), false);
                else
                    _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass],
                                            pDstMemory, k._GetInternal(), _PrntRef, _, k.IsTopLevel(), false);
            }
        }
        
        // 2. val construction
        pDstMemory += (_PairSize - _ValuSize); // move pointer to start of value memory (skip key and padding)
        if(!v)
        {
            ClassInstance me{_ColID, (U8*)this, _PrntRef};
            _Impl::_DoConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMemory, _PrntRef, _, false);
        }
        else
        {
            if(copyv) // copy or move from value
                _Impl::_DoCopyConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal(), _PrntRef, _, v.IsTopLevel(), false);
            else
                _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal(), _PrntRef, _, v.IsTopLevel(), false);
        }
    }
    
    void ClassInstanceCollection::Sort(void *user, CollectionComparatorLess *pComparator)
    {
        AdvanceTransienceFenceInternal();
        U8 fasterMemory[128]{};
        Memory::FastBufferAllocator alloc{};
        TTE_ASSERT(pComparator, "Invalid comparator! Null.");
        // Helper fast locals
        ParentWeakReference _{};
        U32 size = GetSize();
        Bool keyed = IsKeyedCollection();
        Bool bAllOk = false;
        U8* tempMemory;
        if(_PairSize < 128)
            tempMemory = fasterMemory;
        else
            tempMemory = alloc.Alloc(_PairSize, 8);
        memset(fasterMemory, 0, 128);
        U8* tempKeyMemory = tempMemory;
        U8* tempValueMemory = tempKeyMemory + _PairSize - _ValuSize;
        Class* keyClazz = keyed ? const_cast<Class*>(&GetClass(GetKeyClass())) : nullptr;
        Class* valClazz = const_cast<Class*>(&GetClass(GetValueClass()));
        // Classic bubble early out sort.
        while(!bAllOk)
        {
            bAllOk = true;
            for(U32 i = 0; i < size-1; i++)
            {
                Bool comparatorResult = keyed ? pComparator(user, GetKey(i), GetKey(i + 1)) : pComparator(user, GetValue(i), GetValue(i + 1));
                if(!comparatorResult)
                {
                    bAllOk = false;
                    // Swap
                    
                    U8* keyValue1 = _Memory + (i*_PairSize), *keyValue2 = _Memory + ((i+1)*_PairSize);
                    U8* valValue1 = _Memory + (((i+0)*_PairSize) - _ValuSize), *valValue2 = _Memory + (((i+1)*_PairSize) - _ValuSize);
                    
                    // Move val1 into temp
                    if(keyed)
                        _Impl::_DoMoveConstruct(keyClazz, tempKeyMemory, keyValue1, _PrntRef, _, false, false);
                    _Impl::_DoMoveConstruct(valClazz, tempValueMemory, valValue1, _PrntRef, _, false, false);
                    memset(keyValue1, 0, _PairSize); // reset key and value memory
                    
                    // Move val2 into val1
                    if(keyed)
                        _Impl::_DoMoveConstruct(keyClazz, keyValue1, keyValue2, _PrntRef, _, false, false);
                    _Impl::_DoMoveConstruct(valClazz, valValue1, valValue2, _PrntRef, _, false, false);
                    memset(keyValue2, 0, _PairSize); // reset key and value memory
                    
                    // Move temp into val2
                    if(keyed)
                        _Impl::_DoMoveConstruct(keyClazz, keyValue2, tempKeyMemory, _PrntRef, _, false, false);
                    _Impl::_DoMoveConstruct(valClazz, valValue2, tempValueMemory, _PrntRef, _, false, false);
                    memset(tempMemory, 0, _PairSize); // reset key and value memory
                }
            }
        }
    }
    
    Bool ClassInstanceCollection::Pop(U32 index, ClassInstance& keyOut, ClassInstance& valOut)
    {
        AdvanceTransienceFenceInternal(); // any references to objects from array are invalid
        ParentWeakReference _{};
        if(_ColFl & _COL_IS_SARRAY)
        {
            TTE_ASSERT(false, "Cannot pop from statically sized array");
            return false;
        }
        
        if(_Size == 0)
            return false; // if empty, ignore.
        
        // if >= size, set to top
        if(index >= _Size)
            index = _Size - 1;
        
        // Move construct both into new value, also call destructors
        ClassInstance empty{};
        if(IsKeyedCollection())
        {
            keyOut = _Impl::_MakeInstance(State.Classes[_ColID].ArrayKeyClass, empty, Symbol{});
            _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass],
                                    keyOut._GetInternal(), GetKey(index)._GetInternal(), _PrntRef, _, false, true);
            _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], GetKey(index)._GetInternal(), false);
        }
        valOut = _Impl::_MakeInstance(State.Classes[_ColID].ArrayValClass, empty, Symbol{});
        _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass],
                                valOut._GetInternal(), GetValue(index)._GetInternal(), _PrntRef, _, false, true);
        _Impl::_DoDestruct(&State.Classes[State.Classes[_ColID].ArrayValClass], GetValue(index)._GetInternal(), false);
        
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
                //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayKeyClass && State.Classes[State.Classes[_ColID].ArrayKeyClass].MoveConstruct,
                //           "Meta collection class: no key move constructor found");
                _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, _PrntRef, _, false, false);
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
                //TTE_ASSERT(_ColID && State.Classes[_ColID].ArrayValClass && State.Classes[State.Classes[_ColID].ArrayValClass].MoveConstruct,
                //            "Meta collection class: no value move constructor found");
                _Impl::_DoMoveConstruct(&State.Classes[State.Classes[_ColID].ArrayValClass],
                                        pDstMem + offset, pSrcMem + offset, _PrntRef, _, false, false);
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
    
    ClassInstance ClassInstanceCollection::SubRef(U32 id, U8* pMem)
    {
        return ClassInstance{id, pMem, _PrntRef};
    }
    
    ClassInstance ClassInstanceCollection::GetKey(U32 i)
    {
        TTE_ASSERT(IsKeyedCollection(), "GetKey cannot be called on a collection class which is not a keyed collection");
        TTE_ASSERT(i < _Size, "Trying to access element outside array bounds");
        
        return SubRef(State.Classes[_ColID].ArrayKeyClass, _Memory + (i * _PairSize));
    }
    
    ClassInstance ClassInstanceCollection::GetValue(U32 i)
    {
        TTE_ASSERT(i < _Size, "Trying to access element outside array bounds");
        
        return SubRef(State.Classes[_ColID].ArrayValClass, _Memory + (++i * _PairSize) - _ValuSize);
    }
    
    // COERSION IMP
    
    CommonClassAllocator* GetCommonAllocator(U32 clz)
    {
        String clsName = MakeTypeName(GetClass(clz).Name);
        auto& reg = _Impl::_CoersionRegistry();
        for(auto& v: reg)
        {
            if(StringMask::MatchSearchMask(clsName.c_str(), v.ClassName, StringMask::MASKMODE_SIMPLE_MATCH))
            {
                return v.Allocator;
            }
        }
        return nullptr;
    }
    
    Bool CoerceTypeErasedToLua(LuaManager& man, void* pObj, U32 clz)
    {
        String clsName = MakeTypeName(GetClass(clz).Name);
        auto& reg = _Impl::_CoersionRegistry();
        for(auto& v: reg)
        {
            if(StringMask::MatchSearchMask(clsName.c_str(), v.ClassName, StringMask::MASKMODE_SIMPLE_MATCH))
            {
                v.ImportLua(pObj, man);
                return true;
            }
        }
        return false;
    }
    
    Bool CoerceMetaToLua(LuaManager& man, ClassInstance& inst, ClassInstanceCollection* pOwningCollection, I32 ci)
    {
        const auto& clazz = GetClass(inst.GetClassID());
        String clsName = MakeTypeName(clazz.Name);
        auto& reg = _Impl::_CoersionRegistry();
        for(auto& v: reg)
        {
            if(StringMask::MatchSearchMask(clsName.c_str(), v.ClassName, StringMask::MASKMODE_SIMPLE_MATCH))
            {
                v.MetaToLua(man, inst);
                return true;
            }
        }
        if(clazz.Flags & CLASS_CONTAINER || clazz.Flags & _CLASS_PROP)
        {
            if(pOwningCollection && ci >= 0)
                pOwningCollection->PushTransientScriptRef(man, ci, false, inst.ObtainParentRef());
            else
                inst.PushWeakScriptRef(man, inst.ObtainParentRef());
            return true;
        }
        else if((clazz.Flags & CLASS_ENUM_WRAPPER) && clazz.Members.size() > 0 && clazz.Members[0].Flags & MEMBER_ENUM)
        {
            // ENUM
            U32 off = 0;
            const auto* pEnumClazz = &clazz;
            while(true)
            {
                off += pEnumClazz->Members[0].RTOffset;
                const Class& clz = Meta::GetClass(pEnumClazz->Members[0].ClassID);
                if(clz.Members.size() == 0)
                    break;
                U32 memberIndex = 0;
                for(const auto& member: pEnumClazz->Members)
                {
                    if(member.Flags & MEMBER_BASE)
                        memberIndex++;
                    else
                        break;
                }
                pEnumClazz = &Meta::GetClass(pEnumClazz->Members[memberIndex].ClassID);
                // a lot of the times theres like 2 wrapper classes. so stupid i know.
            }
            I32 value = COERCE(inst._GetInternal() + off, I32);
            for(const auto& desc: pEnumClazz->Members[0].Descriptors)
            {
                if(value == desc.Value)
                {
                    man.PushLString(desc.Name);
                    return true;
                }
            }
            TTE_LOG("WARNING: When coercing to Lua enum '%s' the enum value does not correspond to an associated enum. Returning the integer string", clazz.Name.c_str());
            man.PushLString(std::to_string(value));
            return true;
        }
        return false;
    }
    
    Bool CoerceLuaToMeta(LuaManager& man, I32 stackIndex, ClassInstance& inst)
    {
        stackIndex = man.ToAbsolute(stackIndex);
        const auto& clazz = GetClass(inst.GetClassID());
        String clsName = MakeTypeName(clazz.Name);
        auto& reg = _Impl::_CoersionRegistry();
        for(auto& v: reg)
        {
            if(StringMask::MatchSearchMask(clsName.c_str(), v.ClassName, StringMask::MASKMODE_SIMPLE_MATCH))
            {
                v.LuaToMeta(man, stackIndex, inst);
                return true;
            }
        }
        if(clazz.Flags & CLASS_CONTAINER)
        {
            ClassInstance col = AcquireScriptInstance(man, stackIndex);
            // destruct current, copy construct this collection into it (maybe should have added move/copy assignment instead of constructor... oh well)
            ClassChildMap children{};
            Bool bChildren = (inst.IsTopLevel() || (GetClass(inst.GetClassID()).Flags & CLASS_ATTACHABLE));
            if(bChildren)
                children = std::move(*inst._GetInternalChildrenRefs());
            _Impl::_DoDestruct(const_cast<Class*>(&GetClass(inst.GetClassID())), inst._GetInternal(), inst.IsTopLevel());
            ParentWeakReference p = inst.ObtainParentRef();
            _Impl::_DoCopyConstruct(const_cast<Class*>(&GetClass(inst.GetClassID())), inst._GetInternal(), col._GetInternal(),
                                    inst.ObtainParentRef(), p, col.IsTopLevel(), inst.IsTopLevel());
            if(bChildren)
                *inst._GetInternalChildrenRefs() = std::move(children);
            return true;
        }
        else if(clazz.Flags & _CLASS_PROP)
        {
            ClassInstance from = AcquireScriptInstance(man, stackIndex);
            PropertySet::ClearKeys(inst);
            PropertySet::ClearParents(inst);
            PropertySet::ImportKeysValuesAndParents(inst, from, true, true, {}, false, false, ResourceRegistry::GetBoundRegistry(man));
            return true;
        }
        else if((clazz.Flags & CLASS_ENUM_WRAPPER) && clazz.Members.size() > 0 && clazz.Members[0].Flags & MEMBER_ENUM)
        {
            // ENUM
            I32& value = COERCE(inst._GetInternal() + clazz.Members[0].RTOffset, I32);
            if(man.Type(stackIndex) == LuaType::NUMBER)
            {
                value = man.ToInteger(stackIndex);
            }
            else if(man.Type(stackIndex) == LuaType::STRING)
            {
                String v = man.ToString(stackIndex);
                U32 off = 0;
                const auto* pEnumClazz = &clazz;
                while (true)
                {
                    off += pEnumClazz->Members[0].RTOffset;
                    const Class& clz = Meta::GetClass(pEnumClazz->Members[0].ClassID);
                    if (clz.Members.size() == 0)
                        break;
                    U32 memberIndex = 0;
                    for (const auto& member : pEnumClazz->Members)
                    {
                        if (member.Flags & MEMBER_BASE)
                            memberIndex++;
                        else
                            break;
                    }
                    pEnumClazz = &Meta::GetClass(pEnumClazz->Members[memberIndex].ClassID);
                    // a lot of the times theres like 2 wrapper classes. so stupid i know.
                }
                I32 value = COERCE(inst._GetInternal() + off, I32);
                for(const auto& desc: pEnumClazz->Members[0].Descriptors)
                {
                    if(CompareCaseInsensitive(v, desc.Name))
                    {
                        value = desc.Value;
                        return true;
                    }
                }
                TTE_LOG("WARNING: When coercing from Lua, the enum '%s', the input enum '%s' is not a valid enum value for this enum", clazz.Name.c_str(), v.c_str());
            }
            else
            {
                TTE_LOG("WARNING: When coercing from Lua, the enum '%s', the input is not a string or integer: %s", clazz.Name.c_str(), man.Typename(man.Type(stackIndex)));
            }
            return true;
        }
        return false;
    }
    
}

const Meta::RegGame* ToolContext::GetActiveGame()
{
    return Meta::State.GameIndex == -1 ? nullptr : &Meta::State.Games[Meta::State.GameIndex];
}

Bool InstanceTransformation::PerformNormaliseAsync(Ptr<Handleable> pCommonInstanceOut, Meta::ClassInstance Instance, LuaManager& L)
{
    String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.NormaliserStringFn;
    auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
    TTE_ASSERT(normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found: '%s' for class %s", fn.c_str(), Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.Name.c_str());
    
    ScriptManager::GetGlobal(L, fn, true);
    if(L.Type(-1) != LuaType::FUNCTION)
    {
        L.Pop(1);
        TTE_ASSERT(L.LoadChunk(fn, normaliser->second.Binary,
                               normaliser->second.Size, LoadChunkMode::BINARY), "Could not load normaliser chunk for %s", fn.c_str());
    }
    
    Instance.PushWeakScriptRef(L, Instance.ObtainParentRef());
    L.PushOpaque(pCommonInstanceOut.get());
    
    L.CallFunction(2, 1, false);
    
    Bool result;
    
    if(!(result=ScriptManager::PopBool(L)))
    {
        TTE_LOG("Normalise failed for %s", fn.c_str());
        return false;
    }
    else
    {
        pCommonInstanceOut->FinaliseNormalisationAsync();
    }
    return true;
}

Bool InstanceTransformation::PerformSpecialiseAsync(Ptr<Handleable> pCommonInstance, Meta::ClassInstance Instance, LuaManager& L)
{
    String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.SpecialiserStringFn;
    auto sp = Meta::GetInternalState().Normalisers.find(fn);
    TTE_ASSERT(sp != Meta::GetInternalState().Normalisers.end(), "Specialiser not found: '%s'", fn.c_str());
    
    ScriptManager::GetGlobal(L, fn, true);
    if(L.Type(-1) != LuaType::FUNCTION)
    {
        L.Pop(1);
        TTE_ASSERT(L.LoadChunk(fn, sp->second.Binary,
                               sp->second.Size, LoadChunkMode::BINARY), "Could not load specialiser chunk for %s", fn.c_str());
    }
    
    Instance.PushWeakScriptRef(L, Instance.ObtainParentRef());
    L.PushOpaque(pCommonInstance.get());
    
    L.CallFunction(2, 1, false);
    
    Bool result;
    
    if(!(result=ScriptManager::PopBool(L)))
    {
        TTE_LOG("Specialise failed for %s", fn.c_str());
        return false;
    }
    return true;
}

IMPL_COERSION_FN(AnimOrChore, Extract, void)(AnimOrChore& out, Meta::ClassInstance& inst)
{
    HandlePropertySet hProxy{};
    ClassInstance mem = Meta::GetMember(inst, "mhAnim", true);
    ExtractCoercableInstance(hProxy, mem);
    out.HandleAnim = SymbolTable::Find(hProxy.GetObject());
    mem = Meta::GetMember(inst, "mhChore", true);
    ExtractCoercableInstance(hProxy, mem);
    out.HandleChore = SymbolTable::Find(hProxy.GetObject());
}

IMPL_COERSION_FN(AnimOrChore, Import, void)(const AnimOrChore& in, ClassInstance& inst)
{
    HandlePropertySet hProxy{};
    hProxy.SetObject(in.HandleAnim);
    ClassInstance mem = Meta::GetMember(inst, "mhAnim", true);
    ImportCoercableInstance(hProxy, inst);
    hProxy.SetObject(in.HandleChore);
    mem = Meta::GetMember(inst, "mhChore", true);
    ImportCoercableInstance(hProxy, inst);
}

IMPL_COERSION_FN(AnimOrChore, ImportLua, void)(const AnimOrChore& in, LuaManager& man)
{
    if(in.HandleAnim.length())
    {
        man.PushLString(in.HandleAnim);
    }
    else if(in.HandleChore.length())
    {
        man.PushLString(in.HandleChore);
    }
    else
    {
        man.PushLString("");
    }
}

IMPL_COERSION_FN(AnimOrChore, ExtractLua, void)(AnimOrChore& out, LuaManager& man, I32 stackIndex)
{
    if(man.Type(stackIndex) == LuaType::STRING)
    {
        Symbol fileName = ScriptManager::ToSymbol(man, stackIndex);
        String resolved = SymbolTable::Find(fileName);
        if(StringEndsWith(resolved, ".chore"))
        {
            out.HandleAnim = "";
            out.HandleChore = resolved;
        }
        else if(StringEndsWith(resolved, ".anm"))
        {
            out.HandleAnim = resolved;
            out.HandleChore = "";
        }
        else
        {
            out.HandleAnim = out.HandleChore = "";
        }
    }
    else
    {
        TTE_LOG("WARNING: When coercing from Lua AnimOrChore: input is not a string");
    }
}
