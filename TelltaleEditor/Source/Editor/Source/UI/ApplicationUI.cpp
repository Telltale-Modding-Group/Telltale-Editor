#include <UI/ApplicationUI.hpp>
#include <Renderer/RenderContext.hpp>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui_internal.h>

#include <filesystem>

ApplicationUI::ApplicationUI() : _Device(nullptr), _Window(nullptr), _Editor(nullptr), _ImContext(nullptr)
{

}

ApplicationUI::~ApplicationUI()
{

}

class _OnExitHelper
{
public: String& Dir; _OnExitHelper(String& dir) : Dir(dir)
{
} ~_OnExitHelper()
{
    String f = Dir + "TTE_LatestLog.txt"; DumpLoggerCache(f.c_str());
}
};

void ApplicationUI::_Update()
{
    // Ok maybe an implementation is needed here
    ImGui::Begin("Wind");
    ImGui::Text("Hello");
    ImGui::End();
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
            dev = std::filesystem::absolute(dev);
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

    // INIT RENDER CONTEXT (SDL) HERE
    RenderContext::Initialise();

    // INIT IMGUI & MAIN WINDOW
    _Window = SDL_CreateWindow("", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN); // TODO make this inside workspace settings
    SDL_SetWindowPosition(_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(_Window);
    Bool bDebug = false;
#ifdef DEBUG
    bDebug = true;
#endif
    _Device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXBC, bDebug, nullptr);
    SDL_ClaimWindowForGPUDevice(_Device, _Window);
    IMGUI_CHECKVERSION();
    _ImContext = ImGui::CreateContext();
    ImGui::StyleColorsDark(); // TODO MAKE THIS CUSTOM FROM WORKSPACE.
    ImGui_ImplSDL3_InitForSDLGPU(_Window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = _Device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(_Device, _Window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // MAIN APPLICATION LOOP
    Bool bRunning = true;
    while (bRunning)
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                bRunning = false;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(_Window))
                bRunning = false;
        }

        // BEGIN
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // UPDATE
        _Update();

        // END
        ImGui::Render();
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(_Device); // Acquire a GPU command buffer

        SDL_GPUTexture* swapchain_texture;
        SDL_AcquireGPUSwapchainTexture(command_buffer, _Window, &swapchain_texture, nullptr, nullptr); // Acquire a swapchain texture

        if (swapchain_texture != nullptr)
        {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), command_buffer);

            // Setup and start a render pass
            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f };
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;
            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
            ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), command_buffer, render_pass);
            SDL_EndGPURenderPass(render_pass);
        }
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    // SAVE WORKSPACE
    _WorkspaceProps.Save();

    // END
    SDL_WaitForGPUIdle(_Device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    SDL_ReleaseWindowFromGPUDevice(_Device, _Window);
    SDL_DestroyGPUDevice(_Device);
    SDL_DestroyWindow(_Window);
    _Device = nullptr;
    _ImContext = nullptr;
    _Window = nullptr;
    _Editor = nullptr;
    FreeEditorContext();

    return 0;
}
