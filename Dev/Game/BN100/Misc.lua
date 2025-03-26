-- Other implementations of bone classes

function NormaliseInputMapperBone1(state, instance)
    
    CommonInputMapperSetName(state, MetaGetClassValue(MetaGetMember(instance, "mName")))

    mappings = MetaGetMember(instance, "mMappedEvents")
    numMappings = ContainerGetNumElements(mappings)

    for i=1,numMappings do
        mapping = ContainerGetElement(mappings, i - 1)
        CommonInputMapperPushMapping(state, MetaGetClassValue(MetaGetMember(MetaGetMember(mapping, "mInputCode"), "mVal")))
    end

    return true
end