-- Rule class implementation for Bone1

function SerialiseRulesBone1(stream, instance, write)
    ruleMap = MetaGetMember(instance, "_mRules")
    ruleNames = MetaGetMember(instance, "_mRuleNames")
    numNames = ContainerGetNumElements(ruleNames)
    numRules = ContainerGetNumElements(ruleMap)
    TTE_Assert(numNames == numRules, "Number of names and rule maps must be equal as like its a map!") -- should not happen.

    if not MetaSerialiseDefault(stream, instance, write) then return false end

    MetaStreamBeginBlock(stream, write)

    if not MetaSerialise(stream, ruleNames, write) then return false end -- serialised directly
    numRules = ContainerGetNumElements(MetaGetMember(instance, "_mRules")) -- update if read

    for i=1,numRules do
        if write then
            if not MetaSerialise(stream, ContainerGetElement(ruleMap, i - 1)) then return false end
        else
            if not MetaSerialise(stream, ContainerEmplacelement(ruleMap)) then return false end
        end
    end

    MetaStreamEndBlock(stream, write)
end

-- .RULE
function SerialiseRuleBone1(stream, instance, write)
	agentInfo = MetaGetMember(instance, "_mAgentInfo")
	numAgents = ContainerGetNumElements(agentInfo)
	if write and numAgents > 0 then MetaSetClassValue(MetaGetMember(instance, "mbVersionHasAgents"), true) end

	if not MetaSerialiseDefault(stream, instance, write) then return false end
	if not MetaGetClassValue(MetaGetMember(instance, "mbVersionHasAgents")) then return true end -- no agents, ok

	MetaStreamBeginBlock(stream, write)

	if write then
		MetaStreamWriteInt(stream, numAgents)
	else
		numAgents = MetaStreamReadInt(stream)
	end

	for i=1,numAgents do
		inst = nil
		if write then
			inst = ContainerGetElement(agentInfo, i - 1)
		else
			inst = ContainerEmplaceElement(agentInfo)
		end
		if not MetaSerialise(stream, inst, write) then return false end
	end

	MetaStreamEndBlock(stream, write)
	return true
end