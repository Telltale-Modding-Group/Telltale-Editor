#include <UI/UIBase.hpp>
#include <UI/ApplicationUI.hpp>
#include <UI/EditorUI.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <nfd.h>

using namespace ImGui;

DECL_VEC_ADDITION();
DECL_VEC_DOT();

UIProjectSelect::UIProjectSelect(ApplicationUI& app) : UIStackable(app), _Application(app)
{
    TTEProject* proj = _Application.GetProjectManager().GetHeadProject();
    if (proj)
        _Recents.push_back({ proj->ProjectName, proj->ProjectFile.string() });
    std::vector<String> prevs{};
    _Application.GetProjectManager().RetrievePreviousProjects(prevs);
    for (const String& prev : prevs)
        _Recents.push_back({ prev, _Application.GetProjectManager().GetPreviousProject(prev).ProjectFile.string() });
}

void UIProjectSelect::Reset()
{
    _SelectionState = false;
}

void UIProjectSelect::Render()
{
    {
        int w = 0, h = 0;
        SDL_GetWindowSize(_Application._Window, &w, &h);
        U32 w0 = 0, h0 = 0;
        GetWindowSize(w0, h0);
        if(w != w0 || h != h0)
            GetApplication().SetWindowSize((int)w0, (int)h0);
    }
    _LangBoxSelect = _Application.GetLanguage();
    ImVec2 winSize = ImGui::GetMainViewport()->Size;
    Float rhsPanelPixels = SELECT_SIZE(winSize.x, 0.25f, 100.0f, 400.0f);
    SetNextWindowViewportPixels(0,0, winSize.x - rhsPanelPixels, winSize.y);
    winSize = { winSize.x - rhsPanelPixels, winSize.y};

    // WELCOME SCREEN LHS PANEL
    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(50, 50, 50, 255));
    Begin("#welcome", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
                                | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse);
    const String title = _Application.GetLanguageText("project_select.title"), subtitle = _Application.GetLanguageText("project_select.subtitle"), recents = _Application.GetLanguageText("project_select.recents");
    const String projNew = _Application.GetLanguageText("project_select.new"), projExist = _Application.GetLanguageText("project_select.exist");

    // TITLE
    PushFont(_Application.GetEditorFont(), 20.0f);
    ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 titlePos;
    titlePos.x = winSize.x * 0.5f - titleSize.x * 0.5f;
    titlePos.y = winSize.y * 0.35f - titleSize.y * 0.5f - 5.0f;
    ImVec2 titleMin = ImGui::GetCursorScreenPos();
    titleMin.x = titlePos.x + ImGui::GetWindowPos().x;
    titleMin.y = titlePos.y + ImGui::GetWindowPos().y;
    ImVec2 titleMax = titleMin + titleSize;
    Bool titleHovered = ImGui::IsMouseHoveringRect(titleMin, titleMax);
    if(titleHovered)
        PushStyleColor(ImGuiCol_Text, IM_COL32(200,200,200,255));
    ImGui::SetCursorPos(titlePos);
    ImGui::TextUnformatted(title.c_str());
    ImGui::PopFont();
    if(titleHovered)
        PopStyleColor(1);

    // SUBTITLE
    ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle.c_str());
    ImVec2 subtitlePos;
    subtitlePos.x = winSize.x * 0.5f - subtitleSize.x * 0.5f;
    subtitlePos.y = winSize.y * 0.35f + titleSize.y * 0.5f + 5.0f;
    ImVec2 subtitleMin = subtitlePos + ImGui::GetWindowPos();
    ImVec2 subtitleMax = subtitleMin + subtitleSize;
    Bool subtitleHovered = ImGui::IsMouseHoveringRect(subtitleMin, subtitleMax);
    ImGui::SetCursorPos(subtitlePos);
    if (subtitleHovered)
        PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
    ImGui::TextUnformatted(subtitle.c_str());
    if (subtitleHovered)
        PopStyleColor(1);

    // NEW / EXISTING PROJECT BOOTONS
    Float minRenderSize = 70.0f;
    Float scale = winSize.x * 0.25f;
    Float buttonPixelSize = ImClamp(scale, minRenderSize, 190.0f);
    Float buttonWidth = buttonPixelSize;
    Float buttonHeight = buttonPixelSize;
    if (!(winSize.x < minRenderSize || winSize.y < minRenderSize))
    {
        Float xSizeFrac = buttonWidth / winSize.x;
        Float ySizeFrac = buttonHeight / winSize.y;

        Float spacing = 0.05f; // fractional spacing between buttons
        Float totalWidth = (2 * buttonWidth) + (winSize.x * spacing);
        Float startX = (winSize.x - totalWidth) * 0.5f;

        Float newProjectXFrac = startX / winSize.x;
        Float openProjectXFrac = (startX + buttonWidth + winSize.x * spacing) / winSize.x;
        Float titleY = winSize.y * 0.35f;
        Float maxGap = 50.0f;
        Float idealButtonY = winSize.y * 0.46f;
        Float clampedButtonY = ImMin(idealButtonY, titleY + maxGap);
        Float yFrac = clampedButtonY / winSize.y;

        if (buttonWidth >= minRenderSize && buttonHeight >= minRenderSize)
        {
            ImVec2 wmin = ImGui::GetWindowPos();
            Bool newHov = IsMouseHoveringRect(wmin + ImVec2{newProjectXFrac, yFrac} * winSize, wmin + ImVec2{(newProjectXFrac + xSizeFrac), yFrac + ySizeFrac} * winSize);
            Bool existHov = IsMouseHoveringRect(wmin + ImVec2{ openProjectXFrac, yFrac } * winSize, wmin + ImVec2{ (openProjectXFrac + xSizeFrac), yFrac + ySizeFrac } *winSize);
            DrawResourceTexture("Project/ProjectNew.png", newProjectXFrac, yFrac, xSizeFrac, ySizeFrac, newHov ? IM_COL32(200, 200, 200, 255) : IM_COL32(255,255,255,255));
            DrawResourceTexture("Project/ProjectExisting.png", openProjectXFrac, yFrac, xSizeFrac, ySizeFrac, existHov ? IM_COL32(200, 200, 200, 255) : IM_COL32(255, 255, 255, 255));
            ImVec2 newButtonPos = ImVec2(winSize.x * newProjectXFrac, winSize.y * yFrac);
            ImVec2 existingButtonPos = ImVec2(winSize.x * openProjectXFrac, winSize.y * yFrac);
            Float labelYOffset = buttonPixelSize + 8.0f;
            ImVec2 newTextSize = ImGui::CalcTextSize(projNew.c_str());
            ImVec2 existingTextSize = ImGui::CalcTextSize(projExist.c_str());
            ImGui::SetCursorScreenPos(wmin + ImVec2(
                newButtonPos.x + (buttonPixelSize - newTextSize.x) * 0.5f,
                newButtonPos.y + labelYOffset
            ));
            ImGui::TextUnformatted(projNew.c_str());
            ImGui::SetCursorScreenPos(wmin + ImVec2(
                existingButtonPos.x + (buttonPixelSize - existingTextSize.x) * 0.5f,
                existingButtonPos.y + labelYOffset
            ));
            ImGui::TextUnformatted(projExist.c_str());
            if(IsMouseClicked(ImGuiMouseButton_Left))
            {
                if(existHov)
                {
                    nfdchar_t* outPath = NULL;
                    nfdresult_t result = NFD_OpenDialog("tteproj", NULL, &outPath);
                    if(result == NFD_OKAY)
                    {
                        String path = (CString)outPath;
                        free(outPath);
                        if(_Application.GetProjectManager().SetProjectDisk(path))
                        {
                            _SelectionState = true;
                            GetApplication().PopUI();
                            _Application._OnProjectLoad();
                        }
                    }
                }
                if(newHov)
                {
                    GetApplication().PopUI();
                    GetApplication().PushUI(TTE_NEW_PTR(UIProjectCreate, MEMORY_TAG_EDITOR_UI, GetApplication()));
                    _SelectionState = true;
                    _Application._OnProjectLoad();
                }
            }
        }
    }

    // CHANGE LANGUAGE
    const CString newLang = "Change Language...";
    ImVec2 newLangSize = CalcTextSize(newLang);
    ImVec2 wmin = ImGui::GetWindowPos();
    SetCursorScreenPos(wmin + ImVec2{winSize.x * 0.5f - newLangSize.x * 0.5f, winSize.y * 0.05f});
    TextUnformatted(newLang);
    SetCursorScreenPos(wmin + ImVec2{ winSize.x * 0.5f - newLangSize.x * 0.5f, newLangSize.y + winSize.y * 0.05f + 4.0f });
    SetNextItemWidth(newLangSize.x);
    if(BeginCombo("##langselect", _LangBoxSelect.c_str(), ImGuiComboFlags_None))
    {
        for (const String& lang : _Application._AvailLanguages)
        {
            Bool isSelected = (_LangBoxSelect == lang);
            if (Selectable(lang.c_str(), isSelected))
            {
                _LangBoxSelect = lang;
                _Application._SetLanguage(lang);
            }

            if (isSelected)
                SetItemDefaultFocus();
        }
        EndCombo();
    }

    ImGui::End();

    // RECENT PROJECTS RHS PANEL
    winSize = GetMainViewport()->Size;
    SetNextWindowViewportPixels(winSize.x - rhsPanelPixels, 0, rhsPanelPixels, winSize.y);
    Begin("#recents", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                               | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                               | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysVerticalScrollbar
                               | ImGuiWindowFlags_NoSavedSettings);

    ImVec2 panelStart = ImVec2(winSize.x - rhsPanelPixels, 0);
    SetCursorScreenPos(panelStart);
    ImDrawList* draw_list = GetWindowDrawList();
    ImVec2 panelPos = GetCursorScreenPos();
    Float scrollY = GetScrollY();
    Float windowHeight = winSize.y;
    Float itemHeight = 73.0f;
    Float offsetY = 45.f;
    SetWindowFontScale(0.8f);
    PushFont(_Application.GetEditorFont(), 22.0f);
    titleSize = CalcTextSize(recents.c_str());
    SetCursorPosX((rhsPanelPixels * 0.5f) - titleSize.x * 0.5f - 5.0f);
    SetCursorPosY(45.0f * 0.5f - titleSize.y * 0.5f);
    TextUnformatted(recents.c_str());
    PopFont();
    SetWindowFontScale(1.0f);
    I32 projIndex = -1;
    for (const auto& recent : _Recents)
    {
        projIndex++;
        Float localY = offsetY - scrollY;
        ImVec2 topLeft = wmin + ImVec2(panelPos.x, panelPos.y + localY);
        ImVec2 bottomRight = wmin + ImVec2(panelPos.x + rhsPanelPixels, panelPos.y + localY + 70.0f);

        Bool hovered = IsMouseHoveringRect(topLeft, bottomRight);
        ImU32 up = hovered ? IM_COL32(50, 50, 50, 255) : IM_COL32(70, 70, 70, 255);
        ImU32 dn = hovered ? IM_COL32(40, 40, 40, 255) : IM_COL32(60, 60, 60, 255);

        if(hovered && IsMouseClicked(ImGuiMouseButton_Left))
        {
            // select
            if(!_Application.GetProjectManager().GetPreviousProject(recent.Name).ProjectName.empty() 
               || (_Application.GetProjectManager().GetHeadProject() 
               && CompareCaseInsensitive(_Application.GetProjectManager().GetHeadProject()->ProjectName, recent.Name)))
            {
                _Application.GetProjectManager().SetProject(recent.Name);
                _Application._OnProjectLoad();
                _SelectionState = true;
                _Application.PopUI();
            }
            else
            {
                TTE_LOG("WARNING: Clicked on project is invalid - ignoring."); // only realy used for debugging
            }
        }

        draw_list->AddRectFilledMultiColor(topLeft, bottomRight, up, up, dn, dn);
        draw_list->AddRect(topLeft, bottomRight, IM_COL32(30,30,30,255));

        ImVec2 itemMin = ImVec2(panelStart.x, localY);
        ImVec2 itemMax = ImVec2(panelStart.x + rhsPanelPixels, localY + 70.0f);
        ImVec2 clipMin = wmin + ImVec2(itemMin.x + 10.0f, itemMin.y + 10.0f);
        ImVec2 clipMax = wmin + ImVec2(itemMax.x - 10.0f, itemMax.y - 10.0f);

        ImGui::PushClipRect(clipMin, clipMax, true);

        SetCursorPos(panelStart);
        SetCursorPosX(10.0f);
        SetCursorPosY(offsetY + 10.0f);
        SetWindowFontScale(1.2f);
        PushTextWrapPos(clipMax.x - clipMin.x);
        TextUnformatted(recent.Name.c_str());
        SetWindowFontScale(1.0f);
        SetCursorPosX(10.0f);
        PushStyleColor(ImGuiCol_Text, IM_COL32(175, 175, 175, 255));
        TextUnformatted(recent.Location.c_str());
        PopStyleColor(1);
        PopTextWrapPos();

        ImGui::PopClipRect();

        offsetY += 73.0f;
    }

    SetCursorScreenPos(panelStart + wmin);
    Dummy(ImVec2(rhsPanelPixels, offsetY - scrollY));
    End();

    PopStyleColor(1);
}

void UIProjectSelect::GetWindowSize(U32& w, U32& h)
{
    w = 1280 / 2; 
    h = 720 / 2;
}
