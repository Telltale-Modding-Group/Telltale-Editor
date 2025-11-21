#pragma once

#include <Core/Config.hpp>
#include <Core/Math.hpp>
#include <Meta/Meta.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Common/MetaOperations.hpp>

#include <Common/Skeleton.hpp>

#include <type_traits>

// ======================================================== ANIMATION ENUMS
// ========================================================

class PlaybackController;

enum class AnimationValueFlags : U32
{
    DISABLED = 1, // disabled value
    //TIME_BEHAVIOUR = 2,
    //WEIGHT_BEHAVIOUR = 4,
    MOVER_ANIMATION = 16, // node transform changes. common! moving the agent while also deforming vertices (skeletal)
    PROPERTY_ANIMATION = 32, // animates and changes a property value during
    //TEXTURE_MATRIX_ANIMATION = 64,
    //AUDIO_DATA_ANIMATION = 128,
    //DONT_OPTIMIZE = 256,
    HOMOGENEOUS = 512,
    //MIXER_SCALED = 1024,
    //MIXER_HOMOGENEOUS = 2048,
    //STYLE_ANIMATION = 0x1000,
    //PROP_FORCE_UPDATE = 0x2000,
    //MIXER_OWNED = 0x4000,
    MIXER_DIRTY = 0x8000,
    ADDITIVE = 0x10000,
    //EXTERNALLY_OWNED = 0x20000,
    //DONT_MIX_PAUSE_CONTROLLERS = 0x40000,
    //RUNTIME_ANIMATION = 0x80000,
    //TRANSIENT_ANIMATION = 0x100000,
    //TOOL_ONLY = 0x200000,
    //KEYED_ATTACHMENT_ANIMATION = 0x400000,
    //MIXER_WEIGHTED_BLEND = 0x800000,
};

enum class AnimationValueType
{
    NONE = 0x0,
    TIME,
    CONTRIBUTION,
    SKELETAL,
    MOVER,
    PROPERTY,
    ADDITIVE_MASK,
    TARGETED_MOVER,
    SKELETON_POSE,
    SKELETON_ROOT_ANIMATION,
    
    TEXTURE_START = 0x10,
    
    TEXTURE_MOVE_U,
    TEXTURE_MOVE_V,
    TEXTURE_SCALE_U,
    TEXTURE_SCALE_V,
    TEXTURE_ROTATE,
    TEXTURE_OVERRIDE,
    TEXTURE_VISIBILITY,
    TEXTURE_SHEAR_U,
    TEXTURE_SHEAR_V,
    TEXTURE_SHEAR_ORIGIN_U,
    TEXTURE_SHEAR_ORIGIN_V,
    TEXTURE_ROTATE_ORIGIN_U,
    TEXTURE_ROTATE_ORIGIN_V,
    TEXTURE_SCALE_ORIGIN_U,
    TEXTURE_SCALE_ORIGIN_V,
    
    TEXTURE_END = 0x1F,
    
    AUDIO_START = 0x20,
    
    AUDIO_PAN = 0x21,
    AUDIO_PITCH = 0x22,
    AUDIO_REVERB_WET = 0x23,
    AUDIO_REVERB_DRY = 0x24,
    AUDIO_LOW_PASS = 0x25,
    AUDIO_HIGH_PASS = 0x26,
    AUDIO_EVENT_PARAM = 0x27,
    AUDIO_SURROUND_DIR = 0x28,
    AUDIO_SURROUND_EXTENT = 0x29,
    AUDIO_LOW_FREQ_SEND = 0x2A,
    AUDIO_LANGRES_VOLUME = 0x2B,
    
    AUDIO_END = 0x3F,
    
    VERTEX_START = 0x40,
    
    VERTEX_POSITION = 0x41,
    VERTEX_NORMAL = 0x42,
    
    VERTEX_END = 0x60,
    
    AUTO_ACT = 0x61,
    EXPLICIT_COMPOUND_VALUE = 0x62,
    
    
};

struct AnimationValueTypeDesc
{
    AnimationValueType Type;
    CString Name;
};

constexpr AnimationValueTypeDesc AnimTypeDescs[] =
{
    {AnimationValueType::TIME,"kAnimationValueTypeTime"},
    {AnimationValueType::CONTRIBUTION,"kAnimationValueTypeContribution"},
    {AnimationValueType::SKELETAL,"kAnimationValueTypeSkeletal"},
    {AnimationValueType::MOVER,"kAnimationValueTypeMover"},
    {AnimationValueType::PROPERTY,"kAnimationValueTypeProperty"},
    {AnimationValueType::ADDITIVE_MASK,"kAnimationValueTypeAdditiveMask"},
    {AnimationValueType::TARGETED_MOVER,"kAnimationValueTypeTargetedMover"},
    {AnimationValueType::SKELETON_POSE,"kAnimationValueTypeSkeletonPose"},
    {AnimationValueType::SKELETON_ROOT_ANIMATION,"kAnimationValueTypeSkeletonRootAnim"},
    {AnimationValueType::TEXTURE_MOVE_U,"kAnimationValueTypeTextureMoveU"},
    {AnimationValueType::TEXTURE_MOVE_V,"kAnimationValueTypeTextureMoveV"},
    {AnimationValueType::TEXTURE_SCALE_U,"kAnimationValueTypeTextureScaleU"},
    {AnimationValueType::TEXTURE_SCALE_V,"kAnimationValueTypeTextureScaleV"},
    {AnimationValueType::TEXTURE_ROTATE,"kAnimationValueTypeTextureRotate"},
    {AnimationValueType::TEXTURE_OVERRIDE,"kAnimationValueTypeTextureOverride"},
    {AnimationValueType::TEXTURE_VISIBILITY,"kAnimationValueTypeTextureVisibility"},
    {AnimationValueType::TEXTURE_SHEAR_U,"kAnimationValueTypeTextureShearU"},
    {AnimationValueType::TEXTURE_SHEAR_V,"kAnimationValueTypeTextureShearV"},
    {AnimationValueType::TEXTURE_SHEAR_ORIGIN_U,"kAnimationValueTypeTextureShearOriginU"},
    {AnimationValueType::TEXTURE_SHEAR_ORIGIN_V,"kAnimationValueTypeTextureShearOriginV"},
    {AnimationValueType::TEXTURE_SCALE_ORIGIN_U,"kAnimationValueTypeTextureScaleOriginU"},
    {AnimationValueType::TEXTURE_SCALE_ORIGIN_V,"kAnimationValueTypeTextureScaleOriginV"},
    {AnimationValueType::TEXTURE_ROTATE_ORIGIN_U,"kAnimationValueTypeTextureRotateOriginU"},
    {AnimationValueType::TEXTURE_ROTATE_ORIGIN_V,"kAnimationValueTypeTextureRotateOriginV"},
    {AnimationValueType::AUDIO_PAN,"kAnimationValueTypeAudioPan"},
    {AnimationValueType::AUDIO_PITCH,"kAnimationValueTypeAudioPitch"},
    {AnimationValueType::AUDIO_LOW_PASS,"kAnimationValueTypeAudioLowPassFilter"},
    {AnimationValueType::AUDIO_HIGH_PASS,"kAnimationValueTypeAudioHighPassFilter"},
    {AnimationValueType::AUDIO_REVERB_DRY,"kAnimationValueTypeAudioReverbDry"},
    {AnimationValueType::AUDIO_REVERB_WET,"kAnimationValueTypeAudioReverbWet"},
    {AnimationValueType::AUDIO_EVENT_PARAM,"kAnimationValueTypeAudioFMODParameter"},
    {AnimationValueType::AUDIO_SURROUND_DIR,"kAnimationValueTypeAudioSurroundDir"},
    {AnimationValueType::AUDIO_SURROUND_EXTENT,"kAnimationValueTypeAudioSurroundExtent"},
    {AnimationValueType::AUDIO_LOW_FREQ_SEND,"kAnimationValueTypeAudioLowFreqSend"},
    {AnimationValueType::AUDIO_LANGRES_VOLUME,"kAnimationValueTypeAudioLanguageResourceVolume"},
    {AnimationValueType::VERTEX_NORMAL,"kAnimationValueTypeVertexNormal"},
    {AnimationValueType::VERTEX_POSITION,"kAnimationValueTypeVertexPosition"},
    {AnimationValueType::AUTO_ACT,"kAnimationValueTypeAutoAct"},
    {AnimationValueType::EXPLICIT_COMPOUND_VALUE,"kAnimationValueTypeExplicitCompoundValue"},
    {AnimationValueType::NONE,""},
    
};

inline const AnimationValueTypeDesc& GetAnimationTypeDesc(AnimationValueType type);

// ======================================================== BASE ANIMATION VALUE
// ========================================================

// Computed value
template<typename T>
struct ComputedValue
{
    T Value, AdditiveValue;
    Float Contribution, AdditiveMix;
    
    inline void AllocateFromFastBuffer(Memory::FastBufferAllocator& fastBufferAllocator, Bool bWithAdditive) {}
    inline void Reset(Bool bAdditive) {}
    
};

// Base class for animated values. This is an interface so mostly just provides functionality
class AnimationValueInterface
{
public:
    
    // Return if this value is empty. Defaults to false.
    virtual Bool IsEmptyValue() const;
    
    // Compute the value into Value at the given time in this animated value
    virtual void ComputeValue(void* Value, Ptr<PlaybackController> Controller, const Float* Contribution) = 0;
    
    virtual const std::type_info& GetValueType() const = 0;
    
    virtual void CleanMixer(); // clean mixer
    
    // IsDeferred, GetSampleValues, CastToMixer, CastToMixerNode, GetNonHomogeneousNames, CleanMixer
    // ComputeDerivativeValue, AddValue, RemoveValue

    inline AnimationValueInterface(String name) :
        _Name(std::move(name)), _Flags(), _Type(AnimationValueType::NONE) { }

    inline const String& GetName() const
    {
        return _Name;
    }

    inline Flags GetFlags() const
    {
        return _Flags;
    }

    inline Bool GetAdditive() const
    {
        return _Flags.Test(AnimationValueFlags::ADDITIVE);
    }

    inline AnimationValueType GetType() const
    {
        return _Type;
    }

    inline virtual Float GetMaxTime() const
    {
        return 0.0f;
    }

    inline virtual void GetNonHomogeneousNames(std::set<String>& names) const
    {
        if (!_Flags.Test(AnimationValueFlags::HOMOGENEOUS))
            names.insert(_Name);
    }
    
    virtual ~AnimationValueInterface() = default;
    
    inline void SetAdditive(Bool bAdditive)
    {
        if (bAdditive)
            _Flags.Add(AnimationValueFlags::ADDITIVE);
        else
            _Flags.Remove(AnimationValueFlags::ADDITIVE);
    }

    inline void SetDisabled(Bool bDisable)
    {
        if (bDisable)
            _Flags.Add(AnimationValueFlags::DISABLED);
        else
            _Flags.Remove(AnimationValueFlags::DISABLED);
    }
    
protected:
    
    String _Name;
    Flags _Flags;
    AnimationValueType _Type;
    
    friend class Animation;
    friend class AnimationAPI;
    friend class TransitionRemapper;
    friend class SkeletonInstance;
    friend class Mover;
    friend class SkeletonPoseCompoundValue;
    
};

// ======================================================== KEYFRAMED<T>
// ========================================================

enum class KeyframedSampleFlag
{
    INTERPOLATE_TO_NEXT_KEY = 1,
};

// Keyframed value
template<typename T>
class KeyframedValue : public AnimationValueInterface
{
public:
    
    struct Sample
    {

        Float Time; // not incremental, the time offset of this sample
        Flags SampleFlags;
        T Value;
        
        inline Sample() : Time(0.0f), SampleFlags((U32)KeyframedSampleFlag::INTERPOLATE_TO_NEXT_KEY), Value{} {}
        
    };
    
    inline KeyframedValue(String name);
    
    // last argument used for coersion. 0 samples will translate to using minvalue
    inline void ComputeValueKeyframed(void* Value, Float Time,
                               const Float* Contribution, Flags& outSelectedFlags, Bool bEmptyContributes);
    
    inline virtual void ComputeValue(void* Value, Ptr<PlaybackController> Controller,
                                              const Float* Contribution) override;
    
    inline virtual Float GetMaxTime() const override;
    
    inline virtual const std::type_info& GetValueType() const override;
    
    inline std::vector<Sample>& GetSamples()
    {
        return _Samples;
    }

    inline Sample& InsertSample(Float time, T value)
    {
        auto it = std::lower_bound(_Samples.begin(), _Samples.end(), time, [](const Sample& s, Float t) { return s.Time < t; });
        it = _Samples.insert(it, Sample{});
        it->Time = time;
        it->Value = std::move(value);
        return *it;
    }
    
private:
    
    friend class Animation;
    friend class AnimationAPI;
    friend class AnimationManager;
    friend class TransitionRemapper;
    friend class SkeletonPoseCompoundValue;
    friend class Mover;
    
    T _MinValue, _MaxValue;
    std::vector<Sample> _Samples;
    Bool Vector3Coerced = true, QuatCoerced = true;
    
};

// ======================================================== TRANSITION MAP
// ========================================================

// TRANSITION MAP COMMON CLASS (TELLTALE .TMAP FILES)

struct TransitionRemapper // for a given bone
{
    
    I32 Priority = 0;
    KeyframedValue<Float> RemapKeys; // key framed bone contribution.
    
    TransitionRemapper();
    
    Float Remap(Float time);
    
};

class TransitionMap : public Handleable
{
public:
    
    TransitionMap(Ptr<ResourceRegistry>);
    
    Float RemapBoneContribution(Float time, SkeletonEntry* pSklEntry, Float controllerContribution);
    
    inline virtual void FinaliseNormalisationAsync() override {}
    
private:
    
    friend class AnimationAPI; // TODO TMAP API
    friend class AnimationManager;
    
    std::map<String, TransitionRemapper> _Remappers; // bone map => remap keys
    
};


// ======================================================== ANIMATION MIXERS - MIXES ANIMATION VALUES WITH WEIGHTS (CONTRIBS)
// ========================================================

class AnimationMixerBase;
class PlaybackController;
extern Float kDefaultContribution[256];

struct AnimationMixerValueInfo
{
    AnimationMixerBase* Owner;
    Ptr<AnimationMixerValueInfo> Next;
    
    Ptr<PlaybackController> Controller;
    Ptr<AnimationValueInterface> AnimatedValue;
    const Float* ContributionScale = kDefaultContribution;
    Ptr<TransitionMap> MixerTransitionMap;
};

// Mixer base class
class AnimationMixerBase : public AnimationValueInterface
{
public:
    
    inline AnimationMixerBase(String Name);
    
    // add value
    virtual void AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float* cscale);
    
    virtual void RemoveValue(Ptr<PlaybackController> pController); // remove all with controller
    
    virtual void CleanMixer() override; // clean to reset
    
    // Impl Compute Value is still abstract
    
    template<typename U>
    friend void AnimationMixer_ComputeValue(AnimationMixerBase* Mixer, ComputedValue<U>* pComputedValue, const Float *Contribution);
    
protected:
    
    Ptr<AnimationMixerValueInfo> _FindMixerInfo(Ptr<PlaybackController> pController, Bool bAdditive);
    
    void _SortValues();
    
    Ptr<AnimationMixerValueInfo> _ActiveValuesHead, _ActiveValuesTail, _PassiveValuesHead, _PassiveValuesTail;
    Ptr<AnimationMixerBase> Parent;
    I32 _AdditivePriority; // minimum priority to be additive instead
    U32 _ActiveCount = 0, _PassiveCount = 0;
    
    friend class Mover;
    
};

// Specialisable. Does actual mixing.

template<typename T>
inline void AnimationMixer_ComputeValue(AnimationMixerBase* Mixer, ComputedValue<T>* pComputedValue, const Float *Contribution);

// Templated animation mixer.
template<typename T>
class AnimationMixer : public AnimationMixerBase
{
public:
    
    inline AnimationMixer(String Name);
    
    inline void ComputeValue(void *Value, Ptr<PlaybackController> Controller, const Float *Contribution) override;
    
    inline virtual void AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float* cscale) override;
    
    inline virtual const std::type_info& GetValueType() const override;
    
};

// Compound (all bones) skeleton pose. Similar to normal mixer but uses optimised skeleton pose SoA layouts in real engine for SIMD batching. Will not bother for now.
class SkeletonPoseCompoundValue : public AnimationMixerBase
{
public:
    
    SkeletonPoseCompoundValue(String Name);
    
    void ComputeValue(void *Value, Ptr<PlaybackController> Controller, const Float *Contribution) override;
    
    virtual void GetNonHomogeneousNames(std::set<String>& names) const override;
    
    Bool HasValue(const String& Name) const;
    
    virtual void AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float* cscale) override;
    
    virtual const std::type_info& GetValueType() const override;
    
private:
    
    void AddSkeletonValue(Ptr<AnimationValueInterface> pValue, Float contribution);
    
    void _ResolveSkeleton(const Skeleton* pSkeleton, Bool bMirrored); // maps bone named animation values to 

    struct Entry
    {
        Ptr<AnimationValueInterface> Value;
        Float Contribution;
        U32 BoneIndex;
    };
    
    std::vector<Entry> _Values, _AdditiveValues;
    
    U32 _ResolvedSkeletonSerial = 0;
    Bool _ResolvedSkeletonIsMirrored = false;
    
    friend class AnimationMixer<SkeletonPose>;

};

// Mixes skeleton poses. Most work delegates to the template specialisation for it (all in manager cpp).
template<>
class AnimationMixer<SkeletonPose>  : public AnimationMixerBase
{
public:
    
    AnimationMixer(String N);
    
    void ComputeValue(void *Value, Ptr<PlaybackController> Controller, const Float *Contribution) override;
    
    virtual void AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float* cscale) override;
    
    virtual void GetNonHomogeneousNames(std::set<String>& names) const override;
    
    virtual const std::type_info& GetValueType() const override;
    
};

template<>
struct ComputedValue<Transform>
{
    
    inline void AllocateFromFastBuffer(Memory::FastBufferAllocator& fastBufferAllocator, Bool bWithAdditive) {}
    inline void Reset(Bool bAdditive) {}
    
    Transform Value, AdditiveValue;
    Float AdditiveMix;
    Float Vec3Contrib, QuatContrib;
};

template<>
struct ComputedValue<SkeletonPose>
{
    
    Skeleton* Skl;
    Float* Contribution, *AdditiveMix;
    SkeletonPose Value, AdditiveValue; // for each bone
    U32 NumEntries;
    
    void AllocateFromFastBuffer(Memory::FastBufferAllocator& fastBufferAllocator, Bool bWithAdditive);
    
};

// Default type impls for below
template<typename T>
struct PerformMixBase
{
    
    static inline void Finalise(T& value) {} // Nothing
    
    static inline void Mirror(T& value) {} // Nothing here. Mirrors it.
    
    // interpolate using catmull-rom curve
    static inline T CatmullRom(const T& p0, const T& p1, const T& p2, const T& p3, Float t)
    {
        Float t2 = t * t;
        Float t3 = t2 * t;
        
        return 0.5f * ((2.0f * p1) + (-p0 + p3) * t + (2.0f * p0 - 5.f * p1 + 4.f * p3 - p2) * t2 +
                         (-p0 + 3.f * p1 - 3.f * p3 + p2) * t3);
    }
    
    // prepare value. sometimes overriden.
    static inline void PrepareValue(ComputedValue<T>& out, const T& value)
    {
        out.Value = value;
        out.Contribution = 0.0f;
        out.AdditiveMix = 1.0f;
    }
    
    // outputs the value with the given contribution.
    static inline void OutputValue(ComputedValue<T>& out, Bool bAdditive, const T& value, Float contribution)
    {
        if(bAdditive)
        {
            out.Contribution = 0.0f;
            out.Value = (T)(value * contribution);
        }
        else
        {
            out.Value = value;
            out.Contribution = contribution;
        }
    }
    
    // main usage. blends T
    static inline void Blend(T& start, const T& end, Float t) // important function. blends this type. slerp/lerp.
    {
        start = (T)(start + ((end - start) * t));
    }
    
    // additive blend. only used for vec3, float and quat.
    static inline T BlendAdditive(const T& cur, const T& adding, Float mix)
    {
        return T{}; // default nothing
    }
    
};

// Handles *all* type specific mixing and interpolation for a given T. Override base class functions.
template<typename T>
struct PerformMix : PerformMixBase<T>
{
    
};

// helper for transform optimisation pair
template<typename T>
struct _AnimationMixerContribution
{
    
    Float Contribution = 0.0f;
    
    inline _AnimationMixerContribution(const ComputedValue<T>& finalVal, Memory::FastBufferAllocator& a) {}
    
    inline Bool GreaterAll(Float test, Bool bAll_elseAny)
    {
        return Contribution > test;
    }
    
    inline Bool GreaterAll(Float test, Bool bAll_elseAny, const ComputedValue<T>& value)
    {
        return value.Contribution > test;
    }
    
    inline void Clear()
    {
        Contribution = 0.0f;
    }
    
    inline void Append(ComputedValue<T>& val)
    {
        Contribution += val.Contribution;
    }
    
    inline void AssignTo(ComputedValue<T>& val)
    {
        val.Contribution = Contribution;
    }
    
    inline void Max(const ComputedValue<T>& val)
    {
        Contribution = fmaxf(val.Contribution, Contribution);
    }
    
};

// helper for transform optimisation pair
template<>
struct _AnimationMixerContribution<Transform>
{
    
    Float ContributionV = 0.0f, ContributionQ = 0.0f;
    
    inline _AnimationMixerContribution(const ComputedValue<Transform>& finalVal, Memory::FastBufferAllocator& a) {}
    
    inline Bool GreaterAll(Float test, Bool bAll_elseAny)
    {
        return bAll_elseAny ? (ContributionV > test && ContributionQ > test) : (ContributionV > test || ContributionQ > test);
    }
    
    inline Bool GreaterAll(Float test, Bool bAll_elseAny, const ComputedValue<Transform>& val)
    {
        return bAll_elseAny ? (val.Vec3Contrib > test && val.QuatContrib > test) : (val.Vec3Contrib > test || val.QuatContrib > test);
    }
    
    inline void Clear()
    {
        ContributionV = ContributionQ = 0.0f;
    }
    
    inline void Append(ComputedValue<Transform>& val)
    {
        ContributionV += val.Vec3Contrib;
        ContributionQ += val.QuatContrib;
    }
    
    inline void AssignTo(ComputedValue<Transform>& val)
    {
        val.Vec3Contrib = ContributionV;
        val.QuatContrib = ContributionQ;
    }
    
    inline void Max(const ComputedValue<Transform>& val)
    {
        ContributionV = fmaxf(val.Vec3Contrib, ContributionV);
        ContributionQ = fmaxf(val.QuatContrib, ContributionQ);
    }
    
};

// Accumulates contributions. PerformMix<T> does type specific handling.
template<typename T>
struct AnimationMixerAccumulater
{
    
    static inline void AccumulateCurrent(const ComputedValue<T>* pCurrentValues, U32 numCurrentValues,
                                          ComputedValue<T>& finalValue, _AnimationMixerContribution<T> contributionAtPriority);
    
    static inline void AccumulateFinal(const ComputedValue<T>* finalPriorityValues, U32 numFinalValues,
                                       ComputedValue<T>& outValue, _AnimationMixerContribution<T> computedContrib);
    
};

// Specialisation for transform. Does nothing. Work is delegated to vec3+quat mixers seperately
template<>
struct AnimationMixerAccumulater<Transform>
{
    
    static inline void AccumulateCurrent(const ComputedValue<Transform>* pCurrentValues, U32 numCurrentValues,
                                         ComputedValue<Transform>& finalValue, _AnimationMixerContribution<Transform> p);
    
    static inline void AccumulateFinal(const ComputedValue<Transform>* finalPriorityValues, U32 numFinalValues,
                                       ComputedValue<Transform>& outValue, _AnimationMixerContribution<Transform> computed);
    
};

template<>
struct AnimationMixerAccumulater<SkeletonPose>
{
    
    static void AccumulateCurrent(const ComputedValue<SkeletonPose>* pCurrentValues, U32 numCurrentValues,
                                 ComputedValue<SkeletonPose>& finalValue, Skeleton* pSkeleton, U32 numBones, Float* Contributions, Memory::FastBufferAllocator& al);
    
    static void AccumulateFinal(const ComputedValue<SkeletonPose>* finalPriorityValues, U32 numFinalValues,
                                       ComputedValue<SkeletonPose>& outValue, U32 numBones, Float* ComputedContribution);
    
};

// ======================================================== ANIMATION CORE CLASS
// ========================================================

// ANIMATION COMMON CLASS (TELLTALE .ANM FILES)

class Animation : public HandleableRegistered<Animation>, public MetaOperationsBucket_ChoreResource
{
public:
    
    static constexpr CString ClassHandle = "Handle<Animation>";
    static constexpr CString Class = "Animation";
    static constexpr CString Extension = "anm";
    
    inline Animation(Ptr<ResourceRegistry> reg) : HandleableRegistered<Animation>(std::move(reg)) {}
    
    Animation(const Animation& rhs) = default;
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    virtual void FinaliseNormalisationAsync() override;
    
    inline const String& GetName() const
    {
        return _Name;
    }
    
    inline virtual Float GetLength() const override
    {
        return _Length;
    }

    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::ANIMATION;
    }

    inline std::vector<Ptr<AnimationValueInterface>>& GetAnimatedValues() 
    {
        return _Values;
    }
    
    inline Bool HasValue(Symbol name) const
    {
        for(const auto& val: _Values)
        {
            if(Symbol(val->GetName()) == name)
            {
                return true;
            }
        }
        return false;
    }

    virtual void GetRenderParameters(Vector3& bgColourOut, CString& iconName) const override;
    
    virtual void AddToChore(const Ptr<Chore>& pChore, String myName) override;
    
private:
    
    friend class AnimationAPI;
    friend class AnimationManager;
    
    String _Name;
    Float _Length;
    
    std::vector<Ptr<AnimationValueInterface>> _Values;
    
};
