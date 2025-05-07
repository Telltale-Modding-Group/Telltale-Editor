#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>
#include <Common/Common.hpp>
#include <Meta/Meta.hpp>
#include <Resource/TTArchive.hpp>
#include <Resource/TTArchive2.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <type_traits>
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
    
    virtual ~EditorTask() = default;
    
};

// Normalises a mesh into a scene agent renderable's meshes list. inserts a new mesh instance.
struct MeshNormalisationTask : EditorTask
{
    
    inline MeshNormalisationTask(U32 id, Ptr<ResourceRegistry> registry) : EditorTask(false, id), Renderable(registry) {}
    
    virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
    
    virtual void Finalise(TelltaleEditor&) override;
    
    Scene* Output; // scene
    
    Symbol Agent; // agent name
    
    Mesh::MeshInstance Renderable; // internal object.
    
    Meta::ClassInstance Instance; // D3DMesh instance in the meta system
    
};

// Normalises a texture.
struct TextureNormalisationTask : EditorTask
{
    
    inline TextureNormalisationTask(U32 id, Ptr<ResourceRegistry> registry) : EditorTask(false, id), Local(registry) {}
    
    virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
    
    virtual void Finalise(TelltaleEditor&) override;
    
    const Ptr<RenderTexture> Output; // output object. Do not access while normalisation. Use Local object.
    
    RenderTexture Local; // local object
    
    Meta::ClassInstance Instance; // D3DTexture/T3Texture meta instance
    
};

// Normalises any other common type
template<typename T>
struct CommonNormalisationTask : EditorTask
{
    
    static_assert(std::is_base_of<Handleable, T>::value, "T must be handleable");
    
    inline CommonNormalisationTask(U32 id, Ptr<ResourceRegistry> registry) : EditorTask(false, id), Local(registry) {}
    
    virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override
    {
        String fn = Meta::GetInternalState().Classes.find(Instance.GetClassID())->second.NormaliserStringFn;
        auto normaliser = Meta::GetInternalState().Normalisers.find(fn);
        TTE_ASSERT(normaliser != Meta::GetInternalState().Normalisers.end(), "Normaliser not found: '%s'", fn.c_str());
        
        ScriptManager::GetGlobal(thread.L, fn, true);
        if(thread.L.Type(-1) != LuaType::FUNCTION)
        {
            thread.L.Pop(1);
            TTE_ASSERT(thread.L.LoadChunk(fn, normaliser->second.Binary,
                                          normaliser->second.Size, LoadChunkMode::BINARY), "Could not load normaliser chunk for %s", fn.c_str());
        }
        
        Instance.PushWeakScriptRef(thread.L, Instance.ObtainParentRef());
        thread.L.PushOpaque(&Local);
        
        thread.L.CallFunction(2, 1, false);
        
        Bool result;
        
        if(!(result=ScriptManager::PopBool(thread.L)))
        {
            TTE_LOG("Normalise failed for %s", fn.c_str());
        }
        else
        {
            Local.FinaliseNormalisationAsync();
        }
        return true;
    }
    
    virtual void Finalise(TelltaleEditor&) override
    {
        *Output.get() = std::move(Local);
    }
    
    Ptr<Handleable> Output; // output object. Do not access while normalisation. Use Local object.
    
    T Local; // local object
    
    Meta::ClassInstance Instance; // meta instance
    
};

// Extracts a set of file names from an archive. Simpler version. Doesnt give option to sort files into folders 
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

// Called when filename has been extracted
using ResourceExtractCallback = void(const String& fileName);

// Extracts from resource sys
struct ResourcesExtractionTask : EditorTask
{
    
    inline ResourcesExtractionTask(U32 id) : EditorTask(false, id) {}
    
    virtual Bool PerformAsync(const JobThread& thread, ToolContext* pLockedContext) override;
    
    virtual void Finalise(TelltaleEditor&) override;
    
    Ptr<ResourceRegistry> Registry;
    
    String Folder;
    
    String Logical;
    StringMask Mask{""};
    
    Bool UseMask = false;
    Bool Folders = false; // split into folders
    
    U32 AsyncWorkers = 0; // per thread
    std::set<String>* WorkingFiles;
    ResourceExtractCallback* Callback = nullptr;
    
};

