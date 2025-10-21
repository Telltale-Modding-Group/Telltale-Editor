#include <UI/EditorUI.hpp>
#include <UI/ApplicationUI.hpp>

#include <imgui.h>
#include <imgui_internal.h>

using namespace ImGui;
DECL_VEC_ADDITION();

// ===================================================== MAIN EDITOR UI 


EditorUI::EditorUI(ApplicationUI& app) : UIStackable(app), _MenuBar(*this), _EditorScene(app.GetRegistry()), _UpdateTicker(30), _LoadingScenePreload(UINT32_MAX)
{
    SDL_SetWindowResizable(GetApplication()._Window, true);
    _Views.push_back(TTE_NEW_PTR(FileView, MEMORY_TAG_EDITOR_UI, *this));
    _Views.push_back(TTE_NEW_PTR(OutlineView, MEMORY_TAG_EDITOR_UI, *this));

    Ptr<InspectorView> inspectorView = TTE_NEW_PTR(InspectorView, MEMORY_TAG_EDITOR_UI, *this);
    _Views.push_back(inspectorView);
    _InspectorView = inspectorView;

    Ptr<SceneView> sceneView = TTE_NEW_PTR(SceneView, MEMORY_TAG_EDITOR_UI, *this);
    _Views.push_back(sceneView);
    _SceneView = sceneView;
}

void EditorUI::OnFileClick(const String& resourceLocation)
{
    if(_TickAsyncLoadingScene())
    {
        PlatformMessageBoxAndWait("Scene already loading", "There is currently a scene loading! Please wait...");
    }
    else
    {
        String ext = ToLower(FileGetExtension(resourceLocation));
        if(ext == "scene")
        {
            if(!_EditorScene.GetName().empty())
            {
                TTE_LOG("Unloading scene %s", resourceLocation.c_str(), _LoadingScenePreload);
                _SceneView.lock()->_OnSceneUnload(TTE_PROXY_PTR(&_EditorScene, Scene));
            }
            ResourceAddress addr = GetApplication().GetRegistry()->CreateResolvedAddress(resourceLocation, false);
            GetApplication().GetRegistry()->ResourceSetEnable(addr.GetLocationName()); // ensure resource set for scene (new engine)
            std::vector<HandleBase> handles{};
            HandleBase hScene{};
            hScene.SetObject(addr.GetName());
            handles.push_back(std::move(hScene));
            _LoadingScenePreload = GetApplication().GetRegistry()->Preload(std::move(handles), true);
            _LoadingAsyncScene = addr.GetName();
            InspectingNode.reset();
            IsInspectingAgent = false;
            TTE_LOG("Attempting to load scene %s with preload batch %u", resourceLocation.c_str(), _LoadingScenePreload);
        }
        else
        {
            TTE_LOG("WARNING: Cannot open file type %s yet!", resourceLocation.c_str());
        }
    }
}

Bool EditorUI::_SceneInitialiseJob(const JobThread& thread, void* uA, void* uB)
{
    TTE_LOG("Initialising and setting up scene for editor: %s", ((Scene*)uA)->Name.c_str());
    ((Scene*)uA)->_SetupAgentsModules();
    return true;
}

Bool EditorUI::_TickAsyncLoadingScene()
{
    if(_InitSceneJob)
    {
        if(JobScheduler::Instance->IsFinished(_InitSceneJob))
        {
            _InspectorView.lock()->OnSceneChange();

            _EditorScene.~Scene(); // hacky :( (move constructor is annoying)
            new (&_EditorScene) Scene(std::move(*_AsyncInitScene.get()));
            _AsyncInitScene.reset();
            _InitSceneJob.Reset();
            _LoadingAsyncScene.clear();

            // ON SCENE LOAD
            _SceneView.lock()->_OnSceneLoad(TTE_PROXY_PTR(&_EditorScene, Scene));

            return false;
        }
        return true;
    }
    if(_LoadingScenePreload == UINT32_MAX)
        return false;
    if (GetApplication().GetRegistry()->GetPreloadOffset() >= _LoadingScenePreload)
    {
        Handle<Scene> hScene{};
        hScene.SetObject(_LoadingAsyncScene);
        Ptr<Scene> loaded = hScene.GetObject(GetApplication().GetRegistry(), true);
        if(loaded)
        {
            _AsyncInitScene = std::static_pointer_cast<Scene, Handleable>(loaded->Clone());
            JobDescriptor desc{};
            // kick off init job
            desc.AsyncFunction = &_SceneInitialiseJob;
            desc.UserArgA = _AsyncInitScene.get();
            desc.UserArgB = nullptr;
            desc.Priority = JOB_PRIORITY_NORMAL;
            _InitSceneJob = JobScheduler::Instance->Post(desc);
        }
        else
        {
            TTE_LOG("ERROR: The scene %s could not be loaded from the resource registry", _LoadingAsyncScene.c_str());
        }
        _LoadingScenePreload = UINT32_MAX;
    }
    return true;
}

void EditorUI::Render()
{

    if(_UpdateTicker.Tick())
    {
        _TickAsyncLoadingScene();
    }

    _MenuBar.Render();
    for(auto& c: _Views)
        c->Render();
}

U32 EditorUIComponent::GetUICondition()
{
#ifdef DEBUG
    return ImGuiCond_Always; // keep window sizes for debugging
#else
    return _EditorUI.UIFlags.Test(EditorFlag::RESETTING_UI) ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
#endif
}

// ===================================================== FILE VIEW

FileView::FileView(EditorUI& app) : EditorUIComponent(app)
{
    _IconMap["*.dlg"] = { "FileView/Dialog_DLG_DLOG.png", "StoryBoard Dialog File" };
    _IconMap["*.dlog"] = { "FileView/Dialog_DLG_DLOG.png", "StoryBoard Dialog File" };
    _IconMap["*.anm"] = { "FileView/Animation_ANM.png", "Animation File" };
    _IconMap["*.scene"] = { "FileView/Scene_SCENE.png", "Scene File" };
}

void FileView::_Gather(Ptr<ResourceRegistry> pRegistry, std::vector<String>& entries, FolderGroup group, String parent)
{
    if(group == MOUNTS)
    {
        _StrippedToLocation.clear();
        pRegistry->GetResourceLocationNames(entries);
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            String orig = *it;
            if(StringEndsWith(*it, "/"))
                *it = it->substr(0, it->length() - 1);
            StringReplace(*it, "<", "");
            StringReplace(*it, ">", "");
            if(orig != *it)
                _StrippedToLocation[*it] = orig;
        }
        for (auto it = entries.begin(); it != entries.end(); it++)
        {
            if (it->empty())
            {
                entries.erase(it); // remove main
                break;
            }
        }
    }
    else if(group == RESOURCE_GROUPS)
    {
        String location{};
        auto it = _StrippedToLocation.find(parent);
        location = it == _StrippedToLocation.end() ? parent : it->second;
        std::set<String> r{};
        for(const auto& pr: Meta::GetInternalState().GetActiveGame().FolderAssociates)
        {
            StringMask mask = pr.first;
            pRegistry->GetLocationResourceNames(location, r, &mask);
            if(!r.empty())
            {
                String v = pr.second;
                if (StringEndsWith(v, "/"))
                    v = v.substr(0, v.length() - 1);
                entries.push_back(v);
                r.clear();
            }
        }
    }
    else
    {
        String location{};
        auto it = _StrippedToLocation.find(_ViewStack.front());
        location = it == _StrippedToLocation.end() ? _ViewStack.front() : it->second;
        std::set<String> r{};
        for (const auto& pr : Meta::GetInternalState().GetActiveGame().FolderAssociates)
        {
            if(pr.second == parent || pr.second == (parent + "/"))
            {
                StringMask mask = pr.first;
                pRegistry->GetLocationResourceNames(location, r, &mask);
                for(const auto& res: r)
                    entries.push_back(res);
            }
        }
    }
}

Bool FileView::_Update(Ptr<ResourceRegistry> pRegistry, I32 overrideIndex)
{
    U64 now = GetTimeStamp();
    if(overrideIndex == -1)
    {
        if (GetTimeStampDifference(_Group[_CurGroup].UpdateStamp, now) > 3.0f)
        {
            _Group[_CurGroup].UpdateStamp = now;
            _Group[_CurGroup].Entries.clear();
            _Gather(pRegistry, _Group[_CurGroup].Entries, _CurGroup, _ViewStack.empty() ? "" : _ViewStack.back());
            return false; // dont reset scroll
        }
    }
    else
    {
        String selected = _Group[_CurGroup].Entries[overrideIndex];
        if(_CurGroup == FILES)
        {
            String location{};
            auto it = _StrippedToLocation.find(_ViewStack.front());
            location = it == _StrippedToLocation.end() ? _ViewStack.front() : it->second;
            String resLoc = "logical://" + location + selected;
            _EditorUI.OnFileClick(resLoc);
        }
        else
        {
            _ViewStack.push_back(selected);
            _Group[_CurGroup].Entries.clear();
            _Group[_CurGroup].UpdateStamp = 0;
            _CurGroup = (FolderGroup)(_CurGroup + 1);
            _Group[_CurGroup].Entries.clear();
            _Group[_CurGroup].UpdateStamp = now;
            _Gather(pRegistry, _Group[_CurGroup].Entries, _CurGroup, selected);
            return true;
        }
    }
    return false;
}

void FileView::Render()
{
    Ptr<ResourceRegistry> pRegistry = GetApplication().GetRegistry();
    Bool bReset = _Update(pRegistry, -1);
    SetNextWindowViewport(0.2f, 0.70f, 0, 0, 0.80f, 0.30f, 200, 100, GetUICondition());
    if(ImGui::Begin(GetApplication().GetLanguageText("fv.title").c_str(), NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        if(bReset)
            SetScrollY(0.0f);
        Float scrollY = GetScrollY();
        ImVec2 size = GetWindowSize();
        ImVec2 winPos = GetWindowPos();
        Float x = 150.0f, y = ImGui::GetCursorPos().y;

        // BG
        GetWindowDrawList()->AddRectFilledMultiColor(winPos, winPos + ImVec2{140.f, size.y}, IM_COL32(60, 60, 60, 255), IM_COL32(60, 60, 60, 255), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 50, 255));
        GetWindowDrawList()->AddLine(winPos + ImVec2{ 140.f, 0.0f }, winPos + ImVec2{ 140.f, size.y }, IM_COL32(120, 17, 47, 255), 2.0f);

        // OPTIONS
        Bool bResetScroll = false;
        DrawCenteredWrappedText(GetApplication().GetLanguageText("fv.sidebar").c_str(), 120.f, winPos.x + 70.f, winPos.y + 30.f, 1, GetFontSize());
        SetCursorScreenPos(winPos + ImVec2{ 15.0f, 50.0f });
        PushFont(GetApplication().GetEditorFont(), 12.0f);
        if(Button(GetApplication().GetLanguageText("fv.back").c_str()) && (I32)_CurGroup > 0)
        {
            _ViewStack.pop_back();
            _Group[_CurGroup].Entries.clear();
            _Group[_CurGroup].UpdateStamp = 0;
            _CurGroup = (FolderGroup)(_CurGroup - 1);
            _Group[_CurGroup].Entries.clear();
            _Group[_CurGroup].UpdateStamp = GetTimeStamp();
            _Gather(pRegistry, _Group[_CurGroup].Entries, _CurGroup, _ViewStack.size() ? _ViewStack.back() : "");
            bResetScroll = true;
        }
        SetCursorScreenPos(winPos + ImVec2{10.0f, 80.0f});
        PushStyleColor(ImGuiCol_Text, IM_COL32(30, 30, 30, 255));
        PushTextWrapPos(130.0f);
        String text = "> ";
        if(_ViewStack.size())
        {
            text = "";
            I32 i = 0;
            for(auto it = _ViewStack.begin(); it != _ViewStack.end(); it++, i++)
            {
                text += *it;
                if(i != _ViewStack.size() - 1)
                {
                    text += " > ";
                }
            }
        }
        TextUnformatted(text.c_str());
        PopStyleColor();
        PopTextWrapPos();
        PopFont();

        // ENTRIES
        PushFont(GetApplication().GetEditorFont(), 10.0f);
        const Float Spacing = 110.0f, Icon = 90.f;
        I32 index = 0;
        String ico = "";
        String localDesc{};
        const String* pDesc = nullptr;
        for(const auto& mp: _Group[_CurGroup].Entries)
        {
            ImGui::PushID(index);
            if(x + 110.0f >= size.x)
            {
                x = 150.0f;
                y += Spacing;
            }
            Float localY = y - scrollY;
            SetCursorPosX(x);
            if(_CurGroup == FILES)
            {
                if (ico.empty())
                {
                    for (const auto& [icomp, mapp] : _IconMap)
                    {
                        if (icomp == mp)
                        {
                            pDesc = &mapp.Description;
                            ico = mapp.IconFile;
                            break;
                        }
                    }
                    if(ico.empty())
                        ico = "FileView/UnknownFile.png";
                }
                DrawResourceTexturePixels(ico, x, localY, Icon, Icon, 0xFFFFFFFFu, mp.c_str());
                if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("TT_ASSET", mp.c_str(), mp.length() + 1);
                    ImGui::TextUnformatted(mp.c_str());
                    ImGui::EndDragDropSource();
                }
            }
            else
            {
                //localDesc = "Virtual Folder";
                //pDesc = &localDesc;
                DrawResourceTexturePixels("FileView/Folder.png", x, localY, Icon, Icon);
            }
            ImVec2 center = winPos + ImVec2{ x + Icon * 0.5f, localY + Icon };
            DrawCenteredWrappedText(mp.length() > 25 ? "..." + mp.substr(mp.length() - 25, 25) : mp, Icon * 0.95f, center.x, center.y, 2, 10.f);
            Bool Hov = ImGui::IsMouseHoveringRect(winPos + ImVec2{ x, localY }, winPos + ImVec2{ x + Icon, localY + Icon });
            if(Hov)
            {
                ImGui::BeginTooltip();
                if(pDesc)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                    ImGui::TextUnformatted(pDesc->c_str());
                    ImGui::PopStyleColor();
                }
                else if(_CurGroup == FILES)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(250, 150, 150, 255));
                    ImGui::TextUnformatted(GetApplication().GetLanguageText("fv.unsupported_file").c_str());
                    ImGui::PopStyleColor();
                }
                ImGui::TextUnformatted(mp.c_str());
                ImGui::EndTooltip();
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && Hov)
            {
                if (_CurGroup != FILES)
                    bResetScroll = true;
                _Update(pRegistry, index);
                ImGui::PopID();
                break; // continue next
            }
            x += Spacing;
            index++;
            ImGui::PopID();
        }
        SetCursorScreenPos(winPos);
        Dummy(ImVec2(150.0f, y - scrollY + Spacing));
        PopFont();
        if(bResetScroll)
        {
            ImGui::SetScrollY(0.0f);
        }
    }
    ImGui::End();
}

// ===================================================== OUTLINE VIEW

NewAgentPopup::NewAgentPopup(OutlineView* ov) : EditorPopup("New Agent"), _OV(ov)
{
    memset(_Input, 0, 48);
}

Bool NewAgentPopup::Render()
{
    Bool end = false;
    ImGui::Text("Name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250.0f);
    ImGui::InputText("##in", _Input, 48);
    Bool ok = Editor->GetActiveScene().GetAgents().find(Symbol(_Input)) == Editor->GetActiveScene().GetAgents().end();
    if(!ok)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 50, 50, 255));
        ImGui::Text("An agent already exists with this name!");
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::TextUnformatted("");
    }
    if(ImGui::Button("Create") && ok && _Input[0] != 0)
    {
        end = true;
        Editor->GetActiveScene().AddAgent(_Input, {}, {}, {}, true); // no props by default (maybe create defaults?) this is done by scene 
        memset(_Input, 0, 48);
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel"))
    {
        end = true;
    }
    return end;
}

ImVec2 NewAgentPopup::GetPopupSize()
{
    return { 400.f, 125.f };
}

OutlineView::OutlineView(EditorUI& ui) : EditorUIComponent(ui)
{
    memset(_ContainTextFilter, 0, 32);
}

Bool OutlineView::_RenderSceneNode(WeakPtr<Node> pNodeWk)
{
    Node* pNode = pNodeWk.lock().get();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if(pNode->FirstChild.expired())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if(TreeNodeEx(pNode->Name.c_str(), flags))
    {
        if(IsItemHovered(ImGuiHoveredFlags_None) && IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            _EditorUI.InspectingNode = pNodeWk;
            _EditorUI.IsInspectingAgent = pNode->Parent.use_count() == 0;
        }
        Ptr<Node> pChild = pNode->FirstChild.lock();
        while (pChild)
        {
            _RenderSceneNode(pChild);
            pChild = pChild->NextSibling.lock();
        }
        TreePop();
    }
    return false;
}

void OutlineView::Render()
{
    Float yOff = MAX(0.04f, 40.f / GetMainViewport()->Size.y);
    SetNextWindowViewport(0.0f, yOff, 0, 0, 0.20f, 1.00f - yOff, 100, 300, GetUICondition());
    if (ImGui::Begin(GetApplication().GetLanguageText("ov.title").c_str(), NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        if (!_EditorUI.GetActiveScene().GetName().empty())
        {
            SetCursorPosY(50.0f); // option bar
            ImGui::GetWindowDrawList()->PushClipRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
            // BG
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2{ 0.0f, 40.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - 15.0f, ImGui::GetWindowSize().y }, IM_COL32(40, 40, 40, 255));
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos() + ImVec2{ 0.0f, 21.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - 15.0f, 40.0f }, IM_COL32(60, 60, 60, 255));
            ImGui::GetWindowDrawList()->AddLine(ImGui::GetWindowPos() + ImVec2{ 0.0f, 40.0f }, ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - 15.0f, 40.0f }, IM_COL32(30, 30, 30, 255), 1.0f);

            // FILTER BAR
            ImVec2 textMin{ MAX(50.0f, ImGui::GetWindowSize().x - 150.0f), 22.0f };
            ImVec2 textMax{ ImGui::GetWindowSize().x - 20.0f, 38.0f };
            ImGui::SetNextItemWidth(textMax.x - textMin.x);
            ImVec2 c = ImGui::GetCursorPos();
            ImGui::SetCursorPos(textMin);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
            ImGui::PushFont(ImGui::GetFont(), textMax.y - textMin.y - 5.0f);
            ImGui::InputTextWithHint("##filterbox", _EditorUI.GetApplication().GetLanguageText("ov.filter").c_str(), _ContainTextFilter, sizeof(_ContainTextFilter));
            ImGui::PopFont();
            ImGui::SetCursorPos(c);
            ImGui::PopStyleVar();

            ImVec2 buttonMin{ ImVec2{2.0f, 24.0f} };
            ImVec2 buttonExtent{ 14.0f, 14.0f };
            _EditorUI.DrawResourceTexturePixels("Scene/AgentNew.png", buttonMin.x, buttonMin.y, buttonExtent.x, buttonExtent.y);
            if (ImGui::IsMouseHoveringRect(GetWindowPos() + buttonMin, GetWindowPos() + buttonMin + buttonExtent, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                _EditorUI.GetApplication().SetCurrentPopup(TTE_NEW_PTR(NewAgentPopup, MEMORY_TAG_EDITOR_UI, this), _EditorUI);
            }

            ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(60, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(60, 60, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(80, 80, 80, 255));
            for (const auto& agent : _EditorUI.GetActiveScene().GetAgents())
            {
                if (_ContainTextFilter[0] != 0)
                {
                    std::size_t pos = 0;
                    Bool no = false;
                    for (const char* v = _ContainTextFilter; *v; v++)
                    {
                        if ((pos = agent.second->Name.find(v, pos)) == std::string::npos)
                        {
                            no = true;
                            break;
                        }
                    }
                    if (no)
                        continue;
                }
                if (_RenderSceneNode(agent.second->AgentNode))
                    break;
            }
            ImGui::GetWindowDrawList()->PopClipRect();
            ImGui::PopStyleColor(3);
            Dummy(GetCursorPos());
        }
    }
    End();
}

// ===================================================== OUTLINE VIEW

InspectorView::InspectorView(EditorUI& ui) : EditorUIComponent(ui)
{
}

extern TelltaleEditor* _MyContext;

void InspectorView::OnSceneChange()
{
    _CurrentIsAgent = false;
    _CurrentNode = {};
    _LastPropFlush = 0; // unless user clicks under 100ms an edit of a prop to clicking a new scene, this is ok lol
    _RuntimeModuleProps.clear();
}

void InspectorView::_InitModuleCache(Meta::ClassInstance agentProps, SceneModuleType module)
{
    if(_RuntimeModuleProps.find(module) == _RuntimeModuleProps.end())
    {
        SceneModuleUtil::_GetModuleInfo Getter{module};
        String moduleID{};
        String moduleName{};
        Getter.OutName = &moduleName;
        Getter.OutID = &moduleID;
        SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, std::move(Getter));
        auto it = _MyContext->_ModuleVisualProperties.find(moduleID);
        if(it != _MyContext->_ModuleVisualProperties.end())
        {
            std::vector<PropertyRuntimeInstance> props{};
            for(const auto& entry: it->second.VisualProperties)
            {
                PropertyRuntimeInstance& runtime = props.emplace_back();
                runtime.Specification = &entry;
                runtime.Property = PropertySet::Get(agentProps, entry.Key, true, _EditorUI.GetApplication().GetRegistry());
                if (runtime.Property)
                {
                    for (const auto& sp : runtime.Specification->SubPaths)
                    {
                        Meta::ClassInstance mem = Meta::GetMember(runtime.Property, sp, false);
                        if (!mem)
                        {
                            TTE_LOG("ERROR: Sub paths in %s, specifically %s do not point to an existing member in %s", moduleName.c_str(), sp.c_str(), Meta::GetClass(runtime.Property.GetClassID()).Name.c_str());
                            return;
                        }
                        runtime.Property = std::move(mem);
                    }
                }
            }

            std::sort(props.begin(), props.end(), [](const PropertyRuntimeInstance& lhs, const PropertyRuntimeInstance& rhs)
            {
                return lhs.Specification->Name < rhs.Specification->Name;
            });
            _RuntimeModuleProps[module] = PropertyRuntimeGroup{ std::move(moduleName), it->second.ImagePath, std::move(props), 40.0f, false };
        }
    }
}

namespace PropertyRenderFunctions
{

    void RenderEnum(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {

        if(adapter.SubPaths.empty())
        {
            ImGui::Text("!EnumSubPathsEmpty!"); // need sub paths for member name which is an enum member (as a class isnt an enum, a MEMBER is)
            return;
        }

        const Meta::Class* EnumClass = &Meta::GetClass(adapter.ClassID);
        U32& EnumSelected = *((U32*)datum._GetInternal());

        // Mid path members
        for(U32 i = 0; i + 1 < adapter.SubPaths.size(); i++)
        {
            Bool bOk = false;
            for(const auto& member: EnumClass->Members)
            {
                if(member.Name == adapter.SubPaths[i])
                {
                    bOk = true;
                    EnumClass = &Meta::GetClass(member.ClassID);
                    break;
                }
            }
            if(!bOk)
            {
                ImGui::Text("!SubPathMemberNotFound!");
                return;
            }
        }

        // Final path member
        for (const auto& member : EnumClass->Members)
        {
            if (member.Name == adapter.SubPaths[adapter.SubPaths.size()-1])
            {
                CString curSelected = "UNKNOWN_VALUE";
                for(const auto& enumDesc: member.Descriptors)
                {
                    if(EnumSelected == enumDesc.Value)
                    {
                        curSelected = enumDesc.Name.c_str();
                        break;
                    }
                }
                if(ImGui::BeginCombo("##cmb", curSelected))
                {
                    for(const auto& enumDesc: member.Descriptors)
                    {
                        if(ImGui::Selectable(enumDesc.Name.c_str()))
                        {
                            EnumSelected = enumDesc.Value;
                        }
                    }
                    ImGui::EndCombo();
                }
                return;
            }
        }

        ImGui::Text("!SubPathMemberNotFound!");
        return;
    }

    // 2 to 4
    Bool _DrawVectorInput(const char* label, float* valArray, U32 n, Bool bCol)
    {
        if (label)
        {
            ImGui::PushID(label);
            ImGui::Text("%s", label);
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        bool changed = false;
        const float item_width = 55.0f;
        const ImVec2 label_size = ImGui::CalcTextSize("X");
        char Txt[8]{ 'X', 0, 'Y', 0, 'Z', 0, 'W', 0 };
        if(bCol)
        {
            Txt[0] = 'R';
            Txt[2] = 'G';
            Txt[4] = 'B';
            Txt[6] = 'A';
        }

        // Colors for RGB
        ImU32 colors[4] = {
            IM_COL32(200,  40,  40, 255),  // red background
            IM_COL32(40, 200,  40, 255),   // green background
            IM_COL32(40,  40, 200, 255),   // blue background
            IM_COL32(130, 130, 130, 255),   // grey background
        };

        ImGui::SetCursorPos(ImVec2(8.0f, ImGui::GetCursorPosY())); // start x=8, keep y

        for (int i = 0; i < n; i++)
        {
            ImVec2 pos = ImGui::GetCursorPos();  // relative to window content

            ImVec2 rect_size = ImVec2(label_size.x + 8, label_size.y + 4);

            // Convert pos to screen pos for drawing rect/text
            ImVec2 screen_pos = ImGui::GetCursorScreenPos();

            drawList->AddRectFilled(screen_pos, screen_pos + rect_size, colors[i], 0.0f);

            ImVec2 text_pos = ImVec2(
                screen_pos.x + (rect_size.x - label_size.x) * 0.5f,
                screen_pos.y + (rect_size.y - label_size.y) * 0.5f + 1.0f
            );

            drawList->AddText(text_pos, IM_COL32(255, 255, 255, 255), Txt + (i << 1));

            // Move cursor right after colored box (relative)
            ImGui::SetCursorPosX(pos.x + rect_size.x);

            ImGui::PushItemWidth(item_width);

            if (ImGui::InputFloat(
                (std::string("##") + (label ? label : "") + (char)('X' + i)).c_str(),
                &valArray[i],
                0.0f, 0.0f, "%.3f",
                ImGuiInputTextFlags_CharsDecimal))
            {
                changed = true;
            }
            ImGui::PopItemWidth();

            if (i < n-1)
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2.0f); // gap between groups
            }
        }

        if (label)
            ImGui::PopID();

        return changed;
    }

}

struct AddModulePopup : EditorPopup
{

    struct ModuleDescription
    {
        String Name, Desc;
        SceneModuleType Ty;
    };

    String Selected = "";
    const String* Desc = nullptr;
    SceneModuleType SelType;
    std::vector<ModuleDescription> AvailModules{};
    WeakPtr<Node> AgentNode;

    AddModulePopup(SceneModuleTypes existingModules, Ptr<Node> nod, EditorUI* editor) : EditorPopup("Add Module"), AgentNode(nod), SelType()
    {
        Editor = editor;
        std::vector<std::pair<SceneModuleType, CString>> moduleIDs{};
        SceneModuleUtil::PerformRecursiveModuleOperation<SceneModuleUtil::_ModuleIDCollector>(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_ModuleIDCollector{ moduleIDs });
        for (const auto& id : moduleIDs)
        {
            if (!existingModules[id.first] && id.first != SceneModuleType::SCENE)
            {
                AvailModules.push_back(ModuleDescription{ Editor->GetLanguageText(String(String("mod.") + String(id.second) + ".name").c_str()),
                                                         Editor->GetLanguageText(String(String("mod.") + String(id.second) + ".desc").c_str()),
                                                         id.first });
            }
        }
    }

    ImVec2 GetPopupSize() override
    {
        return { 350.0f, 130.f };
    }

    Bool Render() override
    {
        Ptr<Node> pNode = AgentNode.lock();
        if (!pNode)
        {
            return true;
        }
        Bool closing = false;
        if (AvailModules.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
            ImGui::TextUnformatted(Editor->GetLanguageText("mod.used").c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextUnformatted(Editor->GetLanguageText("mod.select").c_str());
            if (ImGui::BeginCombo("##cmb", Selected.c_str()))
            {
                for (const auto& avail : AvailModules)
                {
                    if (ImGui::Selectable(avail.Name.c_str()))
                    {
                        Selected = avail.Name;
                        Desc = &avail.Desc;
                        SelType = avail.Ty;
                    }
                }
                ImGui::EndCombo();
            }
            if (!Selected.empty() && Desc)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 150, 50, 255));
                ImGui::TextWrapped("%s",Desc->c_str());
                ImGui::PopStyleColor();
            }
            if (ImGui::Button("Add"))
            {
                closing = true;
                pNode->AttachedScene->AddAgentModule(pNode->AgentName, SelType);
                SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_SetupAgentModule{ *pNode->AttachedScene, pNode, SelType });
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Cancel"))
        {
            closing = true;
        }
        return closing;
    }

};

U32 luaRegisterModuleUI(LuaManager& man)
{
    String modid = man.ToString(1);
    ModuleUI elem{};

    // PROP KEYS
    ITERATE_TABLE(it, 3)
    {

        PropertyVisualAdapter adapter{};

        adapter.Name = man.ToString(it.KeyIndex());

        man.PushLString("Key");
        man.GetTable(it.ValueIndex());
        adapter.Key = ScriptManager::PopString(man);

        man.PushLString("Class");
        man.GetTable(it.ValueIndex());
        String ty = ScriptManager::PopString(man);
        adapter.ClassID = Meta::FindClass(ty, 0, false);

        man.PushLString("UI");
        man.GetTable(it.ValueIndex());

        man.PushLString("InputType");
        man.GetTable(man.GetTop()-1);
        String inputType = ScriptManager::PopString(man);

        man.PushLString("SubPath");
        man.GetTable(man.GetTop()-1);
        String subpath = ScriptManager::PopString(man);

        man.Pop(1);

        if (adapter.ClassID == 0)
        {
            TTE_LOG("ERROR: For module %s, visual property %s class '%s' not found", modid.c_str(), adapter.Name.c_str(), ty.c_str());
        }
        else
        {
            for(const PropertyRenderInstruction* i = PropertyRenderInstructions; i->Name; i++)
            {
                if(inputType == i->Name)
                {
                    adapter.RenderInstruction = i;
                    break;
                }
            }
            if(!adapter.RenderInstruction)
            {
                if(StringStartsWith(inputType, "handle:"))
                {
                    inputType = inputType.substr(7);
                    adapter.HandleClassID = Meta::FindClassByExtension(inputType, 0);
                    if(!adapter.HandleClassID)
                    {
                        TTE_LOG("ERROR: Unknown handle class extension %s for %s", inputType.c_str(), modid.c_str());
                        return 0;
                    }
                }
                else
                {
                    TTE_LOG("ERROR: Invalid render instruction input type: %s for %s", inputType.c_str(), modid.c_str());
                    return 0;
                }
            }
            if(subpath != "this")
            {
                std::vector<String> result{};
                std::stringstream ss(subpath);
                std::string token{};
                while (std::getline(ss, token, '.')) 
                {
                    result.push_back(token);
                }
                if (!result.empty() && result[0] == "this")
                {
                    result.erase(result.begin());
                }
                adapter.SubPaths = std::move(result);
            }
            elem.VisualProperties.push_back(std::move(adapter));
        }

    }

    elem.ImagePath = man.ToString(2);
    _MyContext->_ModuleVisualProperties[std::move(modid)] = std::move(elem);

    return 0;
}

void luaModuleUI(LuaFunctionCollection& Col)
{
    PUSH_FUNC(Col, "RegisterModuleUI", &luaRegisterModuleUI, "nil RegisterModuleUI(moduleID, moduleImage, dataTable)", "Registers data which describes how the inspector view should render this module."
        " Data table is a table of UI element to a table of that elements info. Class, PropKey and Default are required keys. In each entry, 'UI' is a required table which describes how to render it."
        " It should contain InputType which is a string of the valid kPropRenderXXX types. Or 'handle:xxx' where xxx is the *extension* of the file name to have a handle set as. Note in this case that"
        " the handle property must be a string or symbol. Along with InputType, SubPath is a string. By default set it to 'this', else a chain of variables to get to the input type variable, eg this.mHandle"
        " if the type is a handle and a string is required if mHandle is a string in the Handle class specifying the file name. There can be as many this.sub.paths.as.needed .");

    const PropertyRenderInstruction* Instruction = PropertyRenderInstructions;
    while(Instruction->Name)
    {
        PUSH_GLOBAL_S(Col, Instruction->ConstantName, Instruction->Name, "Module property render instructions");
        Instruction++;
    }
}

static void _DoHandleRender(EditorUI& editor, String* handleFile, Symbol* sym, U32 clazz)
{
    const Meta::Class& c = Meta::GetClass(clazz);
    if(sym && sym->GetCRC64() && handleFile->empty())
    {
        *handleFile = editor.GetApplication().GetRegistry()->FindResourceName(*sym);
        if(handleFile->empty())
        {
            *handleFile = "<!!>"; // not found placeholder
        }
    }
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(60, 60, 60, 255));
    const char* textShow = *handleFile == "<!!>" ? "<Unknown>" : *handleFile == "" ? editor.GetApplication().GetLanguageText("misc.empty").c_str() : handleFile->c_str();
    ImGui::InputText("##inpHandle", (char*)textShow, strlen(textShow)+1, ImGuiInputTextFlags_ReadOnly);
    if(ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("TT_ASSET");
        if(pl)
        {
            String nHandle = (CString)pl->Data;
            if(StringEndsWith(nHandle, c.Extension))
            {
                handleFile->assign(nHandle);
                if(sym)
                    *sym = Symbol(nHandle);
                // on change
            }
            else
            {
                PlatformMessageBoxAndWait("Invalid file type", "Only " + c.Extension + " files can be used in this file handle!");
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::SetTooltip(editor.GetApplication().GetLanguageText("misc.drag_here").c_str(), c.Extension.c_str());
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor();
    ImGui::PushFont(ImGui::GetFont(), 11.0f);
    ImGui::SameLine();
    if(ImGui::Button(editor.GetApplication().GetLanguageText("misc.reset_lower").c_str()) && !handleFile->empty() && *handleFile != "<!!>")
    {
        *handleFile = "";
        if(sym)
            *sym = Symbol();
        // on change
    }
    ImGui::PopFont();
}

static Bool BeginModule(EditorUI& editor, const char* title, Float bodyHeight, Bool& isOpen, Float sY, const char* icon, Bool* pAddModule = nullptr, Bool* outDeleteClicked = nullptr)
{
    ImVec2 wpos = ImGui::GetWindowPos() + ImVec2{ 0.0f, editor.InspectorViewY - sY } + ImVec2{ 1.0f, 20.0f };
    ImVec2 wsize = ImGui::GetWindowSize() + ImVec2{ -15.0f, -20.0f };
    ImDrawList* list = ImGui::GetWindowDrawList();

    const Float headerHeight = 20.0f;

    // Header background
    list->AddRectFilled(
        wpos,
        wpos + ImVec2(wsize.x, headerHeight),
        IM_COL32(70, 70, 70, 255)
    );

    // Header bottom border
    list->AddLine(
        wpos + ImVec2(0.0f, headerHeight),
        wpos + ImVec2(wsize.x, headerHeight),
        IM_COL32(30, 30, 30, 255)
    );

    // Arrow icon
    if (ImGui::IsMouseHoveringRect(wpos, wpos + ImVec2{ 18.0f, 18.0f }, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        isOpen = !isOpen;

    if (isOpen)
        list->AddTriangleFilled(
        wpos + ImVec2(6.0f, 6.0f),
        wpos + ImVec2(14.0f, 6.0f),
        wpos + ImVec2(10.0f, 14.0f),
        IM_COL32(120, 120, 120, 255)
        );
    else
        list->AddTriangleFilled(
        wpos + ImVec2(6.0f, 6.0f),
        wpos + ImVec2(6.0f, 14.0f),
        wpos + ImVec2(14.0f, 10.0f),
        IM_COL32(120, 120, 120, 255)
        );

    if (icon)
    {
        editor.DrawResourceTexturePixels(icon, wpos.x + 15.0f - ImGui::GetWindowPos().x, wpos.y + 1.0f - ImGui::GetWindowPos().y, 18.0f, 18.0f);
    }

    // Title text
    list->AddText(
        wpos + ImVec2(icon ? 35.0f : 25.0f, 4.0f),
        IM_COL32(190, 190, 190, 255),
        title
    );

    ImVec2 pos{ wpos.x + wsize.x - 20.0f - ImGui::GetWindowPos().x, wpos.y + 1.0f - ImGui::GetWindowPos().y };
    if (pAddModule)
    {
        editor.DrawResourceTexturePixels("Misc/AddModule.png", pos.x, pos.y, 18.0f, 18.0f);
        if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + pos, ImGui::GetWindowPos() + pos + ImVec2{ 18.0f, 18.0f }, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            *pAddModule = true;
        }
        else
        {
            *pAddModule = false;
        }
    }
    if(outDeleteClicked)
    {
        if(pAddModule)
        {
            pos.x -= 20.0f;
        }
        editor.DrawResourceTexturePixels("Scene/AgentRemove.png", pos.x, pos.y, 18.0f, 18.0f);
        if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos() + pos, ImGui::GetWindowPos() + pos + ImVec2{ 18.0f, 18.0f }, false) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            *outDeleteClicked = true;
        }
        else
        {
            *outDeleteClicked = false;
        }
    }

    if (isOpen)
    {
        // Body BG
        list->AddRectFilled(
            wpos + ImVec2(0.0f, headerHeight),
            wpos + ImVec2(wsize.x, bodyHeight + headerHeight),
            IM_COL32(60, 60, 60, 255)
        );
        list->AddLine(
            wpos + ImVec2(0.0f, headerHeight),
            wpos + ImVec2(wsize.x, headerHeight),
            IM_COL32(45, 45, 45, 255)
        );
        // Full border
        list->AddRect(
            wpos,
            wpos + ImVec2(wsize.x, bodyHeight + headerHeight),
            IM_COL32(40, 40, 40, 255)
        );
    }
    else
    {
        // Full border
        list->AddRect(
            wpos,
            wpos + ImVec2(wsize.x, headerHeight),
            IM_COL32(40, 40, 40, 255)
        );
    }

    editor.InspectorViewY += headerHeight + (isOpen ? bodyHeight : 0.0f);
    ImGui::SetCursorScreenPos(wpos + ImVec2{ 0.0f, headerHeight + 8.0f }); // set pointer to little offset for drawing.

    return isOpen;
}

Bool InspectorView::RenderNode(Float scrollY)
{
    Bool delAgent = false;
    static Bool IsOpen = true;
    Ptr<Node> pNode = _EditorUI.InspectingNode.lock();
    Bool clicked = false;
    Bool* pclicked = IsWeakPtrUnbound(pNode->Parent) ? &clicked : nullptr;
    if (pNode->AgentName == pNode->AttachedScene->GetName())
        pclicked = nullptr; // cannot add to scene agent
    if (pNode && BeginModule(_EditorUI, "Node", 100.0f, IsOpen, scrollY, "Module/TransformGizmo.png", pclicked, &delAgent) && !clicked)
    {
        Transform orig = pNode->LocalTransform;
        Float* pos = (Float*)&pNode->LocalTransform._Trans;
        ImGui::SetCursorPosX(ImGui::GetCursorStartPos().x + 0.0f);
        Bool bChanged = PropertyRenderFunctions::_DrawVectorInput("Position", (Float*)&pNode->LocalTransform._Trans, 3, false);
        Vector3 Euler{};
        pNode->LocalTransform._Rot.GetEuler(Euler);
        Euler *= Vector3(180.0f / 3.14159265f);
        if (PropertyRenderFunctions::_DrawVectorInput("Rotation", (Float*)&Euler, 3, false))
            bChanged = true;
        if (bChanged)
        {
            Transform newTransform = pNode->LocalTransform;
            Euler /= Vector3(180.0f / 3.14159265f);
            newTransform._Rot.SetEuler(Euler.x, Euler.y, Euler.z);
            pNode->LocalTransform = orig;
            Scene::UpdateNodeLocalTransform(pNode, newTransform, true);
        }
    }
    if (clicked)
    {
        // add new agent module (ie entity component)
        GetApplication().SetCurrentPopup(TTE_NEW_PTR(AddModulePopup, MEMORY_TAG_EDITOR_UI, pNode->AttachedScene->GetAgentModules(pNode->AgentName), pNode, &_EditorUI), _EditorUI);
    }
    return delAgent;
}

InspectorView::~InspectorView()
{
    _CurrentNode = {};
    _ResetModulesCache();
}

void InspectorView::_ResetModulesCache()
{
    _LastPropFlush = 0;
    _FlushProps(); // force flush if req
    _CurrentIsAgent = false;
    _RuntimeModuleProps.clear();
    _CurrentNode = {};
}

void InspectorView::_FlushProps()
{
    if(_CurrentNode && _CurrentIsAgent && GetTimeStampDifference(_LastPropFlush, GetTimeStamp()) > 100.0f * 1e-3f) // 100ms flush (10hz)
    {
        PropertySet::CallAllCallbacks(_CurrentNode->AttachedScene->GetAgentProps(_CurrentNode->AgentName), _CurrentNode->AttachedScene->GetRegistry());
    }
}

void InspectorView::Render()
{
    Float yOff = MAX(0.04f, 40.f / GetMainViewport()->Size.y);
    SetNextWindowViewport(0.80f, yOff, 0, 0, 0.20f, 0.70f - yOff, 100, 300, GetUICondition());
    Ptr<Node> pNode = _EditorUI.InspectingNode.lock();
    String title = {};
    if(pNode)
    {
        title = GetApplication().GetLanguageText("iv.title_prefix") + " " + pNode->Name + "##inspector";
    }
    else
    {
        title = GetApplication().GetLanguageText("iv.title_default") + "##inspector";
    }
    if (ImGui::Begin(title.c_str(), NULL,ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysHorizontalScrollbar))
    {
        Float scrollY = ImGui::GetScrollY();
        if (_EditorUI.InspectingNode.expired())
        {
            PushFont(GetApplication().GetEditorFont(), 10.0f);
            TextUnformatted(GetApplication().GetLanguageText("iv.select").c_str());
            PopFont();
            _EditorUI.InspectingNode.reset();
            _EditorUI.IsInspectingAgent = false;
        }
        else
        {
            if(pNode != _CurrentNode)
            {
                _ResetModulesCache();
                _CurrentNode = pNode;
                _CurrentIsAgent = _EditorUI.IsInspectingAgent;
                if(_EditorUI.IsInspectingAgent)
                {
                    _LastPropFlush = GetTimeStamp();
                }
            }
            if(_EditorUI.IsInspectingAgent)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                String agent = _EditorUI.InspectingNode.lock()->AgentName;
                _EditorUI.InspectorViewY = 0.0f;
                if(RenderNode(scrollY) && _EditorUI.IsInspectingAgent)
                {
                    if (pNode->AgentName == _EditorUI.GetActiveScene().GetName())
                    {
                        PlatformMessageBoxAndWait("Cannot delete this agent!", "The scene agent cannot be deleted!");
                    }
                    else
                    {
                        _CurrentNode = {};
                        _CurrentIsAgent = false;
                        _EditorUI.InspectingNode = {};
                        _EditorUI.IsInspectingAgent = false;
                        _EditorUI.GetActiveScene().RemoveAgent(pNode->AgentName);
                    }
                }
                else
                {
                    Meta::ClassInstance& props = _EditorUI.GetActiveScene()._Agents[agent]->Props;
                    U32 r = 0;
                    for (U32 i = 0; i < (U32)SceneModuleType::NUM; i++)
                    {
                        if (_EditorUI.GetActiveScene()._Agents[agent]->ModuleIndices[i] != -1)
                        {
                            _InitModuleCache(props, (SceneModuleType)i);
                            auto it = _RuntimeModuleProps.find((SceneModuleType)i);
                            Float yCur = ImGui::GetCursorPosY();
                            if (it != _RuntimeModuleProps.end())
                            {
                                if (BeginModule(_EditorUI, it->second.ModuleName.c_str(),
                                    it->second.LastHeight, it->second.IsOpen, scrollY, it->second.ModuleIcon.c_str()))
                                {
                                    for (const auto& prop : it->second.Properties)
                                    {
                                        if(!prop.Property)
                                            continue;
                                        ImGui::PushID(r++);
                                        ImGui::SetCursorPosX(ImGui::GetCursorStartPos().x + 0.0f);
                                        ImGui::Dummy(ImVec2{ 1.0f, 2.0f });
                                        ImGui::PushFont(ImGui::GetFont(), 13.0f);
                                        ImGui::TextUnformatted(prop.Specification->Name.c_str());
                                        String lang = StringToSnake("mod." + ToLower(it->second.ModuleName) + ".iv." + prop.Specification->Name);
                                        if (ImGui::IsItemHovered() && GetApplication().HasLanguageText(lang.c_str()))
                                        {
                                            ImGui::PushTextWrapPos(150.0f);
                                            ImGui::SetTooltip("%s", GetApplication().GetLanguageText(lang.c_str()).c_str());
                                            ImGui::PopTextWrapPos();
                                        }
                                        ImGui::PopFont();
                                        ImGui::Dummy(ImVec2{ 1.0f, 2.0f });
                                        if (prop.Specification->RenderInstruction)
                                        {
                                            prop.Specification->RenderInstruction->Render(*prop.Specification, prop.Property);
                                        }
                                        else if (prop.Specification->HandleClassID)
                                        {
                                            if (Meta::IsSymbolClass(Meta::GetClass(prop.Property.GetClassID())))
                                            {
                                                _DoHandleRender(_EditorUI, &prop.Specification->RuntimeCache.SymbolCache, (Symbol*)prop.Property._GetInternal(), prop.Specification->HandleClassID);
                                            }
                                            else if (Meta::IsStringClass(Meta::GetClass(prop.Property.GetClassID())))
                                            {
                                                _DoHandleRender(_EditorUI, (String*)prop.Property._GetInternal(), 0, prop.Specification->HandleClassID);
                                            }
                                            else
                                            {
                                                ImGui::Text("FIXME!Handle:%s", Meta::GetClass(prop.Property.GetClassID()).Name.c_str()); // incorrect not a handle. render error here
                                            }
                                        }
                                        ImGui::PopID();
                                    }
                                }
                                it->second.LastHeight = MAX(40.0f, ImGui::GetCursorPosY() - yCur);
                            }
                        }
                    }
                    Dummy(ImVec2(0.0f, _EditorUI.InspectorViewY - scrollY));
                }
                ImGui::PopStyleVar(3);
            }
            else
            {
                TextUnformatted("im a node do me pls");
            }
            _FlushProps();
        }
    }
    End();
}

// ===================================================== SCENE VIEW

struct SceneViewData
{
    Vector3 CameraPosition = { 0.0f, 0.0f, 10.0f };
    Vector3 CameraRot = Vector3::Zero;
    Vector2 LastMouse = Vector2::Zero;
    Vector2 MouseAmount = Vector2::Zero;
    String HoveringAgent = "";
};

SceneView::SceneView(EditorUI& ui) : EditorUIComponent(ui), _SceneRenderer(ui.GetApplication().GetRenderContext())
{
    _SceneData = nullptr;
    _SceneRenderer.SetEditMode(true);
    _SceneData = TTE_NEW(SceneViewData, MEMORY_TAG_EDITOR_UI); // default scene data allowed for empty scene
    _EditorSceneCache = TTE_PROXY_PTR(&_EditorUI.GetActiveScene(), Scene);
}

RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format);

void SceneView::_UpdateViewNoActiveScene(Ptr<Scene> pEditorScene, SceneViewData& viewData, Bool bWindowFocused) // scene isnt active but use here for camera
{
    // 1. Update camera
    if(pEditorScene->GetViewStack().empty())
    {
        // Ensure default camera
        if(!_SceneViewCamDefault)
        {
            _SceneViewCamDefault = TTE_NEW_PTR(Camera, MEMORY_TAG_EDITOR_UI);
        }
        Camera& editorCam = *_SceneViewCamDefault;
        editorCam.SetHFOV(50.0f);
        editorCam.SetNearClip(0.2f);
        editorCam.SetFarClip(1000.0f);
        pEditorScene->GetViewStack().push_back(_SceneViewCamDefault);
    }
    ::RenderFrame& frame = _SceneRenderer.GetRenderer()->GetFrame(true);
    if (bWindowFocused)
    {
        Quaternion viewRot;
        viewRot.SetEuler(viewData.CameraRot.x, viewData.CameraRot.y, 0.0f); // pitch, yaw, roll
        Matrix4 rotMatrix = MatrixRotation(viewRot);

        Vector3 forward = rotMatrix.GetForward();
        if(_EditorUI.GetApplication().GetRenderContext()->IsLeftHanded())
        {
            forward *= -1.0f;
        }
        Vector3 right = rotMatrix.GetRight();
        Vector3 up = rotMatrix.GetUp();

        const Float moveSpeed = 0.25f;
        const Float mouseSensitivity = 0.4f;

        for (auto& event : frame._Events)
        {
            if (event.Code == InputCode::MOUSE_MOVE)
            {
                // Only rotate if mouse button is down
                if (_ViewInputMgr.IsKeyDown(InputCode::MOUSE_LEFT_BUTTON) && !event.BeenHandled())
                {
                    Float xCursorDiff = event.X - viewData.LastMouse.x;
                    Float yCursorDiff = event.Y - viewData.LastMouse.y;

                    viewData.MouseAmount.x -= xCursorDiff * mouseSensitivity; // Yaw
                    viewData.MouseAmount.y -= yCursorDiff * mouseSensitivity; // Pitch
                    viewData.MouseAmount.y = std::clamp(viewData.MouseAmount.y, -PI_F * 0.5f + 0.01f, PI_F * 0.5f - 0.01f);

                    viewData.CameraRot = Vector3(-viewData.MouseAmount.y, -viewData.MouseAmount.x, 0.0f);
                    event.SetHandled();
                }

                viewData.LastMouse = Vector2(event.X, event.Y);
            }

            // Movement keys (WASD + arrows)
            if (event.Type == InputMapper::EventType::BEGIN)
            {
                switch (event.Code)
                {
                    case InputCode::W:
                        viewData.CameraPosition += forward * moveSpeed;
                        event.SetHandled();
                        break;
                    case InputCode::S:
                        viewData.CameraPosition -= forward * moveSpeed;
                        event.SetHandled();
                        break;
                    case InputCode::A:
                        viewData.CameraPosition -= right * moveSpeed;
                        event.SetHandled();
                        break;
                    case InputCode::D:
                        viewData.CameraPosition += right * moveSpeed;
                        event.SetHandled();
                        break;
                    case InputCode::UP_ARROW:
                        viewData.CameraPosition += up * moveSpeed;
                        event.SetHandled();
                        break;
                    case InputCode::DOWN_ARROW:
                        viewData.CameraPosition -= up * moveSpeed;
                        event.SetHandled();
                        break;
                    default:
                        break;
                }
            }
        }

        Quaternion updatedRotation;
        updatedRotation.SetEuler(viewData.CameraRot.x, viewData.CameraRot.y, 0.0f);
        auto cam = pEditorScene->GetViewStack().front().lock();
        cam->SetWorldRotation(updatedRotation);
        cam->SetWorldPosition(viewData.CameraPosition);
    }
    else
    {
        // If not focused, still track mouse position to avoid jumps
        for (auto& event : frame._Events)
        {
            if (event.Code == InputCode::MOUSE_MOVE)
            {
                viewData.LastMouse = Vector2(event.X, event.Y);
            }
        }
    }
}

void SceneView::_UpdateView(Ptr<Scene> pEditorScene, SceneViewData& viewData, Bool bWindowFocused)
{
    _UpdateViewNoActiveScene(pEditorScene, viewData, bWindowFocused); // update base without scene
    // with active scene specific render stuff
}

void SceneView::_FreeSceneData()
{
    if (_SceneData)
    {
        TTE_DEL(_SceneData);
        _SceneData = nullptr;
    }
}

void SceneView::_OnSceneLoad(Ptr<Scene> pEditorScene)
{
    _FreeSceneData();
    _SceneData = TTE_NEW(SceneViewData, MEMORY_TAG_EDITOR_UI);
}

void SceneView::_OnSceneUnload(Ptr<Scene> pEditorScene)
{
    _FreeSceneData();
    _SceneRenderer.ResetScene(TTE_PROXY_PTR(&_EditorUI.GetActiveScene(), Scene));
    _SceneData = TTE_NEW(SceneViewData, MEMORY_TAG_EDITOR_UI); // default
}

SceneView::~SceneView()
{
    _FreeSceneData();
}

void SceneView::PostRender(void* me, const SceneFrameRenderParams& params, RenderSceneView* mv)
{
    SceneView* self = (SceneView*)me;
    // Camera& drawCam = params.RenderScene->GetViewStack().front(); parameter is scene view wide already
    RenderViewPassParams outlineParams{};
    RenderViewport vp = params.Viewport.GetAsViewport();
    vp.x *= (Float)params.TargetWidth;  vp.w *= (Float)params.TargetWidth;
    vp.y *= (Float)params.TargetHeight;  vp.h *= (Float)params.TargetHeight;

    RenderViewPass* outlinePass = mv->PushPass(outlineParams);
    outlinePass->SetName("Post-EditorOutlines");
    outlinePass->SetViewport(vp);
    outlinePass->SetRenderTarget(0, params.Target, 0, 0);
    outlinePass->SetDepthTarget(RenderTargetID(RenderTargetConstantID::DEPTH), 0, 0);

    RenderStateBlob defRenderState{};
    defRenderState.SetValue(RenderStateType::Z_ENABLE, true);
    defRenderState.SetValue(RenderStateType::Z_WRITE_ENABLE, false);
    defRenderState.SetValue(RenderStateType::Z_COMPARE_FUNC, SDL_GPU_COMPAREOP_LESS_OR_EQUAL);

    if(self->_SceneData)
    {
        for (const auto& renderable : self->_EditorSceneCache->GetModuleView<SceneModuleType::RENDERABLE>())
        {
            if (renderable.AgentNode == self->_EditorUI.InspectingNode.lock())
            {
                // render bounding sphere(s)
                Matrix4 agentMat = MatrixTransformation(Scene::GetNodeWorldTransform(renderable.AgentNode));
                for(const auto& mesh: renderable.Renderable.MeshList)
                {
                    if(self->_EditorSceneCache->HasAgentModule(renderable.AgentNode->AgentName, SceneModuleType::LIGHT))
                    {
                        Matrix4 meshMat = MatrixTransformation(Vector3(mesh->BSphere._Radius), Quaternion(), mesh->BSphere._Center);
                        RenderUtility::DrawWireSphere(*self->_EditorUI.GetApplication().GetRenderContext(), nullptr, 
                            agentMat * meshMat, Colour(1.0f, 1.0f, 0.0f, 0.5f), outlinePass, &defRenderState);
                    }
                    else
                    {
                        Vector3 center = (mesh->BBox._Max + mesh->BBox._Min) * 0.5f;
                        Vector3 halfExtents = (mesh->BBox._Max - mesh->BBox._Min) * 0.5f;
                        Matrix4 meshMat = MatrixTransformation(halfExtents,Quaternion(),center);
                        RenderUtility::DrawWireBox(*self->_EditorUI.GetApplication().GetRenderContext(), nullptr, 
                            agentMat * meshMat, Colour(1.0f, 0.5f, 0.0f, 0.5f), outlinePass, &defRenderState);
                    }
                }
                break;
            }
        }
    }
}

void SceneView::Render()
{
    Float yOff = MAX(0.04f, 40.f / GetMainViewport()->WorkSize.y);
    SetNextWindowViewport(0.20f, yOff, 0, 0, 0.60f, 0.70f - yOff, 300, 200, GetUICondition());
    ImGui::Begin(GetApplication().GetLanguageText("sv.title").c_str(), NULL, 
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground, _SceneData ? _SceneData->HoveringAgent.c_str() : 0);

    {
        ImVec2 winSize = GetCurrentWindowRead()->Size;
        winSize.y -= GetCurrentWindowRead()->TitleBarHeight;
        U32 winX = (U32)winSize.x, winY = (U32)winSize.y;
        if(winX > 500 && winY > 400)
        {
            U32 targetX = 0, targetY = 0, _ = 0;
            RenderNDCScissorRect scissor{};
            ImVec2 mnView = GetMainViewport()->Size;
            Float winTopLeftX = 0.2f + 2.0f / mnView.x;
            Float winTopLeftY = yOff + GetCurrentWindowRead()->TitleBarHeight / mnView.y;
            Float winBottomRightX = 0.2f + winSize.x / mnView.x;
            Float winBottomRightY = yOff + GetCurrentWindowRead()->TitleBarHeight / mnView.y + winSize.y / mnView.y;
            scissor.SetFractional(winTopLeftX, winTopLeftY, winBottomRightX, winBottomRightY);

            // Input
            const std::vector<RuntimeInputEvent>& events = _SceneRenderer.GetRenderer()->GetFrame(true)._Events;
            _ViewInputMgr.ProcessEvents(events);
            for(const RuntimeInputEvent& event: events)
            {
                if(event.Code == InputCode::MOUSE_MOVE && !(event.X >= winTopLeftX && event.X <= winBottomRightX 
                   && event.Y >= winTopLeftY && event.Y <= winBottomRightY))
                {
                    event.SetHandled(); // Ignore mouse move outside region
                }
            }

            // update view
            Ptr<Scene>& pEditorScene = _EditorSceneCache;
            if(_EditorUI.GetActiveScene().GetName().empty())
            {
                _UpdateViewNoActiveScene(pEditorScene, *(SceneViewData*)_SceneData, IsWindowFocused());
            }
            else
            {
                _UpdateView(pEditorScene, *(SceneViewData*)_SceneData, IsWindowFocused());
            }
            ImVec2 tSize = ImGui::GetMainViewport()->Size;

            // update selected agent
            RenderViewport vp = scissor.GetAsViewport();
            U32 viewportWidth = (U32)(vp.w * (Float)tSize.x);
            U32 viewportHeight = (U32)(vp.h * (Float)tSize.y);
            U32 viewportX = (U32)(vp.x * (Float)tSize.x);
            U32 viewportY = (U32)(vp.y * (Float)tSize.y);
            U32 cursorX = (U32)(ImGui::GetMousePos().x - ImGui::GetMainViewport()->Pos.x);
            U32 cursorY = (U32)(ImGui::GetMousePos().y - ImGui::GetMainViewport()->Pos.y);
            if(_SceneData && cursorX > viewportX && cursorY > viewportY && cursorX < viewportX + viewportWidth && cursorY < viewportY + viewportHeight && !_EditorSceneCache->GetViewStack().empty())
            {
                // mouse is in scene viewport. screen pos x and y are the coords in that viewport 0 0 being TL
                U32 screenPosX = cursorX - viewportX, screenPosY = cursorY - viewportY;
                _SceneData->HoveringAgent = _EditorSceneCache->GetAgentAtScreenPosition(*_EditorSceneCache->GetViewStack().back().lock(), screenPosX, screenPosY, false);
                if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if(_SceneData->HoveringAgent.empty())
                    {
                        _EditorUI.InspectingNode = {};
                        _EditorUI.IsInspectingAgent = false;
                    }
                    else
                    {
                        for(const auto& agent: _EditorSceneCache->GetAgents())
                        {
                            if(agent.second->Name == _SceneData->HoveringAgent)
                            {
                                _EditorUI.IsInspectingAgent = true;
                                _EditorUI.InspectingNode = agent.second->AgentNode;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                _SceneData->HoveringAgent = ""; // selected is still ok!
            }

            // render scene
            SceneFrameRenderParams params{};
            params.RenderScene = pEditorScene;
            params.Target = RenderTargetID(RenderTargetConstantID::BACKBUFFER);
            params.Viewport = scissor;
            params.TargetWidth = (U32)tSize.x;
            params.TargetHeight = (U32)tSize.y;
            params.UserData = this;
            params.RenderPost = &PostRender;
            _SceneRenderer.RenderScene(params);

        }
    }
    End();
}
