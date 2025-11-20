// TEMPLATE ANIMATION IMPL

// **MOST IMPORTANT PART** MIXERS: THESE MIX AND INTERPOLATE BETWEEN SAMPLES.

template<>
struct PerformMix<Vector4> : PerformMixBase<Vector4>
{
    
    // Defaults
    
};

template<>
struct PerformMix<Vector3> : PerformMixBase<Vector3>
{
    
    // Defaults
    
    static inline void Mirror(Vector3& value)
    {
        value.x = -value.x;
    }
    
    static inline Vector3 BlendAdditive(const Vector3& cur, const Vector3& adding, Float mix)
    {
        return cur + (adding * Vector3(mix));
    }
    
};

template<>
struct PerformMix<Vector2> : PerformMixBase<Vector2>
{
    
    // Defaults
    
};

template<>
struct PerformMix<Float> : PerformMixBase<Float>
{
    
    // Defaults
    
    static inline Float BlendAdditive(const Float& cur, const Float& adding, Float mix)
    {
        return cur + (adding * mix);
    }
    
};

template<>
struct PerformMix<I32> : PerformMixBase<I32>
{
    
    // Defaults
    
};

template<>
struct PerformMix<Quaternion> : PerformMixBase<Quaternion>
{
    
    static inline void Finalise(Quaternion& value)
    {
        value.Normalize();
    }
    
    static inline void Mirror(Quaternion& value)
    {
        value.y = -value.y;
        value.z = -value.z;
    }
    
    static inline void OutputValue(ComputedValue<Quaternion>& out, Bool bAdditive, const Quaternion& v, Float c)
    {
        if(bAdditive)
        {
            if(c < 0.99999f)
                out.Value = Quaternion::Slerp(Quaternion::kIdentity, v, c);
            else
                out.Value = v;
            out.Contribution = 0.0f;
        }
        else
        {
            out.Contribution = c;
            out.Value = v;
        }
    }
    
    static inline void Blend(Quaternion& start, const Quaternion& end, Float t)
    {
        start = Quaternion::Slerp(start, end, t);
    }
    
    static inline Quaternion BlendAdditive(const Quaternion& cur, const Quaternion& adding, Float mix)
    {
        Quaternion delta = Quaternion::Slerp(Quaternion::kIdentity, adding, mix);
        return delta * cur;
    }
    
    static inline Quaternion CatmullRom(const Quaternion& p0, const Quaternion& p1, const Quaternion& p2, const Quaternion& p3, Float t)
    {
        return Quaternion::Slerp(p1, p2, t); // can change to approximate below in el futuro
        // Align all quaternions to the same hemisphere
        /*Quaternion q0 = Quaternion::Dot(p0, p1) < 0.0f ? -p0 : p0;
        Quaternion q1 = p1;
        Quaternion q2 = Quaternion::Dot(p1, p2) < 0.0f ? -p2 : p2;
        Quaternion q3 = Quaternion::Dot(p2, p3) < 0.0f ? -p3 : p3;
        
        // Estimate tangents (finite difference)
        Quaternion tan1 = (q2 - q0) * 0.5f;
        Quaternion tan2 = (q3 - q1) * 0.5f;
        
        // Interpolate the Hermite-style tangents
        Quaternion m1 = Quaternion::NLerp(q1, q1 + tan1, t);
        Quaternion m2 = Quaternion::NLerp(q2, q2 - tan2, 1.0f - t);
        
        // Interpolate along path and along tangents
        Quaternion path = Quaternion::NLerp(q1, q2, t);
        Quaternion tangent = Quaternion::NLerp(m1, m2, t);
        
        // Blend the two paths
        Float s = t * (1.0f - t) * 2.0f;
        return Quaternion::NLerp(path, tangent, s);*/
    }

    
};

template<>
struct PerformMix<Bool> : PerformMixBase<Bool>
{
    
    static inline Bool CatmullRom(const Bool& p0, const Bool& p1, const Bool& p2, const Bool& p3, Float t)
    {
        return PerformMix<Float>::CatmullRom(p0 ? 1.0f : 0.0f, p1 ? 1.0f : 0.0f, p2 ? 1.0f : 0.0f, p3 ? 1.0f : 0.0f, t) != 0.0f;
    }
    
    static inline void OutputValue(ComputedValue<Bool>& out, Bool bAdditive, const Bool& value, Float contribution)
    {
        if(bAdditive)
        {
            out.Contribution = 0.0f;
            if(contribution >= 0.5f)
                out.Value = value;
        }
        else
        {
            out.Value = value;
            out.Contribution = contribution;
        }
    }
    
    static inline void Blend(Bool& start, const Bool& end, Float t)
    {
        //return t >= 0.5f ? end : start;
    }
    
};

template<>
struct PerformMix<String> : PerformMixBase<String>
{
    
    static inline String CatmullRom(const String& p0, const String& p1, const String& p2, const String& p3, Float t)
    {
        return p1; // main point (p0 and p3 are surrounding, p2 is just above) - telltale do this
    }
    
    static inline void OutputValue(ComputedValue<String>& out, Bool bAdditive, const String& value, Float contribution)
    {
        if(bAdditive)
        {
            out.Contribution = 0.0f;
            if(contribution >= 0.5f)
                out.Value = value;
        }
        else
        {
            out.Value = value;
            out.Contribution = contribution;
        }
    }
    
    static inline void Blend(String& start, const String& end, Float t)
    {
        //return t >= 0.5f ? end : start;
        // start = start!
    }
    
};

template<>
struct PerformMix<Colour> : PerformMixBase<Colour>
{
    
    static inline Colour CatmullRom(const Colour& p0, const Colour& p1, const Colour& p2, const Colour& p3, Float t)
    {
        return Vector4ToColour(PerformMix<Vector4>::CatmullRom(ColourToVector4(p0), ColourToVector4(p1),
                                                               ColourToVector4(p2), ColourToVector4(p3), t));
    }
    
    static inline void OutputValue(ComputedValue<Colour>& out, Bool bAdditive, const Colour& value, Float contribution)
    {
        if(bAdditive)
        {
            out.Contribution = 0.0f;
            out.Value = Vector4ToColour(ColourToVector4(value) * contribution);
        }
        else
        {
            out.Value = value;
            out.Contribution = contribution;
        }
    }
    
    static inline void Blend(Colour& start, const Colour& end, Float t)
    {
        start = Vector4ToColour(ColourToVector4(start) + ((ColourToVector4(end) - ColourToVector4(start)) * t));
    }
    
};

template<>
struct PerformMix<Transform> : PerformMixBase<Transform>
{
    
    static inline void Finalise(Transform& value)
    {
        value._Rot.Normalize();
    }
    
    static inline void Mirror(Transform& value)
    {
        PerformMix<Vector3>::Mirror(value._Trans);
        PerformMix<Quaternion>::Mirror(value._Rot);
    }
    
    static inline void PrepareValue(ComputedValue<Transform>& out, const Transform& value)
    {
        out.Value = value;
        out.Vec3Contrib = out.QuatContrib = 0.f;
        out.AdditiveMix = 1.0f;
    }
    
    static inline Transform CatmullRom(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, Float t)
    {
        return Transform(PerformMix<Quaternion>::CatmullRom(p0._Rot, p1._Rot, p2._Rot, p3._Rot, t),
                         PerformMix<Vector3>::CatmullRom(p0._Trans, p1._Trans, p2._Trans, p3._Trans, t));
    }
    
    static inline void OutputValue(ComputedValue<Transform>& out, Bool bAdditive, const Transform& v, Float c)
    {
        if(bAdditive)
        {
            if(c < 0.99999f)
                out.Value._Rot = Quaternion::Slerp(v._Rot, Quaternion::kIdentity, c);
            else
                out.Value._Rot = v._Rot;
            out.Value._Trans = v._Trans * Vector3(c);
        }
        else
        {
            out.Value = v;
        }
        out.QuatContrib = out.Vec3Contrib = c;
    }
    
    static inline void Blend(Transform& start, const Transform& end, Float t)
    {
        PerformMix<Quaternion>::Blend(start._Rot, end._Rot, t);
        PerformMix<Vector3>::Blend(start._Trans, end._Trans, t);
    }
    
    static inline Transform BlendAdditive(const Transform& start, const Transform& end, Float t)
    {
        return Transform{PerformMix<Quaternion>::BlendAdditive(start._Rot, end._Rot, t), PerformMix<Vector3>::BlendAdditive(start._Trans, end._Trans, t)};
    }
    
};

// UTIL

inline const AnimationValueTypeDesc& GetAnimationTypeDesc(AnimationValueType type)
{
    const AnimationValueTypeDesc* pDesc = AnimTypeDescs;
    while(pDesc->Type != AnimationValueType::NONE)
    {
        if(pDesc->Type == type)
            return *pDesc;
        pDesc++;
    }
    TTE_ASSERT(false, "Unknown value type");
    return *pDesc;
}

// KEYFRAMED<T>

template<typename T> KeyframedValue<T>::KeyframedValue(String name) : AnimationValueInterface(std::move(name)) {}

template<typename T> const std::type_info& KeyframedValue<T>::GetValueType() const
{
    return typeid(T);
}

template<typename T> Float KeyframedValue<T>::GetMaxTime() const
{
    return _Samples.size() == 0 ? 0.0f : _Samples.back().Time;
}

template<typename T> void KeyframedValue<T>::ComputeValue(void* pValue,
                Ptr<PlaybackController> Controller, const Float* Contribution)
{
    Float Time = Controller->GetTime() * Controller->GetTimeScale();
    Flags _f{};
    ComputeValueKeyframed(pValue, Time, Contribution, _f, false);
}

template<typename T> void KeyframedValue<T>::ComputeValueKeyframed(void* pValue,
            Float Time, const Float* Contribution, Flags& out, Bool bEmpty)
{
    if(_Flags.Test(AnimationValueFlags::MIXER_DIRTY))
        CleanMixer();
    ComputedValue<T>* pOutput = (ComputedValue<T>*)pValue;
    Bool additive = GetAdditive();
    if(_Samples.size())
    {
        if(_Samples.size() == 1 || _Samples[0].Time > Time)
        {
            T only = _Samples[0].Value;
            out = _Samples[0].SampleFlags;
            PerformMix<T>::Finalise(only);
            PerformMix<T>::OutputValue(*pOutput, additive, only, Contribution[0]); // use first value
        }
        else
        {
            if(Time >= _Samples.back().Time)
            {
                // out of range
                T last = _Samples.back().Value;
                out = _Samples.back().SampleFlags;
                PerformMix<T>::Finalise(last);
                PerformMix<T>::OutputValue(*pOutput, additive, last, Contribution[0]); // use last value
            }
            else
            {
                // binary search!
                U32 low = 0, high = (U32)_Samples.size() - 1, mid = 0;
                while (high - low > 1)
                {
                    mid = (low + high) >> 1;
                    
                    if (Time < _Samples[mid].Time)
                        high = mid;
                    else
                        low = mid;
                }
                if(_Samples[low].SampleFlags.Test(KeyframedSampleFlag::INTERPOLATE_TO_NEXT_KEY))
                {
                    U32 p0 = (low >= 1) ? low - 1 : low;
                    U32 p1 = low;
                    U32 p2 = (low + 1 < _Samples.size()) ? low + 1 : low;
                    U32 p3 = (low + 2 < _Samples.size()) ? low + 2 : p2;
                    Float t1 = _Samples[p1].Time;
                    Float t2 = _Samples[p2].Time;
                    Float dt = t2 - t1;
                    Float f = (dt > 0.0f) ? (Time - t1) / dt : 0.0f;
                    T interp = PerformMix<T>::CatmullRom(_Samples[p0].Value, _Samples[p1].Value,
                                                         _Samples[p2].Value, _Samples[p3].Value, f);
                    PerformMix<T>::Finalise(interp);
                    PerformMix<T>::OutputValue(*pOutput, additive, interp, Contribution[0]);
                }
                else
                {
                    T value = _Samples[low].Value;
                    PerformMix<T>::Finalise(value);
                    PerformMix<T>::OutputValue(*pOutput, additive, value, Contribution[0]);
                }
                out = _Samples[low].SampleFlags;
            }
        }
        return;
    }
    if(bEmpty)
        PerformMix<T>::OutputValue(*pOutput, additive, _MinValue, Contribution[0]); // no samples, return min
    else
    {
        T empty{};
        PerformMix<T>::PrepareValue(*pOutput, empty);
        PerformMix<T>::OutputValue(*pOutput, additive, empty, 0.0f);
    }
}

// MIXER ACCUMULATER <T>

template<typename T> inline void
AnimationMixerAccumulater<T>::AccumulateCurrent(const ComputedValue<T>* pCurrentValues, U32 numCurrentValues,
                        ComputedValue<T>& finalValue, _AnimationMixerContribution<T> contributionAtPriority)
{ // THIS AND FINAL ASSUME CONTRIBUTOIN<T> IS NORMAL AND NOT SPECIALISED. use specialsed accumulaters elsewise
    T accumulated{};
    Float denominator = 1.0f / fmaxf(contributionAtPriority.Contribution, 0.000001f);
    Float maxContrib = 0.0f;
    for(U32 i = 0; i < numCurrentValues; i++)
    {
        PerformMix<T>::Blend(accumulated, pCurrentValues[i].Value, denominator * pCurrentValues[i].Contribution);
        maxContrib = MAX(maxContrib, pCurrentValues[i].Contribution);
    }
    finalValue.Value = accumulated;
    finalValue.AdditiveMix = 1.0f;
    finalValue.AdditiveValue = T{};
    finalValue.Contribution = maxContrib;
}

template<typename T> inline void
AnimationMixerAccumulater<T>::AccumulateFinal(const ComputedValue<T>* finalPriorityValues, U32 numFinalValues,
                                              ComputedValue<T>& outValue, _AnimationMixerContribution<T> computedContrib /*used for unweighted*/)
{
    TTE_ASSERT(numFinalValues > 0, "Invalid arguments"); // should have exited before call
    // weighted blend = ON. blend different priorities. in reverse order to accumulate from last.
    
    U32 curIndex = numFinalValues - 1;
    Float curContribution = finalPriorityValues[curIndex].Contribution;
    outValue.Value = finalPriorityValues[curIndex].Value;
    
    for(I32 i = curIndex-1; curIndex >= 0; curIndex--)
    {
        curContribution += finalPriorityValues[i].Contribution;
        PerformMix<T>::Blend(outValue.Value, finalPriorityValues[i].Value,
                                        finalPriorityValues[i].Contribution / fmaxf(curContribution, 0.000001f));
    }
    PerformMix<T>::Finalise(outValue);
    outValue.Contribution = curContribution;
}

// MIXER COMPUTE <T>

template<>
void AnimationMixer_ComputeValue<SkeletonPose>(AnimationMixerBase* Mixer, ComputedValue<SkeletonPose>* pOutput, const Float *Contribution);

template<typename T>
inline void AnimationMixer_ComputeValue(AnimationMixerBase* Mixer, ComputedValue<T>* pOutput, const Float *Contribution)
{
    // IF CHANGING, UPDATE SKELETON POSE VERSION.
    if(Mixer->_Flags.Test(AnimationValueFlags::MIXER_DIRTY))
        Mixer->CleanMixer();
    
    Memory::FastBufferAllocator allocator{};
    _AnimationMixerContribution<T> maxContrib{*pOutput, allocator};
    
    if(!Mixer->_ActiveValuesHead)
    {
        maxContrib.AssignTo(*pOutput);
        return; // nothing
    }
    
    ComputedValue<T>* pThisPriorityComputedValues = (ComputedValue<T>*)allocator.Alloc(sizeof(ComputedValue<T>) * Mixer->_ActiveCount,
                                                                                       alignof(ComputedValue<T>));
    ComputedValue<T>* pFinalComputedValues = (ComputedValue<T>*)allocator.Alloc(sizeof(ComputedValue<T>) * Mixer->_ActiveCount,
                                                                                alignof(ComputedValue<T>));
    
    Ptr<AnimationMixerValueInfo> pCurrent = Mixer->_ActiveValuesHead;
    
    Bool bAdditive = true;
    Float additive = 1.0f;
    I32 lastPriority = pCurrent->Controller->GetPriority();
    U32 numFinalValues = 0;
    T additiveFinal{};
    
    _AnimationMixerContribution<T> accumulatedContributionThisPriority{*pOutput, allocator};
    U32 numAccumulatedThisPriority = 0;
    Float minAdditiveThisPriority = 1.0f;
    
    while(pCurrent)
    {
        I32 currentPriority = pCurrent->Controller->GetPriority();
        
        bAdditive = Mixer->_AdditivePriority > pCurrent->Controller->GetPriority();
        
        if(!pCurrent->Controller->IsPaused() && pCurrent->Controller->IsActive())
        {
            ComputedValue<T>* pThisComputed = pThisPriorityComputedValues + numAccumulatedThisPriority;
            new (pThisComputed) ComputedValue<T>();
            PerformMix<T>::PrepareValue(*pThisComputed, pOutput->Value);
            // HERE. (see wdc exe line 333 in transform mixer template fn. derivative value if abs node?? and some controller flag 0x10000000 ??)
            pCurrent->AnimatedValue->ComputeValue(pThisComputed, pCurrent->Controller, kDefaultContribution);
            if(pCurrent->Controller->IsMirrored())
            {
                PerformMix<T>::Mirror(pOutput->Value);
                PerformMix<T>::Mirror(pOutput->AdditiveValue);
            }
            if(maxContrib.GreaterAll(0.00001f, true, *pThisComputed))
            {
                numAccumulatedThisPriority++;
                accumulatedContributionThisPriority.Append(*pThisComputed);
            }
            else
            {
                pThisComputed->~ComputedValue<T>();
            }
            if(bAdditive)
            {
                minAdditiveThisPriority = fminf(minAdditiveThisPriority, 1.0f + (Contribution[0] *
                                                                                 ((pCurrent->Controller->GetAdditiveMix() * pThisComputed->AdditiveMix) - 1.0f)));
                if(additive > 0.00001f)
                {
                    additiveFinal = PerformMix<T>::BlendAdditive(additiveFinal, pThisComputed->AdditiveValue, additive);
                }
            }
        }
        
        if(lastPriority != currentPriority || pCurrent->Next == nullptr)
        {
            // finalise and accumulate all currents
            additive *= minAdditiveThisPriority;
            minAdditiveThisPriority = 1.0f;
            if(numAccumulatedThisPriority > 0)
            {
                new (pFinalComputedValues + numFinalValues) ComputedValue<T>();
                AnimationMixerAccumulater<T>::AccumulateCurrent(pThisPriorityComputedValues,
                                                            numAccumulatedThisPriority, pFinalComputedValues[numFinalValues], accumulatedContributionThisPriority);
                maxContrib.Max(pFinalComputedValues[numFinalValues]);
                numFinalValues++;
                for(U32 i = 0; i < numAccumulatedThisPriority; i++)
                {
                    pThisPriorityComputedValues[i].~ComputedValue();
                }
                if(maxContrib.GreaterAll(0.9999f, true) && additive < 0.00001f)
                    break; // no point continuing
            }
            accumulatedContributionThisPriority.Clear();
            numAccumulatedThisPriority = 0;
        }
    
        lastPriority = currentPriority;
        pCurrent = pCurrent->Next;
    }
    if(numFinalValues > 0 && maxContrib.GreaterAll(0.00001f, false))
    {
        AnimationMixerAccumulater<T>::AccumulateFinal(pFinalComputedValues, numFinalValues, *pOutput, maxContrib);
    }
    else
    {
        maxContrib.AssignTo(*pOutput);
    }
    for(U32 i = 0; i < numFinalValues; i++)
    {
        pFinalComputedValues[i].~ComputedValue();
    }
    pOutput->AdditiveMix = additive;
    pOutput->AdditiveValue = std::move(additiveFinal);
}

// MIXER<TRANSFORM>

inline void
AnimationMixerAccumulater<Transform>::AccumulateCurrent(const ComputedValue<Transform>* pCurrentValues, U32 numCurrentValues,
                                                ComputedValue<Transform>& finalValue, _AnimationMixerContribution<Transform> priorityContrib)
{
    Transform accumulated{};
    Float denominatorV = 1.0f / fmaxf(priorityContrib.ContributionV, 0.000001f);
    Float denominatorQ = 1.0f / fmaxf(priorityContrib.ContributionQ, 0.000001f);
    Float maxContribV = 0.0f, maxContribQ = 0.0f;
    for(U32 i = 0; i < numCurrentValues; i++)
    {
        PerformMix<Vector3>::Blend(accumulated._Trans, pCurrentValues[i].Value._Trans, denominatorV * pCurrentValues[i].Vec3Contrib);
        PerformMix<Quaternion>::Blend(accumulated._Rot, pCurrentValues[i].Value._Rot, denominatorQ * pCurrentValues[i].QuatContrib);
        maxContribV = MAX(maxContribV, pCurrentValues[i].Vec3Contrib);
        maxContribQ = MAX(maxContribQ, pCurrentValues[i].QuatContrib);
    }
    finalValue.AdditiveValue = Transform{};
    finalValue.Value = accumulated;
    finalValue.AdditiveMix = 1.0f;
    finalValue.Vec3Contrib = maxContribV;
    finalValue.QuatContrib = maxContribQ;
}
inline void
AnimationMixerAccumulater<Transform>::AccumulateFinal(const ComputedValue<Transform>* finalPriorityValues, U32 numFinalValues,
                                                      ComputedValue<Transform>& outValue, _AnimationMixerContribution<Transform> computedContrib)
{
    TTE_ASSERT(numFinalValues > 0, "Invalid arguments"); // should have exited before call
    // weighted blend = ON. blend different priorities. in reverse order to accumulate from last.
    
    U32 curIndex = numFinalValues - 1;
    Float curContributionV = finalPriorityValues[curIndex].Vec3Contrib;
    Float curContributionQ = finalPriorityValues[curIndex].QuatContrib;
    outValue.Value = finalPriorityValues[curIndex].Value;
    
    for(I32 i = curIndex-1; i >= 0; i--)
    {
        curContributionV += finalPriorityValues[i].Vec3Contrib;
        curContributionQ += finalPriorityValues[i].QuatContrib;
        PerformMix<Vector3>::Blend(outValue.Value._Trans, finalPriorityValues[i].Value._Trans,
                                        finalPriorityValues[i].Vec3Contrib / fmaxf(curContributionV, 0.000001f));
        PerformMix<Quaternion>::Blend(outValue.Value._Rot, finalPriorityValues[i].Value._Rot,
                                                     finalPriorityValues[i].QuatContrib / fmaxf(curContributionQ, 0.000001f));
    }
    PerformMix<Transform>::Finalise(outValue.Value);
    outValue.Vec3Contrib = curContributionV;
    outValue.QuatContrib = curContributionQ; // not max, weighted blend. total contribution accumulated.
}

// MIXER<T>

template<typename T>
AnimationMixer<T>::AnimationMixer(String name) : AnimationMixerBase(name)
{
    
}

template<typename T> const std::type_info& AnimationMixer<T>::GetValueType() const
{
    return typeid(T);
}

// priorities are already sorted through clean mixer.
template<typename T> inline void
AnimationMixer<T>::ComputeValue(void *Value, Ptr<PlaybackController> C/*ignore controller, multiple exist - mixing!*/,
                                const Float *Contribution)
{
    AnimationMixer_ComputeValue<T>(this, (ComputedValue<T>*)Value, Contribution);
}

template<typename T> inline void
AnimationMixer<T>::AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float *cscale)
{
    AnimationMixerBase::AddValue(pController, pAnimValue, cscale);
}

