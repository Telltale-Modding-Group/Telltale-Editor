#pragma once

#include <set>

#include <Core/Config.hpp>
#include <Core/Context.hpp>
#include <EditorTasks.hpp>
#include <Meta/Meta.hpp>

#include <Common/Common.hpp>

// C++ API

/// The ToolLibrary is the underlying library. This is the main part of the editor which allows for this post job and wait system for any task related. This also includes GUI.
/// This represents the application state if using the editor and not the tool library - which this uses internally privately.
/// VERY IMPORTANT: If you are using this API and no the tool context (you should always use this API) then do not access the ToolContext directly, interact through this.
/// See EditorTasks.h for information on about the tasks you can post.
///
class TelltaleEditor
{
public:
	
	TelltaleEditor(GameSnapshot snapshot, Bool ui);
	~TelltaleEditor();
	
	// Main thread update. Executes render commands and may be slow. Returns TRUE if we can call update again (ie not quitting)
	// Pass in if you want to force it to quit (ie make user click window X)
	Bool Update(Bool bQuit);
	
	/**
	 Returns if the context is unaccessible because it is currently busy with a job which required it completely. The UI can still be drawn and will be interactive.
	 */
	Bool ContextIsBusy();
	
	/**
	 Waits for the context to be free, to finish all blocking jobs. Should not really be used UNLESS you just want to complete tasks asap eg from a modding script.
	 */
	void Wait();
	
	/**
	 Queries if the given task has finished. If this returns true, it has been finalised and all updates will have completed.
	 This can also be ensured after Wait() finishes.
	 */
	Bool QueryTask(U32 task);
	
	/**
	 Enqueues a task which will normalise the given scene instance into the pScene argument. This standardises from any game version the given scene
	 into the common scene. The scene passed is safe to edit after this call but it will be overriden possibly if you call any function part of this class.
	 At that point the scene will be replaced (its data) with the normalised scene.
	 Please note that for any of the results of this task to take effect, you must call Wait or QueryTask.
	 Returns the task handle which you can query the completion with.
	 */
	U32 EnqueueNormaliseSceneTask(Scene* pScene, Meta::ClassInstance sceneInstance);
	
	/**
	 Enqueues a task which normalises the given D3D mesh instance for the current game snapshot.
	 See EnqueueMeshNormaliseTask.
	 */
	U32 EnqueueNormaliseMeshTask(Scene* pScene, Symbol agent, Meta::ClassInstance d3dmeshInstance);
	
	/**
	 Enqueues a task which will asynchronously extract all of the files in the given archive. If the files array is empty, all files will be extracted.
	 */
	U32 EnqueueArchiveExtractTask(TTArchive* pArchive, std::vector<String>&& files, String outputFolder);
	
	/**
	 Enqueues a task which will asynchronously extract all of the files in the given archive. If the files array is empty, all files will be extracted.
	 */
	U32 EnqueueArchive2ExtractTask(TTArchive2* pArchive, std::vector<String>&& files, String outputFolder);
	
	// See ToolContext version. Delegates and thread safe.
	inline DataStreamRef LoadLibraryResource(String name)
	{
		return _ModdingContext->LoadLibraryResource(name);
	}
	
private:
	
	// probes any tasks, if any finished their finalisers are called on main thread.
	// always called on every TTE function in this class. specify if we want to wait for all to finish
	// check specific is used for return value. if non -1 will return if the given has finished
	Bool _ProbeTasks(Bool bWaitOnAll, U32 _CheckSpecific = (U32)-1);
	
	// enqueues blocking job which requires the lua env
	void _EnqueueTask(EditorTask* pTask);
	
	ToolContext* _ModdingContext = nullptr;
	Bool _UI = false; // if running renderer
	Bool _Running = false; // if render window is still running and no exit request
	
	U32 _TaskFence = 0; // counter
	
	std::vector<std::pair<EditorTask*, JobHandle>> _Active;
	
};

Bool AsyncTTETaskDelegate(const JobThread& thread, void* argA, void* argB);

/// Creatae the global editor context (use once). Creates tool context internally. Set UI to true to run the UI as well.
TelltaleEditor* CreateEditorContext(GameSnapshot snapshot, Bool UI);

void FreeEditorContext();
