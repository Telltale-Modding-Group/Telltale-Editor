#include <TelltaleEditor.hpp>
#include <Renderer/RenderContext.hpp>

static TelltaleEditor* _MyContext = nullptr;

TelltaleEditor* CreateEditorContext(GameSnapshot s, Bool ui)
{
    TTE_ASSERT(_MyContext == nullptr, "A context already exists");
    _MyContext = TTE_NEW(TelltaleEditor, MEMORY_TAG_TOOL_CONTEXT, s, ui);
    return _MyContext;
}

void FreeEditorContext()
{
    if(_MyContext)
        TTE_DEL(_MyContext);
    _MyContext = nullptr;
}

TelltaleEditor::TelltaleEditor(GameSnapshot s, Bool ui)
{
    _UI = ui;
    _Running = ui;
    
    LuaFunctionCollection commonAPI = CreateScriptAPI();
    _ModdingContext = CreateToolContext(std::move(commonAPI));
    
    _ModdingContext->Switch(s); // create context loads the symbols. in the editor lets resave them in close
    
    RenderContext::Initialise();
}

TelltaleEditor::~TelltaleEditor()
{
    TTE_ASSERT(!_Running, "Update was not called properly in Telltale Editor: still running");
    
    Wait(); // wait for all tasks to finish
    
    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/RuntimeSymbols.symmap"); // Save out runtime symbols
    RuntimeSymbols.SerialiseOut(symbols);
    
    RenderContext::Shutdown();
    
    DestroyToolContext();
    _ModdingContext = nullptr;
}

Bool TelltaleEditor::Update(Bool forceQuit)
{
    _ProbeTasks(false);
    // TODO editor UI
    return true;
}

Bool TelltaleEditor::_ProbeTasks(Bool wait, U32 t)
{
    Bool result = true;
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    for(auto it = _Active.begin(); it != _Active.end();)
    {
        if(wait) // wait for it then finalise it then remove it, we know it will finish
        {
            JobScheduler::Instance->Wait(it->second);
            it->first->Finalise(*this); // finalise on this mean
            TTE_DEL(it->first);
            _Active.erase(it);
            continue;
        }
        JobResult result0 = JobScheduler::Instance->GetResult(it->second);
        if(result0 == JobResult::JOB_RESULT_RUNNING) // job still running, leave it.
        {
            if(it->first->TaskID == t)
                result = false;
            it++;
            continue;
        }
        // job is finished so remove it and finalise it
        it->first->Finalise(*this); // finalise on this mean
        TTE_DEL(it->first);
        _Active.erase(it);
    }
    return result;
}

Bool TelltaleEditor::ContextIsBusy()
{
    _ProbeTasks(false);
    for(auto& active: _Active)
    {
        if(active.first->IsBlocking)
            return true;
    }
    return false;
}

void TelltaleEditor::Wait()
{
    _ProbeTasks(true);
}

Bool TelltaleEditor::QueryTask(U32 task)
{
    return _ProbeTasks(false, task);
}

Ptr<ResourceRegistry> TelltaleEditor::CreateResourceRegistry()
{
    TTE_ASSERT(IsCallingFromMain(), "Can only be called from the main thread! Resource registies can only be created and mounted with resource sets on the main thread.");
    return _ModdingContext->CreateResourceRegistry();
}

U32 TelltaleEditor::EnqueueArchive2ExtractTask(TTArchive2* pArchive, std::set<String>&& files, String outputFolder)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    ArchiveExtractionTask* task = TTE_NEW(ArchiveExtractionTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->Files = std::move(files);
    task->Folder = std::move(outputFolder);
    task->Archive2 = pArchive;
    task->Archive1 = nullptr;
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueResourceLocationExtractTask(Ptr<ResourceRegistry> registry, const String& logical, String outputFolder, StringMask mask, Bool f)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    ResourcesExtractionTask* task = TTE_NEW(ResourcesExtractionTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->Folder = std::move(outputFolder);
    task->Logical = logical;
    task->Folders = f;
    task->Mask = mask;
    if(mask.length())
        task->UseMask = true;
    task->Registry = std::move(registry);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueArchiveExtractTask(TTArchive* pArchive, std::set<String>&& files, String outputFolder)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    ArchiveExtractionTask* task = TTE_NEW(ArchiveExtractionTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->Files = std::move(files);
    task->Folder = std::move(outputFolder);
    task->Archive1 = pArchive;
    task->Archive2 = nullptr;
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseTextureTask(Meta::ClassInstance instance, Ptr<RenderTexture> outputTexture)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    TextureNormalisationTask* task = TTE_NEW(TextureNormalisationTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    *const_cast<Ptr<RenderTexture>*>(&task->Output) = std::move(outputTexture);
    task->Instance = std::move(instance);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseMeshTask(Scene *pScene, Symbol agent, Meta::ClassInstance instance)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    MeshNormalisationTask* task = TTE_NEW(MeshNormalisationTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->Agent = agent;
    task->Output = pScene;
    task->Instance = std::move(instance);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseSceneTask(Scene *pScene, Meta::ClassInstance sceneInstance)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    SceneNormalisationTask* task = TTE_NEW(SceneNormalisationTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->OutputUnsafe = pScene;
    task->Instance = std::move(sceneInstance);
    _EnqueueTask(task);
    return _TaskFence++;
}

void TelltaleEditor::_EnqueueTask(EditorTask* pTask)
{
    _ProbeTasks(false);
    JobDescriptor desc{};
    desc.AsyncFunction = &AsyncTTETaskDelegate;
    desc.UserArgA = pTask;
    desc.UserArgB = pTask->IsBlocking ? _ModdingContext : nullptr;
    desc.Priority = JobPriority::JOB_PRIORITY_NORMAL;
    if(_Active.size() != 0)
    {
        for(auto it = _Active.rbegin(); it != _Active.rend(); it++)
        {
            if(!it->first->IsBlocking)
                continue; // ignore non blocking, they don't need an order
            JobScheduler::Instance->EnqueueOne(it->second, std::move(desc)); // found a blocking job, enqueue it after
            return;
        }
    }
    JobHandle handle = JobScheduler::Instance->Post(std::move(desc));
    _Active.push_back(std::make_pair(pTask, std::move(handle)));
}
