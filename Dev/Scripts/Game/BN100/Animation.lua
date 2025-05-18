-- Bone100 Animation implementation

-- NORMALISERS

function NormaliseAnimationBone1(instance, state)

    local dbgname = MetaGetClassValue(MetaGetMember(instance, "mName"))
    CommonAnimationSetName(state, dbgname)
    CommonAnimationSetLength(state, MetaGetClassValue(MetaGetMember(instance, "mLength")))

    local function FlagsToAnimType(flags) 
        local animType = kAnimationValueTypeSkeletonPose 
        -- flags is 0 in idle anims and others. CHECK THIS! (additive??) obj_mailboxPossum_sk11_action_discoveredMailbox.anm
        if MetaFlagQuery(flags, 32) then
            animType = kAnimationValueTypeProperty -- only seen Render Axis Scale animated in this game
        elseif MetaFlagQuery(flags, 16) then
            animType = kAnimationValueTypeMover
        else 
            if MetaFlagQuery(flags, 512) then
                flags = flags - 512 -- homogeneous flag
            end
            TTE_Assert(flags == 0, "Animation value flags for BN100 " .. tostring(flags) .. " not supported")
        end -- check this!
        return animType
    end

    local animationValueChildren = MetaGetChildrenNames(instance)
    for _, ihash in pairs(animationValueChildren) do
        local interface = MetaGetChild(instance, ihash)
        local clazz, clazzVersion = MetaGetClass(interface)
        if clazz == "class CompressedVector3Keys" then
            local flags = MetaGetClassValue(MetaGetMember(interface, "_mFlags"))
            local minTime = MetaGetClassValue(MetaGetMember(interface, "_mMinTime"))
            local maxTime = MetaGetClassValue(MetaGetMember(interface, "_mMaxTime"))
            local samples = MetaGetMember(interface, "_mBuffer")
            local name = MetaGetClassValue(MetaGetMember(interface, "_mName"))
            local extentMinX = MetaGetClassValue(MetaGetMember(interface, "_mV1X"))
            local extentMinY = MetaGetClassValue(MetaGetMember(interface, "_mV1Y"))
            local extentMinZ = MetaGetClassValue(MetaGetMember(interface, "_mV1Z"))
            local extentMaxX = MetaGetClassValue(MetaGetMember(interface, "_mV2X"))
            local extentMaxY = MetaGetClassValue(MetaGetMember(interface, "_mV2Y"))
            local extentMaxZ = MetaGetClassValue(MetaGetMember(interface, "_mV2Z"))
            local animType = FlagsToAnimType(flags)
            CommonAnimationPushCompressedVector3Keys(state, name, minTime, maxTime, flags, animType, kCompressedVector3KeysFormatLegacy0, 
                                                    samples, extentMinX, extentMinY, extentMinZ, extentMaxX, extentMaxY, extentMaxZ)
        elseif clazz == "class CompressedQuaternionKeys" then
            local flags = MetaGetClassValue(MetaGetMember(interface, "_mFlags"))
            local minTime = MetaGetClassValue(MetaGetMember(interface, "_mMinTime"))
            local maxTime = MetaGetClassValue(MetaGetMember(interface, "_mMaxTime"))
            local samples = MetaGetMember(interface, "_mBuffer")
            local name = MetaGetClassValue(MetaGetMember(interface, "_mName"))
            local animType = FlagsToAnimType(flags)
            CommonAnimationPushCompressedQuatKeys(state, name, minTime, maxTime, flags, animType, kCompressedQuatKeysFormatLegacy0, samples)
        elseif clazz:sub(1, #"class KeyframedValue<") == "class KeyframedValue<" then
            local keyframedType = clazz:match("^class KeyframedValue%<(.-)%>$")
            local baseType = MetaGetMember(interface, "Baseclass_AnimatedValueInterface<T>")
            baseType = MetaGetMember(baseType, "mVal")
            baseType = MetaGetMember(baseType, "Baseclass_AnimationValueInterfaceBase")
            local name = MetaGetMember(baseType, "mName")
            local flags = MetaGetClassValue(MetaGetMember(baseType, "mFlags"))
            local animType = FlagsToAnimType(flags)
            CommonAnimationPushKeyframedValues(state, MetaGetClassValue(name), keyframedType:gsub("class ", ""), MetaGetMember(interface, "mMinVal"),
                                                MetaGetMember(interface, "mMaxVal"), MetaGetMember(interface, "mSamples"), flags, animType)
        else
            print("WARNING: Skipping unimplemented normaliser for animated value class " .. clazz)
        end
    end

    return true
end

function NormaliseSkeletonBone1(instance, state)
    local entries = MetaGetMember(instance, "mEntries")
    local num = ContainerGetNumElements(entries)
    for i=1,num do
        local entry = ContainerGetElement(entries, i - 1)
        local entryInfo = {}
        entryInfo["Joint Name"] = SymbolTableFind(MetaGetClassValue(MetaGetMember(entry, "mJointName")))
        entryInfo["Parent Name"] = SymbolTableFind(MetaGetClassValue(MetaGetMember(entry, "mParentName")))
        entryInfo["Parent Index"] = MetaGetClassValue(MetaGetMember(entry, "mParentIndex"))
        entryInfo["Local Position"] = MetaGetMember(entry, "mLocalPos")
        entryInfo["Local Rotation"] = MetaGetMember(entry, "mLocalQuat")
        print(i-1, entryInfo["Joint Name"])
        entryInfo["Global Scale"] = MetaGetMember(entry, "mGlobalTranslationScale")
        entryInfo["Local Scale"] = MetaGetMember(entry, "mLocalTranslationScale")
        entryInfo["Anim Scale"] = MetaGetMember(entry, "mAnimTranslationScale")
        entryInfo["Group Membership"] = MetaGetMember(entry, "mResourceGroupMembership")
        CommonSkeletonPushEntry(state, entryInfo)
    end
    return true
end

-- SERIALISERS

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
        local numOfType = MetaStreamReadInt(stream)
        for j=1,numOfType do
            local typeInst = MetaCreateInstance(typeSymbol, 0, tostring(totalNumRead + 1), instance)
            if not MetaSerialise(stream, typeInst, write) then return false end
            totalNumRead = totalNumRead + 1
        end
    end

    TTE_Assert(totalNumRead == numTotalValues, "Animation is corrupt!")

    MetaStreamEndBlock(stream, write)
    return true
end

function SerialiseCQKeysBone1(stream, instance, write)
    local name = MetaStreamReadString(stream)
    MetaSetClassValue(MetaGetMember(instance, "_mName"), name)

    if write then return false end
    local flags = MetaStreamReadInt(stream)

    MetaSetClassValue(MetaGetMember(instance, "_mFlags"), flags)
    MetaSetClassValue(MetaGetMember(instance, "_mMinTime"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mMaxTime"), MetaStreamReadFloat(stream))

    TTE_Assert(flags == 0 or flags == 512 or flags == 32 or flags == 16, "Check animation CQKeys")

    local numSamples = MetaStreamReadShort(stream)

    MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mBuffer"), numSamples * 6)

    return true
end

function SerialiseCVKeysBone1(stream, instance, write)
    local name = MetaStreamReadString(stream)
    MetaSetClassValue(MetaGetMember(instance, "_mName"), name)

    if write then return false end

    local flags = MetaStreamReadInt(stream)
    TTE_Assert(flags == 0 or flags == 512 or flags == 32 or flags == 16, "Check animation CVKeys")

    local extentMinX = MetaStreamReadFloat(stream)
    local extentMinY = MetaStreamReadFloat(stream)
    local extentMinZ = MetaStreamReadFloat(stream)
    local extentMaxX = MetaStreamReadFloat(stream)
    local extentMaxY = MetaStreamReadFloat(stream)
    local extentMaxZ = MetaStreamReadFloat(stream)

    MetaSetClassValue(MetaGetMember(instance, "_mFlags"), flags) -- 512: homogeneous, 32 = prop anm. 0 for move_walkbackwards_test.anm (test..)
    MetaSetClassValue(MetaGetMember(instance, "_mV1X"), extentMinX)
    MetaSetClassValue(MetaGetMember(instance, "_mV1Y"), extentMinY)
    MetaSetClassValue(MetaGetMember(instance, "_mV1Z"), extentMinZ)
    MetaSetClassValue(MetaGetMember(instance, "_mV2X"), extentMaxX)
    MetaSetClassValue(MetaGetMember(instance, "_mV2Y"), extentMaxY)
    MetaSetClassValue(MetaGetMember(instance, "_mV2Z"), extentMaxZ)

    MetaSetClassValue(MetaGetMember(instance, "_mMinTime"), MetaStreamReadFloat(stream))
    MetaSetClassValue(MetaGetMember(instance, "_mMaxTime"), MetaStreamReadFloat(stream))

    local numSamples = MetaStreamReadShort(stream)
    MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mBuffer"), numSamples * 6)

    return true
end
