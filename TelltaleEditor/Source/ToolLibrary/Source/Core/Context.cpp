#include <Core/Context.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Meta/Meta.hpp>
#include <Resource/PSPKG.hpp>

#include <sstream>
#include <stdio.h>
#include <fstream>
#include <filesystem>

thread_local Bool _IsMain = false;

struct IsMainSetter
{
    
    IsMainSetter()
    {
        _IsMain = true;
    }
    
};

static IsMainSetter _MainSetter{};
[[maybe_unused]] static void* _ForceMainInit = (void*)&_MainSetter;

Bool IsCallingFromMain()
{
    return _IsMain;
}

static ToolContext* GlobalContext = nullptr;

ToolContext* CreateToolContext(LuaFunctionCollection API)
{
    
    if(GlobalContext == nullptr)
    {
        GlobalContext = (ToolContext*)TTE_ALLOC(sizeof(ToolContext), MEMORY_TAG_TOOL_CONTEXT); // alloc raw and construct, as its private.
        new (GlobalContext) ToolContext(std::move(API)); // construct after as some asserts require
    }
    else
    {
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
        TTE_DEL(GlobalContext);
    
    GlobalContext = nullptr;
    
}

LuaManager& ToolContext::GetGameLVM()
{
    TTE_ASSERT(GetActiveGame(), "No active game!");
    return _L[(U32)Meta::GetInternalState().GetActiveGame().LVersion];
}

static Bool _AsyncAttachRegistry(const JobThread& thread, void* A, void* B)
{
    ((ResourceRegistry*)A)->BindLuaManager(thread.L);
    return true;
}

Ptr<ResourceRegistry> ToolContext::CreateResourceRegistry(Bool bAttachAll)
{
    if(GetActiveGame() == nullptr)
    {
        TTE_LOG("Cannot create a resource registry as there is no current game set.");
        return nullptr;
    }
    
    Ptr<ResourceRegistry> registry = TTE_NEW_PTR(ResourceRegistry, MEMORY_TAG_RESOURCE_REGISTRY, GetGameLVM());
    
    Ptr<SnapshotDependentObject> asDependent = std::static_pointer_cast<SnapshotDependentObject>(registry);
    {
        std::lock_guard<std::mutex> G{_DependentsLock};
        _SwitchDependents.push_back(asDependent);
    }
    
    if(bAttachAll)
    {
        JobHandle Handles[NUM_SCHEDULER_THREADS];
        JobDescriptor desc{};
        desc.AsyncFunction = &_AsyncAttachRegistry;
        desc.UserArgA = registry.get();
        desc.Priority = JOB_PRIORITY_HIGHEST;
        for(I32 i = 0; i < NUM_SCHEDULER_THREADS; i++)
        {
            Handles[i] = JobScheduler::Instance->Post(desc, i);
        }
    }
    registry->BindLuaManager(GetLibraryLVM());
    
    return registry;
}

void ToolContext::Switch(GameSnapshot snapshot)
{
    if(snapshot.ID.empty() || !Meta::_Impl::_CheckPlatform(snapshot.Platform))
    {
        TTE_ASSERT(false, "The platform '%s' is not a valid platform name. See valid platforms", snapshot.Platform.c_str());
        return;
    }
    if(_Setup)
        Release();
    
    _Snapshot = snapshot;
    TTE_ASSERT(_Snapshot.ID.length() && _Snapshot.Platform.length(), "Game or platform not specified");
    
    Meta::InitGame();
    JobScheduler::Initialise(_PerStateCollection);
    
    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/Files_" + snapshot.ID + ".symmap");
    GetGameSymbols().SerialiseIn(symbols); // reload
    
    GetLibraryLVM().PushNil();
    ScriptManager::SetGlobal(GetLibraryLVM(), "__ResourceRegistry", true); // remove reg
    
    _Setup = true;
}

ToolContext::~ToolContext()
{
    Release();
    Meta::Shutdown();
    DataStreamManager::Shutdown();
    Memory::Shutdown();
    CommonClassInfo::Shutdown();
    // Lua shuts down automatically in dtor
}

ToolContext::ToolContext(LuaFunctionCollection PerState)
{
    Memory::Initialise();
    DataStreamManager::Initialise();
    Compression::Initialise();
    _L[0].Initialise(LuaVersion::LUA_5_2_3);
    _L[1].Initialise(LuaVersion::LUA_5_1_4);
    _L[2].Initialise(LuaVersion::LUA_5_0_2);
    Meta::Initialise();
    
    // higher level lua api for each thread, including this context
    _PerStateCollection = std::move(PerState);
    ScriptManager::RegisterCollection(GetLibraryLVM(), _PerStateCollection);
    
    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/RuntimeSymbols.symmap"); // Load runtime symbols
    GetRuntimeSymbols().SerialiseIn(symbols);
    
    PlaystationPKG::_RegisterKeys(this);
    
}

String ToolContext::LoadLibraryStringResource(String name)
{
    DataStreamRef stream = LoadLibraryResource(name);
    if(!stream)
        return ""; // Stream could not be opened
    
    String result{};
    U8* tmp = TTE_ALLOC(stream->GetSize() + 1, MEMORY_TAG_TEMPORARY); // temp buffer to read string from
    
    if(!stream->Read(tmp, stream->GetSize())){
        TTE_FREE(tmp); // free buffer
        return ""; // could not read all bytes
    }
    
    tmp[stream->GetSize()] = 0; // add null terminator
    result = (CString)tmp; // cast raw U8 bytes to c char string
    
    TTE_FREE(tmp); // Free buffer
    
    return result;
}

void ToolContext::Release()
{
    if(_Setup)
    {
        
        U32 errors{};
        for(auto& dependent: _SwitchDependents)
        {
            Ptr<SnapshotDependentObject> alive = dependent.lock();
            if(alive)
            {
                errors++;
                TTE_LOG("FATAL: Game dependent object is still alive at a game switch: %s", alive->ObjName);
            }
        }
        if(errors)
            TTE_ASSERT(false, "Found %d alive game dependent objects before a game switch.", errors);
        _SwitchDependents.clear();
        
        JobScheduler::Shutdown();
        Blowfish::Shutdown();
        GetGameSymbols().Clear();
        
        _L[0].GC();
        _L[1].GC();
        _L[2].GC();

        Meta::RelGame();

        _Setup = false;
        
    }
}

// WEAK SLOTS

void ToolContext::_AssertWeakSlot(U64 _Stat)
{
    TTE_ASSERT(_Stat == 0 || ((_Stat & WEAK_SIGNAL_SANITY_NSTATE_MASK) == WEAK_SIGNAL_SANITY_NSTATE_MASK),
               "A weak slot signal has been corrupted! Code somewhere has modified this signal object before "
               "it's slot master and weak references expired. Was the signal object destroyed?");
}

void ToolContext::CreateMasterWeakSlot(WeakSlotMaster &master, WeakSlotSignal *signal)
{
    std::scoped_lock<std::mutex> _Lk{_WkLock};
    if(master._WeakSlot)
    {
        TTE_LOG("Cannot create weak slot as the master slot is not empty");
        TTE_ASSERT(false, "");
    }
    else
    {
        // find slot
        constexpr U32 Ngroups = sizeof(_WkPtrSlots) / sizeof(_WkPtrSlots[0]);
        U32 availGroup = 0;
        while((_WkPtrSlots[availGroup] & 0xF) == 0xF && availGroup < Ngroups)
            availGroup++;
        if(availGroup >= Ngroups)
        {
            // really? over
            TTE_ASSERT(false, "Cannot create master weak slot: ran out of slot entries! Increase the buffer size or restart!"
                       " Only have %d possible master weak slots in this release.", Ngroups << 2);
            return;
        }
        U32 subgroup = 0;
        if(_WkPtrSlots[availGroup] & 1) // simple 4 bit loop unwound FLS/FFS
        {
            if(_WkPtrSlots[availGroup] & 2)
            {
                subgroup = (2 + ((_WkPtrSlots[availGroup] & 4) >> 2));
            }
            else
            {
                subgroup = 1;
            }
        }
        _WkPtrSlots[availGroup] |= (1u << subgroup); // slot acquire
        _WkPtrSlots[availGroup] |= (0x10u << subgroup); // master ref
        master._WeakSlot = 0xFFFF'FFFFu - ((availGroup << 2) + subgroup); // do inverse as slot 0 is for invalid
        master._Signal = signal;
        master._Context = this;
        if(signal)
            signal->_Stat = WEAK_SIGNAL_SANITY; // active signal
    }
}

void ToolContext::_ReleaseRequiredSlotUnlocked(U32 grp, U32 sgrp, U32 nwkrefs)
{
    if(nwkrefs == 0 && (_WkPtrSlots[grp] & (0x10u << sgrp)) == 0)
    {
        _WkPtrSlots[grp] -= (1u << sgrp);
    }
}

void ToolContext::_DetachWeakSlotMaster(U32 slot, WeakSlotSignal *sig)
{
    constexpr U32 Nslots = (sizeof(_WkPtrSlots) / sizeof(_WkPtrSlots[0])) << 2;
    slot = 0xFFFF'FFFF - slot;
    U32 group = slot >> 2;
    U32 subgroup = slot & 3;
    TTE_ASSERT(slot < Nslots, "Corrupt weak slot");
    
    std::scoped_lock<std::mutex> _Lk{_WkLock};

    // ensure the master weak slot actually is acquired (1) and is alive (2)
    TTE_ASSERT(_WkPtrSlots[group] & (1u << subgroup), "Corrupt weak slot referenced (1)");
    TTE_ASSERT(_WkPtrSlots[group] & (0x10u << subgroup), "Corrupt weak slot referenced (2)");
    
    _WkPtrSlots[group] &= ~((U64)0x10 << subgroup);
    if(sig)
        sig->_Stat |= WEAK_SIGNAL_SANITY_SSTATE_MASK; // signal on
    _ReleaseRequiredSlotUnlocked(group, subgroup, (_WkPtrSlots[group] >> (14 * subgroup)) & 0x3FFF);
}

void ToolContext::_DetachWeakSlotRef(U32 slot)
{
    constexpr U32 Nslots = (sizeof(_WkPtrSlots) / sizeof(_WkPtrSlots[0])) << 2;
    slot = 0xFFFF'FFFF - slot;
    U32 group = slot >> 2;
    U32 subgroup = slot & 3;
    TTE_ASSERT(slot < Nslots, "Corrupt weak slot");
    
    std::scoped_lock<std::mutex> _Lk{_WkLock};

    TTE_ASSERT(_WkPtrSlots[group] & (1u << subgroup), "Corrupt weak slot referenced");
    
    U32 nref = (_WkPtrSlots[group] >> (14 * subgroup)) & 0x3FFF;
    if(nref > 0)
    {
        TTE_ASSERT(false, "Cannot detach weak slot reference: no weak reported references");
        return;
    }
    _WkPtrSlots[group] &= ~((U64)0x3FFF00 << (14 * subgroup));
    _WkPtrSlots[group] |= ((U64)(nref - 1) << (14 * subgroup));
    _ReleaseRequiredSlotUnlocked(group, subgroup, nref - 1);
}

void ToolContext::_IncrWeakRef(U32 slot)
{
    constexpr U32 Nslots = (sizeof(_WkPtrSlots) / sizeof(_WkPtrSlots[0])) << 2;
    slot = 0xFFFF'FFFF - slot;
    U32 group = slot >> 2;
    U32 subgroup = slot & 3;
    TTE_ASSERT(slot < Nslots, "Corrupt weak slot");
    
    std::scoped_lock<std::mutex> _Lk{_WkLock};
    
    TTE_ASSERT(_WkPtrSlots[group] & (1u << subgroup), "Corrupt weak slot referenced");
    
    U32 nref = (_WkPtrSlots[group] >> (14 * subgroup)) & 0x3FFF;
    if(nref >= 0x3FFF)
    {
        TTE_ASSERT(false, "Cannot create weak slot reference: reached maximum weak references"); // really?? 16k+ refs?
        return;
    }
    _WkPtrSlots[group] &= ~((U64)0x3FFF00 << (14 * subgroup));
    _WkPtrSlots[group] |= ((U64)(nref + 1) << (14 * subgroup));
}

WeakSlotRef ToolContext::CreateWeakSlotReference(const WeakSlotMaster &master)
{
    if(master._WeakSlot)
    {
        _IncrWeakRef(master._WeakSlot);
        return WeakSlotRef(master._WeakSlot, this);
    }
    else
    {
        return {}; // empty master
    }
}

WeakSlotSignal::~WeakSlotSignal()
{
    ToolContext::_AssertWeakSlot(_Stat);
    _Stat = 0;
}

WeakSlotMaster::~WeakSlotMaster()
{
    if(_WeakSlot)
        _Context->_DetachWeakSlotMaster(_WeakSlot, _Signal);
}

WeakSlotRef::~WeakSlotRef()
{
    if(_WeakSlot)
        _Context->_DetachWeakSlotRef(_WeakSlot);
}

WeakSlotMaster& WeakSlotMaster::operator=(const WeakSlotMaster&)
{
    if(_WeakSlot && _Context)
        _Context->_DetachWeakSlotRef(_WeakSlot);
    _WeakSlot = 0;
    return *this;
}

WeakSlotRef& WeakSlotRef::operator=(const WeakSlotRef& rhs)
{
    _Context = rhs._Context;
    if(rhs._WeakSlot)
    {
        _WeakSlot = rhs._WeakSlot;
        _Context->_IncrWeakRef(_WeakSlot);
    }
    return *this;
}
