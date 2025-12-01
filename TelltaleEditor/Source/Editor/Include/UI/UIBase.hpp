#pragma once

#include <Core/Config.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <algorithm>
#include <set>
#include <vector>

struct ImVec2;

class LoadingTextAnimator
{
public:
    
    inline LoadingTextAnimator(U32 ndots = 3, Float tstep = 0.7f) : _DotsCap((U16)ndots), _TimeStep(tstep) {}

    inline String GetEllipses()
    {
        U64 now = GetTimeStamp();
        if(GetTimeStampDifference(_LastTs, now) >= _TimeStep)
        {
            _LastTs = now;
            _NDots = (_NDots + 1) % _DotsCap;
        }
        return String(_NDots + 1, '.');
    }

private:

    U64 _LastTs = 0;
    Float _TimeStep = 0.0f;
    U16 _NDots = 0;
    U16 _DotsCap = 0;

};

struct MenuOption
{

    String ToolBarOption;
    String SubOption;
    String Shortcut;
    mutable CString OverrideSubOptionText = nullptr;

    U32 ShortcutKey = 0;
    Bool ShortcutLShift = false;
    Bool Separator = false;
    Bool ShortcutLControl = false;

    mutable Bool Requested = false;

};

struct MenuOptionInterface
{

    std::vector<MenuOption> MenuOptions;

    void AddMenuOptions(CString myToolBarOption);
    Bool TestMenuOption(String toolBarOption, String subOption, String shortcut, U32 imguiKey, Bool needsLshift, Bool needsControl, Bool separaterAfter = false, CString overrideText = nullptr);
    Bool OpenContextMenu(CString toolBarOption, ImVec2 boxMin, ImVec2 boxMax);

};

class ApplicationUI;

// UTILS
#define UI_COMPONENT_CONSTRUCTOR(_Ty) inline _Ty(ApplicationUI& appUI) : UIComponent(appUI) {}
#define UI_STACKABLE_CONSTRUCTOR(_Ty) inline _Ty(ApplicationUI& appUI) : UIStackable(appUI) {}
#define SELECT_SIZE(winSize, Frac, MinPix, MaxPix) std::clamp((Frac) * (winSize), (MinPix), (MaxPix))
#define DECL_VEC_ADDITION() inline static ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2{lhs.x + rhs.x, lhs.y + rhs.y}; } \
inline static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2{lhs.x - rhs.x, lhs.y - rhs.y}; }
#define DECL_VEC_DOT() inline static ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2{lhs.x * rhs.x, lhs.y * rhs.y}; }

class UIComponent
{
public:

    UIComponent(ApplicationUI& appUI);

    void SetNextWindowViewport(Float xPosFrac, Float yPosFrac, U32 xMinPix, U32 yMinPix, Float xExtentFrac, Float yExtentFrac, U32 xExtentMinPix, U32 yExtentMinPix, U32 imguiCondition);
    void SetNextWindowViewportPixels(Float posX, Float posY, Float SizeX, Float SizeY);

    // Pass in WINDOW fractional coordinates (inside Begin/End call window region)
    void DrawResourceTexture(const String& name, Float xPosFrac, Float yPosFrac, Float xSizeFrac, 
                             Float ySizeFrac, U32 colorScale = 0xFFFFFFFFu, CString customID = nullptr);

    void DrawResourceTexturePixels(const String& name, Float xPos, Float yPos, Float xSize, Float ySize, 
                                   U32 colorScale = 0xFFFFFFFFu, CString customID = nullptr);

    String TruncateText(const String& src, Float truncWidth);

    void DrawCenteredWrappedText(const String& text, Float maxWidth, Float centerPosX, Float centerPosY, I32 maxLines, Float fontSize);

    virtual Bool Render() = 0;

    ApplicationUI& GetApplication();

    const String& GetLanguageText(CString id);
    
    ApplicationUI& _MyUI;

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
    virtual Bool Render() override;

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
        String PackageKey; // for PKG files
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
    String _PlaystationPackageKey = "";
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
    
    virtual Bool Render() override;

    void GetWindowSize(U32& w, U32& h);

    inline Bool GetCreationState()
    {
        return _CreationState;
    }

    void Reset();

};

// Ideally if a proper engine we would have our own intrusive ref counting system, but is that really needed here.
class UIConsole : public UIStackable
{
    
    Bool _Autoscroll = true;
    
public:
    
    UIConsole(ApplicationUI& app);
    
    virtual Bool Render() override;
    
    virtual ~UIConsole();
    
};

class UIMemoryTracker : public UIStackable
{
    
    struct Tracked : Memory::TrackedAllocation
    {
        
        String SourceFull;
        
        inline Bool operator<(const Tracked& rhs) const
        {
            return Timestamp < rhs.Timestamp;
        }
        
    };
    
    std::set<Tracked> _Tracked;
    U64 _Ts;
    
    U64 _AllocTotal = 0;
    double _AllocAverageSize = 0.0;
    Float _Pressure = 0.0f;
    U32 _TagFilter = 999;
    
public:
    
    UIMemoryTracker(ApplicationUI& app);
    
    virtual Bool Render() override;
    
    virtual ~UIMemoryTracker();
    
};
