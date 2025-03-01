
-- Scene serialisation for Bone: OFB

function SerialiseScene_Bone1(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    MetaStreamBeginBlock(stream, write)
    if not write then
        local numAgents = MetaStreamReadInt(stream)
        local agentArray = MetaGetMember(inst, "_mAgents")
        local agentClass, agentClassVer = MetaStreamFindClass(stream, "class Scene::AgentInfo")
        for i=1,numAgents do
            local agentInfo = ContainerEmplaceElement(agentArray)
            if not MetaSerialise(stream, agentInfo, write) then
                return false
            end
            local nameInst = MetaGetClassValue(MetaGetMember(agentInfo, "mAgentName"))
            TTE_Log("Found agent " .. tostring(nameInst))
        end
    end
    MetaStreamEndBlock(stream, write)
    return true
end

function NormaliseScene_Bone1(boneScene, normalised)
    print("Gonna have to do this soon")
end