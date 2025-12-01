#pragma once

#include <Core/BitSet.hpp>
#include <UI/UIBase.hpp>
#include <UI/MenuBar.hpp>
#include <Common/Scene.hpp>
#include <Runtime/SceneRenderer.hpp>

#include <unordered_set>
#include <unordered_map>
#include <functional>

#define DEFAULT_WINDOW_SIZE 1280, 720

enum class EditorFlag
{
    RESETTING_UI = 1,
};

class EditorUI;
class UIResourceEditorBase;
class EditorUIComponent;
class SceneView;
class InspectorView;
struct ImVec2;

class EditorUI : public UIStackable
{
    
    struct LoadInfo
    {
        String ViewTitle;
        String Resource; // file name
        U32 PreloadBatch;
        std::function<Ptr<UIResourceEditorBase>(const LoadInfo&, EditorUI&, Ptr<ResourceRegistry>)> Callback;
    };

    template<SceneModuleType>
    friend struct SceneModule;

    MenuBar _MenuBar;
    std::vector<Ptr<EditorUIComponent>> _Views; // file view log view etc
    std::vector<Ptr<UIResourceEditorBase>> _TransientViews; // specific file windows
    std::vector<Ptr<UIResourceEditorBase>> _PendingTransientViews; // to be pushed
    std::vector<LoadInfo> _AwaitingLoads;
    Scene _EditorScene;
    Ptr<ReferenceObjectInterface> _EditorSceneGuard; // replaced when editor scene changes, so we want to discard refs (its not a ptr)
    WeakPtr<SceneView> _SceneView;
    WeakPtr<InspectorView> _InspectorView;
    String _PendingCloseView;

    Ptr<Scene> _AsyncInitScene;
    JobHandle _InitSceneJob;
    String _LoadingAsyncScene;
    U32 _LoadingScenePreload;
    Ticker _UpdateTicker;

    static Bool _SceneInitialiseJob(const JobThread& thread, void* uA, void* uB);

    Bool _TickAsyncLoadingScene(); // return if currently loading

    void _OnFileClickCallbackAdapter(String rl);
    
    template<typename CommonT>
    Bool _TestOpenEditor(const String& ext, const String& fileName, const String& rloc);

public:

    void UserRequestOpenFile();

    void DispatchEditor(String viewTitle, String rloc, std::function<Ptr<UIResourceEditorBase>(const LoadInfo&, EditorUI&, Ptr<ResourceRegistry>)> callback);

    void DispatchEditorImmediate(Ptr<UIResourceEditorBase> allocated);

    void CloseEditor(String comparator);

    inline Scene& GetActiveScene()
    {
        return _EditorScene;
    }

    void OnFileClick(const String& resourceLocation);

    Flags UIFlags; // pub
    WeakPtr<Node> InspectingNode;
    Bool IsInspectingAgent = false;
    Float InspectorViewY = 0.0f;

    EditorUI(ApplicationUI& app);

    virtual Bool Render() override;

    friend class MenuBar;
    friend class InspectorView;
    friend class SceneView;

};

class EditorUIComponent : public UIComponent
{
protected:

    EditorUI& _EditorUI;

public:

    inline EditorUIComponent(EditorUI& editor) : UIComponent(editor.GetApplication()), _EditorUI(editor)
    {
    }

    // get ui condition for drawing windows such size is only listened too on user requested UI reset or first use
    U32 GetUICondition();

};

struct EditorPopup
{
    
    const String Name;
    EditorUI* Editor = nullptr;
    
    inline EditorPopup(const String& N) : Name(N) {}
    
    virtual ImVec2 GetPopupSize() = 0;
    virtual Bool Render() = 0; // return true if closing
    
};

class FileView : public EditorUIComponent
{

    enum FolderGroup
    {
        MOUNTS,
        RESOURCE_GROUPS,
        FILES,
        COUNT,
    };

    struct FolderGroupData
    {
        std::vector<String> Entries;
        U64 UpdateStamp = 0;
    };

    struct IconMapping
    {
        String IconFile;
        String Description;
    };

    FolderGroup _CurGroup = MOUNTS;
    FolderGroupData _Group[COUNT];
    std::unordered_map<String, String> _StrippedToLocation;
    std::vector<String> _ViewStack;
    std::map<StringMask, IconMapping> _IconMap;

    void _Gather(Ptr<ResourceRegistry> pRegistry, std::vector<String>& entries, FolderGroup group, String parent);

    Bool _Update(Ptr<ResourceRegistry> pRegistry, I32 clickedIndex);

public:

    FileView(EditorUI& ui);

    virtual Bool Render() override;

};

class OutlineView : public EditorUIComponent
{

    char _ContainTextFilter[32];
    Bool _RenderSceneNode(WeakPtr<Node> pNode);

public:

    OutlineView(EditorUI& ui);

    virtual Bool Render() override;

};

struct PropertyVisualAdapter;

class InspectorView : public EditorUIComponent
{

    struct PropertyRuntimeInstance
    {
        Meta::ClassInstance Property;
        const PropertyVisualAdapter* Specification = nullptr;
    };

    struct PropertyRuntimeGroup
    {
        String ModuleName;
        String ModuleIcon;
        std::vector<PropertyRuntimeInstance> Properties;
        Float LastHeight = 40.0f;
        Bool IsOpen = false;
    };

    std::unordered_map<SceneModuleType, PropertyRuntimeGroup> _RuntimeModuleProps;
    Ptr<Node> _CurrentNode;
    U64 _LastPropFlush = 0;
    Bool _CurrentIsAgent = false;

    void _FlushProps();
    void _ResetModulesCache();
    void _InitModuleCache(Meta::ClassInstance agentProps, SceneModuleType module);

public:

    InspectorView(EditorUI& ui);
    ~InspectorView();

    virtual Bool Render() override;

    Bool RenderNode(Float sY);

    void OnSceneChange(); // called just before scene change

};

struct SceneViewData;

class SceneView : public EditorUIComponent
{

    friend class EditorUI;

    Ptr<Scene> _EditorSceneCache;
    Ptr<Camera> _SceneViewCamDefault;
    SceneViewData* _SceneData;
    SceneRenderer _SceneRenderer;
    RuntimeInputEventManager _ViewInputMgr;

    void _OnSceneLoad(Ptr<Scene> pEditorScene);
    void _OnSceneUnload(Ptr<Scene> pEditorScene);
    void _UpdateViewNoActiveScene(Ptr<Scene> pEditorScene, SceneViewData& viewData, Bool bWindowFocused);
    void _UpdateView(Ptr<Scene> pEditorScene, SceneViewData& viewData, Bool bWindowFocused);
    void _FreeSceneData();

    static void PostRender(void* ud, const SceneFrameRenderParams& params, RenderSceneView* mv);

public:

    SceneView(EditorUI& ui);

    ~SceneView();

    virtual Bool Render() override;

};

// POPUPS


struct NewAgentPopup : EditorPopup
{

    OutlineView* _OV;
    char _Input[48];

    NewAgentPopup(OutlineView* ov);
    virtual Bool Render() override;
    virtual ImVec2 GetPopupSize() override;

};
