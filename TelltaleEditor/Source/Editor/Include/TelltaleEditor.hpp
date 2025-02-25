#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>

#include <Renderer/RenderContext.hpp>

// C++ API

/// The ToolLibrary is the underlying library. This is the main part of the editor which allows for this post job and wait system for any task related. This also includes GUI updates.
/// This represents the application state if using the editor and not the tool library.
class TelltaleEditor
{
public:
	
	TelltaleEditor(GameSnapshot snapshot, Bool ui);
	~TelltaleEditor();
	
	// Main thread update. Executes render commands and may be slow. Returns TRUE if we can call update again (ie not quitting)
	// Pass in if you want to force it to quit (ie make user click window X)
	Bool Update(Bool bQuit);
	
	inline ToolContext* GetToolContext()
	{
		return _ModdingContext;
	}
	
private:
	
	ToolContext* _ModdingContext = nullptr;
	Bool _UI = false; // if running renderer
	Bool _Running = false; // if render window is still running and no exit request
	
};

/// Creatae the global editor context (use once). Creates tool context and render context internally. Set UI to true to run the UI as well.
TelltaleEditor* CreateEditorContext(GameSnapshot snapshot, Bool UI);

void FreeEditorContext();
