#include <ProjectManager.hpp>

void ProjectManager::_SaveProject(TTEProperties& toProperties, const TTEProject& project)
{
    toProperties.SetString(PROJECT_KEY_NAME, project.ProjectName);
    toProperties.SetString(PROJECT_KEY_DESC, project.ProjectDescription);
    toProperties.SetString(PROJECT_KEY_AUTH, project.ProjectAuthor);
    toProperties.SetString(PROJECT_KEY_PSK, project.ProjectPlaystationPackageKey);
    toProperties.SetString(PROJECT_KEY_SNAP_GAMEID, project.ProjectSnapshot.ID);
    toProperties.SetString(PROJECT_KEY_SNAP_PLATFORM, project.ProjectSnapshot.Platform);
    toProperties.SetString(PROJECT_KEY_SNAP_VENDOR, project.ProjectSnapshot.Vendor);
    std::vector<String> mps{};
    toProperties.Remove(PROJECT_KEY_DIRS);
    for(const auto& p: project.MountDirectories)
    {
        toProperties.AddArray(PROJECT_KEY_DIRS, p.string());
    }
}

TTEProject ProjectManager::_LoadProject(TTEProperties& projectProps, std::filesystem::path path)
{
    TTEProject projectInfo{};
    projectInfo.ProjectName = projectProps.GetString(PROJECT_KEY_NAME, "");
    projectInfo.ProjectDescription = projectProps.GetString(PROJECT_KEY_DESC, "");
    projectInfo.ProjectAuthor = projectProps.GetString(PROJECT_KEY_AUTH, "");
    projectInfo.ProjectPlaystationPackageKey = projectProps.GetString(PROJECT_KEY_PSK, "");
    projectInfo.ProjectSnapshot.ID = projectProps.GetString(PROJECT_KEY_SNAP_GAMEID, "");
    projectInfo.ProjectSnapshot.Platform = projectProps.GetString(PROJECT_KEY_SNAP_PLATFORM, "");
    projectInfo.ProjectSnapshot.Vendor = projectProps.GetString(PROJECT_KEY_SNAP_VENDOR, "");
    projectInfo.ProjectFile = path;
    std::vector<String> projectMountPoints = projectProps.GetStringArray(PROJECT_KEY_DIRS);
    if (projectInfo.ProjectName.empty() || projectInfo.ProjectSnapshot.ID.empty() || projectInfo.ProjectSnapshot.Platform.empty())
    {
        TTE_LOG("WARNING: The project '%s' is corrupted", path.string().c_str());
        return {};
    }
    for(const String& mp : projectMountPoints)
    {
        std::filesystem::path p = std::filesystem::absolute(std::filesystem::path(mp));
        if(std::filesystem::is_directory(p))
        {
            projectInfo.MountDirectories.push_back(std::move(p));
        }
        else
        {
            TTE_LOG("WARNING: In project '%s'  the mount point '%s' does not exist anymore.", p.string().c_str());
        }
    }
    if(projectInfo.MountDirectories.empty())
    {
        TTE_LOG("WARNING: In project '%s' there are no mount points. The user will have to add more to be able to locate files.", projectInfo.ProjectName.c_str());
    }
    return projectInfo;
}

void ProjectManager::LoadFromWorkspace()
{
    std::vector<String> previousPaths = _WorkspaceProperties.GetStringArray(PROJECT_KEY_PREVIOUS_PROJECTS);
    for (const String& path : previousPaths)
    {
        if (!StringEndsWith(path, PROJECT_EXT))
        {
            TTE_LOG("WARNING: The project path '%s' is not a telltale editor project. Ignoring.", path.c_str());
            continue;
        }
        std::filesystem::path p{ path };
        if (std::filesystem::is_regular_file(p))
        {
            TTEProperties projectProps{};
            projectProps.Load(std::filesystem::absolute(p).string());
            TTEProject projectInfo = _LoadProject(projectProps, std::filesystem::absolute(p));
            if (projectInfo.ProjectName.length() > 0)
                _PreviousProjects.push_back(std::move(projectInfo));
        }
        else
        {
            TTE_LOG("WARNING: The project path '%s' does not exist anymore. Removing and ignoring.", std::filesystem::absolute(p).string().c_str());
        }
    }
    String activeProj = _WorkspaceProperties.GetString(PROJECT_KEY_ACTIVE_PROJECT, "");
    if (!activeProj.empty())
    {
        std::filesystem::path p{ activeProj };
        if (std::filesystem::is_regular_file(p))
        {
            TTEProperties projectProps{};
            projectProps.Load(std::filesystem::absolute(p).string());
            TTEProject projectInfo = _LoadProject(projectProps, std::filesystem::absolute(p));
            if (projectInfo.ProjectName.length() > 0)
                _ActiveProject = std::move(projectInfo);
            else
            {
                PlatformMessageBoxAndWait("Project Load Error", "The active project at " + p.string() + " could not be loaded or is corrupt.");
            }
        }
        else
        {
            PlatformMessageBoxAndWait("Project Load Error", "The active project at " + p.string() + " does not exist anymore!");
        }
    }
}

ProjectManager::ProjectManager(TTEProperties& workspaceProps) : _WorkspaceProperties(workspaceProps)
{
}

void ProjectManager::SaveToWorkspace()
{
    String value = "";
    TTEProject* head = GetHeadProject();
    if(head)
    {
        value = head->ProjectFile.string();
    }
    _WorkspaceProperties.SetString(PROJECT_KEY_ACTIVE_PROJECT, value);
    _WorkspaceProperties.Remove(PROJECT_KEY_PREVIOUS_PROJECTS);
    for (const auto& p : _PreviousProjects)
        _WorkspaceProperties.AddArray(PROJECT_KEY_PREVIOUS_PROJECTS, p.ProjectFile.string());
    if(head)
    {
        TTEProperties props{ _ActiveProject.ProjectFile.string() };
        _SaveProject(props, _ActiveProject);
        props.Save();
    }
}

void ProjectManager::RetrievePreviousProjects(std::vector<String>& outProjectNames)
{
    for(const auto& prev: _PreviousProjects)
    {
        outProjectNames.push_back(prev.ProjectName);
    }
}

TTEProject* ProjectManager::GetHeadProject()
{
    return _ActiveProject.ProjectName.empty() ? nullptr : &_ActiveProject;
}

Bool ProjectManager::SetProjectDisk(const String& filePath)
{
    std::filesystem::path p{ filePath };
    if (std::filesystem::is_regular_file(p))
    {
        TTEProperties props{p.string()};
        TTEProject proj = _LoadProject(props, p);
        if(!proj.ProjectName.empty())
        {
            if(GetHeadProject() && CompareCaseInsensitive(GetHeadProject()->ProjectName, proj.ProjectName))
            {
                _ActiveProject = std::move(proj); // replace by newer version
                return true;
            }
            else
            {
                I32 index = 0;
                for(const auto& prev: _PreviousProjects)
                {
                    if(CompareCaseInsensitive(prev.ProjectName, proj.ProjectName))
                    {
                        _PreviousProjects.erase(_PreviousProjects.begin() + index);
                        break;
                    }
                    index++;
                }
                // if active, save it and addd to prevs
                if (GetHeadProject())
                {
                    TTEProperties props{ _ActiveProject.ProjectFile.string() };
                    _SaveProject(props, _ActiveProject);
                    props.Save();
                    _PreviousProjects.push_back(std::move(_ActiveProject));
                }
                _ActiveProject = std::move(proj);
                return true;
            }
        }
        else return false;
    }
    TTE_LOG("WARNING: Project path %s could not be found", filePath.c_str());
    return false;
}

void ProjectManager::SetProject(const String& name)
{
    if(CompareCaseInsensitive(_ActiveProject.ProjectName, name))
        return;
    TTEProject prev{};
    I32 index = -1;
    I32 i = -1;
    for (auto& prevProj : _PreviousProjects)
    {
        i++;
        if (CompareCaseInsensitive(name, prevProj.ProjectName))
        {
            index = i;
            prev = std::move(prevProj);
            break;
        }
    }
    if (index == -1)
    {
        TTE_LOG("WARNING: At SetProject: the project %s was not found as a previous project", name.c_str());
    }
    else
    {
        if(_ActiveProject.ProjectName.length())
        {
            TTEProperties props{_ActiveProject.ProjectFile.string()};
            _SaveProject(props, _ActiveProject);
            props.Save();
        }
        _PreviousProjects.erase(_PreviousProjects.begin() + index);
        _PreviousProjects.push_back(std::move(_ActiveProject));
        _ActiveProject = std::move(prev);
    }
    SaveToWorkspace(); // resave
}

void ProjectManager::CreateProject(TTEProject project)
{
    Bool bExists = CompareCaseInsensitive(_ActiveProject.ProjectName, project.ProjectName);
    if(!bExists)
    {
        for (const auto& project : _PreviousProjects)
        {
            if (CompareCaseInsensitive(project.ProjectName, project.ProjectName))
            {
                bExists = true;
                break;
            }
        }
    }
    if(bExists)
    {
        TTE_LOG("WARNING: At CreateProject the project already exists.");
    }
    else
    {
        String name = project.ProjectName;
        _PreviousProjects.push_back(std::move(project));
        SetProject(name);
    }
}

TTEProject ProjectManager::GetPreviousProject(const String& name)
{
    for(const auto& project: _PreviousProjects)
    {
        if(CompareCaseInsensitive(project.ProjectName,name))
            return project;
    }
    return {};
}

