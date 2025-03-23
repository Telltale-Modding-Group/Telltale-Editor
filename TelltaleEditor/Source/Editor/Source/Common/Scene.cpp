#include <Common/Scene.hpp>
#include <Renderer/RenderContext.hpp>

void Scene::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    
}

void Scene::AddAgentModule(const String& Name, SceneModuleType module)
{
    Symbol sym{Name};
    for(auto& agent: _Agents)
    {
        if(agent.NameSymbol == sym)
        {
            if(agent.ModuleIndices[(U32)module] == -1)
            {
                I32 index;
                if(module == SceneModuleType::RENDERABLE)
                {
                    index = (I32)_Renderables.size();
                    _Renderables.emplace_back();
                }
                else
                {
                    TTE_ASSERT(false, "Invalid module type");
                    return;
                }
                agent.ModuleIndices[(U32)module] = index;
            }
            break;
        }
    }
}

void Scene::RemoveAgentModule(const Symbol& sym, SceneModuleType module)
{
    U32 moduleIndexToRemove = (U32)-1;
    
    // First pass: Find the agent and remove the module
    for (auto it = _Agents.begin(); it != _Agents.end(); it++)
    {
        const SceneAgent& agent = *it;
        if (agent.NameSymbol == sym)
        {
            if (agent.ModuleIndices[(U32)module] != -1)
            {
                moduleIndexToRemove = agent.ModuleIndices[(U32)module];
                
                // Remove the module from the current agent
                if (module == SceneModuleType::RENDERABLE)
                {
                    auto it0 = _Renderables.begin();
                    std::advance(it0, moduleIndexToRemove);
                    _Renderables.erase(it0);
                }
                else
                {
                    TTE_ASSERT(false, "Invalid module type");
                    return;
                }
                
                // Clear the module index for the current agent (or mark it as invalid)
                agent.ModuleIndices[(U32)module] = -1;
            }
            break;
        }
    }
    
    if (moduleIndexToRemove != (U32)-1)
    {
        for (auto& agent : _Agents)
        {
            if (agent.ModuleIndices[(U32)module] > moduleIndexToRemove)
            {
                agent.ModuleIndices[(U32)module] = agent.ModuleIndices[(U32)module] - 1;
            }
        }
    }
}


Bool Scene::ExistsAgent(const Symbol& sym)
{
    for(auto& agent: _Agents)
        if(agent.NameSymbol == sym)
            return true;
    return false;
}

Bool Scene::HasAgentModule(const Symbol& sym, SceneModuleType module)
{
    for(auto& agent: _Agents)
        if(agent.NameSymbol == sym)
            return agent.ModuleIndices[(U32)module] != -1;
    return false;
}

void Scene::AddAgent(const String& Name, SceneModuleTypes modules)
{
    TTE_ASSERT(!ExistsAgent(Name), "Agent %s already exists in %s", Name.c_str(), Name.c_str());
    _Agents.insert(SceneAgent{Name});
    for(auto it = modules.begin(); it != modules.end(); it++)
    {
        AddAgentModule(Name, *it);
    }
}
