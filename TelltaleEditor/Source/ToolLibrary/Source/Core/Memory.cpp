#include <Core/Context.hpp>

#include <map>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace Memory
{
    
    static U32 _FastBufferOffset = 0, _FastBufferSize = 0;
    static U8* _FastBuffer = nullptr; // MAIN THREAD BUFFER
    
    void Initialise()
    {
        if(!_FastBuffer)
        {
            _FastBuffer = TTE_ALLOC(FAST_BUFFER_SIZE, MEMORY_TAG_RUNTIME_BUFFER);
            _FastBufferOffset = 0;
            _FastBufferSize = FAST_BUFFER_SIZE;
        }
    }
    
    void Shutdown()
    {
        if(_FastBuffer)
        {
            TTE_FREE(_FastBuffer);
            _FastBuffer = nullptr;
            _FastBufferSize = _FastBufferOffset = 0;
        }
    }
    
    Bool FastBufferAllocator::IsInFastBuffer(const void* p)
    {
        const U8* base = _Worker ? _Worker->FastBuffer : _FastBuffer;
        U32 size = _Worker ? _Worker->FastBufferSize : _FastBufferSize;
        
        return (p >= base) && (p < (base + size));
    }
    
    FastBufferAllocator::FastBufferAllocator()
    {
        _IsMainThread = IsCallingFromMain();
        if(_IsMainThread)
        {
            _InitialMarker = _FastBufferOffset;
        }
        else
        {
            TTE_ASSERT(JobScheduler::IsRunningFromWorker(), "This can only be called from a worker thread or main context thread!");
            _Worker = &JobScheduler::GetCurrentThread();
            _InitialMarker = _Worker->FastBufferOffset;
        }
    }
    
    U8* FastBufferAllocator::Alloc(U64 size, U32 align)
    {
        U8* buf = _Worker ? _Worker->FastBuffer : _FastBuffer;
        U32 avail = _Worker ? _Worker->FastBufferSize : _FastBufferSize;
        U32& marker = _Worker ? _Worker->FastBufferOffset : _FastBufferOffset;
        U32 alignedOffset = align ? (marker + (align - 1)) & ~(align - 1) : 0;
        
        if (alignedOffset + size > avail)
        {
            TTE_ASSERT(false, "Increase fast buffer size! Not enough space to fit 0x%llX bytes into 0x%X buffer"
                       , (U64)alignedOffset + size, FAST_BUFFER_SIZE);
            return nullptr; // Not enough space
        }
        
        U8* outPtr = buf + alignedOffset;
        memset(outPtr, 0, size);
        marker = alignedOffset + static_cast<U32>(size);
        
        return outPtr;
    }
    
    void FastBufferAllocator::Reset()
    {
        U32& marker = _Worker ? _Worker->FastBufferOffset : _FastBufferOffset;
        marker = _InitialMarker;
    }
    
    FastBufferAllocator::~FastBufferAllocator()
    {
        Reset();
    }
    
#ifdef DEBUG
    
    static std::map<U8*, TrackedAllocation> _TrackedAllocs{}; // ptr => info
    static std::mutex _TrackedLock{};

    void AttachDebugString(void* ptr, const String& data)
    {
        {
            std::lock_guard _Lck{_TrackedLock};
            auto it = _TrackedAllocs.find((U8*)ptr);
            if(it != _TrackedAllocs.end())
            {
                it->second.DebugStr = data;
            }
        }
    }

    void ViewTrackedMemory(std::vector<TrackedAllocation>& allocs)
    {
        std::lock_guard _Lck{_TrackedLock};
        allocs.clear();
        allocs.reserve(_TrackedAllocs.size());
        for(const auto& tracked: _TrackedAllocs)
        {
            allocs.push_back(tracked.second);
        }
    }

    U8* _DebugAllocateTracked(U64 _Nbytes, U32 _tag, CString filename, U32 number, CString objName)
    {
        MemoryTag tag = (MemoryTag)_tag;
        U8* Alloc = new U8[_Nbytes];
        if(!Alloc)
            return nullptr;
        
        TrackedAllocation alloc{};
        alloc.Timestamp = (U64)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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
        "Renderer",
        "LinearHeap",
        "TransienceFence",
        "ResourceRegistry",
        "CommonInstance",
        "RenderLayer",
        "AnimationData",
        "ObjOwner::ObjData<T>",
        "MethodImpl",
        "SceneData",
        "EditorUI"
    };

    CString GetMemoryTagString(U32 tag)
    {
        return tag >= sizeof(TAG_NAMES) / sizeof(TAG_NAMES[0]) ? nullptr : TAG_NAMES[tag];
    }
    
    void DumpTrackedMemory()
    {
        std::map<U8*, TrackedAllocation> CopyOfAllocs{};
        {
            std::lock_guard _Lck{_TrackedLock};
            CopyOfAllocs = _TrackedAllocs; // copy it locally in case it gets called in logger.
        }
        
        if(CopyOfAllocs.size() == 0)
            return;
        
        TTE_LOG("================ UNFREED MEMORY ALLOCATION DUMP ================");
        
        for(auto& it: CopyOfAllocs)
        {
            auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(it.second.Timestamp));
            std::time_t time = std::chrono::system_clock::to_time_t(tp);
            std::stringstream ss{};
            ss << std::put_time(std::localtime(&time), "%b %d - %H:%M:%S ");
            ss << it.second.SrcFile << ":" << it.second.SrcLine << "[" << TAG_NAMES[it.second.MemoryTag] << "] {0x" << std::hex << std::uppercase << (std::uintptr_t)it.first << std::dec << "} ";
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
            if(!it.second.DebugStr.empty())
            {
                ss << ", '" << it.second.DebugStr << "'";
            }
            String str = ss.str();
            TTE_LOG(str.c_str());
        }
        
        TTE_LOG("================================================================");
    }
    
}
