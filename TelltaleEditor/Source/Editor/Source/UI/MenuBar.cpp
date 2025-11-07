#include <UI/ApplicationUI.hpp>
#include <nfd.h>
#include <imgui.h>

DECL_VEC_ADDITION();

MenuBar::MenuBar(EditorUI& ui) : UIComponent(ui.GetApplication()), _Editor(ui) {}

static constexpr Float MENU_ITEM_HEIGHT = 24.0f;
static constexpr Float MENU_ITEM_SPACING = 4.0f;

static Bool _RenderMenuItem(MenuBar* mb, const String& langID, CString iconTex, Float& xBack, Float wSizeX, Float wSizeY)
{
    xBack -= (MENU_ITEM_HEIGHT + MENU_ITEM_SPACING);
    mb->DrawResourceTexture(iconTex, xBack / wSizeX, 9.f / wSizeY, MENU_ITEM_HEIGHT / wSizeX, MENU_ITEM_HEIGHT / wSizeY);
    Bool hov = ImGui::IsMouseHoveringRect(ImVec2(xBack, 9.0f), ImVec2(xBack + MENU_ITEM_HEIGHT, 9.0f + MENU_ITEM_HEIGHT));
    Bool cl = false;
    if(hov)
    {
        cl = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        ImGui::SetTooltip("%s", mb->_MyUI.GetLanguageText(langID.c_str()).c_str());
    }
    return cl;
}

static Bool _AsyncScriptExec(const JobThread& thread, void* userA, void* userB)
{
    String src = std::move(*((String*)userA));
    String name = std::move(*((String*)userB));
    TTE_FREE((String*)userA); TTE_FREE((String*)userB); // free input arg
    Bool bResult = false;
    
    if((bResult = ScriptManager::LoadChunk(thread.L, name, src)))
    {
        thread.L.CallFunction(0, 0, true);
    }
    
    return bResult;
}

void MenuBar::Render()
{

    if (ImGui::BeginMainMenuBar())
    {
        _ImGuiMenuHeight = ImGui::GetWindowSize().y;
        if (ImGui::BeginMenu("Game"))
        {
            // OPEN PROJECT, RECENT, CHECK FOR UPDATES, REPORT A BUG, OUTPUT, TIMESTEP
            // USER SETTINGS, CONVERT, LOCALIZATIONS, UPDATE PREFERENCES, PACKAGES, WIZARDS, 
            // CREATE ARM FILES, .., SAVE GAME, LOAD GAME, QUIT
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open","Ctrl+O"))
            {
                _Editor.UserRequestOpenFile();
            }
            // New, Open File, Open
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Editor"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scripts"))
        {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scene"))
        {
            // CREATE SCENE, OPEN SCENE, ADD SCENE, CLEAN SCENE, RECENTS
            ImGui::EndMenu();
        }
        // SCENE, SCRIPTS, CHOREOGRAPHY, AUDIO
        // all most file menus just have open XXX, create XXX + recents
        ImGui::EndMainMenuBar();
    }

    // SHORTCUTS
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyReleased(ImGuiKey_O))
        {
            _Editor.UserRequestOpenFile();
        }
    }

    // BEGIN
    SetNextWindowViewport(0.0f, 0.0f, 0, 0, 1.0f, 0.04f, 0, 40, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.0f, 0.0f, 0.0f, 1.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("#menubar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
                                    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);

    // BACKGROUND
    ImVec2 size = ImGui::GetWindowSize();
    ImVec2 pos = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pos, pos + ImVec2{ size.x * 0.5f, size.y }, IM_COL32(120, 14, 57,255), 0xff000000u, 0xff000000u, IM_COL32(120, 14, 57, 255));
    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(pos + ImVec2{ size.x, 0.0f }, pos + ImVec2{ size.x * 0.5f, size.y }, IM_COL32(120, 14, 57, 255), 0xff000000u, 0xff000000u, IM_COL32(120, 14, 57, 255));

    // LOGO AND TITLE BAR
    Float xBack = size.x - 4.0f;
    DrawResourceTexture("LogoSquare.png", 2.f / size.x, 2.f / size.y, 36.f / size.x, 36.f / size.y);
    const String title = "Telltale Editor";
    ImGui::PushFont(GetApplication().GetEditorFont(), 20.0f);
    ImGui::SetCursorPos({ 50.0f, size.y * 0.5f - ImGui::CalcTextSize(title.c_str()).y * 0.5f });
    ImGui::TextUnformatted(title.c_str());
    ImGui::PopFont();
    ImGui::PushFont(GetApplication().GetEditorFont(), 10.0f);
    String proj = GetApplication().GetProjectManager().GetHeadProject()->ProjectName + " [" + GetApplication().GetProjectManager().GetHeadProject()->ProjectFile.filename().string() + "]";
    if(!_Editor.GetActiveScene().GetName().empty())
    {
        proj = proj + " | " + _Editor.GetActiveScene().GetName();
    }
    else if(!_Editor._LoadingAsyncScene.empty())
    {
        proj = proj + " | Loading " + _Editor._LoadingAsyncScene + "...";
    }
    else
    {
        proj = proj + " | No Scene Loaded";
    }
    ImVec2 projNameSize = ImGui::CalcTextSize(proj.c_str());
    ImGui::SetCursorPos({ size.x * 0.5f - 0.5f * projNameSize.x, size.y * 0.5f - projNameSize.y * 0.5f });
    ImGui::TextUnformatted(proj.c_str());
    ImGui::PopFont();
    
    if(_RenderMenuItem(this, "menu.run_script", "Misc/RunScript.png", xBack, size.x, size.y))
    {
        nfdchar_t* outp{};
        if (NFD_OpenDialog("lua", NULL, &outp) == NFD_OKAY)
        {
            DataStreamRef source = DataStreamManager::GetInstance()->CreateFileStream(String(outp));
            if(source)
            {
                String* src = TTE_NEW(String, MEMORY_TAG_TEMPORARY_ASYNC);
                String* nm = TTE_NEW(String, MEMORY_TAG_TEMPORARY_ASYNC);
                *src = DataStreamManager::GetInstance()->ReadAllAsString(source);
                *nm = FileGetName(String(outp));
                JobDescriptor desc{};
                desc.AsyncFunction = &_AsyncScriptExec;
                desc.UserArgA = src;
                desc.UserArgB = nm;
                JobScheduler::Instance->Post(desc);
            }
            else
            {
                TTE_LOG("Cannot open %s: open failed", outp);
            }
            free(outp);
        }
    }

    // END
    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
}
