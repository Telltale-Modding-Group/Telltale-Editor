#pragma once

#include <Core/Context.hpp>
#include <TelltaleEditor.hpp>

struct ImGuiContext;

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

private:

    void _Update();

    ImGuiContext* _ImContext;
    TelltaleEditor* _Editor;
    SDL_GPUDevice* _Device;
    SDL_Window* _Window;
    TTEProperties _WorkspaceProps; // workspace props

};
