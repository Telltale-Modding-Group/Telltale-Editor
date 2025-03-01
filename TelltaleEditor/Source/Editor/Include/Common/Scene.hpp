#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>

#include <BitSet.hpp>
#include <Meta/Meta.hpp>

#include <Common/Mesh.hpp>

#include <vector>

// Scene modules are module_xxx.prop parent properties. they are 'components' or 'behaviour's of game objects.
// in the engine these correspond to XXX::OnSetupAgent(Ptr<Agent> pAgentGettingCreated, Handle<PropertySet>& hRenderableProps) functions.
enum class SceneModuleType : I32
{
	RENDERABLE, // module_renderable.prop
	NUM,
	UNKNOWN = -1,
};

// Bitset for specifying module types attached to an agent
using SceneModuleTypes = BitSet<SceneModuleType, (U32)SceneModuleType::NUM, (SceneModuleType)0>;

template<SceneModuleType> struct SceneModule;
class Scene;
struct SceneAgent;

// Renderable scene component (RenderObject_Mesh* obj). Any actual render types (eg RenderVertexState) can be filled up, but not created
// As in the actual buffers should only be in data streams on CPU side.
template<> struct SceneModule<SceneModuleType::RENDERABLE>
{
	
	static constexpr SceneModuleType ModuleType = SceneModuleType::RENDERABLE;
	
	Mesh Renderable; // the renderable mesh
	
	// Gets this module for the scene
	static inline SceneModule<SceneModuleType::RENDERABLE>& GetForScene(Scene& scene, const SceneAgent& agent);
	
};

/// An agent in the scene, or game object unity terms.
struct SceneAgent
{
	
	Symbol NameSymbol; // as a symbol
	String Name; // agent name
	
	// points into arrays inside scene.
	mutable I32 ModuleIndices[(I32)SceneModuleType::NUM];
	
	inline SceneAgent() : Name("")
	{
		for(I32 i = 0; i < (I32)SceneModuleType::NUM; i++)
			ModuleIndices[i] = -1;
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
	
	// Add a new agent
	void AddAgent(const String& Name, SceneModuleTypes modules);
	
	// Adds an agent module to the given agent
	void AddAgentModule(const String& Name, SceneModuleType module);
	
	// Removes an agent module from a given agent
	void RemoveAgentModule(const Symbol& Name, SceneModuleType module);
	
	// Returns if the given agent exists
	Bool ExistsAgent(const Symbol& Name);
	
	// Returns if the given agent has the given module
	Bool HasAgentModule(const Symbol& Name, SceneModuleType module);
	
	// Gets the given module for the given agent. This agent must have this module!
	template<SceneModuleType Type>
	inline SceneModule<Type>& GetAgentModule(const Symbol& Agent)
	{
		SceneAgent ag{};
		ag.NameSymbol = Agent;
		auto it = _Agents.find(ag);
		TTE_ASSERT(it != _Agents.end(), "The agent does not exist [GetAgentModule<>]");
		return SceneModule<Type>::GetForScene(*this, *it);
	}
	
	// Registers scene normalisers and specialisers
	static void RegisterScriptAPI(LuaFunctionCollection& Col);
	
private:
	
	// ==== RENDER CONTEXT USE ONLY. THESE FUNCTIONS ARE DEFINED IN RENDERSCENE.CPP, SEPARATE TO REST DEFINED IN COMMON/SCENE.CPP
	
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
	
	std::vector<SceneModule<SceneModuleType::RENDERABLE>> _Renderables;
	
	friend class RenderContext;
	friend struct SceneAgent;
	
	friend struct SceneModule<SceneModuleType::RENDERABLE>;
	
};

inline SceneModule<SceneModuleType::RENDERABLE>& SceneModule<SceneModuleType::RENDERABLE>::GetForScene(Scene& scene, const SceneAgent& agent)
{
	TTE_ASSERT(agent.ModuleIndices[(U32)SceneModuleType::RENDERABLE] != -1, "Agent %s does not have this module", agent.Name.c_str());
	return scene._Renderables[agent.ModuleIndices[(U32)SceneModuleType::RENDERABLE]];
}
