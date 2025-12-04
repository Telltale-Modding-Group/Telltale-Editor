#include <UI/UIEditors.hpp>
#include <Common/Chore.hpp>
#include <AnimationManager.hpp>
#include <UI/ApplicationUI.hpp>

#include <imgui_internal.h>
DECL_VEC_ADDITION();

template<>
void UIResourceEditor<Chore>::OnExit()
{
}

// LERPS
#define TO_SCREENSPACE(Xval) resourceBoxMin.x + ((Xval) - ViewStart) / (ViewEnd - ViewStart) * timelineWidth
#define TO_SCREENSPACE_CLAMPED(Xval) ImClamp(resourceBoxMin.x + ((Xval) - ViewStart) / (ViewEnd - ViewStart) * timelineWidth, resourceBoxMin.x, resourceBoxMax.x)
#define FROM_SCREENSPACE(Xval) ViewStart + ((((Xval) - resourceBoxMin.x) / timelineWidth) * (ViewEnd - ViewStart))

void UIResourceEditorRuntimeData<Chore>::AddAgentResourcePostLoadCallback(const std::vector<Symbol>* resources)
{
    String name{};
    auto it = EditorInstance->PreloadingResourceToAgent.begin();
    for(; it != EditorInstance->PreloadingResourceToAgent.end(); it++)
    {
        if(Symbol(it->first) == (*resources)[0])
        {
            name = it->first;
            break;
        }
    }
    if(!name.empty())
    {
        EditorInstance->PreloadingResourceToAgent.erase(it);
        DoAddAgentResourcePostLoadCallback(std::move(name));
    }
}

void UIResourceEditorRuntimeData<Chore>::DoAddAgentResourcePostLoadCallback(String animFile)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    Chore::Agent* pAgent = const_cast<Chore::Agent*>(pChore->GetAgent(SelectedAgent));
    Ptr<ResourceRegistry> reg = EditorInstance->GetApplication().GetRegistry();
    if(animFile == "look")
    {
        Ptr<Procedural_LookAt> pLookAt = TTE_NEW_PTR(Procedural_LookAt, MEMORY_TAG_COMMON_INSTANCE, reg);
        TTE_ATTACH_DBG_STR(pLookAt.get(), "Chore Agent " + pChore->GetName() + "/" + pAgent->Name + " Look At");
        // add procedural look at embed
        Chore::Resource& res = pChore->EmplaceResource("Procedural Look At");
        res.Priority = 1;
        res.Embed = pLookAt;
        res.ResFlags.Add(Chore::Resource::ENABLED);
        res.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
        res.ResFlags.Add(Chore::Resource::EMBEDDED);
        res.Properties = TelltaleEditor::Get()->CreatePropertySet();
        if(!TelltaleEditor::Get()->TestCapability(GameCapability::UNINHERITED_LOOK_ATS))
        {
            // inherited look ats
            if(!reg->ResourceExists(kProceduralLookAtPropName))
            {
                TTE_LOG("WARNING: When creating procedural look at for %s, the module property set doesn't exist in the resource system: %s", 
                    pAgent->Name.c_str(), kProceduralLookAtPropName.c_str());
            }
            PropertySet::AddParent(res.Properties, kProceduralLookAtPropNameSymbol, reg);
        }
        res.ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, reg);
        pLookAt->AddToChore(EditorInstance->GetCommonObject(), res);
        pLookAt->Attach(EditorInstance->GetCommonObject(), res);
        pAgent->Resources.push_back((I32)pChore->GetResources().size() - 1);
    }
    else
    {
        WeakPtr<MetaOperationsBucket_ChoreResource> pRes = AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(reg, animFile, true);
        if (pRes.lock())
        {
            // new resource
            Chore::Resource& res = pChore->EmplaceResource(animFile);
            res.Priority = 1;
            res.ResFlags.Add(Chore::Resource::ENABLED);
            res.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
            res.Properties = TelltaleEditor::Get()->CreatePropertySet();
            res.ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, reg);
            pRes.lock()->AddToChore(EditorInstance->GetCommonObject(), res);
            pRes.lock()->Attach(EditorInstance->GetCommonObject(), res);
            pAgent->Resources.push_back((I32)pChore->GetResources().size() - 1);
        }
        else
        {
            TTE_LOG("ERROR: Cannot add '%s' to '%s' as a chore resource, failed to load it!", animFile.c_str(), EditorInstance->FileName.c_str());
        }
    }
}

void UIResourceEditorRuntimeData<Chore>::AddAgentResourceCallback(String animFile)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    Chore::Agent* pAgent = const_cast<Chore::Agent*>(pChore->GetAgent(SelectedAgent));
    I32 index = 0;
    Ptr<ResourceRegistry> reg = EditorInstance->GetApplication().GetRegistry();
    for(const auto& res: pChore->GetResources())
    {
        if(res.Name == animFile)
        {
            break;
        }
        index++;
    }
    if(index >= pChore->GetResources().size())
    {
        HandleBase hObject{};
        hObject.SetObject(animFile);
        if(hObject.IsLoaded(reg))
        {
            DoAddAgentResourcePostLoadCallback(animFile);
        }
        else
        {
            PreloadingResourceToAgent[animFile] = SelectedAgent;
            std::vector<HandleBase> handles{};
            handles.push_back(std::move(hObject));
            reg->PreloadWithCallback(std::move(handles), false, ALLOCATE_METHOD_CALLBACK_1(this,
                    AddAgentResourcePostLoadCallback, UIResourceEditorRuntimeData<Chore>, const std::vector<Symbol>*), false, EditorInstance->FileName);
            TTE_LOG("Async loading unloaded resource '%s' to add to chore '%s'...", animFile.c_str(), EditorInstance->FileName.c_str());
        }
    }
    else
    {
        for(const auto idx : pAgent->Resources)
        {
            if(idx == index)
            {
                PlatformMessageBoxAndWait("Error", EditorInstance->GetApplication().GetLanguageText("misc.chore_resource_error"));
                return;
            }
        }
        pAgent->Resources.push_back(index);
    }
}

void UIResourceEditorRuntimeData<Chore>::AddAgentCallback(Meta::ClassInstance stringName)
{
    DoAddAgent(COERCE(stringName._GetInternal(), String));
}

void UIResourceEditorRuntimeData<Chore>::DoAddAgent(String agentName)
{
    Chore* pChore = EditorInstance->GetCommonObject().get();
    if(pChore->GetAgent(agentName))
    {
        PlatformMessageBoxAndWait("Error", EditorInstance->GetLanguageText("misc.chore_agent_error"));
    }
    else
    {
        auto& agent = pChore->EmplaceAgent(agentName);
        agent.Properties = TelltaleEditor::Get()->CreatePropertySet();
    }
}

template<>
Bool UIResourceEditor<Chore>::RenderEditor()
{
    GetApplication().GetRegistry()->Update(0.5f, FileName);
    EditorInstance = this;
    // TABS: FILE, EDIT, VIEW (TIDY = COLLAPSES ALL (CLEAR OPEN MAP), AGENTS, RESOURCES, BEHAVIOUR
    // file: set length, compute length, insert time, add 1 sec, add 5 sec (or use 5/1 keys), sync lang resources
    // per chore agent: agent, resources, filter. resources: add animation, add vox, add aud, (add bank etc), add lang res, add proclook, add other
    // THEY ONLY BLEND WHEN THEY ARE THE SAME PRIORITY
    
    // CHORE TRACKER MENU (CHECK AROUND  TOOL 102_2 MP4 39 MINS

    if(!SubRefFence)
    {
        SubRefFence = TTE_NEW_PTR(U8, MEMORY_TAG_TRANSIENT_FENCE, 0);
    }

    // SUB RESOURCE MGR
    if(!PreloadFinished)
    {
        if(!PreloadAwaiting)
        {
            // Dispatch
            std::vector<HandleBase> preloadHandles{};
            preloadHandles.reserve(GetCommonObject()->GetResources().size());
            for(const auto& resource: GetCommonObject()->GetResources())
            {
                if(!resource.ResFlags.Test(Chore::Resource::EMBEDDED))
                {
                    HandleBase hResource{};
                    hResource.SetObject(resource.Name);
                    preloadHandles.push_back(hResource);
                }
            }
            if(preloadHandles.empty())
            {
                PreloadFinished = true;
            }
            else
            {
                PreloadAwaiting = true;
                PreloadOffset = GetApplication().GetRegistry()->Preload(std::move(preloadHandles), false);
            }
        }
        else
        {
            if(GetApplication().GetRegistry()->GetPreloadOffset() >= PreloadOffset)
            {
                FailedResources.clear();
                ResourcesCache.clear();
                PreloadAwaiting = false;
                PreloadFinished = true;
            }
        }
    }

    Bool closing = false;
    Ptr<Chore> pChore = GetCommonObject();
    ImGui::SetNextWindowSizeConstraints(ImVec2{ 450.0f, 600.0f }, ImVec2{ 99999.0f, 99999.0f });
    if(ImGui::Begin(GetTitle().c_str(), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar))
    {
        // SPLIT SECTIONS
        ImGui::GetWindowDrawList()->ChannelsSplit(2);

        ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
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
                AddMenuOptions("Edit");
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("View"))
            {
                AddMenuOptions("View");
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Agent"))
            {
                if(ImGui::BeginMenu("Include an agent"))
                {
                    if(ImGui::MenuItem("By Name"))
                    {
                        GetApplication().QueueMetaInstanceEditPopup(_EditorUI, "New Agent for Chore",
                                ALLOCATE_METHOD_CALLBACK_1(this, AddAgentCallback, UIResourceEditorRuntimeData<Chore>, Meta::ClassInstance),
                                        "Agent Name", Meta::CreateInstance(Meta::FindClass("String", 0)));
                    }
                    if(!_EditorUI.GetActiveScene().GetName().empty() && ImGui::BeginMenu(_EditorUI.GetActiveScene().GetName().c_str()))
                    {
                        for(const auto& agent: _EditorUI.GetActiveScene().GetAgents())
                        {
                            String agentName = _EditorUI.GetActiveScene().GetAgentNameString(agent.first);
                            if(ImGui::MenuItem(agentName.c_str()))
                            {
                                DoAddAgent(agentName);
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            /*if(ImGui::BeginMenu("Resources"))
            {
                AddMenuOptions("Resources");
                ImGui::EndMenu();
            }*/
            /*if(ImGui::BeginMenu("Behaviour"))
            {
                ImGui::EndMenu();
            }*/
            if(!SelectedAgent.empty() && ImGui::BeginMenu(SelectedAgent.c_str()))
            {
                if(ImGui::MenuItem("Add Animation"))
                {
                    GetApplication().QueueResourcePickerPopup(_EditorUI, "Choose Animation", "*.anm",
                                    ALLOCATE_METHOD_CALLBACK_1(this, AddAgentResourceCallback, UIResourceEditorRuntimeData<Chore>, String));
                }
                // AUD, VOX, LANGUAGE RESOURCES
                if (ImGui::BeginMenu("Add Procedural Animation"))
                {
                    if(ImGui::MenuItem("Look At"))
                    {
                        Bool exist = false;
                        // Ill keep these here in case. but you can have multiple look ats (obviously, might want to lookat different characters at different times from this same one)
                        /*const Chore::Agent* agent = pChore->GetAgent(SelectedAgent);
                        for(const auto& res: agent->Resources)
                        {
                            auto pRes = pChore->GetConcreteResource(agent->Name, pChore->GetResources()[res].Name);
                            if(pRes && std::dynamic_pointer_cast<Procedural_LookAt>(pRes))
                            {
                                exist = true;
                                PlatformMessageBoxAndWait("Error", GetApplication().GetLanguageText("misc.procedural_error"));
                                break;
                            }
                        }*/
                        if(!exist)
                        {
                            DoAddAgentResourcePostLoadCallback("look");
                        }
                    }
                    // TODO EYES
                    ImGui::EndMenu();
                }
                // OTHER RESOURCE
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        ImVec2 wpos = ImGui::GetWindowPos();
        ImVec2 wsize = ImVec2{ ImGui::GetCurrentWindowRead()->ScrollbarY ? ImGui::GetContentRegionAvail().x + 10.0f : ImGui::GetWindowSize().x, ImGui::GetWindowSize().y };
        CurrentY = 45.0f;

        // SECTION: MENU OPTIONS

        if(TestMenuOption("View", "Clear Selection", "[ESC]", ImGuiKey_Escape, false, false))
        {
            SelectedKeyframeSamples.clear();
            SelectedResourceBlocks.clear();
            SelectedResourceIndex = -1;
        }
        if(TestMenuOption("View", "Tidy", "[CTRL + T]", ImGuiKey_T, false, true))
        {
            // close all graphs and properties
            for(auto& resource : pChore->_Resources)
            {
                resource.ResFlags.Remove(Chore::Resource::VIEW_GRAPHS);
                resource.ResFlags.Remove(Chore::Resource::VIEW_PROPERTIES);
                resource.ResFlags.Remove(Chore::Resource::VIEW_GROUPS);
            }
        }
        if(TestMenuOption("View", "Untidy", "[SHIFT + CTRL + T]", ImGuiKey_T, true, true))
        {
            // open all graphs and properties
            for(auto& resource : pChore->_Resources)
            {
                resource.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
                resource.ResFlags.Add(Chore::Resource::VIEW_PROPERTIES);
                resource.ResFlags.Add(Chore::Resource::VIEW_GROUPS);
            }
        }
        if(TestMenuOption("View", "Collapse All", "", 0, false, false))
        {
            OpenChoreAgents.clear();
        }
        
        // SECTION: PLAYBACK CONTROLLER

        ImGui::GetWindowDrawList()->AddRectFilled(wpos, wpos + ImVec2{wsize.x, 105.0f}, IM_COL32(45, 45, 45, 255));
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
            IsPlaying = true; // reset and play from start
            CurrentTime = ViewStart;
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

        Float choreLen = GetCommonObject()->GetLength();
        if (choreLen <= 0.000001f) choreLen = 0.00001f; // avoid divide by zero


        // Fix invalid end range
        if (ViewEnd <= ViewStart && choreLen > 0.000001f)
        {
            ViewEnd = choreLen;
        }

        // length
        char Temp[16]{0};
        Float textStartLen = 0.0f;
        snprintf(Temp, 16, "%.2f", CurrentTime);
        ImGui::PushFont(ImGui::GetFont(), 25.f);
        Float size = ImGui::CalcTextSize(Temp).x;
        ImGui::SetCursorScreenPos(wpos + ImVec2{textStartLen = wsize.x - 10.0f - size, 45.0f});
        ImGui::TextUnformatted(Temp);
        ImGui::PopFont();

        if (PreloadAwaiting || !PreloadingResourceToAgent.empty())
        {
            const String& langTextLoading = GetLanguageText(PreloadingResourceToAgent.empty() ? "misc.loading_resource" : "misc.loading_chore");
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(196, 94, 35, 255));
            ImGui::PushFont(ImGui::GetFont(), 13.0f);
            ImGui::SetCursorScreenPos(wpos + ImVec2{ textStartLen - ImGui::CalcTextSize(langTextLoading.c_str()).x - 50.0f, CurrentY + 7.0f });
            if(PreloadingResourceToAgent.empty())
            {
                ImGui::Text("%s%s", langTextLoading.c_str(), PreloadAnimator.GetEllipses().c_str());
            }
            else
            {
                ImGui::Text("%s (%d)", langTextLoading.c_str(), (U32)PreloadingResourceToAgent.size());
            }
            ImGui::PopFont();
            ImGui::PopStyleColor();
        }

        CurrentY += 30.f;

        // konst
        const Float margin = 5.0f;
        const Float sliderHeight = 25.0f;
        const Float backBarHeight = 15.0f;
        const Float viewBarHeight = 22.0f;
        const Float handleWidth = 6.0f;
        const Float minPixelWidth = 30.0f;

        // pos
        ImVec2 sliderMin{ margin, CurrentY };
        ImVec2 sliderMax{ wsize.x - margin, CurrentY + sliderHeight };
        ImVec2 sliderBackPos{ margin, CurrentY + (sliderHeight - backBarHeight) * 0.5f };
        ImVec2 sliderBackSize{ wsize.x - (margin * 2), backBarHeight };

        // bg
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(80, 80, 80, 255));
        dl->AddRect(wpos + sliderBackPos, wpos + sliderBackPos + sliderBackSize, IM_COL32(100, 100, 100, 255));

        // lerps
        Float usableWidth = sliderBackSize.x;
        Float startX = sliderBackPos.x + (ViewStart / choreLen) * usableWidth;
        Float endX = sliderBackPos.x + (ViewEnd / choreLen) * usableWidth;

        // grab checks
        Float distancePixels = endX - startX;
        Float distanceTime = ViewEnd - ViewStart;
        if (distancePixels < minPixelWidth && distanceTime > 0.0f)
        {
            Float scale = minPixelWidth / distancePixels;
            Float newDistanceTime = distanceTime * scale;
            Float center = (ViewStart + ViewEnd) * 0.5f;

            ViewStart = center - newDistanceTime * 0.5f;
            ViewEnd = center + newDistanceTime * 0.5f;

            if (ViewStart < 0.0f)
            {
                ViewEnd -= ViewStart;
                ViewStart = 0.0f;
            }
            if (ViewEnd > choreLen)
            {
                Float diff = ViewEnd - choreLen;
                ViewStart -= diff;
                ViewEnd = choreLen;
            }
        }

        // clamps
        Float maxOffset = handleWidth * 0.5f;
        startX = ImClamp(startX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        endX = ImClamp(endX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        if (endX < startX) endX = startX + 1.0f;

        // active region
        Float viewY = CurrentY + (sliderHeight - viewBarHeight) * 0.5f;
        ImVec2 rangeMin = { startX, viewY };
        ImVec2 rangeMax = { endX,   viewY + viewBarHeight };
        ImVec2 activeRegionInnerMin = rangeMin - ImVec2{ handleWidth * 0.5f, 0.0f } + ImVec2{ 5.0f, 5.0f };
        ImVec2 activeRegionInnerMax = rangeMax + ImVec2{ handleWidth * 0.5f, 0.0f } - ImVec2{ 5.0f, 5.0f };
        dl->AddRectFilled(wpos + rangeMin - ImVec2{ handleWidth * 0.5f, 0.0f }, wpos + rangeMax + ImVec2{ handleWidth * 0.5f, 0.0f }, IM_COL32(60, 60, 60, 255));
        dl->AddRectFilled(wpos + activeRegionInnerMin, wpos + activeRegionInnerMax, IM_COL32(200, 200, 200, 255));

        // current time
        Float curX = sliderBackPos.x + (CurrentTime / choreLen) * usableWidth;
        curX = ImClamp(
            curX,
            activeRegionInnerMin.x + 5.0f,   // small inset so it doesn’t touch the border
            activeRegionInnerMax.x - 5.0f
        );
        //curX = ImClamp(curX, sliderBackPos.x + maxOffset, sliderBackPos.x + sliderBackSize.x - maxOffset);
        ImVec2 curMin{ curX - 5.0f, viewY + 5.0f };
        ImVec2 curMax{ curX + 5.0f, viewY + viewBarHeight - 5.0f };
        dl->AddRectFilled(wpos + curMin, wpos + curMax, IM_COL32(50, 150, 50, 255));

        // mouse
        ImVec2 mouse = ImGui::GetMousePos();
        Float localX = mouse.x - wpos.x;
        Bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        // click grab
        if (leftClicked) 
        {
            SliderGrabbed = ViewStartGrabbed = ViewEndGrabbed = false;

            if (ImGui::IsMouseHoveringRect(wpos + curMin, wpos + curMax, false))
                SliderGrabbed = true;
            else if (ImGui::IsMouseHoveringRect(wpos + ImVec2{ startX - handleWidth, CurrentY + 5.0f },
                wpos + ImVec2{ startX + handleWidth, CurrentY + 5.0f + viewBarHeight }, false))
                ViewStartGrabbed = true;
            else if (ImGui::IsMouseHoveringRect(wpos + ImVec2{ endX - handleWidth, CurrentY + 5.0f },
                wpos + ImVec2{ endX + handleWidth, CurrentY + 5.0f + viewBarHeight }, false))
                ViewEndGrabbed = true;
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
        {
            SliderGrabbed = ViewStartGrabbed = ViewEndGrabbed = false;
        }

        // update pos
        if (ViewStartGrabbed)
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            ViewStart = t * choreLen;
            if (ViewStart > ViewEnd - 0.001f) ViewStart = ViewEnd - 0.001f;
        }

        if (ViewEndGrabbed) 
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            ViewEnd = t * choreLen;
            if (ViewEnd < ViewStart + 0.001f) ViewEnd = ViewStart + 0.001f;
        }

        if (SliderGrabbed) 
        {
            float t = ImClamp((localX - sliderBackPos.x) / usableWidth, 0.0f, 1.0f);
            CurrentTime = t * choreLen;
        }
        CurrentTime = ImClamp(CurrentTime, ViewStart, ViewEnd); // always ensure this

        CurrentY = 105.0f; // start of rest of the data

        // TOOL BAR
        {
            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 35.0f }, IM_COL32(30, 30, 30, 255));

            Float curX = 5.0f;
            SelectionBox(curX, CurrentY + 5.0f, 26.0f, 26.0f, false); // for highlight
            if(ImageButton("Chore/ChoreAdd1.png", curX, CurrentY + 5.0f, 26.0f, 26.0f))
            {
                pChore->_Length += 1.0f;
                ViewEnd += 1.0f;
            }
            curX += 30.0f;
            SelectionBox(curX, CurrentY + 5.0f, 26.0f, 26.0f, false);
            if (ImageButton("Chore/ChoreAdd5.png", curX, CurrentY + 5.0f, 26.0f, 26.0f))
            {
                pChore->_Length += 5.0f;
                ViewEnd += 5.0f;
            }
            curX += 30.0f;

            CurrentY += 35.0f;
        }

        // EXTRA DATA
        if(ExtraDataOpen)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 74.0f }, IM_COL32(60, 60, 60, 255));

            ImGui::BeginDisabled();
            ImGui::PushItemWidth(250.0f);
            ImGui::PushFont(ImGui::GetFont(), 10.0f);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 5.0f });
            ImGui::TextUnformatted("Length");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 2.0f });
            ImGui::InputFloat("##Len", &choreLen);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 30.0f });
            ImGui::TextUnformatted("View Begin");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 27.0f });
            ImGui::InputFloat("##ViewB", &ViewStart);

            ImGui::SetCursorScreenPos(wpos + ImVec2{ 20.0f, CurrentY + 55.0f });
            ImGui::TextUnformatted("View End");
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 160.0f, CurrentY + 52.0f });
            ImGui::InputFloat("##ViewE", &ViewEnd);

            ImGui::PopFont();
            ImGui::PopItemWidth();
            ImGui::EndDisabled();

            CurrentY += 74.0f;
        }

        ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 3.0f }, IM_COL32(30, 30, 30, 255));
        CurrentY += 3.0f; // divider

        // SECTION: CHORE AGENT VIEW        

        ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);

        CurrentY -= ImGui::GetScrollY();
        Float cache = CurrentY;
        Bool mouseClickedThisFrame = leftClicked && GImGui->OpenPopupStack.empty() && ImGui::IsWindowFocused();
        Bool mouseReleasedThisFrame = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && GImGui->OpenPopupStack.empty()  && ImGui::IsWindowFocused();
        Bool mouseRightReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Right) && GImGui->OpenPopupStack.empty()  && ImGui::IsWindowFocused();
        Bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        Float mouseDeltaX = ImGui::GetMousePos().x - LastMouseX;
        Float mouseDeltaY = ImGui::GetMousePos().y - LastMouseY;
        Bool anySamplesClicked = false;
        Bool anythingClicked = false;
        ImRect selectionRect{ { SelectionBox1X, SelectionBox1Y }, { SelectionBox2X, SelectionBox2Y } };
        Bool usedSelectionReclick = false;
        Bool allowReclickNextFrame = false;
        
        if(mouseReleasedThisFrame && ScaleMode)
        {
            ActiveScaleMode = NONE;
        }

        // Render each agent
        I32 agentID = 100;
        for(auto& agent: pChore->_Agents)
        {
            ImGui::PushID(agentID++);
            Bool open = OpenChoreAgents.find(agent.Name) != OpenChoreAgents.end();
            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 20.0f }, IM_COL32(90, 90, 90, 255));
            ImGui::GetWindowDrawList()->AddRect(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 20.0f }, SelectedAgent == agent.Name ? IM_COL32(40, 40, 140,255) : IM_COL32(10, 10, 10, 255));
            ImGui::SetCursorScreenPos(wpos + ImVec2{ 18.0f, CurrentY + 4.0f });
            ImGui::PushFont(ImGui::GetFont(), 12.0f);
            ImGui::TextUnformatted(agent.Name.c_str());
            ImGui::PopFont();
            if(ImageButton(open ? "Chore/AgentOpen.png" : "Chore/AgentUnopen.png", 0.0f, CurrentY + 1.0f, 18.0f, 18.0f))
            {
                if(open)
                {
                    OpenChoreAgents.erase(OpenChoreAgents.find(agent.Name));
                }
                else
                {
                    OpenChoreAgents.insert(agent.Name);
                }
                open = !open;
            }
            if(leftClicked && ImGui::IsMouseHoveringRect(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 20.0f }, false))
            {
                SelectedResourceIndex = -1;
                SelectedAgent = agent.Name;
                anythingClicked = true;
            }
            // unless we have dynamic menu options
            /*ImGui::PushID(agent.Name.c_str());
            if(SelectedAgent == agent.Name)
                OpenContextMenu(agent.Name.c_str(), wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 20.0f });
            ImGui::PopID();*/
            CurrentY += 20.f;
            if (open)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + 5.0f }, IM_COL32(205, 212, 201, 255));
                CurrentY += 2.0f;
                I32 runningID = 412433;
                I32 rmResource = -1;
                I32 resourceIndex = 0;
                for(const auto& resIndex: agent.Resources)
                {
                    Ptr<MetaOperationsBucket_ChoreResource> resourceInterface{};
                    ImGui::PushID(runningID++);
                    Chore::Resource& resource = pChore->_Resources[resIndex];
                    Symbol resNameSymbol = resource.Name;
                    auto rcacheIterator = ResourcesCache.find(resource.Name);
                    Bool bFail = false;
                    if(PreloadAwaiting)
                    {
                        bFail = true;
                    } 
                    else
                    {
                        Bool bEmbed = resource.ResFlags.Test(Chore::Resource::EMBEDDED);
                        if(bEmbed)
                        {
                            resourceInterface = resource.Embed;
                            bFail = resourceInterface == nullptr;
                            if(bFail && FailedResources.find(resource.Name) == FailedResources.end())
                            {
                                FailedResources.insert(resource.Name);
                                TTE_LOG("ERROR: Embedded chore resource '%s' for chore '%s' is empty!", resource.Name.c_str(), pChore->_Name.c_str());
                            }
                            else if(resourceInterface != nullptr && rcacheIterator == ResourcesCache.end())
                            {
                                ResourcesCache[resource.Name] = resourceInterface;
                            }
                        }
                        else if (rcacheIterator == ResourcesCache.end())
                        {
                            if (FailedResources.find(resource.Name) != FailedResources.end())
                                bFail = true;
                            else
                            {
                                WeakPtr<MetaOperationsBucket_ChoreResource> pResource =
                                    AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(GetApplication().GetRegistry(), resource.Name, false);
                                if (pResource.lock())
                                {
                                    ResourcesCache[resource.Name] = std::move(pResource);
                                }
                                else
                                {
                                    bFail = true;
                                    FailedResources.insert(resource.Name);
                                    TTE_LOG("ERROR: Chore resource '%s' for chore '%s' could not be loaded/found, or is not a chore resource operations bucket yet!", resource.Name.c_str(), pChore->_Name.c_str());
                                }
                            }
                        }
                        if (!bFail && !bEmbed)
                        {
                            if (rcacheIterator == ResourcesCache.end())
                                rcacheIterator = ResourcesCache.find(resource.Name);
                            if (rcacheIterator == ResourcesCache.end() || rcacheIterator->second.expired())
                            {
                                WeakPtr<MetaOperationsBucket_ChoreResource> pResource =
                                    AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(GetApplication().GetRegistry(), resource.Name, false);
                                if (pResource.lock())
                                {
                                    rcacheIterator->second = std::move(pResource);
                                }
                                else
                                {
                                    bFail = true;
                                    if(rcacheIterator != ResourcesCache.end())
                                        ResourcesCache.erase(rcacheIterator);
                                    TTE_LOG("WARNING: Chore resource '%s' was unloaded and could not be reloaded! Disabling this resource...", resource.Name.c_str(), pChore->_Name.c_str());
                                }
                            }
                        }
                    }
                    if(!resourceInterface)
                        resourceInterface = bFail ? nullptr : ResourcesCache[resource.Name].lock();
                    Vector3 paramColour{ 219.0f / 255.0f, 4.0f / 255.0f, 4.0f / 255.0f };
                    CString resicon = "Chore/Unknown.png";
                    Float resourceLength = 0.001f;
                    if (!bFail)
                    {
                        resourceInterface->GetRenderParameters(paramColour, resicon);
                        resourceLength = resourceInterface->GetLength();
                    }
                    const Float RES_HEIGHT = 30.0F;
                    Bool bResourceSelected = SelectedResourceIndex == resIndex && SelectedAgent == agent.Name;
                    ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + RES_HEIGHT + 2.0f }, IM_COL32(205, 212, 201,255));
                    ImVec2 resourceBoxMin = wpos + ImVec2{ 6.0f, CurrentY };
                    ImVec2 resourceBoxMax = wpos + ImVec2{ wsize.x - 6.0f, CurrentY + RES_HEIGHT };
                    ImGui::GetWindowDrawList()->AddRectFilled(resourceBoxMin, resourceBoxMax, ImColor(paramColour.x, paramColour.y, paramColour.z, 1.0f));
                    ImGui::GetWindowDrawList()->AddRect(resourceBoxMin, resourceBoxMax, 
                        bResourceSelected ? IM_COL32(110, 104, 71, 255) : IM_COL32(79, 181, 209, 255), 0.0f, 0, bResourceSelected ? 2.0f : 0.0f);
                    if(ImGui::IsMouseHoveringRect(resourceBoxMin, resourceBoxMax, false) && (mouseClickedThisFrame || mouseRightReleased))
                    {
                        SelectedAgent = agent.Name;
                        SelectedResourceIndex = resIndex;
                        anythingClicked = true;
                        bResourceSelected = true;
                    }
                    if(bResourceSelected)
                        OpenContextMenu("resource", resourceBoxMin, resourceBoxMax);
                    resourceBoxMin.x += 3.0f;
                    resourceBoxMax.x -= 3.0f;
                    resourceBoxMin.y += 2.0f;
                    resourceBoxMax.y -= 2.0f; // make the rest in a slightly smaller inner box
                    const Float timelineWidth = resourceBoxMax.x - resourceBoxMin.x;

                    // OPTIONS

                    Bool bCreateBlock = false, bCreateFades = true, bDeleteBlock = false, bToggleLoop = false;
                    if (bResourceSelected && TestMenuOption("resource", "Disable", "", 0, false, false, false, resource.ResFlags.Test(Chore::Resource::ENABLED) ? nullptr : "Enable"))
                    {
                        resource.ResFlags.Toggle(Chore::Resource::ENABLED);
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Reset Length", "", 0, false, false, true))
                    {
                        for(auto& block: resource.Blocks)
                        {
                            block.Scale = 1.0f; // reset all scales
                        }
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Raise Priority", "[PAGE UP]", ImGuiKey_PageUp, false, false))
                    {
                        resource.Priority++;
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Lower Priority", "[PAGE DOWN]", ImGuiKey_PageDown, false, false, true))
                    {
                        resource.Priority--;
                    }
                    // TODO CREATE GUIDE?
                    else if (bResourceSelected && TestMenuOption("resource", "View Graphs", "[G]", ImGuiKey_G, false, false, false, resource.ResFlags.Test(Chore::Resource::VIEW_GRAPHS) ? "Hide Graphs" : nullptr))
                    {
                        resource.ResFlags.Toggle(Chore::Resource::VIEW_GRAPHS);
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "View Properties", "[P]", ImGuiKey_P, false, false, false, resource.ResFlags.Test(Chore::Resource::VIEW_PROPERTIES) ? "Hide Properties" : nullptr))
                    {
                        resource.ResFlags.Toggle(Chore::Resource::VIEW_PROPERTIES);
                        String propName = "\"" + resource.Name + "\" Chore Properties";
                        if(resource.ResFlags.Test(Chore::Resource::VIEW_PROPERTIES))
                        {
                            _EditorUI.DispatchEditorImmediate(TTE_NEW_PTR(UIPropertySet, MEMORY_TAG_EDITOR_UI, propName, _EditorUI, propName, resource.Properties, WeakPtr<U8>(SubRefFence)));
                        }
                        else
                        {
                            // Close
                            _EditorUI.CloseEditor(propName);
                        }
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "View Resource Groups", "[R]", ImGuiKey_R, false, false, true, resource.ResFlags.Test(Chore::Resource::VIEW_GROUPS) ? nullptr : "Hide Resource Groups"))
                    {
                        resource.ResFlags.Toggle(Chore::Resource::VIEW_GROUPS); // TODO
                    }
                    // TODO TRIM [T] MODE
                    else if (bResourceSelected && TestMenuOption("resource", "Enter Scale Mode", "[S]", ImGuiKey_S, false, false, true,
                        ScaleMode ? "Exit Scale Mode" : nullptr))
                    {
                        ScaleMode = !ScaleMode;
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Create Block", "[INSERT]", ImGuiKey_Insert, false, false, false))
                    {
                        bCreateBlock = true;
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Create Block w/o fades", "[SHIFT + INSERT]", ImGuiKey_Insert, true, false, false))
                    {
                        bCreateBlock = true;
                        bCreateFades = false;
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Delete Block", "[DELETE]", ImGuiKey_Delete, false, false, false))
                    {
                        bDeleteBlock = true;
                    }
                    else if (bResourceSelected && TestMenuOption("resource", "Loop Block", "", ImGuiKey_None, false, false, true))
                    {
                        bToggleLoop = true;
                    }
                    // NEXT: toggle posed, filter out mover data [M], disable lip sync abunatuib [Y], toggle play as music,
                    // swap out (ctrlw)
                    else if (bResourceSelected && TestMenuOption("resource", "Remove this resource", "[CTRL + X]", ImGuiKey_X, false, true, true))
                    {
                        rmResource = resourceIndex;
                        SelectedResourceIndex = -1;
                    }
                    // dup resource (Ctrld), add style res group, groups

                    if(bCreateBlock)
                    {
                        Chore::Resource::Block& newBlock = resource.Blocks.emplace_back();
                        newBlock.Looping = false;
                        newBlock.Scale = 1.0f;
                        newBlock.Start = CurrentTime;
                        newBlock.End = newBlock.Start + resourceLength;
                        if(newBlock.End > ViewEnd)
                        {
                            ViewEnd = newBlock.End;
                        }
                        if(ViewEnd > pChore->_Length)
                        {
                            pChore->_Length = ViewEnd;
                        }
                        Ptr<KeyframedValue<Float>> pContribution{}, pTime{};
                        for (auto& val : resource.ControlAnimation->GetAnimatedValues())
                        {
                            if (val->GetName() == "time")
                            {
                                pTime = std::dynamic_pointer_cast<KeyframedValue<Float>>(val);
                            }
                            else if (val->GetName() == "contribution")
                            {
                                pContribution = std::dynamic_pointer_cast<KeyframedValue<Float>>(val);
                            }
                        }
                        if (pTime)
                        {
                            pTime->InsertSample(newBlock.Start, 0.0f);
                            pTime->InsertSample(newBlock.End, 1.0f);
                        }
                        if(pContribution)
                        {
                            if (bCreateFades)
                            {
                                pContribution->InsertSample(newBlock.Start, 0.0f);
                                pContribution->InsertSample(newBlock.Start + resourceLength * 0.1f, 1.0f);
                                pContribution->InsertSample(newBlock.Start + resourceLength * 0.9f, 1.0f);
                                pContribution->InsertSample(newBlock.End, 0.0f);
                            }
                            else
                            {
                                pContribution->InsertSample(newBlock.Start, 1.0f);
                                pContribution->InsertSample(newBlock.End, 1.0f);
                            }
                        }
                        SelectedKeyframeSamples.clear(); // memory changed!
                    }

                    I32 currentBlockNumber = 0;
                    I32 topHighlightedBlock = -1;
                    for (auto& block : resource.Blocks)
                    {
                        const UIResourceEditorRuntimeData<Chore>::SelectedBlock* pSelectedBlock = nullptr;
                        for(const auto& selectedBlock: SelectedResourceBlocks)
                        {
                            if(selectedBlock.Resource == resNameSymbol && currentBlockNumber == selectedBlock.Index)
                            {
                                pSelectedBlock = &selectedBlock;
                                topHighlightedBlock = currentBlockNumber;
                                break;
                            }
                        }
                        Float timeStart = block.Start;
                        Float timeEnd   = block.Start + (block.End - block.Start) * block.Scale;

                        Float xStart = TO_SCREENSPACE(timeStart);
                        Float xEnd   = TO_SCREENSPACE(timeEnd);

                        if (xEnd < resourceBoxMin.x || xStart > resourceBoxMax.x)
                        {
                            // Out of view
                        }
                        else
                        {
                            xStart = MAX(xStart, resourceBoxMin.x);
                            xEnd   = MIN(xEnd, resourceBoxMax.x);

                            ImVec2 blockMin = ImVec2{ xStart, resourceBoxMin.y + 2.0f };
                            ImVec2 blockMax = ImVec2{ xEnd,   resourceBoxMax.y - 2.0f };

                            ImGui::GetWindowDrawList()->AddRectFilled(
                                blockMin, blockMax,
                                block.Looping ? IM_COL32(199, 26, 138, 255) : IM_COL32(68, 141, 184, 180),
                                2.0f
                            );

                            //  FORCE LOGIC FOR LOOPING BLOCKS
                            if (block.Looping)
                            {
                                block.Scale = 1.0f;
                                Float curLen = block.End - block.Start;
                                if (curLen < resourceLength)
                                    block.End = block.Start + resourceLength;
                            }

                            //  SELECTED BLOCK OUTLINE
                            if (pSelectedBlock)
                            {
                                ImGui::GetWindowDrawList()->AddRect( blockMin, blockMax, IM_COL32(72, 86, 94, 255), 3.0f, 0, 2.0f);
                            }

                            //  HANDLE DRAWING (LOOPING ALWAYS SHOWS)
                            Bool showHandles = block.Looping || (pSelectedBlock && ScaleMode);

                            if (showHandles)
                            {
                                ImU32 col = block.Scale == 1.0f ? IM_COL32(100, 100, 100, 255) : block.Scale > 1.0f
                                            ? IM_COL32(217, 69, 11, 255) : IM_COL32(89, 219, 13, 255);
                                ImGui::GetWindowDrawList()->AddCircleFilled(blockMin + ImVec2(2,2), 4, col);
                                ImGui::GetWindowDrawList()->AddCircleFilled(blockMax - ImVec2(2,2), 4, col);
                                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{blockMin.x, blockMax.y} + ImVec2(2,-2), 4, col);
                                ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2{blockMax.x, blockMin.y} + ImVec2(-2,2), 4, col);
                            }

                            //  INTERACTION LOGIC
                            if (pSelectedBlock)
                            {
                                if (mouseClickedThisFrame)
                                {
                                    if (ImGui::IsMouseHoveringRect(blockMin, ImVec2{blockMin.x + 4, blockMax.y}, false))
                                    {
                                        ActiveScaleMode = START;
                                        anythingClicked = true;
                                    }
                                    else if (ImGui::IsMouseHoveringRect(ImVec2{blockMax.x - 4, blockMin.y}, blockMax, false))
                                    {
                                        ActiveScaleMode = END;
                                        anythingClicked = true;
                                    }
                                }

                                Float deltaTime = (mouseDeltaX / timelineWidth) * (ViewEnd - ViewStart);

                                if (mouseDown)
                                {
                                    if (block.Looping)
                                    {
                                        Float curLen = block.End - block.Start;

                                        if (ActiveScaleMode == START)
                                        {
                                            Float newStart = block.Start + deltaTime;
                                            if (block.End - newStart < resourceLength)
                                                newStart = block.End - resourceLength;

                                            block.Start = newStart;
                                        }
                                        else if (ActiveScaleMode == END)
                                        {
                                            Float newEnd = block.End + deltaTime;
                                            if (newEnd - block.Start < resourceLength)
                                                newEnd = block.Start + resourceLength;

                                            block.End = newEnd;
                                        }
                                        else
                                        {
                                            block.Start = ImClamp(block.Start + deltaTime, 0.0f, pChore->_Length - curLen);
                                            block.End = block.Start + curLen;
                                        }
                                    }
                                    else if (ScaleMode)
                                    {
                                        Float baseLen = (block.End - block.Start);
                                        Float origLen = baseLen * block.Scale;

                                        if (ActiveScaleMode == START)
                                        {
                                            Float origEnd = block.Start + origLen;

                                            Float newLength = origLen - deltaTime;
                                            newLength = ImClamp(
                                                newLength,
                                                (20.0f / timelineWidth) * (ViewEnd - ViewStart),
                                                ViewEnd - ViewStart
                                            );

                                            Float newStart = origEnd - newLength;

                                            block.Start = ImClamp(newStart, 0.0f, origEnd - 0.001f);
                                            block.Scale = newLength / baseLen;
                                            block.End   = block.Start + baseLen;
                                        }
                                        else if (ActiveScaleMode == END)
                                        {
                                            Float newLength = origLen + deltaTime;
                                            newLength = ImClamp(
                                                newLength,
                                                (20.0f / timelineWidth) * (ViewEnd - ViewStart),
                                                ViewEnd - ViewStart
                                            );

                                            block.Scale = newLength / baseLen;
                                            block.End   = block.Start + baseLen;
                                        }
                                    }
                                    else
                                    {
                                        Float baseLen = (block.End - block.Start);
                                        Float scaledLen = baseLen * block.Scale;
                                        block.Start = ImClamp(block.Start + deltaTime, 0.0f, pChore->_Length - scaledLen);
                                        block.End = block.Start + baseLen;
                                    }
                                }
                            }
                            Bool bNeedSelect = pSelectedBlock == nullptr && SelectionBoxReady && selectionRect.Overlaps(ImRect{ blockMin, blockMax });
                            Bool blockHov = ImGui::IsMouseHoveringRect(blockMin, blockMax, false);
                            if (blockHov && mouseClickedThisFrame)
                            {
                                anythingClicked = true;
                                if (SelectionBoxReclickAvail && !usedSelectionReclick)
                                {
                                    usedSelectionReclick = true;
                                }
                                else
                                {
                                    if (pSelectedBlock)
                                    {
                                        if(ImGui::IsKeyDown(ImGuiKey_LeftShift))
                                        {
                                            for (auto it = SelectedResourceBlocks.begin(); it != SelectedResourceBlocks.end();)
                                            {
                                                if (*it == *pSelectedBlock)
                                                {
                                                    it = SelectedResourceBlocks.erase(it);
                                                    pSelectedBlock = nullptr;
                                                    break;
                                                }
                                                else
                                                {
                                                    it++;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (!ImGui::IsKeyDown(ImGuiKey_LeftShift))
                                        {
                                            SelectedResourceBlocks.clear();
                                        }
                                        bNeedSelect = true;
                                    }
                                }
                            }
                            if(bNeedSelect)
                            {
                                SelectedResourceBlocks.push_back({ resource.Name, currentBlockNumber });
                            }
                        }
                        currentBlockNumber++;
                    }
                    if(topHighlightedBlock != -1)
                    {
                        if(bDeleteBlock)
                        {
                            resource.Blocks.erase(resource.Blocks.begin() + topHighlightedBlock);
                            for(auto sit = SelectedResourceBlocks.begin(); sit != SelectedResourceBlocks.end();)
                            {
                                if(sit->Resource == resNameSymbol)
                                {
                                    if(sit->Index == topHighlightedBlock)
                                    {
                                        sit = SelectedResourceBlocks.erase(sit);
                                        continue;
                                    }
                                    else if(sit->Index > topHighlightedBlock)
                                    {
                                        sit->Index--;
                                        continue;
                                    }
                                }
                                sit++;
                            }
                        }
                        else if(bToggleLoop)
                        {
                            resource.Blocks[topHighlightedBlock].Looping = resource.Blocks[topHighlightedBlock].Looping != 0 ? 0 : 1;
                        }
                    }
                    ImGui::PushFont(ImGui::GetFont(), 12.0f);
                    ImVec2 tSize = ImGui::CalcTextSize(resource.Name.c_str());
                    ImGui::PushID(runningID++);
                    DrawResourceTexturePixels(resicon, wsize.x - 30.0f, CurrentY + 5.0f, 21.0f, 21.0f);
                    ImGui::PopID();
                    ImGui::SetCursorScreenPos(wpos + ImVec2{ wsize.x - tSize.x - 33.f, CurrentY + 12.0f });
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
                    ImGui::TextUnformatted(resource.Name.c_str());
                    ImGui::PushFont(ImGui::GetFont(), 16.0f);
                    ImGui::SetCursorScreenPos(wpos + ImVec2{ 10.f, CurrentY + 7.0f });
                    ImGui::Text("Priority: %d", resource.Priority);
                    ImGui::PopFont();
                    ImGui::PopStyleColor();
                    ImGui::PopFont();
                    Float currentX = ImClamp(TO_SCREENSPACE(CurrentTime), resourceBoxMin.x, resourceBoxMax.x);
                    if (currentX + 2.0f <= resourceBoxMax.x)
                    {
                        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ currentX, wpos.y + CurrentY }, ImVec2{ currentX + 4.0f, wpos.y + CurrentY + RES_HEIGHT }, IM_COL32(100, 100, 100, 190));
                    }
                    CurrentY += RES_HEIGHT + 2.0f;
                    if(resource.ResFlags.Test(Chore::Resource::VIEW_GRAPHS))
                    {
                        // GRAPHS FOR CONTROL ANIMATION
                        for(auto& animatedValue: resource.ControlAnimation->GetAnimatedValues())
                        {
                            ImGui::PushID(runningID++);
                            Ptr<KeyframedValue<Float>> fkf = std::dynamic_pointer_cast<KeyframedValue<Float>>(animatedValue);
                            ImGui::GetWindowDrawList()->AddRectFilled(wpos + ImVec2{ 0.0f, CurrentY }, wpos + ImVec2{ wsize.x, CurrentY + RES_HEIGHT + 2.0f }, IM_COL32(205, 212, 201, 255));
                            ImVec2 resourceBoxMin = wpos + ImVec2{ 12.0f, CurrentY };
                            ImVec2 resourceBoxMax = wpos + ImVec2{ wsize.x - 6.0f, CurrentY + RES_HEIGHT };
                            ImGui::GetWindowDrawList()->AddRectFilled(resourceBoxMin, resourceBoxMax, IM_COL32(206, 222, 173, 255));
                            ImGui::GetWindowDrawList()->AddRect(resourceBoxMin, resourceBoxMax, IM_COL32(20, 20, 20, 255));
                            ImGui::GetWindowDrawList()->PushClipRect(resourceBoxMin, resourceBoxMax, false);
                            if(ImGui::IsMouseHoveringRect(resourceBoxMin, resourceBoxMax, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            {
                                OpenContextMenuGraph = animatedValue.get();
                            }
                            Bool bLastClicked = OpenContextMenuGraph == animatedValue.get();
                            resourceBoxMin.x += 3.0f;
                            resourceBoxMax.x -= 3.0f;
                            resourceBoxMin.y += 2.0f;
                            resourceBoxMax.y -= 2.0f;
                            const Float timelineWidth = resourceBoxMax.x - resourceBoxMin.x;
                            ImGui::PushFont(ImGui::GetFont(), 12.0f);
                            ImVec2 tSize = ImGui::CalcTextSize(fkf->GetName().c_str());
                            DrawResourceTexturePixels("Chore/Graph.png", wsize.x - 30.0f, CurrentY + 5.0f, 21.0f, 21.0f);
                            ImGui::SetCursorScreenPos(wpos + ImVec2{ wsize.x - tSize.x - 33.f, CurrentY + 16.0f });
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
                            ImGui::TextUnformatted(fkf->GetName().c_str());
                            ImGui::PopStyleColor();
                            ImGui::PopFont();

                            if (OpenContextMenu("graph", resourceBoxMin, resourceBoxMax))
                                allowReclickNextFrame = true;


                            if (bLastClicked && TestMenuOption("graph", "Disable", "", 0, false, false, false, animatedValue->GetFlags().Test(AnimationValueFlags::DISABLED) ? "Enable" : nullptr))
                            {
                                animatedValue->SetDisabled(!animatedValue->GetFlags().Test(AnimationValueFlags::DISABLED));
                            }
                            if (bLastClicked && TestMenuOption("graph", "Add New Key", "[K]", ImGuiKey_K, false, false) && fkf)
                            {
                                SelectedKeyframeSamples.clear();
                                Float valueAt{1.0f};
                                Flags _{};
                                fkf->ComputeValueKeyframed(&valueAt, CurrentTime, kDefaultContribution, _, true);
                                fkf->InsertSample(CurrentTime, valueAt);
                            }
                            if(bLastClicked && TestMenuOption("graph", "Delete Selected Keys", "[BACKSPACE]", ImGuiKey_Backspace, false, false, true))
                            {
                                std::vector<I32> deleteIndices;

                                for (auto ptr : SelectedKeyframeSamples)
                                {
                                    for (I32 i = 0; i < (I32)fkf->GetSamples().size(); i++)
                                    {
                                        if (&fkf->GetSamples()[i] == ptr)
                                        {
                                            deleteIndices.push_back(i);
                                            break;
                                        }
                                    }
                                }
                                std::sort(deleteIndices.rbegin(), deleteIndices.rend());
                                for (I32 idx : deleteIndices)
                                {
                                    fkf->GetSamples().erase(fkf->GetSamples().begin() + idx);
                                }
                                SelectedKeyframeSamples.clear();
                            }

                            // EDIT SELECTED KEYS, EDIT ALL KEYS (FOR THIS ANIMATED) ??? (update sep bool if adding)
                            // STEPPED, LINEAR, FLAT, SMOOTH, CREATE GUIDE
                            // // FIT RANGE,SET RANGE (these seem to be for somethjing else)

                            if (fkf)
                            {
                                // LINES FIRST
                                Float prevSampleX = 0.0f, prevSampleY = 0.0f;
                                for (I32 i = 0; i < fkf->GetSamples().size(); i++)
                                {
                                    auto& sample = fkf->GetSamples()[i];
                                    Float sampleX = TO_SCREENSPACE(sample.Time);
                                    Float sampleY = resourceBoxMin.y + ImClamp(1.0f - sample.Value, 0.0f, 1.0f) * (resourceBoxMax.y - resourceBoxMin.y);
                                    if (i > 0)
                                    {
                                        ImGui::GetWindowDrawList()->AddLine(ImVec2{ prevSampleX, prevSampleY }, ImVec2{ sampleX, sampleY }, IM_COL32(0, 0, 0, 255));
                                    }
                                    prevSampleX = sampleX; prevSampleY = sampleY;
                                }
                                // GRAB HANDLES
                                for(I32 i = 0; i < fkf->GetSamples().size(); i++)
                                {
                                    auto& sample = fkf->GetSamples()[i];
                                    Float sampleX = TO_SCREENSPACE(sample.Time);
                                    Float sampleY = resourceBoxMin.y + ImClamp(1.0f - sample.Value, 0.0f, 1.0f) * (resourceBoxMax.y - resourceBoxMin.y);
                                    ImVec2 sampleGrabMin = ImVec2{ sampleX - 3.0f, sampleY - 2.0f };
                                    ImVec2 sampleGrabMax = ImVec2{ sampleX + 3.0f, sampleY + 2.0f };
                                    if (SelectionBoxReady && selectionRect.Overlaps(ImRect{ sampleGrabMin, sampleGrabMax }))
                                        SelectedKeyframeSamples.insert(&sample);
                                    if (ImGui::IsMouseHoveringRect(sampleGrabMin - ImVec2{3.0f, 3.0f}, sampleGrabMax + ImVec2{ 3.0f, 3.0f }, false) && mouseClickedThisFrame)
                                    {
                                        if (SelectionBoxReclickAvail && !usedSelectionReclick)
                                        {
                                            usedSelectionReclick = true; // reselect in selection box
                                        }
                                        else
                                        {
                                            if (!ImGui::IsKeyDown(ImGuiKey_LeftShift))
                                            {
                                                SelectedKeyframeSamples.clear();
                                            }
                                            SelectedKeyframeSamples.insert(&sample);
                                        }
                                        anySamplesClicked = true;
                                        anythingClicked = true;
                                    }
                                    Bool bImSelec = SelectedKeyframeSamples.find(&sample) != SelectedKeyframeSamples.end();
                                    ImGui::GetWindowDrawList()->AddRectFilled(sampleGrabMin, sampleGrabMax, bImSelec ? IM_COL32(12, 105, 13, 255) : IM_COL32(8, 69, 9, 255));
                                    if(bImSelec && mouseDown)
                                    {
                                        Float deltaTime = (mouseDeltaX / timelineWidth) * (ViewEnd - ViewStart);
                                        // into 0-1 space
                                        Float deltaValue = -(mouseDeltaY / (resourceBoxMax.y - resourceBoxMin.y));
                                        Float newTime = sample.Time + deltaTime;
                                        Float newValue = sample.Value + deltaValue;
                                        if (i > 0)
                                            newTime = ImMax(newTime, fkf->GetSamples()[i - 1].Time + 0.0001f);
                                        if (i < fkf->GetSamples().size() - 1)
                                            newTime = ImMin(newTime, fkf->GetSamples()[i + 1].Time - 0.0001f);
                                        newTime = ImClamp(newTime, 0.0f, pChore->GetLength());
                                        newValue = ImClamp(newValue, 0.0f, 1.0f);
                                        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
                                        {
                                            // only do one axis, (with most movement)
                                            if (fabsf(mouseDeltaX) > fabsf(mouseDeltaY))
                                                sample.Time = newTime;
                                            else
                                                sample.Value = newValue;
                                        }
                                        else
                                        {
                                            sample.Time = newTime;
                                            sample.Value = newValue;
                                        }

                                        //Float newSampleX = ImClamp(FROM_SCREENSPACE(ImGui::GetMousePos().x),
                                        //    i == 0 ? 0.0f : fkf->GetSamples()[i - 1].Time + 0.0001f, i == fkf->GetSamples().size() - 1 ? pChore->GetLength() : fkf->GetSamples()[i + 1].Time);
                                        //Float newSampleY = ImClamp(((ImGui::GetMousePos().y - resourceBoxMin.y) / (resourceBoxMax.y - resourceBoxMin.y)), 0.0f, 1.0f);
                                        //sample.Time = newSampleX;
                                        //sample.Value = 1.0f - newSampleY;
                                    }
                                }
                                if (fkf->GetSamples().size() > 1)
                                {
                                    ImGui::GetWindowDrawList()->AddLine(ImVec2{ TO_SCREENSPACE(fkf->GetSamples()[0].Time), resourceBoxMin.y - 2.0f }, 
                                        ImVec2{ TO_SCREENSPACE(fkf->GetSamples()[0].Time), resourceBoxMax.y + 2.0f }, IM_COL32(0, 0, 0, 255));
                                    ImGui::GetWindowDrawList()->AddLine(ImVec2{ TO_SCREENSPACE(fkf->GetMaxTime()), resourceBoxMin.y - 2.0f }, 
                                        ImVec2{ TO_SCREENSPACE(fkf->GetMaxTime()), resourceBoxMax.y + 2.0f }, IM_COL32(0, 0, 0, 255));
                                }
                            }
                            else
                            {
                                ImGui::SetCursorScreenPos(resourceBoxMin);
                                ImGui::Text("UNK_GRAPH:%s", animatedValue->GetValueType().name());
                            }
                            ImGui::PopClipRect();
                            CurrentY += RES_HEIGHT + 2.0f;
                            ImGui::PopID();
                        }
                    }
                    ImGui::PopID();
                    resourceIndex++;
                }
                if(rmResource != -1)
                {
                    I32 concreteResourceIndex = agent.Resources[rmResource];
                    agent.Resources.erase(agent.Resources.begin() + rmResource);
                    Bool bHasRef = false;
                    for(auto rit = pChore->_Agents.begin(); rit != pChore->_Agents.end(); rit++)
                    {
                        for(const auto& resIndex: rit->Resources)
                        {
                            if(resIndex == concreteResourceIndex)
                            {
                                bHasRef = true;
                                break;
                            }
                        }
                        if (bHasRef)
                            break;
                    }
                    if(!bHasRef)
                    {
                        // remove actual resource
                        pChore->_DoRemoveResource(concreteResourceIndex);
                    }
                }
            }
            ImGui::PopID();
        }
        SelectionBoxReady = false;
        if(!anythingClicked && mouseClickedThisFrame)
        {
            if (SelectionBoxReclickAvail && !usedSelectionReclick)
            {
                usedSelectionReclick = true; // reselect in selection box
            }
            else
            {
                SelectedKeyframeSamples.clear();
                SelectedResourceBlocks.clear();
                SelectionBox1X = SelectionBox2X = ImGui::GetMousePos().x;
                SelectionBox1Y = SelectionBox2Y = ImGui::GetMousePos().y;
                SelectionBoxDragging = true;
            }
        }
        if(SelectionBoxDragging)
        {
            SelectionBox2X = ImGui::GetMousePos().x;
            SelectionBox2Y = ImGui::GetMousePos().y;
            ImGui::GetWindowDrawList()->AddRect(ImVec2{ MIN(SelectionBox1X, SelectionBox2X), MIN(SelectionBox1Y, SelectionBox2Y) }, 
                ImVec2{ MAX(SelectionBox1X, SelectionBox2X), MAX(SelectionBox1Y, SelectionBox2Y) }, IM_COL32(80, 80, 80, 255), 2.0f, 0, 3.0f);
            if(mouseReleasedThisFrame)
            {
                ImVec2 mn = ImMin(ImVec2{ SelectionBox1X, SelectionBox1Y }, ImVec2{ SelectionBox2X, SelectionBox2Y });
                ImVec2 mx = ImMax(ImVec2{ SelectionBox1X, SelectionBox1Y }, ImVec2{ SelectionBox2X, SelectionBox2Y });
                SelectionBox1X = mn.x; SelectionBox1Y = mn.y;
                SelectionBox2X = mx.x; SelectionBox2Y = mx.y;
                SelectionBoxDragging = false;
                SelectionBoxReady = true;
                SelectionBoxReclickAvail = true;
            }
        }
        if (usedSelectionReclick)
        {
            SelectionBoxReclickAvail = false;
        }
        if (allowReclickNextFrame)
        {
            SelectionBoxReclickAvail = true;
        }
        Float dy = CurrentY - cache;
        ImGui::SetCursorScreenPos(wpos + ImVec2{0.0f, cache});
        ImGui::Dummy(ImVec2{ 1.0f, dy });
        ImGui::SetCursorScreenPos(wpos);
        ImGui::GetWindowDrawList()->ChannelsMerge();
        LastMouseX = ImGui::GetMousePos().x;
        LastMouseY = ImGui::GetMousePos().y;
    }
    ImGui::End();
    return closing;
}
