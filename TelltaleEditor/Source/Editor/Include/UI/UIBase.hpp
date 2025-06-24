#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <algorithm>
#include <vector>

class ApplicationUI;

// UTILS
#define UI_COMPONENT_CONSTRUCTOR(_Ty) inline _Ty(ApplicationUI& appUI) : UIComponent(appUI) {}
#define UI_STACKABLE_CONSTRUCTOR(_Ty) inline _Ty(ApplicationUI& appUI) : UIStackable(appUI) {}
#define SELECT_SIZE(winSize, Frac, MinPix, MaxPix) std::clamp((Frac) * (winSize), (MinPix), (MaxPix))
#define DECL_VEC_ADDITION() inline static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2{lhs.x + rhs.x, lhs.y + rhs.y}; }
#define DECL_VEC_DOT() inline static ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2{lhs.x * rhs.x, lhs.y * rhs.y}; }

class UIComponent
{

    ApplicationUI& _MyUI;

public:

    UIComponent(ApplicationUI& appUI);

    void SetNextWindowViewport(Float xPosFrac, Float yPosFrac, U32 xMinPix, U32 yMinPix, Float xExtentFrac, Float yExtentFrac, U32 xExtentMinPix, U32 yExtentMinPix);

    // Pass in WINDOW fractional coordinates (inside Begin/End call window region)
    void DrawResourceTexture(const String& name, Float xPosFrac, Float yPosFrac, Float xSizeFrac, Float ySizeFrac, U32 colorScale = 0xFFFFFFFFu);

    virtual void Render() = 0;

    ApplicationUI& GetApplication();

    const String& GetLanguageText(CString id);

};

// Stackable UI element inside the app.
class UIStackable : public UIComponent
{
public:

    UI_COMPONENT_CONSTRUCTOR(UIStackable);

};

// Manages UI for project selection and initial creation.
class UIProjectSelect : public UIStackable
{

    struct RecentEntry
    {
        String Name, Location;
    };

    ApplicationUI& _Application;
    std::vector<RecentEntry> _Recents;
    String _LangBoxSelect;
    Bool _SelectionState = false;

public:

    inline Bool GetSelectionState()
    {
        return _SelectionState;
    }

    void Reset();

    UIProjectSelect(ApplicationUI& app);
   
    // Returns true when a project is finally loaded or created
    virtual void Render() override;

    void GetWindowSize(U32& w, U32& h); // window size for this window

};

enum class ProjectCreationPage
{
    INFO = 0,
    LOCATION = 1,
    PROJECT_FOLDER_PICK = 2,
    SNAPSHOT = 3,
};

class UIProjectCreate : public UIStackable
{

    struct SnapshotGuessState : std::enable_shared_from_this<SnapshotGuessState>
    {
        std::vector<String> Mounts;
        GameSnapshot ResolvedSnapshot;
        std::atomic_bool* OnExecute = nullptr;
    };
    
    Ptr<SnapshotGuessState> _AsyncGuessState = {};
    String _SelectedGame = "";
    I32 _SelectedGameIndex = 0;
    String _SelectedPlatform = "";
    String _SelectedVendor = "";
    String _ProjectLocation = "";
    U64 _GuessStart = 0;
    JobHandle _GuessJob = {};
    std::vector<String> _MountPoints;
    ProjectCreationPage _Page = ProjectCreationPage::INFO;
    Bool _CreationState = false;
    Bool _GuessedSnap = false;
    Bool _SchedulerNeedsShutdown = false;
    char _ProjectName[32] = "";
    char _Description[256] = "";
    char _Author[48] = "";
    char _FileName[64] = "MyProject.tteproj";

    void _ReleaseJob();

    static Bool AsyncSnapshotGuess(const JobThread& thread, void* pArg1, void* pArg2);

public:

    inline UIProjectCreate(ApplicationUI& app) : UIStackable(app)
    {
        _CreationState = false;
        Reset();
    }
    
    virtual void Render() override;

    void GetWindowSize(U32& w, U32& h);

    inline Bool GetCreationState()
    {
        return _CreationState;
    }

    void Reset();

};
