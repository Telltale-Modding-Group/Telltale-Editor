#include <UI/ApplicationUI.hpp>
#include <Meta/Meta.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <nfd.h>
#include <filesystem>

using namespace ImGui;

static GameSnapshot _AsyncDoGuess(DataStreamRef fileStream, U8*& Buffer, U32& BufferSize)
{
    if(fileStream->GetSize() > 1024 * 1024 * 30 || fileStream->GetSize() == 0)
        return {}; // 30 MB cap on executables, at least that we know of.
    if(fileStream->GetSize() > BufferSize)
    {
        if(Buffer)
            TTE_FREE(Buffer);
        Buffer = TTE_ALLOC(fileStream->GetSize(), MEMORY_TAG_TEMPORARY);
        BufferSize = (U32)fileStream->GetSize();
    }
    fileStream->Read(Buffer, fileStream->GetSize());
    U64 hash = CRC64(Buffer, BufferSize, 0);
    for(const auto& game: Meta::GetInternalState().Games)
    {
        for(const auto& entry: game.ExecutableHash)
        {
            if(entry.first == hash)
            {
                // Found !
                return GameSnapshot{game.ID, entry.second.first, entry.second.second};
            }
        }
    }
    return {};
}

static String _ProcessInfoPlistBundle(const String& xml)
{
    const String keyTag = "<key>CFBundleExecutable</key>";
    const String stringOpen = "<string>";
    const String stringClose = "</string>";

    size_t keyPos = xml.find(keyTag);
    if (keyPos == String::npos) return "";

    size_t stringStart = xml.find(stringOpen, keyPos);
    if (stringStart == String::npos) return "";

    stringStart += stringOpen.length();

    size_t stringEnd = xml.find(stringClose, stringStart);
    if (stringEnd == String::npos) return "";

    return xml.substr(stringStart, stringEnd - stringStart);
}

static GameSnapshot _ProcessPlaystation_1_2_Config(ISO9660& iso, U8*& Buffer, U32& BufferSize)
{
    String _{};
    DataStreamRef ps12Config = iso.Find("SYSTEM.CNF", _); // PS1/PS2 config file contains the BOOT name
    if (ps12Config)
    {
        ps12Config->SetPosition(8); // skip 'BOOT2 = '
        char Data[64];
        U8* dpos = (U8*)Data;
        memset(Data, 0, 64);
        while (ps12Config->Read(dpos, 1) && *dpos != 0xD && *dpos != 0xA && (char*)dpos != Data + 63)
        {
            dpos++;
        }
        *dpos = 0;
        String executable = Data;
        if (StringStartsWith(executable, "cdrom0:\\"))
        {
            executable = executable.substr(8);
            I32 sc = (I32)executable.find_first_of(';');
            if (sc != String::npos)
                executable = executable.substr(0, sc);
            executable = StringTrim(executable);
            DataStreamRef exec = iso.Find(executable, _);
            GameSnapshot ss = {};
            if (exec)
                ss = _AsyncDoGuess(exec, Buffer, BufferSize);
            if (!ss.ID.empty())
            {
                return ss;
            }
        }
    }
    return {};
}

static GameSnapshot _ProcessPlaystation_3_4(ISO9660& iso, U8*& Buffer, U32& BufferSize)
{
    String _{};
    std::set<String> files{};
    iso.GetFiles(files);
    for(const auto& file: files)
    {
        if(CompareCaseInsensitive(FileGetFilename(file), "EBOOT.BIN"))
        {
            DataStreamRef exec = iso.Find(file, _);
            GameSnapshot ss = {};
            if (exec)
                ss = _AsyncDoGuess(exec, Buffer, BufferSize);
            if (!ss.ID.empty())
            {
                return ss;
            }
        }
    }
    return {};
}

static GameSnapshot _ProcessFolder(std::filesystem::path folder, U8*& Buffer, U32& BufferSize)
{
    using namespace std::filesystem;
    if (StringEndsWith(ToLower(absolute(folder).string()), ".app"))
    {
        // Find info.plist, read from that
        path infoXML = absolute(folder) / "Contents/Info.plist"; // mac
        if(!is_regular_file(infoXML))
            infoXML = absolute(folder) / "Info.plist"; // ios
        if(is_regular_file(infoXML))
        {
            DataStreamRef xmlStream = DataStreamManager::GetInstance()->CreateFileStream(infoXML.string());
            String xml = DataStreamManager::GetInstance()->ReadAllAsString(xmlStream);
            String bundleExecutable = _ProcessInfoPlistBundle(xml);
            path executablePath{};
            if(bundleExecutable.length())
            {
                Bool bFound = false;
                if (executablePath = absolute(folder) / "Contents/MacOS/" / bundleExecutable, is_regular_file(executablePath)) // MacOS
                {
                    bFound = true;
                }
                else if (executablePath = absolute(folder) / bundleExecutable, is_regular_file(executablePath)) // iOS
                {
                    bFound = true;
                }
                if(bFound)
                {
                    GameSnapshot ss = _AsyncDoGuess(DataStreamManager::GetInstance()->CreateFileStream(executablePath.string()), Buffer, BufferSize);
                    if(!ss.ID.empty())
                    {
                        return ss;
                    }
                }
            }
        }
    }
    return {};
}

Bool UIProjectCreate::AsyncSnapshotGuess(const JobThread& thread, void* pArg1, void* pArg2)
{
    using namespace std::filesystem;
    Ptr<SnapshotGuessState> guessState = ((SnapshotGuessState*)pArg1)->shared_from_this();
    guessState->OnExecute->store(true, std::memory_order_relaxed);

    U8* Buffer = nullptr;
    U32 BufferSize = 0;
    Bool Result = false;

    for(const auto& mp: guessState->Mounts)
    {
        if(is_regular_file(path(mp)))
        {
            if(StringEndsWith(mp, ".iso", true))
            {
                ISO9660 iso{};
                DataStreamRef ref = DataStreamManager::GetInstance()->CreateFileStream(absolute(path(mp)).string());
                if(iso.SerialiseIn(ref))
                {
                    GameSnapshot ss{};
                    ss = _ProcessPlaystation_1_2_Config(iso, Buffer, BufferSize);
                    if(!ss.ID.empty())
                    {
                        Result = true;
                        guessState->ResolvedSnapshot = ss;
                        break;
                    }
                    ss = _ProcessPlaystation_3_4(iso, Buffer, BufferSize);
                    if (!ss.ID.empty())
                    {
                        Result = true;
                        guessState->ResolvedSnapshot = ss;
                        break;
                    }
                    // try other console locators... TODO
                }
            }
        }
        else if(StringEndsWith(mp, ".pk2"))
        {
            guessState->ResolvedSnapshot = {"CSI3","PS2",""}; // one exception for this file!
            Result = true;
            break;
        }
        else if(is_directory(path(mp)))
        {
            GameSnapshot ss = _ProcessFolder(path(mp), Buffer, BufferSize);
            if(!ss.ID.empty())
            {
                guessState->ResolvedSnapshot = ss;
                Result = true;
                break;
            }
            for (const directory_entry& entry : recursive_directory_iterator(mp))
            {
                if (entry.is_regular_file())
                {
                    String fileName = entry.path().filename().string();
                    String ext = ToLower(FileGetExtension(fileName));
                    if (CompareCaseInsensitive(fileName, "EBOOT.BIN") || CompareCaseInsensitive(ext, "xex") ||
                        CompareCaseInsensitive(ext, "exe") || CompareCaseInsensitive(ext, "so") || CompareCaseInsensitive(ext, "mll") ||
                       (ext.empty() && StringContains(fileName, "GameApp", true)))
                    {
                        GameSnapshot ss = _AsyncDoGuess(DataStreamManager::GetInstance()->CreateFileStream(absolute(entry.path()).string()), Buffer, BufferSize);
                        if (!ss.ID.empty())
                        {
                            Result = true;
                            guessState->ResolvedSnapshot = ss;
                            break;
                        }
                    }
                }
                else if (entry.is_directory())
                {
                    GameSnapshot ss = _ProcessFolder(entry.path(), Buffer, BufferSize);
                    if(!ss.ID.empty())
                    {
                        guessState->ResolvedSnapshot = ss;
                        Result = true;
                        break;
                    }
                }
            }
        }
        if(Result)
            break;
    }

    if(Buffer)
        TTE_FREE(Buffer);
    
    return Result;
}

static Bool _IsParentPath(const std::filesystem::path& parent, const std::filesystem::path& child)
{
    auto canonicalParent = std::filesystem::weakly_canonical(parent);
    auto canonicalChild = std::filesystem::weakly_canonical(child);
    auto parentIt = canonicalParent.begin();
    auto childIt = canonicalChild.begin();
    for (; parentIt != canonicalParent.end(); ++parentIt, ++childIt)
    {
        if (childIt == canonicalChild.end() || *parentIt != *childIt)
            return false;
    }
    return true;
}

void UIProjectCreate::Reset()
{
    _CreationState = false;
    _Page = ProjectCreationPage::INFO;
    memset(_ProjectName, 0, 32);
    memset(_FileName, 0, 64);
    memset(_Description, 0, 256);
    memset(_Author, 0, 48);
    memcpy(_ProjectName, "My Project", strlen("My Project"));
    memcpy(_FileName, "My Project.tteproj", strlen("My Project.tteproj"));
    _SelectedGame = _SelectedPlatform = "";
    _SelectedVendor = "(Default)";
    _GuessedSnap = false;
    _ProjectLocation = "";
    _GuessJob = {};
    _MountPoints.clear();
    _SelectedGameIndex = 0;
    _ReleaseJob();
    if(_SchedulerNeedsShutdown && JobScheduler::Instance)
    {
        JobScheduler::Shutdown();
        _SchedulerNeedsShutdown = false;
    }
}

static void _AddMountPointFolder(std::vector<String>& _MountPoints, String path)
{
    String name = FileGetFilename(path);
    if(CompareCaseInsensitive(name, "data") || CompareCaseInsensitive(name, "archives") || CompareCaseInsensitive(name, "pack"))
    {
        PlatformMessageBoxAndWait("Incorrect directory", "The directory you entered looks like it's a game data folder! Please input the parent of this folder, "
                                  "the one which contains it, and has the executable inside (.exe etc)! This is required for us to detect your installation type.");
        return;
    }
    Bool exist = false, messaged = false;
    std::filesystem::path absp = std::filesystem::absolute(std::filesystem::path(path));
    for(auto it = _MountPoints.begin(); it != _MountPoints.end();)
    {
        if(CompareCaseInsensitive(*it, absp.string()))
        {
            exist = true;
            break;
        }
        if(std::filesystem::is_directory(std::filesystem::path(*it)))
        {
            if(_IsParentPath(std::filesystem::path{ *it }, absp))
            {
                if (messaged)
                    break;
                PlatformMessageBoxAndWait("Parent directory", "There are one or more existing mount directories you have entered already which are a parent directory of this directory. Those already cover the directory just selected.");
                messaged = true;
                exist = true; // skip intertion
                break;
            }
            else if(_IsParentPath(absp, std::filesystem::path{ *it }))
            {
                it = _MountPoints.erase(it);
                if(messaged)
                    continue;
                PlatformMessageBoxAndWait("Parent directory", "There are one or more existing mount directories you have entered already which are a sub-directory of this directory. The parent-most will be kept, as it includes all sub-directories.");
                messaged = true;
                continue;
            }
        }
        it++;
    }
    if(!exist)
    {
        _MountPoints.push_back(absp.string());
    }
}

void UIProjectCreate::_ReleaseJob()
{
    if(_GuessJob)
    {
        JobScheduler::Instance->Cancel(_GuessJob, false); // no queued anyway
    }
    _AsyncGuessState.reset();
    _GuessJob = {};
    _GuessStart = 0;
}

void UIProjectCreate::Render()
{
    {
        int w = 0, h = 0;
        SDL_GetWindowSize(GetApplication()._Window, &w, &h);
        U32 w0 = 0, h0 = 0;
        GetWindowSize(w0, h0);
        if (w != w0 || h != h0)
            GetApplication().SetWindowSize((int)w0, (int)h0);
    }
    if((SDL_GetWindowFlags(GetApplication()._Window) & SDL_WINDOW_RESIZABLE) != 0)
        SDL_SetWindowResizable(GetApplication()._Window, false);
    if(!JobScheduler::Instance)
    {
        JobScheduler::Initialise();
        _SchedulerNeedsShutdown = true;
    }

    ImVec2 winSize = GetWindowViewport()->Size;
    SetNextWindowSize(winSize);
    SetNextWindowPos({});
    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(50, 50, 50, 255));
    Begin("#page_payload", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                | ImGuiWindowFlags_NoDocking
                                | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 winPos = GetCursorScreenPos();
    Float sidePadding = winSize.x * 0.05f;
    Float topPadding = winSize.y * 0.05f;
    Float labelSpacing = 5.0f;
    Float itemSpacingY = 8.0f;
    Float contentX = winPos.x + sidePadding;
    Float contentWidth = winSize.x - sidePadding * 2.0f;
    Float cursorY = winPos.y + topPadding;

    if(_Page == ProjectCreationPage::INFO)
    {

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.title").c_str());

        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.name").c_str());
        cursorY += GetTextLineHeight() + labelSpacing;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        InputText("##name", _ProjectName, IM_ARRAYSIZE(_ProjectName));
        cursorY += GetItemRectSize().y + itemSpacingY;

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.description").c_str());
        cursorY += GetTextLineHeight() + labelSpacing;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        InputText("##desc", _Description, IM_ARRAYSIZE(_Description));
        cursorY += GetItemRectSize().y + itemSpacingY;

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.author").c_str());
        cursorY += GetTextLineHeight() + labelSpacing;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        InputText("##author", _Author, IM_ARRAYSIZE(_Author));
        cursorY += GetItemRectSize().y + itemSpacingY;

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.file_name").c_str());
        cursorY += GetTextLineHeight() + labelSpacing;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        InputText("##file", _FileName, IM_ARRAYSIZE(_FileName));
        cursorY += GetItemRectSize().y + itemSpacingY;

        Bool validFile = false;
        Bool nextable = false;
        {
            String fname = _FileName;
            String suffix = ".tteproj";
            if (fname.length() >= suffix.length() &&
                fname.compare(fname.length() - suffix.length(), suffix.length(), suffix) == 0)
            {
                validFile = true;
            }
            if (fname.length() <= 8)
                validFile = false;
        }
        if(_ProjectName[0])
            nextable = true;

        if (!validFile)
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            TextUnformatted(GetApplication().GetLanguageText("project_create.invalid_file").c_str());
            cursorY += GetTextLineHeightWithSpacing();
            PopStyleColor();
        }
        else  if (nextable)
        {
            PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            TextUnformatted(GetApplication().GetLanguageText("project_create.next").c_str());
            cursorY += GetTextLineHeightWithSpacing();
            PopStyleColor();
        }
        else
        {
            PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            TextUnformatted(GetApplication().GetLanguageText("project_create.enter_name").c_str());
            cursorY += GetTextLineHeightWithSpacing();
            PopStyleColor();
        }

        Float buttonWidth = 120.0f;
        Float buttonHeight = 0;
        ImVec2 buttonSize(buttonWidth, 0.0f);
        Float spacing = 10.0f;
        Float totalButtonsWidth = (2.0f * buttonWidth) + (1.0f * spacing);
        Float startX = contentX + (contentWidth - totalButtonsWidth) * 0.5f;

        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 0.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.cancel_button").c_str(), buttonSize))
        {
            Reset();
            _CreationState = true;
            GetApplication().PopUI();
            GetApplication().PushUI(TTE_NEW_PTR(UIProjectSelect, MEMORY_TAG_EDITOR_UI, GetApplication()));
        }
        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 1.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.next_button").c_str(), buttonSize))
        {
            _Page = ProjectCreationPage::LOCATION;
        }
    }
    else if(_Page == ProjectCreationPage::PROJECT_FOLDER_PICK)
    {

        // TITLE
        SetCursorScreenPos(ImVec2(contentX + 1.0f, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.pick_location_title").c_str());
        cursorY += GetTextLineHeightWithSpacing() * 2.0f + itemSpacingY;

        Bool bNext = false;
        String message = "";
        ImVec4 col{};

        if(!_ProjectLocation.empty() && std::filesystem::is_directory(std::filesystem::path{_ProjectLocation}))
        {
            if(std::filesystem::exists(std::filesystem::path{ _ProjectLocation } / _FileName))
            {
               message = GetApplication().GetLanguageText("project_create.exists");
               col = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            }
            else
            {
                if(JobScheduler::Instance->GetResult(_GuessJob) != JOB_RESULT_RUNNING)
                {
                    col = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                    message = GetApplication().GetLanguageText("project_create.next");
                    bNext = true;
                }
                else
                {
                    col = ImVec4(0.3f, 0.6f, 0.85f, 1.0f);
                    message = GetApplication().GetLanguageText("project_create.awaiting");
                }
            }
        }
        else
        {
            col = ImVec4{ 0.4f, 0.4f, 0.4f, 1.0f };
            message = GetApplication().GetLanguageText("project_create.location_prompt");
        }

        // SELECT BUTTON
        SetCursorScreenPos(ImVec2(MAX(0.0f, contentX - 1.0f), cursorY));
        if(Button(GetApplication().GetLanguageText(_ProjectLocation.empty() ? "project_create.select" : "project_create.change").c_str()))
        {
            nfdchar_t* outp{};
            if (NFD_PickFolder(NULL, &outp, 0) == NFD_OKAY)
            {
                String path = outp;
                std::filesystem::path absp = std::filesystem::absolute(std::filesystem::path(path));
                _ProjectLocation = absp.string();
                free(outp);
            }
        }
        cursorY += GetTextLineHeightWithSpacing() * 2.0f + itemSpacingY;

        // LOCATION TEXT
        if(bNext)
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            String fn = (std::filesystem::path{ _ProjectLocation } / _FileName).string();
            if (fn.length() > 50)
                fn = fn.substr(0, 30) + "..." + fn.substr(30);

            PushStyleColor(ImGuiCol_Text, ImVec4{ 0.4f, 0.4f, 0.4f, 1.0f });
            TextUnformatted(fn.c_str());
            PopStyleColor();
            cursorY += GetTextLineHeightWithSpacing() * 2.0f + itemSpacingY;
        }

        // MESSAGE
        Float buttonWidth = 120.0f;
        ImVec2 buttonSize(buttonWidth, 0.0f);
        Float spacing = 10.0f;
        Float totalButtonsWidth = (2.0f * buttonWidth) + (1.0f * spacing);
        Float startX = contentX + (contentWidth - totalButtonsWidth) * 0.5f;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        PushStyleColor(ImGuiCol_Text, col);
        PushTextWrapPos(winSize.x - sidePadding);
        TextUnformatted(message.c_str());
        PopTextWrapPos();
        PopStyleColor(1);
        cursorY += GetTextLineHeightWithSpacing();

        // BUTTONS
        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 0.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.back_button").c_str(), buttonSize))
        {
            _Page = ProjectCreationPage::LOCATION;
            _GuessedSnap = false;
            _GuessStart = 0;
        }
        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 1.0f, cursorY + 10.0f });
        Bool bTooLong = _GuessStart && GetTimeStampDifference(_GuessStart, GetTimeStamp()) > 10'000;
        if(bTooLong)
            bNext = true;
        PushItemFlag(ImGuiItemFlags_Disabled, !bNext);
        if (Button(GetApplication().GetLanguageText("project_create.next_button").c_str(), buttonSize))
        {
            if(bTooLong)
            {
                _AsyncGuessState->OnExecute->store(false, std::memory_order_relaxed);
                _ReleaseJob();
                _SelectedGame = _SelectedPlatform = "";
                _SelectedVendor = "(Default)";
                _SelectedGameIndex = 0;
                _GuessedSnap = false;
            }
            if(_AsyncGuessState && !_AsyncGuessState->ResolvedSnapshot.ID.empty())
            {
                _GuessedSnap = true;
                _SelectedGame = _AsyncGuessState->ResolvedSnapshot.ID;
                _SelectedPlatform = _AsyncGuessState->ResolvedSnapshot.Platform;
                _SelectedVendor = _AsyncGuessState->ResolvedSnapshot.Vendor;
                if(_SelectedVendor.empty())
                    _SelectedVendor = "(Default)";
                if (!_SelectedGame.empty())
                {
                    I32 index = 0;
                    for (const auto& g : Meta::GetInternalState().Games)
                    {
                        if (CompareCaseInsensitive(g.ID, _SelectedGame))
                        {
                            _SelectedGame = g.Name;
                            break;
                        }
                        index++;
                    }
                    _SelectedGameIndex = index >= Meta::GetInternalState().Games.size() ? 0 : index;
                }
                _ReleaseJob();
            }
            else
            {
                _GuessedSnap = false;
            }
            _Page = ProjectCreationPage::SNAPSHOT;
            _GuessStart = 0;
        }
        PopItemFlag();
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
        
        if(bTooLong) // 10 seconds is stupid!
        {
            PushStyleColor(ImGuiCol_Text, IM_COL32(200, 50, 50, 255));
            TextUnformatted(GetApplication().GetLanguageText("project_create.await_too_long").c_str());
            PopStyleColor();
        }

    }
    else if(_Page == ProjectCreationPage::LOCATION)
    {
        // TITLE
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.locations_title").c_str());
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
        SetCursorScreenPos(ImVec2(contentX, cursorY));

        // INFO
        PushStyleColor(ImGuiCol_Text, ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f });
        PushTextWrapPos(winSize.x - sidePadding);
        TextUnformatted(GetApplication().GetLanguageText("project_create.location_info").c_str());
        cursorY += GetTextLineHeightWithSpacing() * 3.0f + itemSpacingY;
        PopStyleColor(1);
        PopTextWrapPos();

        // ADD ARCHIVES
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        if(Button(GetApplication().GetLanguageText("project_create.add_archives").c_str()))
        {
            nfdchar_t* outp{};
            if(NFD_PickFolder(NULL, &outp, 0) == NFD_OKAY)
            {
                String path = outp;
                _AddMountPointFolder(_MountPoints, path);
                free(outp);
            }
        }
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        // ADD ISO
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        if (Button(GetApplication().GetLanguageText("project_create.add_iso").c_str()))
        {
            nfdchar_t* outp{};
            if (NFD_OpenDialog("iso", NULL, &outp) == NFD_OKAY)
            {
                String path = outp;
                Bool exist = false;
                std::filesystem::path absp = std::filesystem::absolute(std::filesystem::path(path));
                for (const auto& ep : _MountPoints)
                {
                    if (CompareCaseInsensitive(ep, absp.string()))
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    _MountPoints.push_back(absp.string());
                }
                free(outp);
            }
        }
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        // ADD PK2
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        if (Button(GetApplication().GetLanguageText("project_create.add_pk2").c_str()))
        {
            nfdchar_t* outp{};
            if (NFD_OpenDialog("pk2", NULL, &outp) == NFD_OKAY)
            {
                String path = outp;
                Bool exist = false;
                std::filesystem::path absp = std::filesystem::absolute(std::filesystem::path(path));
                for (const auto& ep : _MountPoints)
                {
                    if (CompareCaseInsensitive(ep, absp.string()))
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    _MountPoints.push_back(absp.string());
                }
                free(outp);
            }
        }
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        // ADD APP
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        if (Button(GetApplication().GetLanguageText("project_create.add_app").c_str()))
        {
            nfdchar_t* outp{};
            if(NFD_PickFolder(NULL, &outp, NFD_FOLDER_MAC_ONLY_APP_FOLDERS) == NFD_OKAY)
            {
                String path = outp;
                _AddMountPointFolder(_MountPoints, path);
                free(outp);
            }
        }
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
        
        I32 mpIndex = 0;
        for (auto it = _MountPoints.begin(); it != _MountPoints.end(); )
        {
            PushID(mpIndex);
            SetCursorScreenPos(ImVec2(contentX, cursorY));

            String fn = *it;
            if (fn.length() > 50)
                fn = fn.substr(0, 30) + "..." + fn.substr(30);

            PushStyleColor(ImGuiCol_Text, ImVec4{ 0.4f, 0.4f, 0.4f, 1.0f });
            TextUnformatted( fn.c_str());
            PopStyleColor();

            SetCursorScreenPos(ImVec2(contentX + CalcTextSize(fn.c_str()).x + 10.f, cursorY));

            if (Button(GetApplication().GetLanguageText("project_create.remove").c_str()))
            {
                it = _MountPoints.erase(it);
            }
            else
            {
                ++it;
            }
            cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
            PopID();
            mpIndex++;
        }

        Float buttonWidth = 120.0f;
        Float buttonHeight = 0;
        ImVec2 buttonSize(buttonWidth, 0.0f);
        Float spacing = 10.0f;
        Float totalButtonsWidth = (2.0f * buttonWidth) + (1.0f * spacing);
        Float startX = contentX + (contentWidth - totalButtonsWidth) * 0.5f;

        if (!_MountPoints.empty())
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            TextUnformatted(GetApplication().GetLanguageText("project_create.next").c_str());
            cursorY += GetTextLineHeightWithSpacing();
            PopStyleColor();
        }
        else
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            TextUnformatted(GetApplication().GetLanguageText("project_create.enter_more").c_str());
            cursorY += GetTextLineHeightWithSpacing();
            PopStyleColor();
        }

        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 0.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.back_button").c_str(), buttonSize))
        {
            _Page = ProjectCreationPage::INFO;
        }
        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 1.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.next_button").c_str(), buttonSize) && !_MountPoints.empty())
        {
            _Page = ProjectCreationPage::PROJECT_FOLDER_PICK;
            _ReleaseJob(); 
            std::atomic_bool executed = false;
            _AsyncGuessState = TTE_NEW_PTR(SnapshotGuessState, MEMORY_TAG_TEMPORARY_ASYNC);
            _AsyncGuessState->ResolvedSnapshot = {};
            _AsyncGuessState->Mounts = _MountPoints; // copy
            _AsyncGuessState->OnExecute = &executed;
            JobDescriptor desc{};
            desc.AsyncFunction = &AsyncSnapshotGuess;
            desc.Priority = JOB_PRIORITY_NORMAL;
            desc.UserArgA = _AsyncGuessState.get();
            _GuessJob = JobScheduler::Instance->Post(desc);
            _GuessStart = GetTimeStamp();
            while(!executed.load(std::memory_order_relaxed))
            {
                ThreadSleep(5); // wait 5 ms to kick off other thread. hacky way but we need to let it take on the shared pointer
            }
        }

    }
    else if(_Page == ProjectCreationPage::SNAPSHOT)
    {

        // TITLE
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.snapshot_title").c_str());
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        // INFO
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        PushStyleColor(ImGuiCol_Text, ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f });
        PushTextWrapPos(winSize.x - sidePadding);;
        TextUnformatted(GetApplication().GetLanguageText("project_create.snapshot_info").c_str());
        cursorY += GetTextLineHeightWithSpacing() * 3.0f + itemSpacingY;
        PopStyleColor(1);
        PopTextWrapPos();

        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.id_combo").c_str());
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;

        // ID COMBO
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        if(BeginCombo("##snapshot", _SelectedGame.c_str()))
        {
            I32 i = 0;
            for(const auto& game : Meta::GetInternalState().Games)
            {
                Bool bSelected = _SelectedGame == game.Name;
                if(Selectable(game.Name.c_str(), &bSelected))
                {
                    _SelectedPlatform = "";
                    _SelectedVendor = "(Default)";
                    _SelectedGame = game.Name;
                    _SelectedGameIndex = i;
                }
                if(bSelected)
                    SetItemDefaultFocus();
                i++;
            }
            EndCombo();
        }

        // PLATFORM COMBO
        cursorY = GetCursorScreenPos().y + itemSpacingY;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.platform_combo").c_str());
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        if (BeginCombo("##platform", _SelectedPlatform.c_str()))
        {
            if(_SelectedGame.length())
            {
                I32 i = 0;
                for (const auto& plat: Meta::GetInternalState().Games[_SelectedGameIndex].ValidPlatforms)
                {
                    PushID(i++);
                    Bool bSelected = _SelectedPlatform == plat;
                    if (Selectable(plat.c_str(), &bSelected))
                    {
                        _SelectedPlatform = plat;
                        _SelectedVendor = "(Default)";
                    }
                    if (bSelected)
                        SetItemDefaultFocus();
                    PopID();
                }
            }
            EndCombo();
        }

        // VENDOR COMBO
        cursorY = GetCursorScreenPos().y + itemSpacingY;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        TextUnformatted(GetApplication().GetLanguageText("project_create.vendor_combo").c_str());
        cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
        SetCursorScreenPos(ImVec2(contentX, cursorY));
        SetNextItemWidth(contentWidth);
        if (BeginCombo("##vendor", _SelectedVendor.c_str()))
        {
            if (!_SelectedGame.length() || Meta::GetInternalState().Games[_SelectedGameIndex].ValidVendors.empty() || Meta::GetInternalState().Games[_SelectedGameIndex].ValidVendors[0].empty())
            {
                _SelectedVendor = "(Default)";
                Selectable("(Default)", true);
            }
            else
            {
                I32 i = 0;
                for (const auto& v : Meta::GetInternalState().Games[_SelectedGameIndex].ValidVendors)
                {
                    PushID(i++);
                    Bool bSelected = _SelectedVendor == v;
                    if (Selectable(v.c_str(), &bSelected))
                        _SelectedVendor = v;;
                    if (bSelected)
                        SetItemDefaultFocus();
                    PopID();
                }
            }
            EndCombo();
        }
        cursorY = GetCursorScreenPos().y + itemSpacingY;

        if(_GuessedSnap)
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            PushTextWrapPos(winSize.x - sidePadding);
            PushStyleColor(ImGuiCol_Text, ImVec4{ 0.4f, 0.5f, 0.4f, 1.0f });
            TextUnformatted(GetApplication().GetLanguageText("project_create.was_guessed").c_str());
            cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
            PopTextWrapPos();
            PopStyleColor(1);
        }
        else
        {
            SetCursorScreenPos(ImVec2(contentX, cursorY));
            PushTextWrapPos(winSize.x - sidePadding);
            PushStyleColor(ImGuiCol_Text, ImVec4{ 0.5f, 0.4f, 0.4f, 1.0f });
            TextUnformatted(GetApplication().GetLanguageText("project_create.not_guessed").c_str());
            cursorY += GetTextLineHeightWithSpacing() + itemSpacingY;
            PopTextWrapPos();
            PopStyleColor(1);
        }

        // BUTTONS
        Float buttonWidth = 120.0f;
        Float buttonHeight = 0;
        ImVec2 buttonSize(buttonWidth, 0.0f);
        Float spacing = 10.0f;
        Float totalButtonsWidth = (2.0f * buttonWidth) + (1.0f * spacing);
        Float startX = contentX + (contentWidth - totalButtonsWidth) * 0.5f;

        SetCursorScreenPos({startX + (buttonWidth + spacing) * 0.0f, cursorY + 10.0f});
        if (Button(GetApplication().GetLanguageText("project_create.back_button").c_str(), buttonSize))
        {
            _Page = ProjectCreationPage::PROJECT_FOLDER_PICK;
            _ReleaseJob();
            _SelectedGame = _SelectedPlatform = "";
            _SelectedVendor = "(Default)";
            _SelectedGameIndex = 0;
            _GuessedSnap = false;
        }
        SetCursorScreenPos({ startX + (buttonWidth + spacing) * 1.0f, cursorY + 10.0f });
        if (Button(GetApplication().GetLanguageText("project_create.create_button").c_str(), buttonSize))
        {
            TTEProject proj{};
            proj.ProjectName = _ProjectName;
            proj.ProjectFile = std::filesystem::path(_ProjectLocation) / _FileName;
            proj.ProjectDescription = _Description;
            proj.ProjectAuthor = _Author;
            for(const auto& game: Meta::GetInternalState().Games)
            {
                if(CompareCaseInsensitive(game.Name, _SelectedGame))
                {
                    proj.ProjectSnapshot.ID = game.ID;
                    break;
                }
            }
            proj.ProjectSnapshot.Platform = _SelectedPlatform;
            proj.ProjectSnapshot.Vendor = _SelectedVendor == "(Default)" ? "" : _SelectedVendor;
            for(const auto& mp: _MountPoints)
            {
                proj.MountDirectories.push_back(std::filesystem::path{mp});
            }
            GetApplication().GetProjectManager().CreateProject(std::move(proj));
            GetApplication()._OnProjectLoad();
            Reset();  // Done!
            GetApplication().PopUI();
        }

    }
    else
    {
        Text("<ERROR>");
    }

    End();
    PopStyleColor();
}

void UIProjectCreate::GetWindowSize(U32& w, U32& h)
{
    w = (1280 / 5) << 1;
    h = (900 / 5) << 1; // 2/5
}
