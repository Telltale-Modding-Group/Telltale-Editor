#include <Common/Chore.hpp>
#include <TelltaleEditor.hpp>

class ChoreAPI
{
public:
    
    static Chore* Task(LuaManager& man)
    {
        return (Chore*)man.ToPointer(1);
    }
    
    static U32 luaSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of set name");
        Chore* c = Task(man);
        c->_Name = man.ToString(2);
        return 0;
    }
    
    static U32 luaSetLength(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of set length");
        Chore* c = Task(man);
        c->_Length = man.ToFloat(2);
        return 0;
    }
    
    static U32 luaPushResource(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of push resource");
        Chore* c = Task(man);
        man.PushLString("Name");
        man.GetTable(2);
        Chore::Resource& resource = c->EmplaceResource(ScriptManager::PopString(man));
        ITERATE_TABLE(it, 2)
        {
            String key = man.ToString(it.KeyIndex());
            if(key == "Priority")
            {
                resource.Priority = man.ToInteger(it.ValueIndex());
            }
            if(key == "Length")
            {
                resource.Length = man.ToFloat(it.ValueIndex());
            }
            else if(key == "ControlAnim")
            {
                Meta::ClassInstance anim = Meta::AcquireScriptInstance(man, it.ValueIndex());
                I32 top = man.GetTop();
                if(!resource.ControlAnimation)
                {
                    resource.ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, c->GetRegistry());
                }
                if(!InstanceTransformation::PerformNormaliseAsync(resource.ControlAnimation, anim, man))
                {
                    resource.ControlAnimation = nullptr;
                }
                man.SetTop(top);
            }
            else if(key == "Properties")
            {
                Meta::ClassInstance props = Meta::AcquireScriptInstance(man, it.ValueIndex());
                resource.Properties = Meta::CopyInstance(props);
            }
            else if(key == "NoPose")
            {
                resource.ResFlags.Add(Chore::Resource::NO_POSE);
            }
            else if(key == "Enabled")
            {
                resource.ResFlags.Add(Chore::Resource::ENABLED);
            }
            else if(key == "AgentResource")
            {
                resource.ResFlags.Add(Chore::Resource::AGENT_RESOURCE);
            }
            else if(key == "ViewGraphs")
            {
                resource.ResFlags.Add(Chore::Resource::VIEW_GRAPHS);
            }
            else if(key == "ViewProps")
            {
                resource.ResFlags.Add(Chore::Resource::VIEW_PROPERTIES);
            }
            else if(key == "ViewInclude")
            {
                resource.ResFlags.Add(Chore::Resource::VIEW_GROUPS);
            }
            else if(key == "Embed")
            {
                Meta::ClassInstance embed = Meta::AcquireScriptInstance(man, it.ValueIndex());
                if(embed)
                {
                    const Meta::Class& cls = Meta::GetClass(embed.GetClassID());
                    if(cls.Extension.empty())
                    {
                        TTE_LOG("ERROR: Normalisation for chore resource: class '%s' has is not a common class with extension yet", cls.Name.c_str());
                    }
                    else
                    {
                        Ptr<Handleable> pEmbedResource = CommonClassInfo::GetCommonClassInfoByExtension(cls.Extension).ClassAllocator(c->GetRegistry());
                        if(pEmbedResource)
                        {
                            Ptr<MetaOperationsBucket_ChoreResource> pResource = AbstractMetaOperationsBucket::CreateBucketReference<MetaOperationsBucket_ChoreResource>(pEmbedResource).lock();
                            if(pResource)
                            {
                                InstanceTransformation::PerformNormaliseAsync(pEmbedResource, std::move(embed), man);
                                resource.Embed = std::move(pResource);
                                resource.ResFlags.Add(Chore::Resource::EMBEDDED);
                            }
                            else
                            {
                                TTE_LOG("ERROR: Normalisation for chore resource: embedded resource common class is not a chore "
                                        "resource operations bucket '%s'", Meta::GetClass(embed.GetClassID()).Name.c_str());
                            }
                        }
                        else
                        {
                            TTE_LOG("ERROR: Normalisation for chore resource: embedded resource common class for found or suppored yet '%s'",
                                    Meta::GetClass(embed.GetClassID()).Name.c_str());
                        }
                    }
                }
                else
                {
                    TTE_LOG("ERROR: Normalisation for chore resource: embedded resource is null");
                }
            }
            else if(key == "Blocks")
            {
                ITERATE_TABLE(btit, it.ValueIndex())
                {
                    Chore::Resource::Block& block = resource.Blocks.emplace_back();
                    ITERATE_TABLE(bit, btit.ValueIndex())
                    {
                        String k = man.ToString(bit.KeyIndex());
                        if(k == "Start")
                        {
                            block.Start = man.ToFloat(bit.ValueIndex());
                        }
                        else if(k == "End")
                        {
                            block.End = man.ToFloat(bit.ValueIndex());
                        }
                        else if(k == "Scale")
                        {
                            block.Scale = man.ToFloat(bit.ValueIndex());
                        }
                        else if(k == "Loop")
                        {
                            block.Looping = man.ToBool(bit.ValueIndex()) ? 1 : 0;
                        }
                    }
                }
            }
        }
        return 0;
    }
    
    static U32 luaPushAgent(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of push agent");
        Chore* c = Task(man);
        man.PushLString("Name");
        man.GetTable(2);
        Chore::Agent& agent = c->EmplaceAgent(ScriptManager::PopString(man));
        ITERATE_TABLE(it, 2)
        {
            String key = man.ToString(it.KeyIndex());
            if(key == "Properties")
            {
                Meta::ClassInstance props = Meta::AcquireScriptInstance(man, it.ValueIndex());
                agent.Properties = Meta::CopyInstance(props);
            }
            else if(key == "ResourceIndices")
            {
                if(man.Type(it.ValueIndex()) == LuaType::TABLE)
                {
                    ITERATE_TABLE(rit, it.ValueIndex())
                    {
                        I32 index = man.ToInteger(rit.ValueIndex());
                        if(index >= c->GetResources().size())
                        {
                            TTE_LOG("ERROR: Chore normaliser tries to add agent %s but chore resource index %d does not exist. "
                                    "Ensure resources are pushed first in the normaliser, before agents.", agent.Name.c_str(), index);
                        }
                        else
                        {
                            agent.Resources.push_back(index);
                        }
                    }
                }
                else
                {
                    Meta::ClassInstance colinst = Meta::AcquireScriptInstance(man, it.ValueIndex());
                    if(colinst && Meta::IsCollection(colinst))
                    {
                        Meta::ClassInstanceCollection& col = Meta::CastToCollection(colinst);
                        for(I32 i = 0; i < col.GetSize(); i++)
                        {
                            I32 index = COERCE(col.GetValue(i)._GetInternal(), I32);
                            if(index >= c->GetResources().size())
                            {
                                TTE_LOG("ERROR: Chore normaliser tries to add agent %s but chore resource index %d does not exist. "
                                        "Ensure resources are pushed first in the normaliser, before agents.", agent.Name.c_str(), index);
                            }
                            else
                            {
                                agent.Resources.push_back(index);
                            }
                        }
                    }
                    else
                    {
                        TTE_LOG("At chore normalise push agent: resource indices is not a meta instance collection");
                    }
                }
            }
        }
        return 0;
    }
    
};

void Chore::GetRenderParameters(Vector3& bgColOut, CString& iconName) const
{
    bgColOut = Vector3(217.0f / 255.0f, 203.0f / 255.0f, 124.0f / 255.0f);
    // icon?
}

void Chore::AddToChore(const Ptr<Chore>& pChore, String myName)
{
    ; // TODO EMBED CHORES
}

void Chore::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    PUSH_FUNC(Col, "CommonChoreSetName", &ChoreAPI::luaSetName, "nil CommonChoreSetName(state, name)", "Sets the common chore name");
    PUSH_FUNC(Col, "CommonChoreSetLength", &ChoreAPI::luaSetLength, "nil CommonChoreSetLength(state, lengthSecs)", "Sets the common chore length in seconds");
    PUSH_FUNC(Col, "CommonChorePushResource", &ChoreAPI::luaPushResource, "nil CommonChorePushResource(state, resTable)",
              "Pushes a resource to the chore. The second argument is a table containing all keys matching kCommonChoreResourceKeyXXX.");
    PUSH_FUNC(Col, "CommonChorePushAgent", &ChoreAPI::luaPushAgent, "nil CommonChorePushAgent(state, agenetTable)",
              "Pushes a choreographed agent to the chore. The second argument is a table containing all keys matching kCommonChoreAgentKeyXXX."
              " You MUST push all resources before pushing any agents, otherwise their resource indices won't register.");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyPriority", "Priority", "The chore resource priority");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyName", "Name", "The chore resource name");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyLength", "Length", "The chore resource length in seconds");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyControlAnimation", "ControlAnim", "Control animation meta instance");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyProperties", "Properties", "Property set for this resource");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyNoPose", "NoPose", "No pose boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyEnabled", "Enabled", "Enabled boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyEmbed", "Embed", "Meta instance for the embedded resource, if there is one (else external).");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyAgentResource", "AgentResource", "Is agent resource boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyViewGraphs", "ViewGraphs", "View graphs on boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyViewGroups", "ViewGroups", "View groups on boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyViewProperties", "ViewProperties", "View properties boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceKeyBlocks", "Blocks", "Table of blocks. See kCommonChoreResourceBlockKeyXXX constants");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceBlockKeyStart", "Start", "Start time of block");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceBlockKeyEnd", "End", "End time of block");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceBlockKeyScale", "Scale", "Time scale of block");
    PUSH_GLOBAL_S(Col, "kCommonChoreResourceBlockKeyLooping", "Looping", "If block is looping boolean");
    PUSH_GLOBAL_S(Col, "kCommonChoreAgentKeyName", "Name", "Agent name string");
    PUSH_GLOBAL_S(Col, "kCommonChoreAgentKeyProperties", "Properties", "Chore agent properties property set meta instance");
    PUSH_GLOBAL_S(Col, "kCommonChoreAgentKeyResourceIndices", "ResourceIndices",
                  "Table of indices (values, keys ignored) or meta collection of 32-bit integers.");
}

void Chore::FinaliseNormalisationAsync()
{
    
}

Chore::Chore(const Chore& rhs) : HandleableRegistered<Chore>(rhs)
{
    _Agents = rhs._Agents;
    _Resources = rhs._Resources;
    for(I32 i = 0; i < _Agents.size(); i++)
    {
        _Agents[i].Properties = _Agents[i].Properties ? Meta::CopyInstance(_Agents[i].Properties) : Meta::ClassInstance{};
    }
    for(I32 i = 0; i < _Resources.size(); i++)
    {
        _Resources[i].Properties = _Resources[i].Properties ? Meta::CopyInstance(_Resources[i].Properties) : Meta::ClassInstance{};
        if(_Resources[i].ControlAnimation)
        {
            const Animation& anim = *_Resources[i].ControlAnimation;
            _Resources[i].ControlAnimation = TTE_NEW_PTR(Animation, MEMORY_TAG_COMMON_INSTANCE, anim);
        }
    }
}

Chore::Chore(Ptr<ResourceRegistry> reg) : HandleableRegistered<Chore>(reg)
{
    
}

Chore::Resource& Chore::EmplaceResource(const String& name)
{
    return _Resources.emplace_back(name);
}

Chore::Agent& Chore::EmplaceAgent(const String& name)
{
    return _Agents.emplace_back(name);
}

void Chore::RemoveAgent(const String &agentName)
{
    for(auto it = _Agents.begin(); it != _Agents.end();)
    {
        if(it->Name == agentName)
        {
            it = _Agents.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void Chore::RemoveResource(const String &name)
{
    auto it = std::find_if(_Resources.begin(), _Resources.end(), [&](const Resource &r) { return r.Name == name; });

    if (it != _Resources.end())
    {
        I32 removedIndex = (I32)std::distance(_Resources.begin(), it);
        _Resources.erase(it);
        for (auto &agent : _Agents)
        {
            for (auto rit = agent.Resources.begin(); rit != agent.Resources.end();)
            {
                if (*rit == removedIndex)
                {
                    rit = agent.Resources.erase(rit);
                }
                else
                {
                    if ((*rit) > static_cast<int>(removedIndex))
                    {
                        --(*rit);
                    }
                    rit++;
                }
            }
        }
    }
}
