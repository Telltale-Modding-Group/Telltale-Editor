#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>

#include <Renderer/RenderAPI.hpp>

#include <vector>

// Scene modules are module_xxx.prop parent properties. they are 'components' or 'behaviour's of game objects.
// in the engine these correspond to XXX::OnSetupAgent(Ptr<Agent> pAgentGettingCreated, Handle<PropertySet>& hRenderableProps) functions.
enum class SceneModuleType : I32
{
	RENDERABLE, // module_renderable.prop
	NUM,
	UNKNOWN = -1,
};

// Renderable scene component (RenderObject_Mesh* obj). Any actual render types (eg RenderVertexState) can be filled up, but not created
// As in the actual buffers should only be in data streams on CPU side.
struct Scene_Renderable
{
	
	static constexpr SceneModuleType ModuleType = SceneModuleType::RENDERABLE;
	
	// separate triangle sets for default and shadow parts
	enum RenderViewType
	{
		DEFAULT = 0,
		SHADOW = 1,
		NUM = 2,
	};
	
	// A mesh batch. This is a optimised way telltale use to draw once per material. Most members TODO.
	struct MeshBatch
	{
		
		BoundingBox BBox;
		Sphere BSphere;
		U32 BatchUsage = 0; // not in use right now but in future. 1:deformable, 2:single deformable, 4:double sided, 8:triangle strip
		U32 MinVertIndex = 0;
		U32 MaxVertIndex = 0;
		U32 BaseIndex = 0;
		U32 StartIndex = 0; // start index buffer indicie (index) index
		U32 NumPrimitives = 0;
		U32 NumIndices = 0;
		I32 TextureIndices[RenderViewType::NUM]{-1, -1}; // index into textures array for the bound texture.
		I32 MaterialIndex = -1; // material index
		U32 AdjacencyStartIndex = 0;
		
	};
	
	// May be extended. Vertex state.
	struct VertexState
	{
		RenderVertexState Default; // default. in future we can have more for skinning (compute etc). DO NOT CREATE. only set info members.
	};
	
	// LOD in mesh
	struct LODInstance
	{
		
		Sphere BSphere; // bounding sphere for this LOD
		BoundingBox BBox; // bounding box for this LOD
		
		U32 VertexStateIndex = 0; // index into MeshInstance::VertexState
		
		std::vector<MeshBatch> Batches[RenderViewType::NUM]; // batches for default and shadow
		
	};
	
	// Renderable objects are a list of meshes (in props it has the 'D3D Mesh List' key or 'D3D Mesh'. base mesh + list
	struct MeshInstance
	{
		
		Sphere BSphere; // bounding sphere for total mesh
		BoundingBox BBox; // bounding box for total mesh
		
		std::vector<LODInstance> LODs; // mesh LODs
		std::vector<VertexState> VertexStates; // vertex states (draw call bind sets), containing verts/inds/etc.
		
	};
	
	std::vector<MeshInstance> MeshList; // list of meshes
	
};

/// An agent in the scene, or game object unity terms.
struct SceneAgent
{
	
	Symbol NameSymbol; // as a symbol
	String Name; // agent name
	
	// points into arrays inside scene.
	SceneModuleType ModuleIndices[(I32)SceneModuleType::NUM];
	
	inline SceneAgent() : Name("")
	{
		for(I32 i = 0; i < (I32)SceneModuleType::NUM; i++)
			ModuleIndices[i] = SceneModuleType::UNKNOWN;
	}
	
	inline Bool operator==(const SceneAgent& rhs) const
	{
		return rhs.NameSymbol == NameSymbol;
	}
	
	inline SceneAgent(String _name) : SceneAgent()
	{
		Name = std::move(_name);
		NameSymbol = Symbol(Name);
	}
	
};

// comparator for scene agents
struct SceneAgentComparator {
	
	using is_transparent = void; // Enables lookup by U64
	
	inline Bool operator()(const SceneAgent& lhs, const SceneAgent& rhs) const {
		return lhs.NameSymbol < rhs.NameSymbol;
	}
	
	inline Bool operator()(const SceneAgent& lhs, Symbol rhs) const {
		return lhs.NameSymbol < rhs;
	}
	
	inline Bool operator()(Symbol lhs, const SceneAgent& rhs) const {
		return lhs < rhs.NameSymbol;
	}
};

enum class SceneMessageType : U32;
struct SceneMessage;

struct RenderFrame;
class RenderContext;

/// A collection of agents. This is the common scene class for all games by telltale. This does not have any of the code for serialistion, that is done when lua injects scene information
/// from its scene meta class into this. This is a common class to all games and represents a common telltale scene.
class Scene
{
public:
	
	Scene() = default;
	
	Scene(const Scene&) = delete; // DISALLOW COPY
	Scene& operator=(const Scene&) = delete;
	
	Scene(Scene&&) = default; // ALLOW MOVE
	Scene& operator=(Scene&&) = default;
	
	inline void SetName(String name)
	{
		Name = name;
	}
	
	inline String GetName()
	{
		return Name;
	}
	
private:
	
	// ==== RENDER CONTEXT USE ONLY
	
	// Internal call. Called by render context in async to process scene messages
	void AsyncProcessRenderMessage(RenderContext& context, SceneMessage message, const SceneAgent* pAgent);
	
	// Async so only access this scene. Setup render information. Thread safe with perform render, detach and async process messages.
	void OnAsyncRenderAttach(RenderContext& context);
	
	// When the scene needs to stop. Free anything needed.
	void OnAsyncRenderDetach(RenderContext& context);
	
	// Called each frame async. To ensure speed this is async and should be performed isolated. Issue render instructions here.
	void PerformAsyncRender(RenderContext& context, RenderFrame& frame, Float deltaTime);
	
	// =====
	
	String Name; // scene name
	std::set<SceneAgent, SceneAgentComparator> _Agents; // scene agent list
	std::vector<Camera> _ViewStack; // view stack
	
	// ==== DATA ORIENTATED AGENT MODULE ATTACHMENTS
	
	std::vector<Scene_Renderable> _Renderables;
	
	friend class RenderContext;
	friend struct SceneAgent;
	
};
