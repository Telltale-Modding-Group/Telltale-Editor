#pragma once

#include <Core/Context.hpp>
#include <TelltaleEditor.hpp>
#include <Renderer/RenderContext.hpp>
#include <ProjectManager.hpp>
#include <Core/Symbol.hpp>

#include <UI/MenuBar.hpp>
#include <UI/EditorUI.hpp>

#include <unordered_map>
#include <set>
#include <map>

#include <SDL3/SDL_surface.h>

#define WORKSPACE_KEY_LANG "Workspace - Language"

struct ImGuiContext;
struct ImVec2;
struct ImFont;
struct EditorUI;

enum class ApplicationFlag
{
    RUNNING = 1,
};


struct EditorPopup
{

    const String Name;
    EditorUI* Editor = nullptr;

    inline EditorPopup(const String& N) : Name(N) {}

    virtual ImVec2 GetPopupSize() = 0;
    virtual Bool Render() = 0;

};

/**
 * Main Telltale Editor application, UI interface. The normal TelltaleEditor class is just the functionality which can be used without the UI.
 * Just call run with the command line arguments to run the UI.
 * Please DO NOT create a Telltale Editor class instance. This class is the most high level if being used and will create an editor context, as well as tool context.
 */
class ApplicationUI
{
public:

    ApplicationUI();

    ~ApplicationUI();

    /**
     * Run the Telltale Editor application. Retunrs the exit code. This will create any required contexts, just create an instance of this class and call this then exit.
     */
    I32 Run(const std::vector<CommandLine::TaskArgument>& args);

    void PushUI(Ptr<UIStackable> pStackable);

    void PopUI();
    
    void SetWindowSize(I32 width, I32 height);

    inline TelltaleEditor* GetEditorContext()
    {
        return _Editor;
    }

    inline TTEProperties& GetWorkspaceProperties()
    {
        return _WorkspaceProps;
    }

    inline SDL_GPUDevice* GetRenderDevice()
    {
        return _Device;
    }

    inline ImFont* GetEditorFont()
    {
        return _EditorFont;
    }

    inline ImFont* GetFallbackFont()
    {
        return _FallbackFont;
    }

    inline ProjectManager& GetProjectManager()
    {
        return _ProjectMgr;
    }

    inline String GetLanguage()
    {
        return _CurrentLanguage.empty() ? "English" : _CurrentLanguage;
    }
    
    inline Ptr<ResourceRegistry> GetRegistry()
    {
        return _EditorResourceReg;
    }

    inline Ptr<RenderContext> GetRenderContext()
    {
        return _EditorRenderContext;
    }

    const String& GetLanguageText(CString id);

    void Quit();

    void SetCurrentPopup(Ptr<EditorPopup>, EditorUI& editor);

private:

    struct ResourceTexture
    {
        Ptr<RenderTexture> CommonTexture; // engine texture
        SDL_GPUTexture* EditorTexture = nullptr; // raw SDL texture
        SDL_GPUTextureSamplerBinding* DrawBind = nullptr;
    };

    // UI CLASSES
    std::vector<Ptr<UIStackable>> _UIStack;

    void _Update();

    void _OnProjectLoad();

    void _SetLanguage(const String& language);

    void _RenderPopups();

    // GENERAL FUNCTIONALITY
    Flags _Flags;
    ImGuiContext* _ImContext;
    TelltaleEditor* _Editor;
    SDL_GPUDevice* _Device;
    SDL_Window* _Window;
    ImFont* _EditorFont, *_FallbackFont;
    TTEProperties _WorkspaceProps; // workspace props
    ProjectManager _ProjectMgr;
    I32 _NewWidth = -1, _NewHeight = -1;
    Ptr<EditorPopup> _ActivePopup;

    // RESOURCE MANAGEMENT
    SDL_Surface* _AppIcon;
    std::unordered_map<String, ResourceTexture> _ResourceTextures;
    std::set<String> _ErrorResources;

    // LANGUAGE
    std::vector<String> _AvailLanguages;
    String _CurrentLanguage;
    std::map<Symbol, String> _LanguageMap;
#ifdef DEBUG
    std::set<String> _UnknownLanguageIDs;
#endif

    // DATA AVAILABLE ONCE PROJECT LOADED
    Ptr<RenderContext> _EditorRenderContext;
    Ptr<ResourceRegistry> _EditorResourceReg;

    friend class UIComponent;
    friend class UIProjectSelect;
    friend class UIProjectCreate;
    friend class EditorUI;

};
