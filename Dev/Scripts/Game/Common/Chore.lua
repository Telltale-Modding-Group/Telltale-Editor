-- Chore implementation

function RegisterChoreResourceBlock()
	local choreBlock = NewClass("class ChoreResource::Block", 0)
	choreBlock.Members[1] = NewMember("mStartTime", kMetaFloat)
	choreBlock.Members[2] = NewMember("mEndTime", kMetaFloat)
	choreBlock.Members[3] = NewMember("mbLoopingBlock", kMetaBool)
	choreBlock.Members[4] = NewMember("mScale", kMetaFloat)
	MetaRegisterClass(choreBlock)
    return choreBlock
end

-- Oldest version (TX etc)
function NormaliseChore0(instance, state)
    
    CommonChoreSetName(state, MetaGetClassValue(MetaGetMember(instance, "mName")))
    CommonChoreSetLength(state, MetaGetClassValue(MetaGetMember(instance, "mLength")))
    local arrayResources = MetaGetMember(instance, "_mResources")
    local arrayAgents = MetaGetMember(instance, "_mAgents")

    -- RESOURCES
    for i = 0, ContainerGetNumElements(arrayResources) - 1 do
        local resource = ContainerGetElement(arrayResources, i)
        local resTab = {}
        resTab[kCommonChoreResourceKeyName] = MetaGetClassValue(MetaGetMember(resource, "mResName"))
        resTab[kCommonChoreResourceKeyLength] = MetaGetClassValue(MetaGetMember(resource, "mResLength"))
        resTab[kCommonChoreResourceKeyPriority] = MetaGetClassValue(MetaGetMember(resource, "mPriority"))
        resTab[kCommonChoreResourceKeyControlAnimation] = MetaGetMember(resource, "mControlAnimation")
        resTab[kCommonChoreResourceKeyProperties] = MetaGetMember(resource, "mResourceProperties")
        resTab[kCommonChoreResourceKeyNoPose] = MetaGetClassValue(MetaGetMember(resource, "mbNoPose"))
        resTab[kCommonChoreResourceKeyEnabled] = MetaGetClassValue(MetaGetMember(resource, "mbEnabled"))
        resTab[kCommonChoreResourceKeyAgentResource] = MetaGetClassValue(MetaGetMember(resource, "mbIsAgentResource"))
        resTab[kCommonChoreResourceKeyViewGraphs] = MetaGetClassValue(MetaGetMember(resource, "mbViewGraphs"))
        resTab[kCommonChoreResourceKeyViewGroups] = MetaGetClassValue(MetaGetMember(resource, "mbViewResourceGroups"))
        resTab[kCommonChoreResourceKeyViewProperties] = MetaGetClassValue(MetaGetMember(resource, "mbViewProperties"))
        resTab[kCommonChoreResourceKeyNoPose] = MetaGetClassValue(MetaGetMember(resource, "mbNoPose"))
        resTab[kCommonChoreResourceKeyBlocks] = {}
        local blocksInst = MetaGetMember(resource, "mBlocks")
        for j = 0, ContainerGetNumElements(blocksInst) - 1 do
            local block = {}
            local blockInstance = ContainerGetElement(blocksInst, j)
            block[kCommonChoreResourceBlockKeyStart] = MetaGetClassValue(MetaGetMember(blockInstance, "mStartTime"))
            block[kCommonChoreResourceBlockKeyEnd] = MetaGetClassValue(MetaGetMember(blockInstance, "mEndTime"))
            block[kCommonChoreResourceBlockKeyScale] = MetaGetClassValue(MetaGetMember(blockInstance, "mScale"))
            block[kCommonChoreResourceBlockKeyLooping] = MetaGetClassValue(MetaGetMember(blockInstance, "mbLoopingBlock"))
            resTab[kCommonChoreResourceKeyBlocks][j] = block
        end
        CommonChorePushResource(state, resTab)
    end

    -- AGENTS
    for i = 0, ContainerGetNumElements(arrayAgents) - 1 do
        local agent = ContainerGetElement(arrayAgents, i)
        local agentTab = {}
        agentTab[kCommonChoreAgentKeyName] = MetaGetClassValue(MetaGetMember(agent, "mAgentName"))
        agentTab[kCommonChoreAgentKeyProperties] = MetaGetMember(agent, "mChoreProps")
        agentTab[kCommonChoreAgentKeyResourceIndices] = MetaGetMember(agent, "mResources")
        CommonChorePushAgent(state, agentTab)
    end

    return true
end

function SerialiseChore0(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end
    if write then return false end

    local numResources = MetaGetClassValue(MetaGetMember(instance, "mNumResources"))
    local numAgents = MetaGetClassValue(MetaGetMember(instance, "mNumAgents"))

    local arrayResources = MetaGetMember(instance, "_mResources")
    local arrayAgents = MetaGetMember(instance, "_mAgents")

    for i = 0, numResources - 1 do
        local resource = ContainerEmplaceElement(arrayResources)
        if not MetaSerialise(stream, resource, write) then return false end
    end

    for i = 0, numAgents - 1 do
        local agent = ContainerEmplaceElement(arrayAgents)
        if not MetaSerialise(stream, agent, write) then return false end
    end

    return true
end

function SerialiseChoreResource0(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end
    if write then return false end

    if MetaGetClassValue(MetaGetMember(instance, "mbEmbedded")) then
        local embeddedTypeName = MetaStreamReadSymbol(stream)
        MetaStreamReadSymbol(stream) -- skip. make sure in write mode to just write 'class Animation', ie any class (exists, it checks). not serialises. stupid ik.
        local embeddedType = MetaCreateInstance(embeddedTypeName, 0, "Embedded", instance)
        if not embeddedType then
            TTE_Log("Could not find types in embedded chore resource: " .. tostring(embeddedTypeName))
            return false
        end
        if not MetaSerialise(stream, embeddedType, write) then return false end
    end

    return true
end
