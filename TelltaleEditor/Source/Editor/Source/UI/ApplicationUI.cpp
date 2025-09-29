#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <UI/ApplicationUI.hpp>
#include <UI/EditorUI.hpp>
#include <Renderer/RenderContext.hpp>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

#include "imgui_internal.h"

#include <filesystem>
#include <sstream>

DECL_VEC_ADDITION();

// =========================================== APPLICATION UI CLASS

ApplicationUI::ApplicationUI() : _Device(nullptr), _Window(nullptr), _Editor(nullptr), _ImContext(nullptr),  _ProjectMgr(_WorkspaceProps)
{

}

ApplicationUI::~ApplicationUI()
{

}

void ApplicationUI::Quit()
{
    _Flags.Remove(ApplicationFlag::RUNNING);
}

// =========================================== UI COMPONENT IMPL

void UIComponent::SetNextWindowViewportPixels(Float posX, Float posY, Float SizeX, Float SizeY)
{
    ImVec2 work = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos(work + ImVec2{posX, posY});
    ImGui::SetNextWindowSize({SizeX, SizeY});
}

void UIComponent::SetNextWindowViewport(Float xPosFrac, Float yPosFrac, U32 xMinPix, U32 yMinPix, Float xExtentFrac, Float yExtentFrac, U32 xExtentMinPix, U32 yExtentMinPix, U32 cnd)
{
    ImVec2 windowSize = ImGui::GetMainViewport()->WorkSize;
    ImVec2 work = ImGui::GetMainViewport()->WorkPos;
    ImGui::SetNextWindowPos({ work.x + MAX(xPosFrac * windowSize.x, (Float)xMinPix), work.y + MAX(yPosFrac * windowSize.y, (Float)yMinPix)}, (ImGuiCond)cnd);
    ImGui::SetNextWindowSize({MAX(xExtentFrac * windowSize.x, (Float)xExtentMinPix), MAX(yExtentFrac * windowSize.y, (Float)yExtentMinPix) }, (ImGuiCond)cnd);
}

UIComponent::UIComponent(ApplicationUI& ui) : _MyUI(ui)
{
}

static U8* _LoadSTBI(String path, ApplicationUI& _MyUI, I32& width, I32& height)
{
    DataStreamRef stream = _MyUI.GetEditorContext()->LoadLibraryResource(path);
    if (stream)
    {
        U8* data = TTE_ALLOC(stream->GetSize(), MEMORY_TAG_TEMPORARY);
        stream->Read(data, stream->GetSize());
        int w = 0, h = 0;
        stbi_uc* rgba = stbi_load_from_memory((stbi_uc*)data, (int)stream->GetSize(), &w, &h, 0, STBI_rgb_alpha);
        width = w;
        height = h;
        TTE_FREE(data);
        return (U8*)rgba;
    }
    return nullptr;
}

void UIComponent::DrawCenteredWrappedText(const String& text, Float maxWidth, Float x, Float y, I32 maxLines, Float z)
{
    const Float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    const I32 textLength = static_cast<I32>(text.length());
    I32 cursor = 0;
    I32 linesRendered = 0;

    while (cursor < textLength && linesRendered < maxLines)
    {
        I32 lineLength = 1;

        // Find the longest substring that fits within maxWidth
        while ((cursor + lineLength) <= textLength)
        {
            String substr = text.substr(cursor, lineLength);
            Float width = ImGui::CalcTextSize(substr.c_str()).x;

            if (width > maxWidth)
                break;

            ++lineLength;
        }

        // Back off by one if we overstepped
        if (lineLength > 1)
            --lineLength;

        String line = text.substr(cursor, lineLength);
        ImVec2 lineSize = ImGui::CalcTextSize(line.c_str());

        Float lineX = x - lineSize.x * 0.5f;
        Float lineY = y + linesRendered * lineHeight;

        ImGui::SetCursorScreenPos({ lineX, lineY });
        ImGui::TextUnformatted(line.c_str());

        cursor += lineLength;
        ++linesRendered;
    }
}


String UIComponent::TruncateText(const String& src, Float truncWidth)
{
    const Float text_width = ImGui::CalcTextSize(src.c_str(), nullptr, true).x;

    if (text_width <= truncWidth)
        return src;

    constexpr CString ELLIPSIS = "...";
    const Float ellipsis_width = ImGui::CalcTextSize(ELLIPSIS).x;

    if (ellipsis_width > truncWidth)
        return "";

    I32 visible_start = 0;
    for (I32 i = 0; i < static_cast<I32>(src.size()); ++i)
    {
        String substring = src.substr(i);
        const Float current_width = ImGui::CalcTextSize(substring.c_str(), nullptr, true).x;

        if (current_width + ellipsis_width <= truncWidth)
        {
            visible_start = i;
            break;
        }
    }

    return ELLIPSIS + src.substr(visible_start);
}

void UIComponent::DrawResourceTexturePixels(const String& name, Float xPos, Float yPos, Float xSize, Float ySize, U32 colorScale)
{
    ImVec2 w = ImGui::GetWindowSize();
    DrawResourceTexture(name, xPos / w.x, yPos / w.y, xSize / w.x, ySize / w.y, colorScale);
}

void UIComponent::DrawResourceTexture(const String& name, Float xPosFrac, Float yPosFrac, Float xSizeFrac, Float ySizeFrac, U32 sc)
{
    auto it = _MyUI._ResourceTextures.find(name);
    if(it == _MyUI._ResourceTextures.end())
    {
        I32 w = 0, h = 0;
        U8* rgba = _LoadSTBI("Resources/Textures/" + name, _MyUI, w, h);

        if (rgba)
        {
            SDL_GPUTextureSamplerBinding* pBinding = TTE_NEW(SDL_GPUTextureSamplerBinding, MEMORY_TAG_EDITOR_UI);
            ImGui_ImplSDLGPU3_Data* bd = ImGui_ImplSDLGPU3_GetBackendData();
            pBinding->sampler = bd->FontSampler;

            // CREATE TEXTURE
            SDL_GPUTextureCreateInfo texture_info = {};
            texture_info.type = SDL_GPU_TEXTURETYPE_2D;
            texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
            texture_info.width = w;
            texture_info.height = h;
            texture_info.layer_count_or_depth = 1;
            texture_info.num_levels = 1;
            texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
            SDL_GPUTexture* tex = SDL_CreateGPUTexture(_MyUI._Device, &texture_info);

            // CREATE TRANSFER BUFFER
            SDL_GPUTransferBufferCreateInfo transferbuffer_info = {};
            transferbuffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferbuffer_info.size = w * h * 4;
            SDL_GPUTransferBuffer* transferbuffer = SDL_CreateGPUTransferBuffer(_MyUI._Device, &transferbuffer_info);

            // TRANSFER TEXTURE MEMORY
            void* texture_ptr = SDL_MapGPUTransferBuffer(_MyUI._Device, transferbuffer, false);
            memcpy(texture_ptr, rgba, w * h * 4);
            stbi_image_free(rgba);
            SDL_UnmapGPUTransferBuffer(_MyUI._Device, transferbuffer);

            // PASS ONTO TEXTURE
            SDL_GPUTextureTransferInfo transfer_info = {};
            transfer_info.offset = 0;
            transfer_info.transfer_buffer = transferbuffer;
            SDL_GPUTextureRegion texture_region = {};
            texture_region.texture = tex;
            texture_region.w = w;
            texture_region.h = h;
            texture_region.d = 1;
            SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_MyUI._Device);
            SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
            SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
            SDL_EndGPUCopyPass(copy_pass);
            SDL_SubmitGPUCommandBuffer(cmd);

            // RELEASE RESOURCES
            SDL_ReleaseGPUTransferBuffer(_MyUI._Device, transferbuffer);

            // INSERT TEXTURE INTO LOADED ARRAY
            pBinding->texture = tex;
            ApplicationUI::ResourceTexture rtex{};
            rtex.DrawBind = pBinding;
            rtex.CommonTexture = {};
            rtex.EditorTexture = tex;
            it = _MyUI._ResourceTextures.insert(std::make_pair(name, std::move(rtex))).first;

        }
    }
    if(it == _MyUI._ResourceTextures.end())
    {
        // Error texture
        if(_MyUI._ErrorResources.count(name) == 0)
        {
            TTE_LOG("The required telltale editor resource '%s' was not found! Using a placeholder.", name.c_str());
            _MyUI._ErrorResources.insert(name);
        }
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 screenSize = ImGui::GetWindowSize(); // ie window size
        ImVec2 tl = ImGui::GetWindowPos();
        ImVec2 topLeft(tl.x + xPosFrac * screenSize.x, tl.y + yPosFrac * screenSize.y);
        ImVec2 bottomRight(topLeft.x + xSizeFrac * screenSize.x, tl.y + topLeft.y + ySizeFrac * screenSize.y);
        ImVec2 topRight(bottomRight.x, topLeft.y);
        ImVec2 bottomLeft(topLeft.x, bottomRight.y);

        drawList->AddTriangleFilled(
            topLeft,
            topRight,
            bottomRight,
            IM_COL32(255, 0, 255, 255)
        );
        drawList->AddTriangleFilled(
            topLeft,
            bottomLeft,
            bottomRight,
            IM_COL32(0, 0, 0, 255)
        );
    }
    else
    {
        ImTextureID id = (ImTextureID)it->second.DrawBind;
        ImVec2 winSize = ImGui::GetWindowSize();
        ImVec2 tl = ImGui::GetWindowPos(); //()->WorkPos;
        ImGui::GetWindowDrawList()->AddImage(id, ImVec2{ tl.x + xPosFrac * winSize.x, tl.y + yPosFrac * winSize.y },
            ImVec2{ tl.x + winSize.x * (xPosFrac + xSizeFrac), tl.y + winSize.y * (yPosFrac + ySizeFrac) }, ImVec2{ 0,0 }, ImVec2{ 1.f,1.f }, sc);
    }
}

ApplicationUI& UIComponent::GetApplication()
{
    return _MyUI;
}

void ApplicationUI::SetWindowSize(I32 width, I32 height)
{
    _NewWidth = width;
    _NewHeight = height;
}

// =========================================== MAIN APPLICATION LOOP

void ApplicationUI::SetCurrentPopup(Ptr<EditorPopup> p, EditorUI& ui)
{
    if (p && !_ActivePopup)
    {
        _ActivePopup = p;
        _ActivePopup->Editor = &ui;
        ImGui::PushOverrideID(9991);
        ImGui::OpenPopup(p->Name.c_str());
        ImGui::PopID();
    }
    else
    {
        TTE_LOG("WARN: Cannot set current popup one is already open or null");
    }
}

class _OnExitHelper
{

    public: String& Dir; 
          
     _OnExitHelper(String& dir) : Dir(dir) {}
         
    ~_OnExitHelper()
    {
        String f = Dir + "TTE_LatestLog.txt"; 
        DumpLoggerCache(f.c_str());
    }

};

void ApplicationUI::PopUI()
{
    if(_UIStack.size())
    {
        _UIStack.pop_back();
    }
}

void ApplicationUI::PushUI(Ptr<UIStackable> pStackable)
{
    if(pStackable)
    {
        _UIStack.push_back(pStackable);
    }
}

void ApplicationUI::_Update()
{
    ImGui::PushFont(_EditorFont, 14.0f);

    if(_UIStack.size())
    {
        Ptr<UIStackable> pStackable = _UIStack.back(); // copy
        pStackable->Render();
    }
    else
    {
        Ptr<UIStackable> pStackable = TTE_NEW_PTR(EditorUI, MEMORY_TAG_EDITOR_UI, *this);
        PushUI(pStackable);
        SetWindowSize(DEFAULT_WINDOW_SIZE);
       // SDL_SetWindowBordered(_Window, false);
        pStackable->Render();
    }

    ImGui::PopFont();
}

void ApplicationUI::_OnProjectLoad()
{

    // SWITCH SNAPSHOT
    GameSnapshot snapshot = _ProjectMgr.GetHeadProject()->ProjectSnapshot;
    _EditorResourceReg.reset();
    _Editor->Switch(snapshot);

    // LOAD REG + CONTEXT
    _EditorResourceReg = _Editor->CreateResourceRegistry();
    _EditorRenderContext = RenderContext::CreateShared(_Device, true, _Window, _EditorResourceReg);

    for(const auto& mp: _ProjectMgr.GetHeadProject()->MountDirectories)
    {
        _EditorResourceReg->MountSystem("<" + FileGetName(mp.string()) + ">/", mp.string(), true); // force legacy needed here?
    }

}

const String& UIComponent::GetLanguageText(CString id)
{
    return _MyUI.GetLanguageText(id);
}

const String& ApplicationUI::GetLanguageText(CString id)
{
    static String _LastUnk{};
    auto it = _LanguageMap.find(Symbol(id));
    if(it == _LanguageMap.end())
    {
        if(_UnknownLanguageIDs.count(id) == 0)
        {
            _UnknownLanguageIDs.insert(id);
            TTE_LOG("WARNING: Language ID %s has no mapping in current language %s!", id, _CurrentLanguage.c_str());
        }
        _LastUnk = id; // OK assign here, one thread for UI dispatch
        return _LastUnk;
    }
    return it->second;
}

void ApplicationUI::_SetLanguage(const String& language)
{
    if(_CurrentLanguage != language)
    {
        String langData = _Editor->LoadLibraryStringResource("Resources/Language/" + language + ".txt");
        if(langData.empty())
        {
            if(CompareCaseInsensitive(language, "English"))
            {
                TTE_LOG("ERROR: Could not load language %s! Language will be using IDs instead of text!", language.c_str());
                _CurrentLanguage = "";
            }
            else
            {
                TTE_LOG("ERROR: Could not load language %s! Defaulting to english.", language.c_str());
                _SetLanguage("English");
            }
            return;
        }
        _UnknownLanguageIDs.clear();
        _LanguageMap.clear();
        String line;
        std::istringstream in{langData};
        I32 lineN = 0;
        while(std::getline(in, line))
        {
            ++lineN;
            line = StringTrim(line);
            if(StringStartsWith(line, "#") || line.empty())
                continue;
            I32 colon = (I32)line.find('=');
            if(colon == String::npos)
            {
                TTE_LOG("WARNING: At language file %s.txt:%d: invalid syntax", language.c_str(), lineN);
            }
            else
            {
                String id = StringTrim(line.substr(0, colon));
                String v = StringTrim(line.substr(colon + 1));
                while(StringStartsWith(v,"\""))
                    v = v.substr(1);
                while (StringEndsWith(v, "\""))
                    v = v.substr(0, v.length() - 1);
                _LanguageMap[id] = v;
            }
        }
        TTE_LOG("Loaded language %s", language.c_str());
        _CurrentLanguage = language;
        _WorkspaceProps.SetString(WORKSPACE_KEY_LANG, _CurrentLanguage);
    }
}

template<ApplicationUI::_UIRenderFilter _MyFilter, Bool _ForceNoClear>
void ApplicationUI::_PerformUIRenderFiltered(RenderFrame* pFrame)
{
    uint32_t filter_type = _MyFilter == _UIRenderFilter::FILTER_OVERLAYS;
    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = _UIRenderBackBuffer;
    target_info.clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f };
    target_info.load_op = (!_ForceNoClear && (_MyFilter == _UIRenderFilter::FILTER_NONE || _MyFilter == _UIRenderFilter::FILTER_NORMAL)) ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    target_info.mip_level = 0;
    target_info.layer_or_depth_plane = 0;
    target_info.cycle = false;
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(_UIRenderCommandBuffer, &target_info, 1, nullptr);
    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), _UIRenderCommandBuffer, render_pass, nullptr, _MyFilter == _UIRenderFilter::FILTER_NONE ? nullptr : &filter_type);
    SDL_EndGPURenderPass(render_pass);
}

I32 ApplicationUI::Run(const std::vector<CommandLine::TaskArgument>& args)
{
    String userDir{};

    // LOGGER SETUP
    _OnExitHelper onExit(userDir);
    ToggleLoggerCache(true); // Log console to file for user to check any errors

    // USER DIR
    userDir = CommandLine::GetStringArgumentOrDefault(args, "-userdir", "./");
    if (!StringEndsWith(userDir, "/") && !StringEndsWith(userDir, "\\"))
        userDir += "/";
    if (userDir == "./")
    {
        // If none is given check if we are running from the bin dbg/rel folder. If so, use the development workspace folder.
        std::filesystem::path dev{ userDir };
        dev = std::filesystem::absolute(dev);
        dev += std::filesystem::path("../../Dev/Workspace");
        if (std::filesystem::is_directory(dev))
        {
            dev = std::filesystem::canonical(std::filesystem::absolute(dev));
            userDir = dev.string();
            std::replace(userDir.begin(), userDir.end(), '\\', '/');
            if (!StringEndsWith(userDir, "/"))
                userDir += "/";
            TTE_LOG("Detected development directory structure: using '%s' as development workspace directory", userDir.c_str());
        }
    }

    // LOAD WORKSPACE
    _WorkspaceProps.Load(userDir + "TTE_WorkspaceProperties.txt");
    if (!_WorkspaceProps.GetLoadState())
    {
        PlatformMessageBoxAndWait("Workspace Error", "There was an error reading the workspace properties. Please check the output log.");
        return 1;
    }
    _ProjectMgr.LoadFromWorkspace();

    // INIT RENDER CONTEXT (SDL) HERE
    RenderContext::Initialise();

    // INIT EMPTY CONTEXT
    _Editor = CreateEditorContext({});

    // INIT IMGUI & MAIN WINDOW
    U32 wW = 0, wH = 0;
    Ptr<UIProjectSelect> pSelect = TTE_NEW_PTR(UIProjectSelect, MEMORY_TAG_EDITOR_UI, *this);
    pSelect->GetWindowSize(wW, wH);
    CString titleBar = "Telltale Editor v" TTE_VERSION;
#ifdef DEBUG
    titleBar = "Telltale Editor [Debug] v" TTE_VERSION;
#endif
    _Window = SDL_CreateWindow(titleBar, wW, wH, SDL_WINDOW_HIDDEN); // TODO make this inside workspace settings
    SDL_SetWindowPosition(_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowMinimumSize(_Window, 340, 250);
    SDL_ShowWindow(_Window);
    PushUI(std::move(pSelect));

    // APP ICON
    I32 iW = 0, iH = 0;
    U8* rgba = _LoadSTBI("Resources/Textures/LogoSquare.png", *this, iW, iH);
    _AppIcon = SDL_CreateSurface(iW, iH, SDL_PIXELFORMAT_RGBA8888);
    memcpy(_AppIcon->pixels, rgba, iW * iH * 4);
    stbi_image_free(rgba);
    rgba = nullptr;
    SDL_SetWindowIcon(_Window, _AppIcon);

    // CREATE DEVICE
    Bool bDebug = false;
#ifdef DEBUG
    bDebug = true;
#endif
    _Device = SDL_CreateGPUDevice(RENDER_CONTEXT_SHADER_FORMAT, bDebug, nullptr);
    SDL_ClaimWindowForGPUDevice(_Device, _Window);

    // CREATE IMGUI CONTEXT
    IMGUI_CHECKVERSION();
    RenderContext::DisableDebugHUD(_Window);
    _ImContext = ImGui::CreateContext();
   // ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDpiScaleFonts = true;
    ImGui::StyleColorsDark(); // TODO MAKE THIS CUSTOM FROM WORKSPACE.
    ImGui_ImplSDL3_InitForSDLGPU(_Window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = _Device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(_Device, _Window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // INIT FONT
    _EditorFont = nullptr;
    DataStreamRef fontStream = _Editor->LoadLibraryResource("Resources/Fonts/EditorFont.ttf");
    DataStreamRef fallbackFontStream = _Editor->LoadLibraryResource("Resources/Fonts/FallbackFont.ttf");
    U8* pFontData = nullptr, *pFallbackData = nullptr;
    if(fontStream && fallbackFontStream)
    {
        pFontData = TTE_ALLOC(fontStream->GetSize(), MEMORY_TAG_TEMPORARY);
        pFallbackData = TTE_ALLOC(fallbackFontStream->GetSize(), MEMORY_TAG_TEMPORARY);
        fontStream->Read(pFontData, fontStream->GetSize());
        fallbackFontStream->Read(pFallbackData, fallbackFontStream->GetSize());
        ImFontConfig c{};
        c.FontDataOwnedByAtlas = false;
        c.OversampleV = 2;
        c.PixelSnapH = true;
        c.MergeMode = false;
        _EditorFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFontData, (int)fontStream->GetSize(), 18.0f, &c);
        c.MergeMode = true;
        _FallbackFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(pFallbackData, (int)fallbackFontStream->GetSize(), 18.0f, &c);
        TTE_FREE(pFontData);
        TTE_FREE(pFallbackData);
    }
    fontStream.reset(); // release file handle
    fallbackFontStream.reset();
    if(!_EditorFont)
    {
        TTE_LOG("Could not load editor font! Using default font");
        _EditorFont = _FallbackFont = ImGui::GetIO().FontDefault;
    }

    // INIT LANGUAGE
    String languages = _Editor->LoadLibraryStringResource("Resources/Language/Languages.txt");
    String line;
    std::istringstream langstream{languages};
    while(std::getline(langstream, line))
    {
        if(StringEndsWith(line,".txt"))
            line = line.substr(0, line.length() - 4);
        _AvailLanguages.push_back(line);
    }
    _CurrentLanguage = "";
    _SetLanguage(_WorkspaceProps.GetString(WORKSPACE_KEY_LANG, "English"));

    // IM GUI STYLE (CHANGE AS WE GO ON!) // 0.47 0.05 0.23

    ImVec4 redAccent = ImVec4(0.47f, 0.05f, 0.23f, 1.00f); // Telltale Logo warm red
    ImVec4 redAccentDim = ImVec4(0.47f, 0.05f, 0.23f, 0.6f);

    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_FrameBgHovered] = redAccentDim;
    colors[ImGuiCol_FrameBgActive] = redAccent;
    colors[ImGuiCol_TitleBgActive] = redAccent;
    colors[ImGuiCol_ButtonActive] = redAccent;
    colors[ImGuiCol_HeaderHovered] = redAccentDim;
    colors[ImGuiCol_HeaderActive] = redAccent;
    colors[ImGuiCol_SeparatorHovered] = redAccentDim;
    colors[ImGuiCol_SeparatorActive] = redAccent;
    colors[ImGuiCol_SliderGrabActive] = redAccent;
    colors[ImGuiCol_CheckMark] = redAccent;
    colors[ImGuiCol_TabHovered] = redAccentDim;
    colors[ImGuiCol_TabSelected] = redAccent;
    colors[ImGuiCol_TabSelectedOverline] = redAccent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(redAccent.x, redAccent.y, redAccent.z, 0.3f); // transparent ish
    colors[ImGuiCol_TextLink] = redAccent;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.54f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.00f, 1.00f, 0.40f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.58f, 0.73f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    // MAIN APPLICATION LOOP
    _Flags.Add(ApplicationFlag::RUNNING);
    while (_Flags.Test(ApplicationFlag::RUNNING))
    {
        // BEGIN
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // UPDATE AND RENDER UI
        _Update();
        _RenderPopups();

        // END
        ImGui::Render();
        if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
        
        if(_NewWidth == -1 && _NewHeight == -1) // If changes, drop this frame
        {
            SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(_Device); // Acquire a GPU command buffer
            SDL_GPUTexture* swapchain_texture;
            SDL_AcquireGPUSwapchainTexture(command_buffer, _Window, &swapchain_texture, nullptr, nullptr); // Acquire a swapchain texture
            
            if (swapchain_texture != nullptr)
            {
                ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);
                _UIRenderBackBuffer = swapchain_texture;
                _UIRenderCommandBuffer = command_buffer;

                if (_EditorRenderContext)
                {
                    // UI RENDER CALLBACK
                    Method<ApplicationUI, false, RenderFrame*>
                        callback(this, static_cast<void (ApplicationUI::*)(RenderFrame*)>(&ApplicationUI::_PerformUIRenderFiltered<_UIRenderFilter::FILTER_NONE, true>));
                    _EditorRenderContext->GetPostRenderMainThreadCallbacks().PushCallback(TTE_PROXY_PTR(&callback, FunctionBase));

                    // PERFORM RENDER & GPU SYNCH
                    if (!_EditorRenderContext->FrameUpdate(!_Flags.Test(ApplicationFlag::RUNNING), command_buffer, swapchain_texture))
                    {
                        Quit();
                    }
                }
                else
                {
                    // NO SCENE VIEW YET. NORMAL RENDER
                    _PerformUIRenderFiltered<_UIRenderFilter::FILTER_NONE, false>(nullptr);
                    SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);
                    SDL_WaitForGPUFences(_Device, true, &fence, 1);
                    SDL_ReleaseGPUFence(_Device, fence);

                    SDL_Event event{};
                    while (SDL_PollEvent(&event))
                    {
                        ImGui_ImplSDL3_ProcessEvent(&event);
                        if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(_Window)))
                        {
                            Quit();
                        }
                    }
                }
                _UIRenderBackBuffer = nullptr;
                _UIRenderCommandBuffer = nullptr;
            }
        }
        else
        {
            SDL_SetWindowSize(_Window, _NewWidth, _NewHeight);
            _NewWidth = _NewHeight = -1;
        } 
    }

    // SAVE WORKSPACE
    _ProjectMgr.SaveToWorkspace();
    _WorkspaceProps.Save();

    // END
    _UIStack.clear();
    for(auto& tex: _ResourceTextures)
    {
        if(tex.second.EditorTexture)
        {
            SDL_ReleaseGPUTexture(_Device, tex.second.EditorTexture);
        }
        if(tex.second.DrawBind)
        {
            TTE_DEL(tex.second.DrawBind);
        }
    }
    _ResourceTextures.clear();
    _EditorRenderContext.reset();
    _EditorResourceReg.reset();
    FreeEditorContext();
    JobScheduler::Shutdown();
    SDL_WaitForGPUIdle(_Device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroySurface(_AppIcon);
    SDL_DestroyGPUDevice(_Device);
    SDL_DestroyWindow(_Window);
    _AppIcon = nullptr;
    _Device = nullptr;
    _ImContext = nullptr;
    _Window = nullptr;
    _Editor = nullptr;

    return 0;
}

void ApplicationUI::_RenderPopups()
{
    if (_ActivePopup)
    {
        ImVec2 size = _ActivePopup->GetPopupSize();
        ImGui::SetNextWindowSize(size);

        ImGui::PushOverrideID(9991);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::BeginPopupModal(_ActivePopup->Name.c_str()))
        {
            if (_ActivePopup->Render())
            {
                ImGui::CloseCurrentPopup();
                _ActivePopup = nullptr;
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopID();
    }
}
