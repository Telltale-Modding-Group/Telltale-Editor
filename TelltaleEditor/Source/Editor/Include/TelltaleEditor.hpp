#pragma once

#include <set>
#include <vector>
#include <unordered_map>

#include <Core/Config.hpp>
#include <Core/Context.hpp>
#include <EditorTasks.hpp>
#include <Meta/Meta.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Common/Common.hpp>
#include <UI/ModuleUI.inl>

class TelltaleEditor;

/// Creatae the global editor context (use once). Creates tool context internally.
TelltaleEditor* CreateEditorContext(GameSnapshot snapshot);

/// Free the global editor context (use once). Do this right at the end when you are finished with the application.
void FreeEditorContext();

// C++ API

/// The ToolLibrary is the underlying library. This is the main part of the editor which allows for this post job and wait system for any task related. This also includes GUI.
/// This represents the application state if using the editor and not the tool library - which this uses internally privately.
/// VERY IMPORTANT: If you are using this API and no the tool context (you should always use this API) then do not access the ToolContext directly, interact through this.
/// See EditorTasks.h for information on about the tasks you can post.
/// Here API is provided which doesn't need the resource registry, such as all normalisation. This allows for quick normalisation in tools. The usual way is that its done automatically
/// in the resource registry where when you load a resoure it serialises and normalises for you.
class TelltaleEditor
{
    
    void _PostSwitch(GameSnapshot snap);
    
    TelltaleEditor(GameSnapshot snapshot);
    
public:
    
    ~TelltaleEditor();
    
    void Switch(GameSnapshot snapshot); // switch to new snapshot
    
    // Main thread update. 
    void Update();
    
    /**
     Returns if the context is unaccessible because it is currently busy with a job which required it completely.
     */
    Bool ContextIsBusy();
    
    /**
     Waits for the context to be free, to finish all blocking jobs. Should not really be used UNLESS you just want to complete tasks asap eg from a modding script.
     */
    void Wait();
    
    /**
     Queries if the given task has finished. If this returns true, it has been finalised and all updates will have completed.
     This can also be ensured after Wait() finishes.
     */
    Bool QueryTask(U32 task);
    
    /**
     Enqueues a task which will normalise the given scene instance into the pScene argument. This standardises from any game version the given scene
     into the common scene. The scene passed is safe to edit after this call but it will be overriden possibly if you call any function part of this class.
     At that point the scene will be replaced (its data) with the normalised scene.
     Please note that for any of the results of this task to take effect, you must call Wait or QueryTask.
     Returns the task handle which you can query the completion with.
     */
    U32 EnqueueNormaliseSceneTask(Ptr<ResourceRegistry> registry, Ptr<Scene> pScene, Meta::ClassInstance sceneInstance);
    
    /**
     Enqueues a task which normalises the given IMAP instance for the current game snapshot.
     See EnqueueMeshNormaliseTask.
     */
    U32 EnqueueNormaliseInputMapperTask(Ptr<ResourceRegistry> registry, Ptr<InputMapper> imap, Meta::ClassInstance metaInstance);
    
    /**
     Enqueues a task which normalises the given D3D mesh instance for the current game snapshot.
     See EnqueueMeshNormaliseTask.
     */
    U32 EnqueueNormaliseMeshTask(Ptr<ResourceRegistry> registry, Scene* pScene, Symbol agent, Meta::ClassInstance d3dmeshInstance);
    
    /**
     Enqueues a task which normalises the given texture from the current game snapshot into the common format.
     */
    U32 EnqueueNormaliseTextureTask(Ptr<ResourceRegistry> registry, Meta::ClassInstance instance, Ptr<RenderTexture> outputTexture);
    
    /**
     Enqueues a task which will asynchronously extract all of the files in the given archive. If the files array is empty, all files will be extracted.
     */
    U32 EnqueueArchiveExtractTask(TTArchive* pArchive, std::set<String>&& files, String outputFolder);
    
    /**
     Enqueues a task which will asynchronously extract all of the files in the given archive. If the files array is empty, all files will be extracted.
     */
    U32 EnqueueArchive2ExtractTask(TTArchive2* pArchive, std::set<String>&& files, String outputFolder);
    
    /**
     Enqueues a task which will asynchronously extract all files into the given output foldes from the given logical resource system locator. Pass empty string for string mask to get all files, else supply a mask.
     Optionally set the last argument to true such that folders will be created for each association: so all dlg or imaps go into Dialogs folder etc.
     The callback will be called each time a file has been extracted. It will be called asyncronously so use synchronisation primitives if needed.s
     */
    U32 EnqueueResourceLocationExtractTask(Ptr<ResourceRegistry> registry, const String& logical, String outputFolder,
                                           StringMask mask, Bool bFolders = false, ResourceExtractCallback* pCallback = nullptr);
    
    /**
     Specailses (on this thread, no enqueueing) the given instance into the meta class instance for the current game, putting it back into a telltale format which can be serialised to a file using Meta.
     */
    Bool QuickSpecialise(Ptr<Handleable> pCommonInstance, Meta::ClassInstance outInstance);
    
    /**
     Normalises (on this thread, no enqueueing) to the given instance from  the meta class instance for the current game.
     */
    Bool QuickNormalise(Ptr<Handleable> pCommonInstanceOut, Meta::ClassInstance inInstance);

    /**
     * Creates a common meta instanced which can be specialised into using QuickSpecialise or one of the task version ones.
     * Pass in the type. This will select the correct class name and version number for the current snapshot.
     */
    Meta::ClassInstance CreateSpecialisedClass(CommonClass clz);

    /**
     * Gets the information struct for the given common class.
     */
    CommonClassInfo GetCommonClassInfo(CommonClass clz);

    /**
     * Creates an empty, new common class. Tend to use this for writing new files.
     * For PropertySet, please use the create property set version!
     * Pass in the resource registry as well.
     */
    Ptr<Handleable> CreateCommonClass(CommonClass clz, Ptr<ResourceRegistry> registry);

    /**
     * CreateCommonClass but for property set, as thats just a meta instance
     */
    Meta::ClassInstance CreatePropertySet();
    
    /**
     Creates a resource registry which can be used to manage telltale games resources. These cannot be used between game switches. Must be destroyed before this object or a switch!
     See ToolContext:CreateResourceRegistry().
     */
    Ptr<ResourceRegistry> CreateResourceRegistry();
    
    // See ToolContext version. Delegates and thread safe.
    inline DataStreamRef LoadLibraryResource(String name)
    {
        return _ModdingContext->LoadLibraryResource(name);
    }
    
    inline String LoadLibraryStringResource(String name)
    {
        return _ModdingContext->LoadLibraryStringResource(name);
    }
    
private:
    
    // probes any tasks, if any finished their finalisers are called on main thread.
    // always called on every TTE function in this class. specify if we want to wait for all to finish
    // check specific is used for return value. if non -1 will return if the given has finished
    Bool _ProbeTasks(Bool bWaitOnAll, U32 _CheckSpecific = (U32)-1);
    
    // enqueues blocking job which requires the lua env
    void _EnqueueTask(EditorTask* pTask);
    
    ToolContext* _ModdingContext = nullptr;
    
    U32 _TaskFence = 0; // counter
    
    std::vector<std::pair<EditorTask*, JobHandle>> _Active;

    std::unordered_map<String, ModuleUI> _ModuleVisualProperties;
    
    friend class InspectorView;
    friend U32 luaRegisterModuleUI(LuaManager& man);

    friend TelltaleEditor* CreateEditorContext(GameSnapshot snapshot);
    
};

Bool AsyncTTETaskDelegate(const JobThread& thread, void* argA, void* argB);

// User properties. Not related to any Telltale systems or files, only use for properties at runtime.
class TTEProperties
{
public:

    // Create a TTE properties from its physical file location on disc.
    void Load(ResourceURL URI);

    TTEProperties();

    TTEProperties(ResourceURL URI);

    void Save(); // Save to disc.

    I32 GetInteger(const String& key, I32 orDefault);

    String GetString(const String& key, String orDefault);

    void SetInteger(const String& key, I32 value);

    void SetString(const String& key, const String& value);

    std::vector<String> GetStringArray(const String& key);

    void AddArray(const String& key, const String& value);

    void RemoveArray(const String& key, const String& value);

    void Remove(const String& key);

    void Clear();

    // Returns true if the props were read OK.
    Bool GetLoadState() const;

private:

    ResourceURL _URI;

    std::unordered_map<String, I32> _IntKeys;
    std::unordered_map<String, String> _StringKeys;
    std::unordered_map<String, std::vector<String>> _StringArrayKeys;

    Bool _LoadState;

};

// Command line helpers
namespace CommandLine
{
    
    enum ArgumentType
    {
        NONE, STRING, INT, BOOL
    };
    
    struct TaskArgument
    {
        String Name = "";
        ArgumentType Type;
        std::vector<String> Aliases;
        Bool Multiple = false;
        
        String StringValue;
        I32 IntValue;
        Bool BoolValue;
    };
    
    using TaskExecutor = I32 (const std::vector<TaskArgument>& args);
    
    struct TaskInfo
    {
        String Name;
        String Desc;
        TaskExecutor* Executor;
        std::vector<TaskArgument> RequiredArguments;
        std::vector<TaskArgument> OptionalArguments;
        Bool DefaultTask = false;
    };
    
    // Get a list of all available editor tasks in this development build.
    std::vector<TaskInfo> CreateTasks();
    
    // Parse command line args into argument stack to be used.
    std::vector<String> ParseArgsStack(int argc, char** argv);
    
    // To automatically run the editor, just call guarded main here which will run all tasks in the command line.
    I32 GuardedMain(int argc, char** argv); // executabe command line and exit.
    
    // Defined also in Main.cpp. The main editor application task.
    I32 Executor_Editor(const std::vector<TaskArgument>& args);
    
    String GetStringArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, String def);
    
    I32 GetIntArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, I32 def);
    
    Bool GetBoolArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, Bool def);
    
    Bool HasArgument(const std::vector<TaskArgument>& args, String arg);
    
    std::vector<String> GetInputFiles(const String& path);
    
    namespace _Impl
    {
        
        Bool MatchArgument(String userArg, const TaskArgument& test);
        
        Bool MatchesAnyArgument(String userArg, const std::vector<TaskArgument>& test);
        
        TaskInfo ResolveTask(const std::vector<TaskInfo>& tasks, String stackArg);
        
        Bool ParseUserArgument(TaskArgument& arg, std::vector<String>& argsStack);
        
        I32 ExecuteTask(std::vector<String>& argsStack, const std::vector<TaskInfo>& availableTasks);
        
    }
    
}
