#include <Meta/Meta.hpp>
#include <Core/Context.hpp>
#include <Scripting/LuaManager.hpp>
#include <Core/Symbol.hpp>
#include <sstream>
#include <map>

thread_local Bool _IsMain = false; // Used in meta.

Bool IsCallingFromMain()
{
    return _IsMain;
}

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

extern LuaFunctionCollection luaGameEngine(); // actual engine game api in EngineLUAApi.cpp
extern LuaFunctionCollection luaLibraryAPI(); // actual api in LibraryLUAApi.cpp

namespace Meta {
    
    std::vector<RegGame> Games{};
    std::map<U32, Class> Classes{};
    std::map<Symbol, CompiledSerialiserScript> Serialisers{}; // map of serialiser script path symbol => compiled script binary
    I32 GameIndex = -1;
    String VersionCalcFun{}; // lua function which calculates version crc for a type.
    
    // ===================================================================         IMPL
    // ===================================================================
    
    namespace _Impl {
        
        U32 _GenerateClassID(U64 typeHash, U32 versionNumber)
        {
            return CRC32((U8*)&typeHash, 8, versionNumber ^ 0xF0F0F0F0);
            // needs to be unique. hash the type hash with initial crc being the version number inverted every 4 bits
        }
        
        // register class argument to meta system
        U32 _Register(LuaManager& man, Class&& cls, I32 stackIndex)
        {
            TTE_ASSERT(_IsMain, "Must only be called from main thread"); // modifications to Classes should only happen at beginning on main.
            
            cls.TypeHash = CRC64LowerCase((const U8*)cls.Name.c_str(), (U32)cls.Name.length());
            U32 crc = _DoLuaVersionCRC(man, stackIndex);
            if(crc == 0)
                return 0; // errored
            U32 id = _GenerateClassID(cls.TypeHash, cls.VersionNumber);
            
            // chcek any duplicate version number
            if(Classes.find(id) != Classes.end())
            {
                TTE_LOG("Duplicate type detected for '%s': multiple same version indices for the given class!", cls.Name.c_str());
                return 0;
            }
            
            // check any duplicate version CRCs
            for(U32 i = 0; i < MAX_VERSION_NUMBER; i++)
            {
                if(i != cls.VersionNumber) {
                    auto c = Classes.find(_GenerateClassID(cls.TypeHash, i));
                    if(c != Classes.end() && c->second.VersionCRC == crc)
                    {
                        TTE_LOG("Duplicate type detected for '%s': version CRCs are the same for the given class!", cls.Name.c_str());
                        return 0;
                    }
                }
            }
            
            if(cls.SerialiseScriptFn.length() > 0)
            {
                // register serialise and compile it
                if(Serialisers.find(cls.SerialiseScriptFn) == Serialisers.end())
                {
                    // compile and add it
                    
                    ScriptManager::GetGlobal(GetToolContext()->GetLibraryLVM(), cls.SerialiseScriptFn, true);
                    
                    if(GetToolContext()->GetLibraryLVM().Type(-1) != LuaType::FUNCTION)
                    {
                        GetToolContext()->GetLibraryLVM().Pop(1);
                        TTE_ASSERT(false, "The serialiser function '%s' does not exist as a function.", cls.SerialiseScriptFn.c_str());
                        return 0;
                    }
                    
                    // function is on the top of the stack, compile it.
                    DataStreamRef localWriter = DataStreamManager::GetInstance()->CreatePrivateCache(cls.SerialiseScriptFn);
                    if(!GetToolContext()->GetLibraryLVM().Compile(localWriter) || localWriter->GetSize() <= 0)
                    {
                        GetToolContext()->GetLibraryLVM().Pop(1); // pop the func
                        TTE_ASSERT(false, "Cannot register class serialiser script %s"
                                   ": compile failed or empty", cls.SerialiseScriptFn.c_str());
                        return 0;
                    }
                    GetToolContext()->GetLibraryLVM().Pop(1); // pop the func
                    
                    localWriter->SetPosition(0); // seek to beginning, then read all bytes.
                    U8* compiledBytes = TTE_ALLOC(localWriter->GetSize(), MEMORY_TAG_SCRIPTING);
                    
                    if(!localWriter->Read(compiledBytes, localWriter->GetSize()))
                    {
                        TTE_ASSERT(false, "Could not read bytes from compiled serialiser script stream");
                        TTE_FREE(compiledBytes);
                        return 0;
                    }
                    
                    CompiledSerialiserScript compiledScript{};
                    compiledScript.Binary = compiledBytes;
                    compiledScript.Size = (U32)localWriter->GetSize();
                    Serialisers[cls.SerialiseScriptFn] = compiledScript; // save it
                    
                }
            }
            
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
            
            cls.VersionCRC = crc;
            cls.ClassID = id;
            Classes[id] = std::move(cls);
            return id;
        }
        
        Bool _Serialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite)
        {
            if(!pMemory || clazz == nullptr)
            {
                TTE_ASSERT(false, "Cannot serialise, type or class passed in is null");
                return false;
            }
            
            if(((clazz->Flags & CLASS_INTRINSIC) == 0)) // add to version header if needed
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

            StreamSection initialSection = stream.CurrentSection; // ensure after the section is the same
            
            Bool result;
            
            if(clazz->Serialise) // has serialiser.
            {
                result = clazz->Serialise(stream, host, clazz, pMemory, IsWrite);
            }
            else if(clazz->SerialiseScriptFn.length() > 0) // RUN LUA SERIALISER FUNCTION
            {
                // TODO ASYNC LUA RUNTIME. (context needs to be passed in as arg if so)
                
                auto it = Serialisers.find(clazz->SerialiseScriptFn);
                if(it == Serialisers.end())
                {
                    TTE_ASSERT(false, "Cannot serialise type %s: serialiser function failed to initilaise", clazz->Name.c_str());
                    return false; // FAIL
                }
                
                if(!GetToolContext()->GetLibraryLVM().LoadChunk(clazz->SerialiseScriptFn, it->second.Binary, it->second.Size, true))
                {
                    TTE_ASSERT(false, "Cannot serialise type %s: compiled serialiser function failed to load", clazz->Name.c_str());
                    return false; // FAIL
                }
                
                // arguments: meta stream, instance, is write
                
                GetToolContext()->GetLibraryLVM().PushOpaque(&stream); // push stream
                
                // if we are serialising the host class, pass that. else put a tmp class with host as parent and pMemory
                ClassInstance tmp;
                U8* acq = host._GetInternal();
                
                if(acq == pMemory) // if host is the same
                {
                    tmp = host;
                }
                else
                {
                    tmp = ClassInstance{clazz->ClassID, (U8*)pMemory, host.ObtainParentRef()};
                }
                
                tmp.PushScriptRef(GetToolContext()->GetLibraryLVM()); // push instance
                
                GetToolContext()->GetLibraryLVM().PushBool(IsWrite); // push is write
                
                GetToolContext()->GetLibraryLVM().CallFunction(3, 1, true); // call, locked.
                
                result = ScriptManager::PopBool(GetToolContext()->GetLibraryLVM()); // check result
                
                if(!result)
                {
                    TTE_LOG("Cannot serialise type %s: script serialisation function returned false", clazz->Name.c_str());
                    return false; // FAIL
                }
                
            }
            else
            {
                result = _DefaultSerialise(stream, host, clazz, pMemory, IsWrite);
            }
            
            stream.CurrentSection = initialSection;
            
            return result;
        }
        
        // Meta serialise all the members (default)
        Bool _DefaultSerialise(Stream& stream, ClassInstance& host, Class* clazz, void* pMemory, Bool IsWrite)
        {
            if(!pMemory || clazz == nullptr)
            {
                TTE_ASSERT(false, "Cannot serialise, type or class passed in is null");
                return false;
            }
            
            StreamSection initialSection = stream.CurrentSection; // ensure after the section is the same
            
            // serialise each member
            for(auto& member: clazz->Members)
            {
                if(member.Flags & MEMBER_SERIALISE_DISABLE)
                    continue; // skip
                
                Bool bBlocked = (Classes[member.ClassID].Flags & CLASS_NON_BLOCKED) == 0;
                
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
                if(!_Serialise(stream, host, &Classes[member.ClassID], (U8*)pMemory + member.RTOffset, IsWrite))
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
                        U64 pos = stream.Sect[stream.CurrentSection].Blocks.back();
                        stream.Sect[stream.CurrentSection].Blocks.pop_back();
                        if(pos != stream.Sect[stream.CurrentSection].Data->GetPosition())
                        {
                            TTE_ASSERT(false, "Block size mismatch when reading %s", clazz->Name.c_str());
                            return false;
                        }
                    }
                }
                
            }
            
            stream.CurrentSection = initialSection; // reset section
            
            return true;
        }
        
        String _PerformToString(U8* pMemory, Class* pClass)
        {
            if(pClass->ToString != nullptr) // we have a function to do it, else use members
            {
                return pClass->ToString(pMemory);
            }
            if(pClass->Members.size() == 0)
                return pClass->Name; // return the name of the class - it has no runtime data to get
            String result = "[";
            for(auto& member: pClass->Members)
            {
                result += " ";
                result += member.Name;
                result += " = ";
                result += _PerformToString(pMemory + member.RTOffset, &Classes[member.ClassID]);
                result += ", ";
            }
            result[result.length() - 2] = ' ';
            result[result.length() - 1] = ']';
            return result;
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
                man.CallFunction(1, 1, true);
                return ScriptManager::PopUnsignedInteger(man);
            }
        }
        
        // get class by crc (Use map)
        Class* _GetClass(U32 i) {
            return &Classes[i];
        }
        
        U32 _ClassChildrenArrayOff(Class& clazz)
        {
            U32 sz = clazz.RTSize;
            
            // for SArray types, store the static memory after the type to save an allocation
            if(clazz.ArraySize > 0)
            {
                sz += TTE_PADDING(sz, 8); // ensure 8 byte align
                sz += clazz.ArraySize * Classes[clazz.ArrayValClass].RTSize;
            }
            
            return sz;
        }
        
        U32 _ClassRuntimeSize(Class& clazz, ParentWeakReference& parentRef)
        {
            U32 sz = _ClassChildrenArrayOff(clazz);
            
            if(IsWeakPtrUnbound(parentRef))
            {
                // top level
                sz += sizeof(std::vector<std::shared_ptr<U8>>); // child ref array
            }
            
            return sz;
        }
    
        ClassInstance _MakeInstance(U32 ClassID, ClassInstance& topLevelParent)
        {
            auto it = Classes.find(ClassID);
            if(it == Classes.end()){
                TTE_LOG("Cannot create class instance for class index %X; not found in registry", ClassID);
                return ClassInstance{};
            }
            
            // obtain the parent reference
            ParentWeakReference host = topLevelParent.ObtainParentRef();
            
            // calculate the size
            U32 sz = _ClassRuntimeSize(it->second, host);
            
            // check if the parent is valid (ie if this new instance will be a top level class)
            Bool topLevel = topLevelParent._InstanceClassID == 0;
            
            // assert top level has not expired
            if(!topLevel)
            {
                if(host.expired())
                {
                    TTE_ASSERT(false, "Cannot make instance of %s: the parent weak reference has previously expired (it no longer exists)"
                               , Classes[ClassID].Name.c_str());
                    return ClassInstance{};
                }
            }

            const Bool upvalue_TopLevel = topLevel;
            const U32 upvalue_ClassID = ClassID;
            
            // allocate
            U8* pMemory = TTE_ALLOC(sz, MEMORY_TAG_META_TYPE);
            
            ClassInstance inst = ClassInstance{ClassID, pMemory, [=](U8* pMem){ // use a c++11 lambda so we get a closure.
                
                // this is a closure, and the only captured values (lua upvalue equivalent) are id/tl, as its not stored in the memory block.
                // this is the only time we use lambdas here, as they work quite well for this case, as otherwise there is no other way to
                // have a reference to these unless its also stored in the memory block, which would not work in collections.
                
                _Impl::_DoDestruct(&Classes[upvalue_ClassID], pMem); // call destructor
                
                if(upvalue_TopLevel) // free children, ensure strong refs == 1 for each
                {
                    auto& vec = *((std::vector<std::shared_ptr<U8>>*)
                                 ((U8*)pMem + _ClassChildrenArrayOff(Classes[upvalue_ClassID])));
                    
                    for(auto& child: vec)
                    {
                        TTE_ASSERT(child.use_count() == 1, "At %s instance destruction: non top-level instance still has references",
                                   Classes[upvalue_ClassID].Name.c_str());
                    }
                    
                    vec.~vector(); // free all and clear vector memory
                }
                
                // free memory
                TTE_FREE((U8*)pMem); // return with deleter
                
            }, host};
            
            if(!upvalue_TopLevel) // insert inst into parent children array
            {
                auto vec = topLevelParent._GetInternalChildrenRefs();
                vec->push_back(inst._InstanceMemory); // ref counted
            }
            else
            {
                // else we need to construct the child refs array
                new (inst._GetInternalChildrenRefs()) std::vector<std::shared_ptr<U8>>();
            }
            
            return inst;
        }
        
        // perform constructor.
        void _DoConstruct(Class* pClass, U8* pInstanceMemory, ParentWeakReference host)
         {
            // If the whole class has a constructor, call it (eg String)
            if(pClass->Constructor)
            {
                pClass->Constructor(pInstanceMemory, pClass->ClassID, host);
            }
            else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit constructor, but is intrinsic, memset.
            {
                memset(pInstanceMemory, 0, pClass->RTSize); // set to zero (eg int types)
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoConstruct(&Classes[member.ClassID], pInstanceMemory + member.RTOffset, host); // again
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
        void _DoCopyConstruct(Class* pClass, U8* pMemory, const U8* pSrc, ParentWeakReference host)
        {
            // If the whole class has a copy constructor, call it (eg String)
            if(pClass->CopyConstruct)
            {
                pClass->CopyConstruct(pSrc, pMemory, host);
            }else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit copy constructor, but is intrinsic, memcpy.
            {
                memcpy(pMemory, pSrc, pClass->RTSize);
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoCopyConstruct(&Classes[member.ClassID], pMemory + member.RTOffset, pSrc + member.RTOffset, host); // again
                }
            }
        }
        
        // perform move
        void _DoMoveConstruct(Class* pClass, U8* pMemory, U8* pSrc, ParentWeakReference host)
        {
            // If the whole class has a move constructor, call it (eg String)
            if(pClass->MoveConstruct)
            {
                pClass->MoveConstruct(pSrc, pMemory, host);
            }else if(pClass->Flags & CLASS_INTRINSIC) // the class has no explicit move constructor, but is intrinsic, memcpy.
            {
                memcpy(pMemory, pSrc, pClass->RTSize);
                memset(pSrc, 0, pClass->RTSize); // move requires setting to zero after for non-trivial (POD) intrinsics
            }
            else // else its just a normal higher level class, so do each member
            {
                for(auto& member: pClass->Members)
                {
                    _DoMoveConstruct(&Classes[member.ClassID], pMemory + member.RTOffset, pSrc + member.RTOffset, host); // again
                }
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
        
        void CtorBinaryBuffer(void* pMemory, U32, ParentWeakReference)
        {
            memset(pMemory, 0, sizeof(BinaryBuffer)); // clear to zeros (its just a pointer and size)
        }
        
        void DtorBinaryBuffer(void* pMemory, U32)
        {
            BinaryBuffer* pBuffer = (BinaryBuffer*)pMemory;
            if(pBuffer->Buffer)
                TTE_FREE(pBuffer->Buffer); // free it
            memset(pBuffer, 0, sizeof(BinaryBuffer)); // just in case
        }
        
        void CopyBinaryBuffer(const void* pSrc, void* pDst,ParentWeakReference host) // copy CONSTRUCT.
        {
            const BinaryBuffer* pSrcBuffer = (const BinaryBuffer*)pSrc;
            BinaryBuffer* pDstBuffer = (BinaryBuffer*)pDst;
            
            pDstBuffer->BufferSize = pSrcBuffer->BufferSize;
            if(pDstBuffer->BufferSize) // if we have a buffer, copy the alloc
            {
                pDstBuffer->Buffer = TTE_ALLOC(pDstBuffer->BufferSize, MEMORY_TAG_RUNTIME_BUFFER);
                memcpy(pDstBuffer->Buffer, pSrcBuffer->Buffer, sizeof(BinaryBuffer));
            }
            else
            {
                pDstBuffer->Buffer = nullptr;
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
                    String value = RuntimeSymbols.Find(sym); // find the string for it
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
                    
                    RuntimeSymbols.Register(symbol); // register it as for remapping back to a string
                    
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
            
            U32 keyClass = pCollection->GetKeyClass();
            U32 valClass = pCollection->GetValueClass();
            
            if(IsWrite)
            {
                U32 size = (U32)pCollection->GetSize();
                SerialiseU32(stream, host, nullptr, &size, true);
                for(U32 i = 0; i < size; i++)
                {
                    ClassInstance instance{};
                    
                    if(bKeyed)
                    {
                        // write key
                        instance = pCollection->GetKey(i);
                        if(!_Serialise(stream, host, &Classes[keyClass], instance._GetInternal(), true))
                            return false;
                        
                    }
                    
                    // write value
                    instance = pCollection->GetValue(i);
                    if(!_Serialise(stream, host, &Classes[valClass], instance._GetInternal(), true))
                        return false;
                    
                }
            }
            else
            {
                pCollection->Clear(); // ensure empty
                U32 size{};
                SerialiseU32(stream, host, nullptr, &size, false);
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
                        if(!_Impl::_Serialise(stream, host, &Classes[keyClass], kinstance._GetInternal(), false))
                            return false;
                    }
                    
                    vinstance = CreateInstance(valClass); // serialise in value
                    if(!_Impl::_Serialise(stream, host, &Classes[valClass], vinstance._GetInternal(), false))
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
    
    // garbage collector function for script refs to class instances
    U32 luaClassInstanceGC(LuaManager& man)
    {
        ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.ToPointer(-1);
        pRef->~ClassInstanceScriptRef(); // release weak ref (it will have a weak pointer slot, may owns stuff).
        return 0;
    }
    
    std::vector<std::shared_ptr<U8>>* ClassInstance::_GetInternalChildrenRefs()
    {
        TTE_ASSERT(IsTopLevel(), "Cannot get internal children references: class is not top level");
        U32 skipBytes = _Impl::_ClassChildrenArrayOff(Classes[_InstanceClassID]);
        return ((std::vector<std::shared_ptr<U8>>*)((U8*)_InstanceMemory.get() + skipBytes));
    }
    
    void ClassInstance::PushScriptRef(LuaManager& man, Bool strong)
    {
        if(_InstanceMemory)
        {
            ClassInstanceScriptRef* pRef = (ClassInstanceScriptRef*)man.CreateUserData((U32)sizeof(ClassInstanceScriptRef));
            new (pRef) ClassInstanceScriptRef(*this); // construct it (previous just allocates in the lua mem)
            if(strong)
                pRef->SwitchStrongRef();

            man.PushTable(); // meta table
            
            man.PushLString("__gc");
            man.PushFn(&luaClassInstanceGC);
            man.SetTable(-3); // set gc method
            
            // other metatable operations?
            
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
        
        return pRef->Acquire();
    }
    
    ClassInstance CreateInstance(U32 ClassID, ClassInstance host)
    {
        ClassInstance instance = _Impl::_MakeInstance(ClassID, host);
        
        _Impl::_DoConstruct(&Classes[ClassID], instance._GetInternal(), host.ObtainParentRef());
        
        return instance;
    }

    ClassInstance CopyInstance(ClassInstance instance, ClassInstance host)
    {
        if(!instance)
            return ClassInstance{};
        
        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID(), host);
        
        _Impl::_DoCopyConstruct(&Classes[instance.GetClassID()],
                                instanceNew._GetInternal(), instance._GetInternal(), host.ObtainParentRef());
        
        return instanceNew;
    }
    
    ClassInstance MoveInstance(ClassInstance instance, ClassInstance host)
    {
        if(!instance)
            return ClassInstance{};

        ClassInstance instanceNew = _Impl::_MakeInstance(instance.GetClassID(), host);
        
        _Impl::_DoMoveConstruct(&Classes[instance.GetClassID()],
                                instanceNew._GetInternal(), instance._GetInternal(), host.ObtainParentRef());
        
        return instanceNew;
    }
    
    ClassInstance GetMember(ClassInstance& inst, const String& name)
    {
        if(!inst)
            return {}; // edge case
        const Class& clazz = Classes[inst.GetClassID()];
        for(auto& mem: clazz.Members)
        {
            if(CompareCaseInsensitive(name, mem.Name))
            {
                return ClassInstance(mem.ClassID, inst._GetInternal() + mem.RTOffset, inst.ObtainParentRef());
            }
        }
        return {}; // not found
    }
    
    Bool PerformLessThan(ClassInstance& lhs, ClassInstance& rhs)
    {
        if(!lhs || !rhs)
            return false;
        if(lhs.GetClassID() != rhs.GetClassID())
            return false;
        auto op = Classes[lhs.GetClassID()].LessThan;
        return op ? op(lhs._GetInternal(), rhs._GetInternal()) : false;
    }
    
    Bool PerformEquality(ClassInstance& lhs, ClassInstance& rhs)
    {
        if(!lhs || !rhs)
            return false;
        if(lhs.GetClassID() != rhs.GetClassID())
            return false;
        auto op = Classes[lhs.GetClassID()].Equals;
        return op ? op(lhs._GetInternal(), rhs._GetInternal()) : false;
    }
    
    String PerformToString(ClassInstance& inst)
    {
        return inst ? _Impl::_PerformToString(inst._GetInternal(), &Classes[inst.GetClassID()]) : "";
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
            if(id && Classes[id].VersionCRC == versionCRC)
                return id;
        }
        return 0; // not found
    }
    
    U32 FindClass(U64 typeHash, U32 versionNumber)
    {
        U32 id = _Impl::_GenerateClassID(typeHash, versionNumber);
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
        U32 gameIdx = 0;
        U32 i=0;
        for(auto& game: Games)
        {
            if(game.ID == snap.ID)
            {
                Found = true;
                gameIdx = i;
                break;
            }
            i++;
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
            ScriptManager::Execute(GetToolContext()->GetLibraryLVM(), 3, 1, true);
            if(!ScriptManager::PopBool(GetToolContext()->GetLibraryLVM()))
            {
                TTE_LOG("RegisterAll failed!");
            }
            else // success, so init blowfish encryption for the game
            {
                BlowfishKey key = Games[gameIdx].MasterKey; // set to master key
                
                // if there is a specific platform encryption key, do that here
                auto it = Games[gameIdx].PlatformToEncryptionKey.find(snap.Platform);
                if(it != Games[gameIdx].PlatformToEncryptionKey.end())
                    key = it->second;
                
                if(key.BfKeyLength == 1 && key.BfKey[0] == 0)
                {
                    TTE_ASSERT(false, "Game encryption key for %s/%s has not been registered. Please contact us with the executable of "
                               "the game for this platform so we can find the encryption key!", snap.ID.c_str(), snap.Platform.c_str());
                    // encryption key '00' means we dont know it yet.
                    RelGame();
                    return;
                }
                
                Blowfish::Initialise(Games[gameIdx].ModifiedBlowfish, key.BfKey, key.BfKeyLength);
            }
        }
        
        TTE_LOG("Meta fully initialised with snapshot of %s: registered %d classes", snap.ID.c_str(), (U32)Classes.size());
        GameIndex = (I32)gameIdx;
    }
    
    void RelGame()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        
        // clear compiled memory
        for(auto& script: Serialisers)
            TTE_FREE(script.second.Binary);
        Serialisers.clear();
        
        Classes.clear();
        VersionCalcFun = "";
        GameIndex = -1;
    }
    
    void Initialise()
    {
        TTE_ASSERT(_IsMain, "Must only be called from main thread");
        TTE_ASSERT(GetToolContext(), "Tool context not created");
        
        ScriptManager::RegisterCollection(GetToolContext()->GetLibraryLVM(), luaLibraryAPI()); // Register editor library
        ScriptManager::RegisterCollection(GetToolContext()->GetLibraryLVM(), luaGameEngine()); // Register telltale engine
        
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
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_MEMORY_DISABLE);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberMemoryDisable", true); // member not in memory (no Class needed)
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_SERIALISE_DISABLE);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberSerialiseDisable", true); // member not on disk
        GetToolContext()->GetLibraryLVM().PushInteger(MEMBER_VERSION_HASH_DISABLE);
        ScriptManager::SetGlobal(GetToolContext()->GetLibraryLVM(), "kMetaMemberVersionDisable", true); // no version include
        
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
        
        if(!_Impl::_Serialise(metaStream, instance, &Classes[instance.GetClassID()], instance._GetInternal(), true))
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
            U32 ClassID = FindClassByCRC(typeHash, versionCRC);
            
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
        if(!_Impl::_Serialise(metaStream, instance, &Classes[ClassID], instance._GetInternal(), false))
        {
            TTE_ASSERT(false, "Could not seralise meta stream primary class");
            return {};
        }
        
        return instance; // OK
    }
    
    // ===================================================================         COLLECTIONS
    // ===================================================================
    
    // constructor for arrays of meta class instances. init flags, and internals
    ClassInstanceCollection::ClassInstanceCollection(ParentWeakReference prnt, U32 type) : _Size(0), _Cap(0), _ValuSize(0), _Memory(nullptr), _PairSize(0), _ColFl(0), _PrntRef(std::move(prnt))
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
            _ColFl |= _COL_IS_SARRAY;
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
        
        if(_ColFl & _COL_IS_SARRAY)
        {
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
                    U8* pMem = _Memory + (i * _ValuSize);
                    
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].Constructor,
                               "Meta collection class: no value constructor found");
                    _Impl::_DoConstruct(&Classes[Classes[_ColID].ArrayValClass], pMem, _PrntRef);
                    
                }
            }
            else
            {
                memset(this + sizeof(ClassInstanceCollection), 0,  _ValuSize * _Size);
            }
        }
        
    }
    
    ClassInstanceCollection::~ClassInstanceCollection()
    {
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
                    
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].Destructor,
                               "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayValClass], pMem);
                    
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
        _ColFl = rhs._ColFl;
        _ValuSize = rhs._ValuSize;
        _PairSize = rhs._PairSize;
        _PrntRef = std::move(host);
        
        // reset RHS stuff to defaults (its still now a valid collection, just size 0)
        rhs._Size = rhs._ColFl & _COL_IS_SARRAY ? Classes[_ColID].ArraySize : 0;
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
                    
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].Destructor,
                               "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayValClass], pMem);
                    
                }
            }
            // check if we we can skip move ctor
            bSkipVal = (_ColFl & _COL_VAL_SKIP_MV) != 0;
            
            if(!bSkipVal) // if any of key or value need a destructor call, do loop. else nothing needed (POD)
            {
                for(U32 i = 0; i < _Size; i++)
                {
                    U8* pDstMem = _Memory + (i * _PairSize);
                    U8* pSrcMem = rhs._Memory + (i * _PairSize);
                    
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].MoveConstruct,
                               "Meta collection class: no value move constructor found");
                    
                    _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem, pSrcMem, host);
                    
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
    }
    
    ClassInstanceCollection::ClassInstanceCollection(const ClassInstanceCollection& rhs, ParentWeakReference host)
    {
        // copy construct. call operator= copy as its a big function (ensuring Clear succeeds)
        _PrntRef = std::move(host);
        _ValuSize = rhs._ValuSize;
        _PairSize = rhs._PairSize;
        _ColFl = rhs._ColFl;
        _Size = rhs._Size;
        _Cap = rhs._Cap;
        
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
                    
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].Destructor,
                               "Meta collection class: no value destructor found");
                    _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayValClass], pMem);
                    
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
                    const U8* pSrcMem = rhs._Memory + (i * _PairSize);
                    U8* pDstMem = _Memory + (i * _PairSize);
                    
                    // construct each element
                    // offset for value from key value pair memory pointer is = _PairSize - _ValuSize
                    U32 offset = _PairSize - _ValuSize;
                    
                    // copy construct the value
                    TTE_ASSERT(_ColID && Classes[_ColID].ArrayValClass && Classes[Classes[_ColID].ArrayValClass].CopyConstruct,
                               "Meta collection class: no value copy constructor found");
                    _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, host);
                    
                }
            }
            
        }
        else
        {
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
                            _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, host);
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
                            _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, host);
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
        return Classes[_ColID].ArrayValClass;
    }
    
    Bool ClassInstanceCollection::IsStaticArray()
    {
        return (_ColFl & _COL_IS_SARRAY) != 0;
    }
    
    U32 ClassInstanceCollection::GetKeyClass()
    {
        return Classes[_ColID].ArrayKeyClass;
    }
    
    void ClassInstanceCollection::Reserve(U32 cap)
    {
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
                        // move construct the key
                        TTE_ASSERT(_ColID && Classes[_ColID].ArrayKeyClass && Classes[Classes[_ColID].ArrayKeyClass].MoveConstruct,
                                   "Meta collection class: no key move constructor found");
                        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, _PrntRef);
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
                        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, _PrntRef);
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
    
    void ClassInstanceCollection::SetIndex(U32 i, ClassInstance k, ClassInstance v, Bool copyk, Bool copyv)
    {
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
            
            SetIndexInternal(i, k, v, copyk, copyv); // set it
        }
    }
    
    void ClassInstanceCollection::SetIndexInternal(U32 i, ClassInstance k, ClassInstance v, Bool copyk, Bool copyv)
    {
        U8* pDstMemory = _Memory + (i * _PairSize); // get destination memory pointer, at size.
        
        // 1. key construction
        if(IsKeyedCollection())
        {
            if(!k)
            {
                ClassInstance me{_ColID, (U8*)this, _PrntRef};
                // construct new key without copy or move
                _Impl::_DoConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory, _PrntRef);
            }
            else
            {
                if(copyk) // copy or move from value
                    _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory, k._GetInternal(), _PrntRef);
                else
                    _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMemory, k._GetInternal(), _PrntRef);
            }
        }
        
        // 2. val construction
        pDstMemory += (_PairSize - _ValuSize); // move pointer to start of value memory (skip key and padding)
        if(!v)
        {
            ClassInstance me{_ColID, (U8*)this, _PrntRef};
            _Impl::_DoConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory, _PrntRef);
        }
        else
        {
            if(copyv) // copy or move from value
                _Impl::_DoCopyConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal(), _PrntRef);
            else
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMemory, v._GetInternal(), _PrntRef);
        }
    }
    
    Bool ClassInstanceCollection::Pop(U32 index, ClassInstance& keyOut, ClassInstance& valOut)
    {
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
        ClassInstance thisProxy{_ColID, (U8*)this, _PrntRef};
        if(IsKeyedCollection())
        {
            keyOut = _Impl::_MakeInstance(Classes[_ColID].ArrayKeyClass, thisProxy);
            _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], keyOut._GetInternal(), GetKey(index)._GetInternal(), _PrntRef);
            _Impl::_DoDestruct(&Classes[Classes[_ColID].ArrayKeyClass], GetKey(index)._GetInternal());
        }
        valOut = _Impl::_MakeInstance(Classes[_ColID].ArrayValClass, thisProxy);
        _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], valOut._GetInternal(), GetValue(index)._GetInternal(), _PrntRef);
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
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayKeyClass], pDstMem, pSrcMem, _PrntRef);
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
                _Impl::_DoMoveConstruct(&Classes[Classes[_ColID].ArrayValClass], pDstMem + offset, pSrcMem + offset, _PrntRef);
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
        
        return SubRef(Classes[_ColID].ArrayKeyClass, _Memory + (i * _PairSize));
    }
    
    ClassInstance ClassInstanceCollection::GetValue(U32 i)
    {
        TTE_ASSERT(i < _Size, "Trying to access element outside array bounds");
        
        return SubRef(Classes[_ColID].ArrayValClass, _Memory + (++i * _PairSize) - _ValuSize);
    }
    
}

const Meta::RegGame* ToolContext::GetActiveGame()
{
    return Meta::GameIndex == -1 ? nullptr : &Meta::Games[Meta::GameIndex];
}
