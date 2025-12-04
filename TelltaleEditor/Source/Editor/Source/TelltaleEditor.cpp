#include <TelltaleEditor.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/Symbol.hpp>

#include <sstream>

void luaCompleteGameEngine(LuaFunctionCollection& Col); // Full game engine (Telltale). See LuaGameEngine.cpp

extern Float kDefaultContribution[256];
TelltaleEditor* _MyContext = nullptr;

TelltaleEditor* CreateEditorContext(GameSnapshot s)
{
    TTE_ASSERT(_MyContext == nullptr, "A context already exists");
    TTE_NEW(TelltaleEditor, MEMORY_TAG_TOOL_CONTEXT, s); // constructor stores into mycontext
    for (U32 i = 0; i < 256; i++)
        kDefaultContribution[i] = 1.0f;
    return _MyContext;
}

TelltaleEditor* TelltaleEditor::Get()
{
    return _MyContext;
}

void FreeEditorContext()
{
    if (_MyContext)
        TTE_DEL(_MyContext);
    _MyContext = nullptr;
}

TelltaleEditor::TelltaleEditor(GameSnapshot s)
{
    _MyContext = this;
    RegisterCommonClassInfo();

    LuaFunctionCollection commonAPI = CreateScriptAPI(); // Common class API
    luaCompleteGameEngine(commonAPI); // Telltale full API

    for(const auto& prop: GetPropKeyConstants())
    {
        PUSH_GLOBAL_S(commonAPI, prop.first, prop.second, "Telltale Property Keys");
    }

    _ModdingContext = CreateToolContext(std::move(commonAPI));

    if(s.ID.length() > 0)
    {

        Switch(s);

    }

    RenderContext::Initialise();
    RenderStateBlob::Initialise();
}

void TelltaleEditor::_PostSwitch(GameSnapshot snap)
{
    PlatformInputMapper::Shutdown();
    PlatformInputMapper::Initialise(snap.Platform);

    _ModuleVisualProperties.clear();

    String func = snap.ID + "_RegisterModuleUI";
    ScriptManager::GetGlobal(_ModdingContext->GetLibraryLVM(), func, true);
    if (_ModdingContext->GetLibraryLVM().Type(-1) != LuaType::FUNCTION)
    {
        _ModdingContext->GetLibraryLVM().Pop(1);
        TTE_LOG("WARNING: Snapshot for game %s does not declare function %s required for module UI registration! Modules won't be editable in the inspector view.", snap.ID.c_str(), func.c_str());
    }
    else
    {
        ScriptManager::Execute(GetToolContext()->GetLibraryLVM(), 0, 0, true);
    }
}

void TelltaleEditor::Switch(GameSnapshot s)
{
    _ModdingContext->Switch(s);
    _PostSwitch(s);
}

TelltaleEditor::~TelltaleEditor()
{

    Wait(); // wait for all tasks to finish

    DataStreamRef symbols = LoadLibraryResource("SymbolMaps/RuntimeSymbols.symmap"); // Save out runtime symbols
    GetRuntimeSymbols().SerialiseOut(symbols);

    RenderContext::Shutdown();
    _ModuleVisualProperties.clear();

    DestroyToolContext();
    _ModdingContext = nullptr;
    PlatformInputMapper::Shutdown();
    CommonClassInfo::Shutdown(); // ensure
}

void TelltaleEditor::Update()
{
    _ProbeTasks(false);
}

Bool TelltaleEditor::_ProbeTasks(Bool wait, U32 t)
{
    Bool result = true;
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    for (auto it = _Active.begin(); it != _Active.end();)
    {
        if (wait) // wait for it then finalise it then remove it, we know it will finish
        {
            JobScheduler::Instance->Wait(it->second);
            it->first->Finalise(*this); // finalise on this mean
            TTE_DEL(it->first);
            it = _Active.erase(it);
            continue;
        }
        JobResult result0 = JobScheduler::Instance->GetResult(it->second);
        if (result0 == JobResult::JOB_RESULT_RUNNING) // job still running, leave it.
        {
            if (it->first->TaskID == t)
                result = false;
            it++;
            continue;
        }
        // job is finished so remove it and finalise it
        it->first->Finalise(*this); // finalise on this mean
        TTE_DEL(it->first);
        it = _Active.erase(it);
    }
    return result;
}

Bool TelltaleEditor::TestCapability(GameCapability cap)
{
    return Meta::GetInternalState().GetActiveGame().Caps[cap];
}

Bool TelltaleEditor::ContextIsBusy()
{
    _ProbeTasks(false);
    for (auto& active : _Active)
    {
        if (active.first->IsBlocking)
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

Ptr<ResourceRegistry> TelltaleEditor::CreateResourceRegistry(Bool batt)
{
    TTE_ASSERT(IsCallingFromMain(), "Can only be called from the main thread! Resource registies can only be created and mounted with resource sets on the main thread.");
    return _ModdingContext->CreateResourceRegistry(batt);
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

static void _DefaultResourceCallback(const String&)
{
}

U32 TelltaleEditor::EnqueueResourceLocationExtractTask(Ptr<ResourceRegistry> registry,
                                                        const String& logical, String outputFolder,
                                                        StringMask mask, Bool f, ResourceExtractCallback* pCb)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    ResourcesExtractionTask* task = TTE_NEW(ResourcesExtractionTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence);
    task->Folder = std::move(outputFolder);
    task->Logical = logical;
    task->Folders = f;
    task->Callback = pCb ? pCb : &_DefaultResourceCallback;
    task->Mask = mask;
    if (mask.length())
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

U32 TelltaleEditor::EnqueueNormaliseTextureTask(Ptr<ResourceRegistry> registry, Meta::ClassInstance instance, Ptr<RenderTexture> outputTexture)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    TextureNormalisationTask* task = TTE_NEW(TextureNormalisationTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence, registry);
    *const_cast<Ptr<RenderTexture>*>(&task->Output) = std::move(outputTexture);
    task->Instance = std::move(instance);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseMeshTask(Ptr<ResourceRegistry> registry, Scene* pScene, Symbol agent, Meta::ClassInstance instance)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    MeshNormalisationTask* task = TTE_NEW(MeshNormalisationTask, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence, registry);
    task->Agent = agent;
    task->Output = pScene;
    task->Instance = std::move(instance);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseInputMapperTask(Ptr<ResourceRegistry> registry, Ptr<InputMapper> imap, Meta::ClassInstance instance)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    CommonNormalisationTask<InputMapper>* task = TTE_NEW(CommonNormalisationTask<InputMapper>, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence, registry);
    task->Output = imap;
    task->Instance = std::move(instance);
    _EnqueueTask(task);
    return _TaskFence++;
}

U32 TelltaleEditor::EnqueueNormaliseSceneTask(Ptr<ResourceRegistry> registry, Ptr<Scene> pScene, Meta::ClassInstance sceneInstance)
{
    TTE_ASSERT(IsCallingFromMain(), "Only can be called from the main thread");
    CommonNormalisationTask<Scene>* task = TTE_NEW(CommonNormalisationTask<Scene>, MEMORY_TAG_TEMPORARY_ASYNC, _TaskFence, registry);
    task->Output = pScene;
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
    if (_Active.size() != 0)
    {
        for (auto it = _Active.rbegin(); it != _Active.rend(); it++)
        {
            if (!it->first->IsBlocking)
                continue; // ignore non blocking, they don't need an order
            JobScheduler::Instance->EnqueueOne(it->second, std::move(desc)); // found a blocking job, enqueue it after
            return;
        }
    }
    JobHandle handle = JobScheduler::Instance->Post(std::move(desc));
    _Active.push_back(std::make_pair(pTask, std::move(handle)));
}

Bool TelltaleEditor::QuickNormalise(Ptr<Handleable> pCommonInstanceOut, Meta::ClassInstance inInstance)
{
    if (pCommonInstanceOut && inInstance)
    {
        return InstanceTransformation::PerformNormaliseAsync(pCommonInstanceOut, inInstance, GetThreadLVM());
    }
    return false;
}

Bool TelltaleEditor::QuickSpecialise(Ptr<Handleable> pCommonInstance, Meta::ClassInstance instance)
{
    if (pCommonInstance && instance)
    {
        return InstanceTransformation::PerformSpecialiseAsync(pCommonInstance, instance, GetThreadLVM());
    }
    return false;
}

Ptr<Handleable> TelltaleEditor::CreateCommonClass(CommonClass cls, Ptr<ResourceRegistry> registry)
{
    if(cls == CommonClass::PROPERTY_SET)
    {
        TTE_LOG("Invalid class: use CreatePropertySet for props.");
        return nullptr;
    }
    const CommonClassInfo& desc = GetCommonClassInfo(cls);
    if(desc.Class != cls)
    {
        TTE_LOG("Invalid class, or not supported yet!");
        return nullptr;
    }
    return desc.Make(std::move(registry));
}

Meta::ClassInstance TelltaleEditor::CreateSpecialisedClass(CommonClass cls)
{
    // Here we need to decide which class and version number to use. The script will decide. 
    U32 clazz = Meta::_Impl::_ResolveCommonClassIDSafe(cls);
    if(clazz == 0)
    {
        return {};
    }
    return Meta::CreateInstance(clazz);
}

Meta::ClassInstance TelltaleEditor::CreatePropertySet()
{
   return CreateSpecialisedClass(CommonClass::PROPERTY_SET); // delegate here
}

CommonClassInfo TelltaleEditor::GetCommonClassInfo(CommonClass cls)
{
    return CommonClassInfo::GetCommonClassInfo(cls);
}

//  PROPS

Bool TTEProperties::GetLoadState() const
{
    return _LoadState;
}

TTEProperties::TTEProperties() : _URI{}, _LoadState(true)
{
}

TTEProperties::TTEProperties(ResourceURL URI) : _URI{URI}, _LoadState(true)
{
    Load(URI);
}

void TTEProperties::Load(ResourceURL physicalURI)
{
    _LoadState = true;
    _URI = std::move(physicalURI);
    U8* Temp{};
    // Load user properties file
    DataStreamRef inputStream = DataStreamManager::GetInstance()->CreateFileStream(_URI);
    String stringParse{};
    Temp = TTE_ALLOC(inputStream->GetSize() + 1, MEMORY_TAG_TEMPORARY);
    Temp[inputStream->GetSize()] = 0;
    inputStream->Read(Temp, inputStream->GetSize());
    stringParse = (CString)Temp;
    TTE_FREE(Temp);
    // PARSE

    std::istringstream stream(stringParse);
    String line;
    String currentKey;
    std::vector<std::string> currentArrayValues;
    Bool bInArray = false;

    while (std::getline(stream, line))
    {
        line = StringTrim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (bInArray)
        {
            String value = StringTrim(line);
            if (value.length())
            {
                if(value == "]")
                {
                    bInArray = false;
                    continue;
                }
                Bool bLast = value[value.length() - 1] != ',';
                if (!bLast)
                    value = value.substr(0, value.length() - 1);
                currentArrayValues.push_back(std::move(value));
                if (bLast)
                {
                    String trimmed;
                    while (std::getline(stream, line))
                    {
                        trimmed = StringTrim(line);
                        if (trimmed.empty())
                            continue;
                        if (trimmed == "]")
                        {
                            bInArray = false;
                            _StringArrayKeys[currentKey] = std::move(currentArrayValues);
                            break;
                        }
                        else
                        {
                            _LoadState = false;
                            TTE_LOG("Could not read properties file: at terminator for string array key '%s': expected a ']' on a new line", currentKey.c_str());
                            return;
                        }
                    }

                    if (stream.eof() && trimmed != "]")
                    {
                        _LoadState = false;
                        TTE_LOG("Could not read properties file: unexpected EOF while reading array for key '%s'", currentKey.c_str());
                        return;
                    }

                    continue;
                }
            }
        }
        else if (line[0] == '[')
        {
            currentKey = StringTrim(line.substr(3, line.find(":") - 3));
            String value = StringTrim(line.substr(line.find(":") + 1));
            if (line[1] == 'S')
            {
                _StringKeys[currentKey] = value;
            }
            else if (line[1] == 'I')
            {
                _IntKeys[currentKey] = std::stoi(value);
            }
            else if (line[1] == 'A')
            {
                bInArray = true;
                if (!std::getline(stream, line) || StringTrim(line) != "[")
                {
                    _LoadState = false;
                    TTE_LOG("Could not read properties file: near string array key '%s': expected a '[' on a new line", currentKey.c_str());
                    return;
                }
            }
            else
            {
                _LoadState = false;
                TTE_LOG("Could not read properties file: near string '%s': unknown or invalid property type", line.c_str());
                return;
            }
        }
        else
        {
            _LoadState = false;
            TTE_LOG("Could not read properties file: near string '%s': unexpected tokens, did you forget a '#'?", line.c_str());
            return;
        }
    }
}

static void WriteStr(DataStreamRef& outStream, String str)
{
    outStream->Write((const U8*)str.c_str(), str.length());
}

void TTEProperties::Save()
{
    DataStreamRef outStream = DataStreamManager::GetInstance()->CreateFileStream(_URI);
    WriteStr(outStream, "# Telltale Editor Properties file (with v" TTE_VERSION ")\n\n");
    for (const auto& p : _StringKeys)
    {
        WriteStr(outStream, "[S] ");
        WriteStr(outStream, p.first);
        WriteStr(outStream, ": ");
        WriteStr(outStream, p.second);
        WriteStr(outStream, "\n");
    }
    for (const auto& p : _IntKeys)
    {
        WriteStr(outStream, "[I] ");
        WriteStr(outStream, p.first);
        WriteStr(outStream, ": ");
        std::ostringstream s{};
        s << p.second;
        WriteStr(outStream, s.str());
        WriteStr(outStream, "\n");
    }
    for (const auto& p : _StringArrayKeys)
    {
        WriteStr(outStream, "[A] ");
        WriteStr(outStream, p.first);
        WriteStr(outStream, ": \n[");
        Bool bFirst = true;
        for (const auto& a : p.second)
        {
            if (!bFirst)
            {
                WriteStr(outStream, ",\n\t");
            }
            else
            {
                WriteStr(outStream, "\n\t");
                bFirst = false;
            }
            WriteStr(outStream, a);
        }
        WriteStr(outStream, "\n]\n");
    }
}

I32 TTEProperties::GetInteger(const String& key, I32 def)
{
    for (auto it = _IntKeys.find(key); it != _IntKeys.end(); it = _IntKeys.end())
        return it->second;
    return def;
}

String TTEProperties::GetString(const String& key, String def)
{
    for (auto it = _StringKeys.find(key); it != _StringKeys.end(); it = _StringKeys.end())
        return it->second;
    return def;
}

void TTEProperties::SetInteger(const String& key, I32 value)
{
    _IntKeys[key] = value;
}

void TTEProperties::SetString(const String& key, const String& value)
{
    _StringKeys[key] = value;
}

std::vector<String> TTEProperties::GetStringArray(const String& key)
{
    for (auto it = _StringArrayKeys.find(key); it != _StringArrayKeys.end(); it = _StringArrayKeys.end())
        return it->second;
    return {};
}

void TTEProperties::AddArray(const String& key, const String& value)
{
    _StringArrayKeys[key].push_back(value);
}

void TTEProperties::RemoveArray(const String& key, const String& value)
{
    for (auto it = _StringArrayKeys.find(key); it != _StringArrayKeys.end(); it = _StringArrayKeys.end())
    {
        for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
        {
            if (CompareCaseInsensitive(*it2, value))
            {
                it->second.erase(it2);
                return;
            }
        }
    }
}

void TTEProperties::Remove(const String& key)
{
    for (auto it = _IntKeys.find(key); it != _IntKeys.end(); it = _IntKeys.end())
        _IntKeys.erase(it);
    for (auto it = _StringKeys.find(key); it != _StringKeys.end(); it = _StringKeys.end())
        _StringKeys.erase(it);
    for (auto it = _StringArrayKeys.find(key); it != _StringArrayKeys.end(); it = _StringArrayKeys.end())
        _StringArrayKeys.erase(it);
}

void TTEProperties::Clear()
{
    _IntKeys.clear();
    _StringKeys.clear();
    _StringArrayKeys.clear();
}

#undef _TTE_SYMBOLS_HPP
#define _TTE_SYMBOLS_IMPL
#include <Symbols.hpp>
#undef _TTE_SYMBOLS_IMPL
