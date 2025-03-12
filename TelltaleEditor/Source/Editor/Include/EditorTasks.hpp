#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>
#include <Common/Scene.hpp>
#include <Meta/Meta.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp>

#include <vector>

class TelltaleEditor;

// High level editor task
struct EditorTask
{
	
	const U32 TaskID;
	const Bool IsBlocking;
	
	// Constructor delegated by each
	inline EditorTask(Bool blocking, U32 id) : IsBlocking(blocking), TaskID(id) {}
	
	// locked context is non-null if IsBlocking is true
	virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) = 0; // perform async
	
	// finalise it: eg moving async finished work into main thread object. always main thread call
	virtual void Finalise(TelltaleEditor&) = 0;
	
	inline virtual ~EditorTask() {}
	
};

// Normalises a scene from high level snapshot dependent lua into the common Scene format (Scene.hpp)
struct SceneNormalisationTask : EditorTask
{
	
	inline SceneNormalisationTask(U32 id) : EditorTask(false, id) {}
	
	virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
	
	virtual void Finalise(TelltaleEditor&) override;
	
	Scene* OutputUnsafe; // output scene
	
	Scene Working; // working scene to normalise to. in finalise this is moved to output
	
	Meta::ClassInstance Instance; // instance in the meta system
	
};

// Normalises a mesh into a scene agent renderable's meshes list. inserts a new mesh instance.
struct MeshNormalisationTask : EditorTask
{
	
	inline MeshNormalisationTask(U32 id) : EditorTask(false, id) {}
	
	virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
	
	virtual void Finalise(TelltaleEditor&) override;
	
	Scene* Output; // scene
	
	Symbol Agent; // agent name
	
	Mesh::MeshInstance Renderable; // output object.
	
	Meta::ClassInstance Instance; // D3DMesh instance in the meta system
	
};

// Extracts a set of file names from an archive
struct ArchiveExtractionTask : EditorTask
{
	
	inline ArchiveExtractionTask(U32 id) : EditorTask(false, id) {}
	
	virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
	
	virtual void Finalise(TelltaleEditor&) override;
	
	std::set<String> Files; // file names to extract
	String Folder; // output folder
	
	TTArchive* Archive1 = nullptr; // archives, either 1 or 2 is set for .ttarch or .ttarch2
	TTArchive2* Archive2 = nullptr;
	
};

