#include <TelltaleEditor.hpp>

namespace sfs = std::filesystem;

Bool AsyncTTETaskDelegate(const JobThread& thread, void* argA, void* argB)
{
    EditorTask* pTask = (EditorTask*)argA;
    return pTask->PerformAsync(thread, (ToolContext*)argB);
}

// SCENE NORMALISATION

Bool SceneNormalisationTask::PerformAsync(const JobThread& thread, ToolContext* pLockedContext)
{
    
    // ... (push working scene ptr)
    
    Working.FinaliseNormalisationAsync();
    return true;
}

void SceneNormalisationTask::Finalise(TelltaleEditor& editor)
{
    *OutputUnsafe = std::move(Working);
}

// MESH NORMALISATION

Bool MeshNormalisationTask::PerformAsync(const JobThread& thread, ToolContext* pLockedContext)
{
    String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.NormaliserStringFn;
    auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
    TTE_ASSERT(normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found for mesh instance: '%s'", fn.c_str());
    
    TTE_ASSERT(thread.L.LoadChunk(fn, normaliser->second.Binary, normaliser->second.Size, LoadChunkMode::BINARY), "Could not load chunk for %s", fn.c_str());
    
    Instance.PushWeakScriptRef(thread.L, Instance.ObtainParentRef());
    thread.L.PushOpaque(&Renderable);
    
    thread.L.CallFunction(2, 1, false);
    
    Bool result;
    
    if(!(result=ScriptManager::PopBool(thread.L)))
    {
        TTE_LOG("Normalise failed for mesh in %s", fn.c_str());
    }
    else
    {
        Renderable.FinaliseNormalisationAsync();
    }
    
    return result;
}

void MeshNormalisationTask::Finalise(TelltaleEditor& editor)
{
    TTE_ASSERT(Output->ExistsAgent(Agent), "Agent does not exist anymore for output mesh normalisation");
    
    // move processed new mesh renderable instance to array, ready to be used
    Ptr<Mesh::MeshInstance> localHandle = TTE_NEW_PTR(Mesh::MeshInstance, MEMORY_TAG_COMMON_INSTANCE, std::move(Renderable));
    Output->GetAgentModule<SceneModuleType::RENDERABLE>(Agent).Renderable.MeshList.push_back(std::move(localHandle));
}

// TEXTURE NORMALISATION

Bool TextureNormalisationTask::PerformAsync(const JobThread& thread, ToolContext* pLockedContext)
{
    String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.NormaliserStringFn;
    auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
    TTE_ASSERT(normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found for texture instance: '%s'", fn.c_str());
    
    TTE_ASSERT(thread.L.LoadChunk(fn, normaliser->second.Binary, normaliser->second.Size, LoadChunkMode::BINARY), "Could not load chunk for %s", fn.c_str());
    
    Instance.PushWeakScriptRef(thread.L, Instance.ObtainParentRef());
    thread.L.PushOpaque(&Local);
    
    thread.L.CallFunction(2, 1, false);
    
    Bool result;
    
    if(!(result=ScriptManager::PopBool(thread.L)))
    {
        TTE_LOG("Normalise failed for texture in %s", fn.c_str());
    }
    else
    {
        Output->FinaliseNormalisationAsync();
    }
    
    return result;
}

void TextureNormalisationTask::Finalise(TelltaleEditor& editor)
{
    RenderTexture* pTexture = const_cast<RenderTexture*>(Output.get());
    RenderContext* pContext = pTexture->_Context;
    SDL_GPUTexture* pTextureH = pTexture->_Handle;
    U32 flags = pTexture->_TextureFlags;
    *pTexture = std::move(Local);
    pTexture->_Context = pContext;
    pTexture->_Handle = pTextureH;
    pTexture->_TextureFlags += flags; // concat flags
}

// RESOURCE SYSTEM EXTRACTION

static Bool _DoResourcesExtract(Ptr<ResourceRegistry>& reg, const String& outputFolder, std::set<String>::const_iterator begin, std::set<String>::const_iterator end, Bool bFolders)
{
    for(auto it = begin; it != end; it++)
    {
        String outFolder = outputFolder;
        
        if(bFolders)
        {
            TTE_ASSERT(Meta::GetInternalState().GameIndex != -1, "No active game present!");
            for(auto& mapping: Meta::GetInternalState().GetActiveGame().FolderAssociates)
            {
                StringMask& coerced = *((StringMask*)&mapping.first);
                if(coerced == *it)
                {
                    outFolder += mapping.second;
                    break;
                }
            }
        }
        
        String fileName = outFolder + *it;
        DataStreamRef src = reg->FindResource(*it);
        
        if(src)
        {
            // ensure directory exists (may have concat paths)
            sfs::path p = fileName;
            p = p.parent_path();
            if(!sfs::exists(p))
                sfs::create_directories(p);
            
            src->SetPosition(0);
            DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(fileName);
            DataStreamManager::GetInstance()->Transfer(src, out, src->GetSize());
        } // quietly ignore for now
    }
    return true;
}

#define RES_EXTRACT_BLK 512

static Bool _DoResourcesExtractAsync(const JobThread& thread, void* pRawTask, void* pRawIndex)
{
    U32 index = (U32)((U64)pRawIndex);
    ResourcesExtractionTask* task = (ResourcesExtractionTask*)pRawTask;
    auto it = task->WorkingFiles->cbegin();
    std::advance(it, index * ((U32)task->WorkingFiles->size() / task->AsyncWorkers));
    auto itend = it;
    std::advance(itend, ((U32)task->WorkingFiles->size() / task->AsyncWorkers));
    return _DoResourcesExtract(task->Registry, task->Folder, it, itend, task->Folders);
}

Bool ResourcesExtractionTask::PerformAsync(const JobThread &thread, ToolContext *pLockedContext)
{
    if(!StringEndsWith(Folder, "/") && !StringEndsWith(Folder, "\\"))
        Folder += "/";
    std::set<String> files{};
    
    {
        std::lock_guard<std::mutex> G{Registry->_Guard};
        Ptr<ResourceLocation> loc = Registry->_Locate(Logical);
        TTE_ASSERT(loc != nullptr, "The location %s could not be found!", Logical.c_str());
        loc->GetResourceNames(files, UseMask ? &Mask : nullptr);
    }
    
    WorkingFiles = &files;
    AsyncWorkers = files.size() <= RES_EXTRACT_BLK ? 1 : (U32)files.size() / RES_EXTRACT_BLK;
    JobHandle H[7]{};
    if(AsyncWorkers > 8)
        AsyncWorkers = 8;
    
    for(U32 worker = 0; worker < AsyncWorkers - 1; worker++)
    {
        JobDescriptor J{};
        J.Priority = JobPriority::JOB_PRIORITY_NORMAL;
        J.UserArgA = this;
        J.UserArgB = reinterpret_cast<void*>(static_cast<uintptr_t>(worker));;
        J.AsyncFunction = &_DoResourcesExtractAsync;
        H[worker] = JobScheduler::Instance->Post(J);
    }
    
    auto it = files.cbegin();
    std::advance(it, (AsyncWorkers - 1) * ((U32)files.size() / AsyncWorkers));
    Bool bResult = _DoResourcesExtract(Registry, Folder, it, files.cend(), Folders);
    
    return bResult && JobScheduler::Instance->Wait(AsyncWorkers - 1, H);
}

void ResourcesExtractionTask::Finalise(TelltaleEditor& editorContext)
{
    // No finalisation needed
}


// ARCHIVE EXTRACTION

Bool ArchiveExtractionTask::PerformAsync(const JobThread &thread, ToolContext *pLockedContext)
{
    if(!StringEndsWith(Folder, "/") && !StringEndsWith(Folder, "\\"))
        Folder += "/";
    if(Archive1 != nullptr)
    {
        // EXTRACT .TTARCH
        if(Files.size() == 0)
            Archive1->GetFiles(Files);
        
        for(auto& fileName: Files)
        {
            DataStreamRef file = Archive1->Find(fileName, nullptr);
            file = Meta::MapDecryptingStream(file); // ensure its not encrypted.
            DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, Folder + fileName));
            if(out && file)
            {
                DataStreamManager::GetInstance()->Transfer(file, out, file->GetSize());
            }
            else
            {
                TTE_LOG("Could not extract %s: file streams were not valid", fileName.c_str());
            }
        }
    }
    else
    {
        TTE_ASSERT(Archive2, "No archive set in extraction task!");
        // EXTRACT .TTARCH2
        if(Files.size() == 0)
            Archive2->GetFiles(Files);
        
        for(auto& fileName: Files)
        {
            DataStreamRef file = Archive2->Find(fileName, nullptr);
            DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, Folder + fileName));
            if(out && file)
            {
                DataStreamManager::GetInstance()->Transfer(file, out, file->GetSize());
            }
            else
            {
                TTE_LOG("Could not extract %s: file streams were not valid", fileName.c_str());
            }
        }
    }
    Files.clear(); // not needed
    return true;
}

void ArchiveExtractionTask::Finalise(TelltaleEditor & editorContext)
{
    // No finalisation needed
}
