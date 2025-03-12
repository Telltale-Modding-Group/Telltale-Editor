#include <Core/Context.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Meta/Meta.hpp>

thread_local Bool _IsMain = false; // Used in meta.

Bool IsCallingFromMain()
{
	return _IsMain;
}

// ===================================================================         TOOL CONTEXT
// ===================================================================

static ToolContext* GlobalContext = nullptr;

ToolContext* CreateToolContext(LuaFunctionCollection API)
{
	
	if(GlobalContext == nullptr){
		_IsMain = true; // CreateToolContext sets this current thread as the main thread
		GlobalContext = (ToolContext*)TTE_ALLOC(sizeof(ToolContext), MEMORY_TAG_TOOL_CONTEXT); // alloc raw and construct, as its private.
		new (GlobalContext) ToolContext(std::move(API)); // construct after as some asserts require
	}else{
		TTE_ASSERT(false, "Trying to create more than one ToolContext. Only one instance is allowed per process.");
	}
	
	return GlobalContext;
}

ToolContext* GetToolContext()
{
	return GlobalContext;
}

void DestroyToolContext()
{
	TTE_ASSERT(_IsMain, "Must only be called from main thread");
	
	if(GlobalContext)
		TTE_DEL(GlobalContext);
	
	GlobalContext = nullptr;
	
}

LuaManager& ToolContext::GetGameLVM()
{
	TTE_ASSERT(GetActiveGame(), "No active game!");
	return _L[(U32)Meta::GetInternalState().GetActiveGame().LVersion];
}

Ptr<ResourceRegistry> ToolContext::CreateResourceRegistry()
{
	if(GetActiveGame() == nullptr)
	{
		TTE_LOG("Cannot create a resource registry as there is no current game set.");
		return nullptr;
	}
	
	Ptr<ResourceRegistry> registry = TTE_NEW_PTR(ResourceRegistry, MEMORY_TAG_RESOURCE_REGISTRY, GetGameLVM());
	
	Ptr<GameDependentObject> asDependent = std::static_pointer_cast<GameDependentObject>(registry);
	{
		std::lock_guard<std::mutex> G{_DependentsLock};
		_SwitchDependents.push_back(asDependent);
	}
	
	return registry;
}
