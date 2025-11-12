-- ====== LEGACY OLD ANIMATION

-- CLASS DESCS

function RegisterProceduralLookAts0(MetaVec3, animatedQuat, anm)
    local procval = NewClass("class Procedural_LookAt_Value", 0)
    procval.Flags = kMetaClassIntrinsic + kMetaClassNonBlocked
    procval.Members[1] = NewMember("Baseclass_AnimatedValueInterface<Quaternion>", animatedQuat,
        kMetaMemberSerialiseDisable)
    MetaRegisterClass(procval)

    local proc = NewClass("class Procedural_LookAt", 0)
    proc.Serialiser = "SerialiseProceduralLookAt0"
    proc.Flags = kMetaClassIntrinsic -- none of this is in headers. also only animation is serialised?
    proc.Members[1] = NewMember("Baseclass_Animation", NewProxyClass("class Animation *", "Animation", anm, true))
    proc.Members[2] = NewMember("mHostNode", kMetaClassString)
    proc.Members[3] = NewMember("mTargetAgent", kMetaClassString)
    proc.Members[4] = NewMember("mTargetNode", kMetaClassString)
    proc.Members[5] = NewMember("mTargetOffset", MetaVec3)
    MetaRegisterClass(proc)

    return proc, procval
end

function RegisterCompressedVecAndQuats0()
    local quatKeys = NewClass("class CompressedQuaternionKeys", 0)
    quatKeys.Flags = kMetaClassIntrinsic -- not in headers
    quatKeys.Serialiser = "SerialiseCQKeys0"
    quatKeys.Members[1] = NewMember("_mName", kMetaClassString, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    quatKeys.Members[2] = NewMember("_mBuffer", kMetaClassInternalBinaryBuffer,
        kMetaMemberSerialiseDisable + kMetaMemberVersionDisable) -- nSamples = size / 6
    quatKeys.Members[3] = NewMember("_mMinTime", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    quatKeys.Members[4] = NewMember("_mMaxTime", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    quatKeys.Members[5] = NewMember("_mFlags", kMetaInt, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    MetaRegisterClass(quatKeys)

    local vecKeys = NewClass("class CompressedVector3Keys", 0)
    vecKeys.Flags = kMetaClassIntrinsic -- not in headers
    vecKeys.Serialiser = "SerialiseCVKeys0"
    vecKeys.Members[1] = NewMember("_mName", kMetaClassString, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[2] = NewMember("_mBuffer", kMetaClassInternalBinaryBuffer,
        kMetaMemberSerialiseDisable + kMetaMemberVersionDisable) -- nSamples = size / 6
    vecKeys.Members[3] = NewMember("_mMinTime", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[4] = NewMember("_mV1X", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[5] = NewMember("_mV1Y", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[6] = NewMember("_mV1Z", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[7] = NewMember("_mV2X", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[8] = NewMember("_mV2Y", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[9] = NewMember("_mV2Z", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[10] = NewMember("_mFlags", kMetaInt, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    vecKeys.Members[11] = NewMember("_mMaxTime", kMetaFloat, kMetaMemberSerialiseDisable + kMetaMemberVersionDisable)
    MetaRegisterClass(vecKeys)

    return vecKeys, quatKeys
end

function RegisterSkeleton0(collectionRegistrar, MetaVec3, MetaQuat, transform, mapStringFloat)
	local sklEntry = NewClass("class Skeleton::Entry", 0)
	sklEntry.Members[1] = NewMember("mJointName", kMetaClassString)
	sklEntry.Members[2] = NewMember("mParentName", kMetaClassString)
	sklEntry.Members[3] = NewMember("mParentIndex", kMetaInt)
	sklEntry.Members[4] = NewMember("mLocalPos", MetaVec3)
	sklEntry.Members[5] = NewMember("mLocalQuat", MetaQuat)
	sklEntry.Members[6] = NewMember("mRestXform", transform)
	sklEntry.Members[7] = NewMember("mGlobalTranslationScale", MetaVec3)
	sklEntry.Members[8] = NewMember("mLocalTranslationScale", MetaVec3)
	sklEntry.Members[9] = NewMember("mAnimTranslationScale", MetaVec3)
	sklEntry.Members[10] = NewMember("mResourceGroupMembership", mapStringFloat)
	MetaRegisterClass(sklEntry)

	local arraySklEntry = collectionRegistrar("class DCArray<class Skeleton::Entry>", nil, sklEntry)

	local skl = NewClass("class Skeleton", 0)
	skl.Extension = "skl"
	skl.Normaliser = "NormaliseSkeleton0"
	skl.Members[1] = NewMember("mEntries", arraySklEntry)
	MetaRegisterClass(skl)
    return skl
end

-- NORMALISERS

function NormaliseAnimation0(instance, state)
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
        elseif MetaFlagQuery(flags, 2) then
            animType = kAnimationValueTypeTime
        elseif MetaFlagQuery(flags, 4) then
            animType = kAnimationValueTypeContribution
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
            CommonAnimationPushCompressedVector3Keys(state, name, minTime, maxTime, flags, animType,
                kCompressedVector3KeysFormatLegacy0,
                samples, extentMinX, extentMinY, extentMinZ, extentMaxX, extentMaxY, extentMaxZ)
        elseif clazz == "class CompressedQuaternionKeys" then
            local flags = MetaGetClassValue(MetaGetMember(interface, "_mFlags"))
            local minTime = MetaGetClassValue(MetaGetMember(interface, "_mMinTime"))
            local maxTime = MetaGetClassValue(MetaGetMember(interface, "_mMaxTime"))
            local samples = MetaGetMember(interface, "_mBuffer")
            local name = MetaGetClassValue(MetaGetMember(interface, "_mName"))
            local animType = FlagsToAnimType(flags)
            CommonAnimationPushCompressedQuatKeys(state, name, minTime, maxTime, flags, animType,
                kCompressedQuatKeysFormatLegacy0, samples)
        elseif clazz:sub(1, #"class KeyframedValue<") == "class KeyframedValue<" then
            local keyframedType = clazz:match("^class KeyframedValue%<(.-)%>$")
            local baseType = MetaGetMember(interface, "Baseclass_AnimatedValueInterface<T>")
            baseType = MetaGetMember(baseType, "mVal")
            baseType = MetaGetMember(baseType, "Baseclass_AnimationValueInterfaceBase")
            local name = MetaGetMember(baseType, "mName")
            local flags = MetaGetClassValue(MetaGetMember(baseType, "mFlags"))
            local animType = FlagsToAnimType(flags)
            CommonAnimationPushKeyframedValues(state, MetaGetClassValue(name), keyframedType:gsub("class ", ""),
                MetaGetMember(interface, "mMinVal"),
                MetaGetMember(interface, "mMaxVal"), MetaGetMember(interface, "mSamples"), flags, animType)
        else
            print("WARNING: Skipping unimplemented normaliser for animated value class " .. clazz)
        end
    end

    return true
end

function NormaliseSkeleton0(instance, state)
    local entries = MetaGetMember(instance, "mEntries")
    local num = ContainerGetNumElements(entries)
    for i = 1, num do
        local entry = ContainerGetElement(entries, i - 1)
        local entryInfo = {}
        entryInfo["Joint Name"] = SymbolTableFind(MetaGetClassValue(MetaGetMember(entry, "mJointName")))
        entryInfo["Parent Name"] = SymbolTableFind(MetaGetClassValue(MetaGetMember(entry, "mParentName")))
        entryInfo["Parent Index"] = MetaGetClassValue(MetaGetMember(entry, "mParentIndex"))
        entryInfo["Local Position"] = MetaGetMember(entry, "mLocalPos")
        entryInfo["Local Rotation"] = MetaGetMember(entry, "mLocalQuat")
        entryInfo["Global Scale"] = MetaGetMember(entry, "mGlobalTranslationScale")
        entryInfo["Local Scale"] = MetaGetMember(entry, "mLocalTranslationScale")
        entryInfo["Anim Scale"] = MetaGetMember(entry, "mAnimTranslationScale")
        entryInfo["Group Membership"] = MetaGetMember(entry, "mResourceGroupMembership")
        CommonSkeletonPushEntry(state, entryInfo)
    end
    return true
end

-- SERIALISERS

function SerialiseProceduralLookAt0(stream, instance, write)
    return MetaSerialise(stream, MetaGetMember(instance, "Baseclass_Animation"), write)
end

function SerialiseAnimation0(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end
    MetaStreamBeginBlock(stream, write)

    if write then return false end

    local numTotalValues = MetaStreamReadInt(stream)
    local numValueTypes = MetaStreamReadInt(stream)
    local totalNumRead = 0

    for i = 1, numValueTypes do
        local typeSymbol = MetaStreamReadSymbol(stream)
        local numOfType = MetaStreamReadInt(stream)
        for j = 1, numOfType do
            local typeInst = MetaCreateInstance(typeSymbol, 0, tostring(totalNumRead + 1), instance)
            if not MetaSerialise(stream, typeInst, write) then return false end
            totalNumRead = totalNumRead + 1
        end
    end

    TTE_Assert(totalNumRead == numTotalValues, "Animation is corrupt!")

    MetaStreamEndBlock(stream, write)
    return true
end

function SerialiseCQKeys0(stream, instance, write)
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

function SerialiseCVKeys0(stream, instance, write)
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
