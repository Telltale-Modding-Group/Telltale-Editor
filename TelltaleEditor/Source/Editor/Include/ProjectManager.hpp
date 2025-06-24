#include <TelltaleEditor.hpp>

#include <vector>
#include <filesystem>

#define PROJECT_KEY_PREVIOUS_PROJECTS "Project Manager - Previous Projects"
#define PROJECT_KEY_ACTIVE_PROJECT "Project Manager - Active Project"

#define PROJECT_EXT ".tteproj"

#define PROJECT_KEY_NAME "Project - Name"
#define PROJECT_KEY_DESC "Project - Description"
#define PROJECT_KEY_AUTH "Project - Author"
#define PROJECT_KEY_SNAP_GAMEID "Project - Snapshot Game ID"
#define PROJECT_KEY_SNAP_PLATFORM "Project - Snapshot Platform"
#define PROJECT_KEY_SNAP_VENDOR "Project - Snapshop Vendor"
#define PROJECT_KEY_DIRS "Project - Mount Points"

struct TTEProject
{

    String ProjectName;
    String ProjectAuthor;
    String ProjectDescription;
    std::filesystem::path ProjectFile;
    GameSnapshot ProjectSnapshot;
    std::vector<std::filesystem::path> MountDirectories; // game-specific mounted data (files)

    TTEProject() = default;

};

class ProjectManager
{
    
    TTEProperties& _WorkspaceProperties;

    std::vector<TTEProject> _PreviousProjects;
    TTEProject _ActiveProject;

    TTEProject _LoadProject(TTEProperties& fromProperties, std::filesystem::path path);

    void _SaveProject(TTEProperties& toProperties, const TTEProject& project);

public:

    ProjectManager(TTEProperties& workspaceProps);

    void LoadFromWorkspace();

    void SaveToWorkspace(); // save workspace project manager settings (current project, previous ones etc)

    void RetrievePreviousProjects(std::vector<String>& outProjectNames);

    TTEProject* GetHeadProject(); // currently active proj. return null if none yet
    
    void CreateProject(TTEProject project); // create a project and set as active project. ensure name doesnt already exist.

    void SetProject(const String& name); // set active project 

    Bool SetProjectDisk(const String& filePath); // set active project to one on disk (not in previous) returns if ok. False if corrupt etc

    TTEProject GetPreviousProject(const String& name); // get previous project information, or empty if not found

};