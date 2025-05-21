
-- Scene serialisation for Bone: OFB

function SerialiseScene0(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    MetaStreamBeginBlock(stream, write)
    if not write then
        local numAgents = MetaStreamReadInt(stream)
        local agentArray = MetaGetMember(inst, "_mAgents")
        for i=1,numAgents do
            local agentInfo = ContainerEmplaceElement(agentArray)
            if not MetaSerialise(stream, agentInfo, write) then
                return false
            end
        end
    end
    MetaStreamEndBlock(stream, write)
    return true
end

-- Bone100 has this imported thing.
function NormaliseScene_Bone1(inst, boneScene)
    CommonSceneSetName(boneScene, MetaGetClassValue(MetaGetMember(inst, "mName")))
    CommonSceneSetHidden(boneScene, MetaGetClassValue(MetaGetMember(inst, "mbHidden")))
    local agentArray = MetaGetMember(inst, "_mAgents")
    local numAgents = ContainerGetNumElements(agentArray)
    for i=1,numAgents do
        local agentInfo = ContainerGetElement(agentArray, i-1)
        local agentName = MetaGetClassValue(MetaGetMember(agentInfo, "mAgentName"))
        local agentProps = MetaGetMember(agentInfo, "mAgentSceneProps")
        -- TODO non prop key stuff. (mbMembersimported too??)
        print("imported?: ", MetaGetClassValue(MetaGetMember(agentInfo, "mbMembersImportedIntoSceneProps")))
        CommonScenePushAgent(boneScene, agentName, agentProps)
    end
    return true
end