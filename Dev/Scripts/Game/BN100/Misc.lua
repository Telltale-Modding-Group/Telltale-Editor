-- Other implementations of bone classes

function NormaliseInputMapperBone1(state, instance)
    
    CommonInputMapperSetName(state, MetaGetClassValue(MetaGetMember(instance, "mName")))

    local mappings = MetaGetMember(instance, "mMappedEvents")
    local numMappings = ContainerGetNumElements(mappings)

    for i=1,numMappings do
        local mapping = ContainerGetElement(mappings, i - 1)
        CommonInputMapperPushMapping(state, MetaGetClassValue(MetaGetMember(MetaGetMember(mapping, "mInputCode"), "mVal")),
                                            MetaGetClassValue(MetaGetMember(MetaGetMember(mapping, "mEventType"), "mVal")),
                                            MetaGetClassValue(MetaGetMember(mapping, "mScriptFunction")), -1)
    end

    return true
end