#include <UI/UILayer.hpp>

#include <imgui_internal.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>

#include <array>

// NEEDS IMPL. MULTI WINDOWS TODO.

extern ImGuiContext* GImGui;

struct UILayer::Attachment
{
    
    ImGuiContext* ImContext;
    ImDrawData* DrawData[2];
    
    Attachment() // : DrawList({&GImGui->DrawListSharedData, &GImGui->DrawListSharedData})
    {
        
    }
    
};

UILayer::UILayer(RenderContext& context) : RenderLayer("Editor UI", context)
{
    _Attachment = TTE_NEW(Attachment, MEMORY_TAG_EDITOR_UI);
    _MyselfProxy = TTE_PROXY_PTR(this, UILayer);
    _RenderCallbackCache = ALLOCATE_METHOD_CALLBACK_1(_MyselfProxy, OnAsyncRender, UILayer, RenderFrame*);
    
    ImGuiContext* pContext = _Attachment->ImContext = ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLGPU(context.GetWindowUnsafe());
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = context.GetDeviceUnsafe();
    init_info.ColorTargetFormat = context.GetDeviceSwapChainFormat();
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    ImGui_ImplSDLGPU3_Init(&init_info);
    _Attachment->DrawData[0] = IM_NEW(ImDrawData);
    _Attachment->DrawData[1] = IM_NEW(ImDrawData);
}

UILayer::~UILayer()
{
    IM_DELETE(_Attachment->DrawData[0]);
    IM_DELETE(_Attachment->DrawData[1]);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();
    TTE_DEL(_Attachment);
    _Attachment = nullptr;
}

// Render previous frame draw commands from ImGui UI
void UILayer::OnAsyncRender(RenderFrame *frame)
{
}

// Issue UI draw commands (ie update thread)
RenderNDCScissorRect UILayer::AsyncUpdate(RenderFrame &frame, RenderNDCScissorRect parentScissor, Float deltaTime)
{
    ImGui::SetCurrentContext(_Attachment->ImContext);
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui::NewFrame();
    
    // draw gui
    //ImGui::Text("This is the best tool ever.");
    
    ImGui::EndFrame();
    ImGui::Render();
    *_Attachment->DrawData[frame.FrameNumber & 1] = std::move(*ImGui::GetDrawData());
    GetRenderContext().PushPreRenderCallback(frame, _RenderCallbackCache);
    
    return parentScissor;
}

void UILayer::AsyncProcessEvents(const std::vector<RuntimeInputEvent> &events)
{
    
}
