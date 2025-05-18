-- Chore implementation for Bone

function SerialiseChoreBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end
    if write then return false end

    local numResources = MetaGetClassValue(MetaGetMember(instance, "mNumResources"))
    local numAgents = MetaGetClassValue(MetaGetMember(instance, "mNumAgents"))

    local arrayResources = MetaGetMember(instance, "_mResources")
    local arrayAgents = MetaGetMember(instance, "_mAgents")

    for i=0,numResources-1 do
        local resource = ContainerEmplaceElement(arrayResources)
        if not MetaSerialise(stream, resource, write) then return false end
    end

    for i=0,numAgents-1 do
        local agent = ContainerEmplaceElement(arrayAgents)
        if not MetaSerialise(stream, agent, write) then return false end
    end

    return true
end

function SerialiseChoreResourceBone1(stream, instance, write)
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