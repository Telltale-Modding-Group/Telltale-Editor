#include <Common/Procedural.hpp>
#include <Common/Chore.hpp>
#include <AnimationManager.hpp>

#define PROCEDURAL_SERIAL 0x49813fd

class ProceduralAPI
{
public:

    static Procedural_LookAt* State(LuaManager& man)
    {
        return (Procedural_LookAt*)man.ToPointer(1);
    }
    
    static U32 luaProcSetTargets(LuaManager& man)
    {
        Procedural_LookAt* look = State(man);
        look->_HostNode = man.ToString(2);
        look->_TargetAgent = man.ToString(3);
        look->_TargetNode = man.ToString(4);
        Meta::ExtractCoercableInstance<Vector3>(look->_TargetOffset, Meta::AcquireScriptInstance(man, 5));
        return 0;
    }
    
};

// ======= LOOK AT

void Procedural_LookAt::RegisterScriptAPI(LuaFunctionCollection& Col)
{
    PUSH_FUNC(Col, "CommonProceduralSetTargets", &ProceduralAPI::luaProcSetTargets, "nil CommonProceduralSetTargets(state, host, target, node, offset)",
        "Sets target information for the look at. Host is the host node string, target agent, target node are all strings too. Offset is a Vector3 instance of the offset from the node.");
}

void Procedural_LookAt::FinaliseNormalisationAsync()
{
    
}

void Procedural_LookAt::GetRenderParameters(Vector3& bgColourOut, CString& iconName) const
{
    bgColourOut = VECTOR3_COL(255, 247, 25);
    iconName = "Chore/Animation.png";
}

void Procedural_LookAt::SetHostNode(String node)
{
    _HostNode = node;
}

void Procedural_LookAt::SetTargetNode(String node)
{
    _TargetNode = node;
}

void Procedural_LookAt::SetTargetAgent(String agent)
{
    _TargetAgent = agent;
}

void Procedural_LookAt::SetTargetOffset(Vector3 offset)
{
    _TargetOffset = offset;
}

Procedural_LookAt::Procedural_LookAt(const Procedural_LookAt& rhs) : HandleableRegistered(rhs)
{

}
Procedural_LookAt::Procedural_LookAt(Procedural_LookAt&& rhs) : HandleableRegistered(rhs)
{
    if (_LookAtProperties)
        PropertySet::ClearCallbacks(_LookAtProperties, PROCEDURAL_SERIAL);
}

void Procedural_LookAt::Attach(const Ptr<Chore>& pChore, ChoreResource& resource)
{
    if(_LookAtProperties != resource.Properties)
    {
        if (_LookAtProperties)
            PropertySet::ClearCallbacks(_LookAtProperties, PROCEDURAL_SERIAL);
        _LookAtProperties = resource.Properties;
        PropertySet::ClearCallbacks(resource.Properties, PROCEDURAL_SERIAL);
        PropertySet::AddMethodCallbackT(resource.Properties, kProceduralLookAtHostAgentNode, GetReference(), &Procedural_LookAt::SetHostNode, PROCEDURAL_SERIAL);
        PropertySet::AddMethodCallbackT(resource.Properties, kProceduralLookAtTargetAgent, GetReference(), &Procedural_LookAt::SetTargetAgent, PROCEDURAL_SERIAL);
        PropertySet::AddMethodCallbackT(resource.Properties, kProceduralLookAtTargetAgentNode, GetReference(), &Procedural_LookAt::SetTargetNode, PROCEDURAL_SERIAL);
        PropertySet::AddMethodCallbackT(resource.Properties, kProceduralLookAtTargetAgentNodeOffset, GetReference(), &Procedural_LookAt::SetTargetOffset, PROCEDURAL_SERIAL);
    }

}

void Procedural_LookAt::AddToChore(const Ptr<Chore>& pChore, ChoreResource& resource)
{
    // no time
    if(!resource.ControlAnimation->HasValue("contribution"))
    {
        resource.ControlAnimation->GetAnimatedValues().push_back(TTE_NEW_PTR(KeyframedValue<Float>, MEMORY_TAG_ANIMATION_DATA, "contribution"));
    }
}

Float Procedural_LookAt::GetLength() const
{
    return 1.0f; // WHY?
}
