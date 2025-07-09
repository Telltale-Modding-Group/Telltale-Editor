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
    _Views.push_back(TTE_NEW_PTR(InspectorView, MEMORY_TAG_EDITOR_UI, *this));

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
    _IconMap["*.dlg"] = "FileView/Dialog_DLG_DLOG.png";
    _IconMap["*.dlog"] = "FileView/Dialog_DLG_DLOG.png";
    _IconMap["*.anm"] = "FileView/Animation_ANM.png";
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
        for(const auto& mp: _Group[_CurGroup].Entries)
        {
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
                    for (const auto& icomp : _IconMap)
                    {
                        if (icomp.first == mp)
                        {
                            ico = icomp.second;
                            break;
                        }
                    }
                    if(ico.empty())
                        ico = "FileView/UnknownFile.png";
                }
                DrawResourceTexturePixels(ico, x, localY, Icon, Icon);
            }
            else
            {
                DrawResourceTexturePixels("FileView/Folder.png", x, localY, Icon, Icon);
            }
            ImVec2 center = winPos + ImVec2{ x + Icon * 0.5f, localY + Icon };
            DrawCenteredWrappedText(mp.length() > 25 ? "..." + mp.substr(mp.length() - 25, 25) : mp, Icon * 0.95f, center.x, center.y, 2, 10.f);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(winPos + ImVec2{ x, localY }, winPos + ImVec2{ x + Icon, localY + Icon }))
            {
                _Update(pRegistry, index);
                break; // continue next
            }
            x += Spacing;
            index++;
        }
        SetCursorScreenPos(winPos);
        Dummy(ImVec2(150.0f, y - scrollY + 50.f));
        PopFont();
    }
    ImGui::End();
}

// ===================================================== OUTLINE VIEW

OutlineView::OutlineView(EditorUI& ui) : EditorUIComponent(ui) {}

void OutlineView::_RenderSceneNode(WeakPtr<Node> pNodeWk)
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
        while(pChild)
        {
            _RenderSceneNode(pChild);
            pChild = pChild->NextSibling.lock();
        }
        TreePop();
    }
}

void OutlineView::Render()
{
    Float yOff = MAX(0.04f, 40.f / GetMainViewport()->Size.y);
    SetNextWindowViewport(0.0f, yOff, 0, 0, 0.20f, 1.00f - yOff, 100, 300, GetUICondition());
    if (ImGui::Begin(GetApplication().GetLanguageText("ov.title").c_str(), NULL, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        SetCursorPosY(30.0f); // option bar
        for(const auto& agent: _EditorUI.GetActiveScene().GetAgents())
        {
            _RenderSceneNode(agent.second->AgentNode);
        }
        Dummy(GetCursorPos());
    }
    End();
}

// ===================================================== OUTLINE VIEW

InspectorView::InspectorView(EditorUI& ui) : EditorUIComponent(ui)
{
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
    if (ImGui::Begin(title.c_str(), NULL,ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
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
            if(_EditorUI.IsInspectingAgent)
            {
                String agent = _EditorUI.InspectingNode.lock()->AgentName;
                SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, // ModuleUI.cpp
                                 SceneModuleUtil::_RenderUIModules{_EditorUI.GetActiveScene(), *_EditorUI.GetActiveScene()._Agents[agent], _EditorUI});
            }
            else
            {
                TextUnformatted("im a node do me pls");
            }
        }
        Dummy(GetCursorPos());
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
};

SceneView::SceneView(EditorUI& ui) : EditorUIComponent(ui), _SceneRenderer(ui.GetApplication().GetRenderContext())
{
    _SceneData = nullptr;
    _SceneRenderer.SetEditMode(true);
    _ViewTarget = _SceneRenderer.GetRenderer()->AllocateRuntimeTexture();
    _ViewTargetID = _SceneRenderer.GetRenderer()->CreateDynamicTarget(_ViewTarget);
    _SceneData = TTE_NEW(SceneViewData, MEMORY_TAG_EDITOR_UI); // default scene data allowed for empty scene
}

RenderSurfaceFormat FromSDLFormat(SDL_GPUTextureFormat format);

void SceneView::_UpdateViewNoActiveScene(Ptr<Scene> pEditorScene, SceneViewData& viewData, Bool bWindowFocused) // scene isnt active but use here for camera
{
    // 1. Update camera
    if(pEditorScene->GetViewStack().empty())
    {
        // Ensure default camera
        Camera editorCam{};
        editorCam.SetHFOV(50.0f);
        editorCam.SetNearClip(0.2f);
        editorCam.SetFarClip(1000.0f);
        pEditorScene->GetViewStack().push_back(editorCam);
    }
    ::RenderFrame& frame = _SceneRenderer.GetRenderer()->GetFrame(true);
    if(bWindowFocused)
    {
        for (auto& event : frame._Events)
        {
            Bool bRotate = false;
            if (event.Code == InputCode::MOUSE_MOVE)
            {
                if (_ViewInputMgr.IsKeyDown(InputCode::MOUSE_LEFT_BUTTON) && !event.BeenHandled())
                {
                    Float xCursorDiff = event.X - viewData.LastMouse.x;
                    Float yCursorDiff = event.Y - viewData.LastMouse.y;

                    // Apply sensitivity
                    viewData.MouseAmount.x -= (PI_F * xCursorDiff * 0.6f); // Yaw
                    viewData.MouseAmount.y -= (PI_F * yCursorDiff * 0.6f); // Pitch

                    // Clamp pitch
                    viewData.MouseAmount.y = fmaxf(-PI_F * 0.5f, fminf(viewData.MouseAmount.y, PI_F * 0.5f));

                    viewData.CameraRot = Vector3(viewData.MouseAmount.y, viewData.MouseAmount.x, 0.0f);
                }
                viewData.LastMouse = Vector2(event.X, event.Y);
                bRotate = true;
            }

            Vector3 forward = Vector3(
                                      cosf(viewData.CameraRot.x) * sinf(viewData.CameraRot.y),
                                      0.0f,
                                      cosf(viewData.CameraRot.x) * cosf(viewData.CameraRot.y)
            );
            forward.Normalize();

            Vector3 right = Vector3(
                                    sinf(viewData.CameraRot.y - PI_F * 0.5f),
                                    0.0f,
                                    cosf(viewData.CameraRot.y - PI_F * 0.5f)
            );
            right.Normalize();

            Vector3 up = -Vector3::Cross(forward, right);
            up.Normalize();

            const Float speed = 0.25f;

            if (event.Type == InputMapper::EventType::BEGIN)
            {
                switch (event.Code)
                {
                    case InputCode::S:
                        event.SetHandled();
                        viewData.CameraPosition += forward * speed;
                        break;
                    case InputCode::W:
                        event.SetHandled();
                        viewData.CameraPosition -= forward * speed;
                        break;
                    case InputCode::D:
                        event.SetHandled();
                        viewData.CameraPosition -= right * speed;
                        break;
                    case InputCode::A:
                        event.SetHandled();
                        viewData.CameraPosition += right * speed;
                        break;
                    case InputCode::DOWN_ARROW:
                        event.SetHandled();
                        viewData.CameraPosition += up * speed;
                        break;
                    case InputCode::UP_ARROW:
                        event.SetHandled();
                        viewData.CameraPosition -= up * speed;
                        break;
                    default:
                        break;
                }
                pEditorScene->GetViewStack().front().SetWorldPosition(viewData.CameraPosition);
            }

            if(bRotate)
            {
                Quaternion rot{};
                rot.SetEuler(viewData.CameraRot.x, viewData.CameraRot.y, viewData.CameraRot.z);
                pEditorScene->GetViewStack().front().SetWorldRotation(rot);
            }
        }
    }
    else
    {
        for (auto& event : frame._Events)
        {
            if(event.Code == InputCode::MOUSE_MOVE)
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

void SceneView::Render()
{
    Float yOff = MAX(0.04f, 40.f / GetMainViewport()->WorkSize.y);
    SetNextWindowViewport(0.20f, yOff, 0, 0, 0.60f, 0.70f - yOff, 300, 200, GetUICondition());
    ImGui::Begin(GetApplication().GetLanguageText("sv.title").c_str(), NULL, 
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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

            // Target sizing
            if (_ViewTarget->_Handle == nullptr ||
               (_ViewTarget->GetDimensions(targetX, targetY, _, _), winX != targetX || winY != targetY))
            {
                // Resize target
                _ViewTarget->Release();
                RenderSurfaceFormat scFormat = FromSDLFormat(
                    SDL_GetGPUSwapchainTextureFormat(_SceneRenderer.GetRenderer()->GetDeviceUnsafe(),
                    _SceneRenderer.GetRenderer()->GetWindowUnsafe()));
                _ViewTarget->CreateTarget(*_SceneRenderer.GetRenderer().get(), 0, scFormat,
                                          winX, winY, 1, 1, 1, false);
            }

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

            // Perform view render
            Ptr<Scene> pEditorScene = TTE_PROXY_PTR(&_EditorUI.GetActiveScene(), Scene);
            if(_EditorUI.GetActiveScene().GetName().empty())
            {
                _UpdateViewNoActiveScene(pEditorScene, *(SceneViewData*)_SceneData, IsWindowFocused());
            }
            else
            {
                _UpdateView(pEditorScene, *(SceneViewData*)_SceneData, IsWindowFocused());
            }
            _SceneRenderer.RenderScene(pEditorScene, _ViewTargetID, scissor, winX, winY);
        }
    }
    End();
}