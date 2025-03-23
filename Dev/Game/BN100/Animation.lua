-- Bone100 Animation implementation

function SerialiseProceduralLookAtBone1(stream, instance, write)
    return MetaSerialise(stream, MetaGetMember(instance, "Baseclass_Animation"), write)
end

function SerialiseAnimationBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end
    MetaStreamBeginBlock(stream, write)

    if write then return false end

    local numTotalValues = MetaStreamReadInt(stream)
    local numValueTypes = MetaStreamReadInt(stream)
    local totalNumRead = 0

    for i=1,numValueTypes do
        local typeSymbol = MetaStreamReadSymbol(stream)
        numOfType = MetaStreamReadInt(stream)
        for j=1,numOfType do
            typeInst = MetaCreateInstance(typeSymbol, 0, tostring(totalNumRead + 1), instance)
            if not MetaSerialise(stream, typeInst, write) then return false end
            totalNumRead = totalNumRead + 1
        end
    end

    TTE_Assert(totalNumRead == numTotalValues, "Animation is corrupt!")

    MetaStreamEndBlock(stream, write)
    return true
end

function SerialiseCQKeysBone1(stream, instance, write)
    MetaSetClassValue(MetaGetMember(instance, "_mName"), MetaStreamReadString(stream))

    if write then return false end

    a = MetaStreamReadInt(stream)
    b = MetaStreamReadInt(stream)
    
    MetaSetClassValue(MetaGetMember(instance, "_mLength"), MetaStreamReadFloat(stream))

    numSamples = MetaStreamReadShort(stream)

    TTE_Assert(a == 0 and b == 0, "CompressedQuaternionKeys: unknown header")

    MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mBuffer"), numSamples * 6)

    return true
end

function SerialiseCVKeysBone1(stream, instance, write)
    name = MetaStreamReadString(stream)
    MetaSetClassValue(MetaGetMember(instance, "_mName"), name)

    if write then return false end

    a = MetaStreamReadInt(stream)

    MetaSetClassValue(MetaGetMember(instance, "_mTypeUnk"), a) -- 512/32/0 idk
    MetaSetClassValue(MetaGetMember(instance, "_mV1X"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mV1Y"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mV1Z"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mV2X"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mV2Y"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mV2Z"), MetaStreamReadFloat(stream))

    idk = MetaStreamReadFloat(stream)
    
    MetaSetClassValue(MetaGetMember(instance, "_mLength"), MetaStreamReadFloat(stream))

    numSamples = MetaStreamReadShort(stream)

    TTE_Assert(idk == 0, "CompressedVector3Keys: unknown header")

    MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mBuffer"), numSamples * 6)

    return true
end
