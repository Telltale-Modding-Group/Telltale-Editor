#include <Core/Config.hpp>

#include <map>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef DEBUG

struct TrackedAllocation
{
    String SrcFile; // src file name
    CString ObjName;
    U64 Timestamp;
    U64 Size; // allocation size
    U32 SrcLine; // line in src file
    U32 MemoryTag; // MEMORY_TAG enum
};

static std::map<U8*, TrackedAllocation> _TrackedAllocs{}; // ptr => info
static std::mutex _TrackedLock{};

U8* _DebugAllocateTracked(U64 _Nbytes, MemoryTag tag, CString filename, U32 number, CString objName)
{
    
    U8* Alloc = new U8[_Nbytes];
    if(!Alloc)
        return nullptr;
    
    TrackedAllocation alloc{};
    alloc.Timestamp = (U64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    alloc.MemoryTag = (U32)tag;
    alloc.SrcLine = number;
    alloc.Size = _Nbytes;
    alloc.ObjName = objName;
    
    // ensure only file name not full path
    String str = filename;
    size_t slash = str.find_last_of("/\\");
    if(slash != String::npos)
        str = str.substr(slash + 1);
    alloc.SrcFile = std::move(str);

    {
        std::lock_guard _Lck{_TrackedLock};
        _TrackedAllocs[Alloc] = std::move(alloc);
    }
    
    return Alloc;
}

void _DebugDeallocateTracked(U8* Ptr)
{
    if(Ptr)
    {
        delete[] Ptr;
        {
            std::lock_guard _Lck{_TrackedLock};
            _TrackedAllocs.erase(Ptr);
        }
    }
}

#endif

static CString TAG_NAMES[]
{
    "Scheduler",
    "Scripting",
    "DataStream",
    "Temporary",
    "Context",
    "Meta::ClassInstance",
    "Meta::CollectionData",
    "RuntimeBuffer",
    "Blowfish",
    "ScriptObj",
    "TemporaryAsync",
};

void DumpTrackedMemory()
{
    std::map<U8*, TrackedAllocation> CopyOfAllocs{};
    {
        std::lock_guard _Lck{_TrackedLock};
        CopyOfAllocs = _TrackedAllocs; // copy it locally in case it gets called in logger.
    }
    
    TTE_LOG("================ UNFREED MEMORY ALLOCATION DUMP ================");
    
    for(auto& it: CopyOfAllocs)
    {
        auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(it.second.Timestamp));
        std::time_t time = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss{};
        ss << std::put_time(std::localtime(&time), "%b %d - %H:%M:%S ");
        ss << it.second.SrcFile << ":" << it.second.SrcLine << "[" << TAG_NAMES[it.second.MemoryTag] << "] ";
        if(it.second.ObjName)
        {
            ss << "CXX Object: " << it.second.ObjName;
        }
        else
        {
            ss << it.second.Size << " bytes => [ ";
            U64 bytes = MIN(8, it.second.Size);
            for(U64 i = 0; i < bytes; i++)
            {
                U8 byte = it.first[i];
                if (std::isprint(byte))
                    ss << static_cast<char>(byte);
                else
                    ss << "\\x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
                ss << " ";
            }
            ss << "]";
        }
        String str = ss.str();
        TTE_LOG(str.c_str());
    }
    
    TTE_LOG("================================================================");
}
