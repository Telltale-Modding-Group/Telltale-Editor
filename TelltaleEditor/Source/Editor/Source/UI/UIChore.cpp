#include <UI/UIEditors.hpp>
#include <Common/Chore.hpp>

template<>
void UIResourceEditor<Chore>::OnExit()
{
}

template<>
Bool UIResourceEditor<Chore>::RenderEditor()
{
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
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    // TABS: FILE, EDIT, VIEW (TIDY = COLLAPSES ALL (CLEAR OPEN MAP), AGENTS, RESOURCES, BEHAVIOUR
    // file: set length, compute length, insert time, add 1 sec, add 5 sec (or use 5/1 keys), sync lang resources
    // per chore agent: agent, resources, filter. resources: add animation, add vox, add aud, (add bank etc), add lang res, add proclook, add other
    // per resource in agent: create block (insert), w/o fader (shift insert), delete (del), loop block,
    // toggle posed, filter out mover data, disable lip sync, toggle as music,
    // swap out (ctrlw), remove resource (ctrlx), dup resource (Ctrld), add style res group, groups
    // raise / low priority (page up and down) by 1.
    // THEY ONLY BLEND WHEN THEY ARE THE SAME PRIORITY
    return closing;
}

