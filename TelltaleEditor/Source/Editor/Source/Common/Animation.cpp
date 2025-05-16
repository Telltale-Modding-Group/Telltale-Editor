#include <Common/Animation.hpp>
#include <AnimationManager.hpp>

// INTERFACE IMPL

Bool AnimationValueInterface::IsEmptyValue() const
{
    return false;
}

// SCRIPT NORMALISATION / SPEC API

class AnimationAPI
{
public:
    
    static Animation* Task(LuaManager& man)
    {
        return (Animation*)man.ToPointer(1);
    }
    
    static U32 luaSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of set name");
        Animation* anm = Task(man);
        anm->_Name = man.ToString(2);
        return 0;
    }
    
    static U32 luaSetLen(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of set length");
        Animation* anm = Task(man);
        anm->_Length = man.ToFloat(2);
        return 0;
    }
    
    static U32 luaPushCompressedVec3s(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() >= 7, "Invalid arguments");
        Animation* anm = Task(man);
        String name = man.ToString(2);
        
        Float minTime = man.ToFloat(3);
        Float maxTime = man.ToFloat(4);
        Float len = maxTime - minTime;
        U32 flags = (U32)man.ToInteger(5);
        U32 type = (U32)man.ToInteger(6);
        U32 format = man.ToInteger(7);
        
        Ptr<KeyframedValue<Vector3>> keys = TTE_NEW_PTR(KeyframedValue<Vector3>,
                                                        MEMORY_TAG_ANIMATION_DATA, std::move(name));
        keys->_Flags = flags;
        keys->_Type = (AnimationValueType)type;
        
        if(format == 0)
        {
            // version 0 legacy: push(state, name, minTime, maxTime, flags, type, (format)0, samplebuf, minx, miny.... maxz)
            TTE_ASSERT(man.GetTop() == 14, "Invalid arguments");
            Meta::ClassInstance bufcls = Meta::AcquireScriptInstance(man, 8);
            Vector3 minExtent{}, maxExtent{};
            minExtent = Vector3(man.ToFloat(9), man.ToFloat(10), man.ToFloat(11));
            maxExtent = Vector3(man.ToFloat(12), man.ToFloat(13), man.ToFloat(14));
            Vector3 delta = maxExtent - minExtent;
            TTE_ASSERT(delta.x >= 0.0f && delta.y >= 0.0f && delta.z >= 0.0f, "Animation bounds corrupt");
            Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufcls._GetInternal());
            U32 nSamples = buf.BufferSize / 6;
            keys->_Samples.reserve(nSamples);
            keys->_MinValue = minExtent;
            keys->_MaxValue = maxExtent;
            for(U32 i = 0; i < nSamples; i++)
            {
                auto& sample = keys->_Samples.emplace_back();
                U16 time = *((U16*)(buf.BufferData.get() + 6 * i + 0));
                U32 compressed = *((U32*)(buf.BufferData.get() + 6 * i + 2));
                if((time & 0x8000) != 0)
                    sample.SampleFlags.Add(KeyframedSampleFlag::INTERPOLATE_TO_NEXT_KEY);
                sample.Time = (((Float)(time & 0x7FFF)) / 32767.0f) * len + minTime;
                U32 xbits = compressed & 0x3FF;
                U32 ybits = (compressed >> 10) & 0x5FF;
                U32 zbits = compressed >> 21;
                sample.Value.x = ((Float)xbits / 1023.0f) * delta.x + minExtent.x; // one less precision in X
                sample.Value.y = ((Float)ybits / 2047.0f) * delta.y + minExtent.y;
                sample.Value.z = ((Float)zbits / 2047.0f) * delta.z + minExtent.z;
            }
        }
        else
        {
            TTE_ASSERT(false, "Invalid argument to decompressor");
        }
        anm->_Values.push_back(std::move(keys));
        return 0;
    }
    
    static U32 luaPushCompressedQuats(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() >= 7, "Invalid arguments");
        Animation* anm = Task(man);
        String name = man.ToString(2);
        
        Float minTime = man.ToFloat(3);
        Float maxTime = man.ToFloat(4);
        Float len = maxTime - minTime;
        
        U32 flags = (U32)man.ToInteger(5);
        U32 type = (U32)man.ToInteger(6);
        U32 format = man.ToInteger(7);
        
        Ptr<KeyframedValue<Quaternion>> keys = TTE_NEW_PTR(KeyframedValue<Quaternion>,
                                                        MEMORY_TAG_ANIMATION_DATA, std::move(name));
        keys->_Flags = flags;
        keys->_Type = (AnimationValueType)type;
        
        if(format == 0)
        {
            // version 0 legacy: push(state, name, minTime, maxTime, flags, type, (format)0, samplebuf)
            TTE_ASSERT(man.GetTop() == 8, "Invalid arguments");
            Meta::ClassInstance bufcls = Meta::AcquireScriptInstance(man, 8);
            Meta::BinaryBuffer& buf = *((Meta::BinaryBuffer*)bufcls._GetInternal());
            U32 nSamples = buf.BufferSize / 6;
            keys->_Samples.reserve(nSamples);
            // min and max value not used
            for(U32 i = 0; i < nSamples; i++)
            {
                auto& sample = keys->_Samples.emplace_back();
                U16 time = *((U16*)(buf.BufferData.get() + 6 * i + 0));
                U32 compressed = *((U32*)(buf.BufferData.get() + 6 * i + 2));
                if((time & 0x8000) != 0)
                    sample.SampleFlags.Add(KeyframedSampleFlag::INTERPOLATE_TO_NEXT_KEY);
                sample.Time = (((Float)(time & 0x7FFF)) / 32767.0f) * len + minTime;
                U32 quatX = compressed & 0x3FF; /*1s complement. will remove top bit if needed below for each XYZ*/
                U32 quatY = (compressed >> 10) & 0x3FF;
                U32 quatZ = (compressed >> 20) & 0x3FF;
                Bool negatedW = compressed >> 31; // bit index 30 is unused
                TTE_ASSERT(((compressed >> 30) & 1) == 0, "Corrupt compressed quaternion");
                Float x = (Float)(quatX & 0x1FF) / 511.0f;
                if(quatX & 0x200) x = -x;
                Float y = (Float)(quatY & 0x1FF) / 511.0f;
                if(quatY & 0x200) y = -y;
                Float z = (Float)(quatZ & 0x1FF) / 511.0f;
                if(quatZ & 0x200) z = -z;
                sample.Value.x = x;
                sample.Value.y = y;
                sample.Value.z = z;
                sample.Value.w = sqrtf(fmaxf(1.0f - (x * x + y * y + z * z), 0.0f));
                sample.Value.Normalize();
                if(negatedW)
                    sample.Value.w = -sample.Value.w; // LONG WAY ROUND rotation!
            }
        }
        else
        {
            TTE_ASSERT(false, "Invalid argument to decompressor");
        }
        anm->_Values.push_back(std::move(keys));
        return 0;
    }
    
    template<typename T>
    static inline Bool TestPushKeyframed(LuaManager& man, const String& type,
                                         Animation& anm, String& name, CString typeName)
    {
        return TestPushKeyframed<T>(man, type, anm, name, typeName, &Meta::ExtractCoercableInstance<T>);
    }
    
    template<typename T>
    static inline Bool TestPushKeyframed(LuaManager& man, const String& type,
                                         Animation& anm, String& name, CString typeName,
                                         void (*Extractor)(T& out, Meta::ClassInstance& instance))
    {
        if(CompareCaseInsensitive(type, typeName))
        {
            
            Meta::ClassInstance minVal = Meta::AcquireScriptInstance(man, 4);
            Meta::ClassInstance maxVal = Meta::AcquireScriptInstance(man, 5);
            Meta::ClassInstance samplesInst = Meta::AcquireScriptInstance(man, 6);
            TTE_ASSERT(minVal && maxVal && samplesInst, "Invalid arguments in push keyframed. Not classes.");
            Meta::ClassInstanceCollection& samples = Meta::CastToCollection(samplesInst);
            U32 nSamples = samples.GetSize();
            Ptr<KeyframedValue<T>> keyframed = TTE_NEW_PTR(KeyframedValue<T>, MEMORY_TAG_ANIMATION_DATA, std::move(name));
            Extractor(keyframed->_MinValue, minVal);
            Extractor(keyframed->_MaxValue, maxVal);
            keyframed->_Flags = (U32)man.ToInteger(7);
            keyframed->_Type = (AnimationValueType)man.ToInteger(8);
            for(U32 i = 0; i < nSamples; i++)
            {
                auto& sample = keyframed->_Samples.emplace_back();
                Meta::ClassInstance samplesInst = samples.GetValue(i);
                sample.Time = Meta::GetMember<Float>(samplesInst, "mTime");
                if(Meta::GetMember<Bool>(samplesInst, "mbInterpolateToNextKey"))
                    sample.SampleFlags.Add(KeyframedSampleFlag::INTERPOLATE_TO_NEXT_KEY);
                Meta::ClassInstance value = Meta::GetMember(samplesInst, "mValue", false);
                TTE_ASSERT(value, "Value type not found in keyframed value class sample");
                Extractor(sample.Value, value);
            }
            anm._Values.push_back(std::move(keyframed));
            return true;
        }
        return false;
    }
    
    // remove 'class' prefix etc
    // push keyframed (state, name, keyframedTypeNameMetaClassTypeNameDecayed,
    // mininst, maxinst, samplesarrayinst,flags, typeAnimType)
    // all telltale games keyframed<>::sample has mTime, mbInterpolateToNextKey and mValue.
    static U32 luaPushKeyframed(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 8, "Invalid arguments");
        Animation* anm = Task(man);
        String name = man.ToString(2);
        String type = man.ToString(3);
        if(TestPushKeyframed<Bool>(man, type, *anm, name, "bool"))
            return 0;
        if(TestPushKeyframed<Float>(man, type, *anm, name, "float"))
            return 0;
        if(TestPushKeyframed<Colour>(man, type, *anm, name, "Color"))
            return 0;
        if(TestPushKeyframed<Vector2>(man, type, *anm, name, "Vector2"))
            return 0;
        if(TestPushKeyframed<Vector3>(man, type, *anm, name, "Vector3"))
            return 0;
        if(TestPushKeyframed<Vector4>(man, type, *anm, name, "Vector3"))
            return 0;
        if(TestPushKeyframed<Quaternion>(man, type, *anm, name, "Quaternion"))
            return 0;
        if(TestPushKeyframed<String>(man, type, *anm, name, "String"))
            return 0;
        TTE_ASSERT(false, "Keyframed value support for %s needs to be implemented into API!", type.c_str());
        return 0;
    }
    
};

void Animation::FinaliseNormalisationAsync()
{

}

void Animation::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    
    PUSH_FUNC(Col, "CommonAnimationSetName", &AnimationAPI::luaSetName, "nil CommonAnimationSetName(state, name)", "Set common animation name");
    PUSH_FUNC(Col, "CommonAnimationSetLength", &AnimationAPI::luaSetLen, "nil CommonAnimationSetLength(state, length)", "Set common animation length");
    PUSH_FUNC(Col, "CommonAnimationPushCompressedVector3Keys", &AnimationAPI::luaPushCompressedVec3s,
              "nil CommonAnimationPushCompressedVector3Keys(state, name, minTime, maxTime, flags, type, format, format_options --[[see examples / Animation.cpp]])",
              "Push compressed position keys to the animation as an animated value");
    PUSH_FUNC(Col, "CommonAnimationPushCompressedQuatKeys", &AnimationAPI::luaPushCompressedQuats,
              "nil CommonAnimationPushCompressedQuatKeys(state, name, minTime, maxTime, flags, type, format, format_options --[[see examples / Animation.cpp]])",
              "Push compressed position keys to the animation as an animated value");
    PUSH_FUNC(Col, "CommonAnimationPushKeyframedValues", &AnimationAPI::luaPushKeyframed,
              "nil CommonAnimationPushKeyframedValues(state, name, keyframedClassNameDecayed, minVal, maxVal, sampleContainer, flags, type)",
              "Pyush compressed quaternion rotation keys to the animation as an animated value.");
    
    PUSH_GLOBAL_I(Col, "kCompressedVector3KeysFormatLegacy0", 0, "Legacy normed compressed vector3 keys (0).");
    PUSH_GLOBAL_I(Col, "kCompressedQuatKeysFormatLegacy0", 0, "Legacy normed quaternion compressed keys (0)");
    
    const AnimationValueTypeDesc* pDesc = AnimTypeDescs;
    while(pDesc->Type != AnimationValueType::NONE)
    {
        PUSH_GLOBAL_I(Col, pDesc->Name, (U32)pDesc->Type, "Animation value types");
        pDesc++;
    }
    
}

TransitionRemapper::TransitionRemapper() : RemapKeys("transition map")
{
    RemapKeys._Type = AnimationValueType::CONTRIBUTION;
    RemapKeys._MinValue = 0.0f;
    RemapKeys._MaxValue = 1.0f;
    auto& start = RemapKeys._Samples.emplace_back();
    auto& end = RemapKeys._Samples.emplace_back();
    end.Time = 1.0f;
    end.Value = 1.0f;
}

Float TransitionRemapper::Remap(Float c)
{
    ComputedValue<Float> computed{};
    PerformMix<Float>::PrepareValue(computed, 0.0f);
    Flags _{};
    RemapKeys.ComputeValueKeyframed(&computed, c, kDefaultContribution, _, false);
    return ClampFloat(computed.Value + computed.AdditiveValue, 0.0f, 1.0f);
}

Float TransitionMap::RemapBoneContribution(Float C, SkeletonEntry* pSklEntry, Float controllerContrib)
{
    Float finalContribution = controllerContrib;
    I32 highestPriority = -1;
    for(auto& remap: _Remappers)
    {
        const auto it = pSklEntry->ResourceGroupMembership.find(remap.first);
        if(it != pSklEntry->ResourceGroupMembership.end() && it->second >= 0.5f && remap.second.Priority > highestPriority)
        {
            highestPriority = remap.second.Priority;
            finalContribution = remap.second.Remap(C);
        }
    }
    return finalContribution;
}

TransitionMap::TransitionMap(Ptr<ResourceRegistry> reg) : Handleable(reg) {}
