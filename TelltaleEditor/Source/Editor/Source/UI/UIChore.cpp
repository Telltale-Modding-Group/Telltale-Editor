#include <UI/UIEditors.hpp>
#include <Common/Chore.hpp>

DECL_VEC_ADDITION();

template<>
void UIResourceEditor<Chore>::OnExit()
{
}

template<>
Bool UIResourceEditor<Chore>::RenderEditor()
{
    // TABS: FILE, EDIT, VIEW (TIDY = COLLAPSES ALL (CLEAR OPEN MAP), AGENTS, RESOURCES, BEHAVIOUR
    // file: set length, compute length, insert time, add 1 sec, add 5 sec (or use 5/1 keys), sync lang resources
    // per chore agent: agent, resources, filter. resources: add animation, add vox, add aud, (add bank etc), add lang res, add proclook, add other
    // per resource in agent: create block (insert), w/o fader (shift insert), delete (del), loop block,
    // toggle posed, filter out mover data, disable lip sync, toggle as music,
    // swap out (ctrlw), remove resource (ctrlx), dup resource (Ctrld), add style res group, groups
    // raise / low priority (page up and down) by 1.
    // THEY ONLY BLEND WHEN THEY ARE THE SAME PRIORITY
    Bool closing = false;
    ImGui::SetNextWindowSizeConstraints(ImVec2{ 450.0f, 600.0f }, ImVec2{ 99999.0f, 99999.0f });
    if(ImGui::Begin(GetTitle().c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        if(ImGui::BeginMenuBar())
        {
            if(ImGui::BeginMenu("File"))
            {
                if(ImGui::MenuItem("Close"))
                {
                    closing = true;
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Edit"))
            {
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("View"))
            {
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Agents"))
            {
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Resources"))
            {
                ImGui::EndMenu();
            }
            /*if(ImGui::BeginMenu("Behaviour"))
            {
                ImGui::EndMenu();
            }*/
            ImGui::EndMenuBar();
        }
        
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsize = ImGui::GetWindowSize();
        CurrentY = 45.0f;
        
        // SECTION: PLAYBACK CONTROLLER
        
        ImGui::GetWindowDrawList()->AddRectFilled(wpos, wpos + ImVec2{wsize.x, 100.0f}, IM_COL32(45, 45, 45, 255));
        if(ImageButton("Chore/Back.png", 10.0f, CurrentY, 25.0f, 25.0f))
        {
            CurrentTime = ViewStart;
        }
        if(ImageButton("Chore/Stop.png", 40.0f, CurrentY, 25.0f, 25.0f))
        {
            CurrentTime = ViewStart;
            IsPlaying = false;
        }
        if(ImageButton(IsPlaying ? "Chore/Pause.png" : "Chore/Play.png", IsPlaying ? 70.0f : 68.0f,
                       IsPlaying ? CurrentY : CurrentY - 2.0f, IsPlaying ? 25.0f : 29.0f, IsPlaying ? 25.0f : 29.0f))
        {
            IsPlaying = !IsPlaying;
        }
        if(ImageButton("Chore/Forward.png", 100.0f, CurrentY, 25.0f, 25.0f))
        {
            // what does this do??
        }
        SelectionBox(160.0f, CurrentY, 25.0f, 25.0f, ExtraDataOpen);
        if(ImageButton("PropertySet/AnimMonitor.png", 160.0f, CurrentY, 25.0f, 25.0f))
        {
            ExtraDataOpen = !ExtraDataOpen;
        }
        SelectionBox(200.0f, CurrentY, 25.0f, 25.0f, IsLooping);
        if(ImageButton("Chore/Loop.png", 200.0f, CurrentY, 25.0f, 25.0f))
        {
            IsLooping = !IsLooping;
        }
        Float choreLen = GetCommonObject()->GetLength(); if(choreLen <= 0.000001f) choreLen = 0.00001f; // no divide by zero
        CurrentY += 30.f;
        ImVec2 sliderMin{ 5.0f, CurrentY };
        ImVec2 sliderMax{ wsize.x - 5.f, CurrentY + 25.0f};
        ImVec2 sliderBackPos{ 8.0f, CurrentY + 4.0f};
        ImVec2 sliderBackSize{ wsize.x - 16.f, 17.0f};
        
        // Constant back bar
        ImGui::GetWindowDrawList()->AddRectFilled(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(90, 90, 90, 255));
        ImGui::GetWindowDrawList()->AddRect(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(110, 110, 110, 255));
        
        // User draggable part
        ImVec2 sliderDeltaMin{(ViewStart / choreLen) * (sliderMax.x - sliderMin.y) , 0.0f};
        ImVec2 sliderDeltaMax{(1.0f - (ViewEnd / choreLen)) * (sliderMax.x - sliderMin.y) , 0.0f};
        ImVec2 sliderBeginMin = sliderMin + sliderDeltaMin;
        ImVec2 sliderEndMin = sliderMax - sliderDeltaMax;
        ImGui::GetWindowDrawList()->AddRectFilled(wpos + sliderBeginMin, wpos + sliderEndMin, IM_COL32(60, 60, 60, 255));
        ImGui::GetWindowDrawList()->AddRectFilled(wpos + sliderBeginMin + ImVec2{6.0f, 6.0f}, wpos + sliderEndMin - ImVec2{6.0f, 6.0f}, IM_COL32(200, 200, 200, 255));
        // Current position
        Float curX = sliderMin.x + ((CurrentTime / choreLen) * (sliderMax.x - sliderMin.x));
        ImVec2 sliderPosMin = ImVec2{curX - 2.0f, CurrentY + 6.0f};
        ImVec2 sliderPosMax = ImVec2{curX + 2.0f, CurrentY + 19.0f};
        ImGui::GetWindowDrawList()->AddRectFilled(sliderMin, sliderMax, IM_COL32(50, 200, 50, 255));
        
        // Dragging Start
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if(ImGui::IsMouseHoveringRect(wpos + sliderPosMin, wpos + sliderPosMax, false))
            {
                SliderGrabbed = true;
                ViewStartGrabbed = ViewEndGrabbed = false;
            }
            if(ImGui::IsMouseHoveringRect(sliderBeginMin, sliderBeginMin + ImVec2{6.0f, sliderMax.y - sliderMin.y}, false))
            {
                ViewStartGrabbed = true;
                SliderGrabbed = ViewEndGrabbed = false;
            }
            if(ImGui::IsMouseHoveringRect(sliderEndMin, sliderEndMin + ImVec2{6.0f, sliderMax.y - sliderMin.y}, false))
            {
                ViewEndGrabbed = true;
                SliderGrabbed = ViewStartGrabbed = false;
            }
        }
        else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            SliderGrabbed = ViewStartGrabbed = ViewEndGrabbed = false;
        }
        
        // Grab sliding
        Float mouseX = ImGui::GetMousePos().x - wpos.x;
        if(ViewStartGrabbed)
        {
            if(mouseX <= sliderMin.x)
            {
                ViewStart = 0.0f;
            }
            else if(mouseX >= sliderMax.x)
            {
                ViewStart = choreLen;
            }
            else
            {
                ViewStart = ((mouseX - sliderMin.x) / (sliderMax.x - sliderMin.x)) * choreLen;
            }
        }
        if(ViewEndGrabbed)
        {
            if(mouseX <= sliderMin.x)
            {
                ViewEnd = 0.0f;
            }
            else if(mouseX >= sliderMax.x)
            {
                ViewEnd = choreLen;
            }
            else
            {
                ViewEnd = ((mouseX - sliderMin.x) / (sliderMax.x - sliderMin.x)) * choreLen;
            }
        }
        if(SliderGrabbed)
        {
            if(mouseX <= sliderBeginMin.x)
            {
                CurrentTime = 0.0f;
            }
            else if(mouseX >= sliderEndMin.x)
            {
                CurrentTime = choreLen;
            }
            else
            {
                CurrentTime = ((mouseX - sliderBeginMin.x) / (sliderEndMin.x - sliderBeginMin.x)) * choreLen;
            }
        }
        
        
    }
    ImGui::End();
    return closing;
}

