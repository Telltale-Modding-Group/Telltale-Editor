#include <TelltaleEditor.hpp>

Bool AsyncTTETaskDelegate(const JobThread& thread, void* argA, void* argB)
{
	EditorTask* pTask = (EditorTask*)argA;
	return pTask->PerformAsync(thread, (ToolContext*)argB);
}

// SCENE NORMALISATION

Bool SceneNormalisationTask::PerformAsync(const JobThread& thread, ToolContext* pLockedContext)
{
	return true;
}

void SceneNormalisationTask::Finalise(TelltaleEditor& editor)
{
	*OutputUnsafe = std::move(Working);
}

// MESH NORMALISATION

Bool MeshNormalisationTask::PerformAsync(const JobThread& thread, ToolContext* pLockedContext)
{
	String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.NormaliserStringFn;
	auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
	TTE_ASSERT(normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found for mesh instance: '%s'", fn.c_str());
	
	TTE_ASSERT(thread.L.LoadChunk(fn, normaliser->second.Binary, normaliser->second.Size, true), "Could not load chunk for %s", fn.c_str());
	
	Instance.PushWeakScriptRef(thread.L, Instance.ObtainParentRef());
	thread.L.PushOpaque(this);
	
	thread.L.CallFunction(2, 1, false);
	
	Bool result;
	
	if(!(result=ScriptManager::PopBool(thread.L)))
	{
		TTE_LOG("Normalise failed for mesh in %s", fn.c_str());
	}
	
	return result;
}

void MeshNormalisationTask::Finalise(TelltaleEditor& editor)
{
	TTE_ASSERT(Output->ExistsAgent(Agent), "Agent does not exist anymore for output mesh normalisation");
	
	Output->GetAgentModule<SceneModuleType::RENDERABLE>(Agent) = std::move(Renderable); // move processed new mesh renderable
}

// ARCHIVE EXTRACTION

Bool ArchiveExtractionTask::PerformAsync(const JobThread &thread, ToolContext *pLockedContext)
{
	if(!StringEndsWith(Folder, "/") && !StringEndsWith(Folder, "\\"))
		Folder += "/";
	if(Archive1 != nullptr)
	{
		// EXTRACT .TTARCH
		if(Files.size() == 0)
			Archive1->GetFiles(Files);
		
		for(auto& fileName: Files)
		{
			DataStreamRef file = Archive1->Find(fileName);
			file = Meta::MapDecryptingStream(file); // ensure its not encrypted.
			DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, Folder + fileName));
			if(out && file)
			{
				DataStreamManager::GetInstance()->Transfer(file, out, file->GetSize());
			}
			else
			{
				TTE_LOG("Could not extract %s: file streams were not valid", fileName.c_str());
			}
		}
	}
	else
	{
		TTE_ASSERT(Archive2, "No archive set in extraction task!");
		// EXTRACT .TTARCH2
		if(Files.size() == 0)
			Archive2->GetFiles(Files);
		
		for(auto& fileName: Files)
		{
			DataStreamRef file = Archive2->Find(fileName);
			DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(ResourceURL(ResourceScheme::FILE, Folder + fileName));
			if(out && file)
			{
				DataStreamManager::GetInstance()->Transfer(file, out, file->GetSize());
			}
			else
			{
				TTE_LOG("Could not extract %s: file streams were not valid", fileName.c_str());
			}
		}
	}
	Files.clear(); // not needed
	return true;
}

void ArchiveExtractionTask::Finalise(TelltaleEditor & editorContext)
{
	// No finalisation needed
}
