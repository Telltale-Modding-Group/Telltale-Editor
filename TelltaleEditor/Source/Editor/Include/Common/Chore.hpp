#pragma once

#include <Core/Config.hpp>
#include <Core/BitSet.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Meta/Meta.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Common/Animation.hpp>
#include <Common/MetaOperations.hpp>

#include <map>

template<typename T>
class UIResourceEditor;

/**
 Choregraphy file
 */
class Chore : public HandleableRegistered<Chore>, public MetaOperationsBucket_ChoreResource
{
public:
    
    struct Resource
    {
        
        enum Flag
        {
            NO_POSE = 1,
            EMBEDDED = 2,
            ENABLED = 4,
            AGENT_RESOURCE = 8,
            VIEW_GRAPHS = 16, // view graphs is open in UI (this is persistent in the file, not sure why..., same for other view flags)
            VIEW_PROPERTIES = 32, // view properties is open in UI
            VIEW_GROUPS = 64, // view resource groups is open in UI
        };
        
        struct Block
        {
            Float Start = 0.0f, End = 0.0f, Scale = 1.0f;
            U32 Looping = 0; // 1 = true
        };
        
        String Name;
        Float Length;
        I32 Priority;
        Ptr<Animation> ControlAnimation; // controls 'time' and 'contribution' keyframed<float>'s for this chore (see in view graph)
        std::vector<Block> Blocks; // instances of this resource
        Flags ResFlags; // see Flag enum
        Meta::ClassInstance Properties; // resource props
        
        Ptr<MetaOperationsBucket_ChoreResource> Embed; // When embed = true, the embedded resource is here and not an external resource.
        
        // std::map<String, Float> ResourceGroupInclusion; 
        
        inline Resource(const String& N) : Name(N) {}
        
    };
    
    // Chore agent. One of the agents in the scene which this chore will choreograph.
    struct Agent
    {
        
        String Name; // Scene agent name
        Meta::ClassInstance Properties; // Properties
        std::vector<I32> Resources; // Resource indices
        // Transform Trans;
        
        inline Agent(const String& N) : Name(N) {}
        
    };
    
    static constexpr CString ClassHandle = "Handle<Chore>";
    static constexpr CString Class = "Chore";
    static constexpr CString Extension = "chore";
    
    virtual void FinaliseNormalisationAsync() override;

    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::CHORE;
    }
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    Chore(Ptr<ResourceRegistry>);
    
    Chore(Chore&& rhs) = default;
    Chore(const Chore& rhs);
    
    // adds a resource
    Resource& EmplaceResource(const String& resourceName);
    
    Agent& EmplaceAgent(const String& agentName);
    
    void RemoveResource(const String& name);
    
    void RemoveAgent(const String& agentName);
    
    inline const std::vector<Agent>& GetAgents() const
    {
        return _Agents;
    }
    
    inline const std::vector<Resource>& GetResources() const
    {
        return _Resources;
    }

    inline const Resource* GetResource(Symbol name) const
    {
        for(const auto& res: _Resources)
        {
            if(name == res.Name)
            {
                return &res;
            }
        }
        return nullptr;
    }
    
    inline const Agent* GetAgent(Symbol name) const
    {
        for(const auto& ag: _Agents)
        {
            if(name == ag.Name)
            {
                return &ag;
            }
        }
        return nullptr;
    }
    
    inline virtual Float GetLength() const override
    {
        return _Length;
    }

    virtual void GetRenderParameters(Vector3& bgColOut, CString& iconName) const override;
    
    virtual void AddToChore(const Ptr<Chore>& pChore, String myName) override;
    
private:
    
    String _Name;
    Float _Length;
    std::vector<Agent> _Agents;
    std::vector<Resource> _Resources;
    
    friend class ChoreAPI;
    friend class UIResourceEditor<Chore>;
    
};
