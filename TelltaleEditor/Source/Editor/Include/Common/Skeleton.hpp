#pragma once

#include <Core/Config.hpp>
#include <Core/Math.hpp>
#include <Meta/Meta.hpp>
#include <Core/Math.hpp>
#include <Resource/ResourceRegistry.hpp>

#include <map>

struct SkeletonEntry
{
    
    String JointName; // entry name
    String ParentJointName; // parent entry name.
    I32 ParentIndex; // can be -1 if root node.
    
    Vector3 LocalPosition; // local position in mesh
    Quaternion LocalRotation; // local rotation in mesh
    // Transform RestTransform; // rest Xform. Always zero. Checked with all games. This is the *RUNTIME* global rest transform calculated from locals.
    
    Vector3 GlobalTranslationScale;
    Vector3 LocalTranslationScale;
    Vector3 AnimTranslationScale;
    
    std::map<String, Float> ResourceGroupMembership; // contrib?
    
    // TODO OTHER INFORMATION IN THIS CLASS IN NEWER GAMES
    I32 MirrorBoneIndex = -1; // this isnt in old games. what to do here?
    
};

// A skeleton resource describes bone layouts for a mesh in terms of its vertex groupings.
class Skeleton : public HandleableRegistered<Skeleton>
{
public:
    
    static constexpr CString ClassHandle = "Handle<Skeleton>";
    static constexpr CString Class = "Skeleton";
    static constexpr CString Extension = "skl";
    
    Skeleton(Ptr<ResourceRegistry> reg);
    
    inline const std::vector<SkeletonEntry>& GetEntries() const
    {
        return _Entries;
    }
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    virtual void FinaliseNormalisationAsync() override;

    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::SKELETON;
    }

    Skeleton(Skeleton&&);
    Skeleton(const Skeleton&);
    
    U32 GetSerial() const; // unique ID
    
private:
    
    static std::atomic<U32> _sSerial;
    
    friend class SkeletonAPI;
    friend class Animation;
    
    std::vector<SkeletonEntry> _Entries;
    U32 _Serial; // unique ID
    
};

struct SkeletonPose
{
    Transform* Entries;
};

// ACTUAL ENGINE IMPL STUFF FOR FUTURE: (using SoA, will split and need extra code). might make a cpp for this eventually
//  future use if we want to separate like actual engine skeleton pose calculation with SIMD intrinsics to batch groups of 4.
// would replace ComputedValueSkeletonPose type.

/*
struct SkeletonPose
{
    
    struct Entry
    {
        Float v[4]{0.0f, 0.0f, 0.0f, 0.0f};
    };
    
    Entry* Entries; // SoA layout, 7 Vector4s per group of 4 bones
    
    SkeletonPose() : Entries(nullptr) {}
    
    // Set the transform of a single bone at index i
    inline void SetTransform(U32 i, const Transform& src)
    {
        U32 lane = i & 3;             // which of the 4 lanes in a SIMD register
        U32 groupIndex = i / 4;       // which group of 4 bones
        U32 baseIndex = 7 * groupIndex;
        
        Entries[baseIndex + 0].v[lane] = src._Trans.x;
        Entries[baseIndex + 1].v[lane] = src._Trans.y;
        Entries[baseIndex + 2].v[lane] = src._Trans.z;
        Entries[baseIndex + 3].v[lane] = src._Rot.x;
        Entries[baseIndex + 4].v[lane] = src._Rot.y;
        Entries[baseIndex + 5].v[lane] = src._Rot.z;
        Entries[baseIndex + 6].v[lane] = src._Rot.w;
    }
    
    // Get the transform of a single bone at index i
    inline Transform GetTransform(U32 i) const
    {
        Transform out{};
        U32 lane = i & 3;
        U32 groupIndex = i / 4;
        U32 baseIndex = 7 * groupIndex;
        
        out._Trans.x = Entries[baseIndex + 0].v[lane];
        out._Trans.y = Entries[baseIndex + 1].v[lane];
        out._Trans.z = Entries[baseIndex + 2].v[lane];
        out._Rot.x   = Entries[baseIndex + 3].v[lane];
        out._Rot.y   = Entries[baseIndex + 4].v[lane];
        out._Rot.z   = Entries[baseIndex + 5].v[lane];
        out._Rot.w   = Entries[baseIndex + 6].v[lane];
        
        return out;
    }
};

*/


/*
 enum class SkeletonPoseContributionState
 {
 ALL_ZERO,
 ALL_ONE,
 DEFAULT,
 };
 
 // Specialised template for above
 template<>
 struct ComputedValue<SkeletonPose>
 {
 
 SkeletonPose Value, AdditiveValue;
 const Float* InputContribution = nullptr;
 Float *Contribution = nullptr, *AdditiveMix = nullptr; // for each bone
 Ptr<Skeleton> Skl;
 U32 NumBones = 0;
 SkeletonPoseContributionState ContribState = SkeletonPoseContributionState::ALL_ZERO;
 SkeletonPoseContributionState AdditiveContribState = SkeletonPoseContributionState::ALL_ZERO;
 
 void AllocateFromFastBuffer(Memory::FastBufferAllocator& fastBufferAllocator, Bool bWithAdditive);
 
 // Specialisation for skeleton pose
 template<>
 struct AnimationMixerAccumulater<SkeletonPose>
 {
 
 static void AccumulateCurrent(const ComputedValue<SkeletonPose>* pCurrentValues, U32 numCurrentValues,
 ComputedValue<SkeletonPose>& finalValue, Ptr<Skeleton>& pSkeleton,
 U32 numBones, const Float* Contribution, Memory::FastBufferAllocator& outer);
 
 static void AccumulateFinal(const ComputedValue<SkeletonPose>* finalPriorityValues, U32 numFinalValues,
 ComputedValue<SkeletonPose>& outValue, Ptr<Skeleton>& pSkeleton, U32 numBones);
 
 };
 
 
 void ComputedValue<SkeletonPose>::AllocateFromFastBuffer(Memory::FastBufferAllocator &fastBufferAllocator, Bool bWithAdditive)
 {
 U32 size = (7 * sizeof(Float) * NumBones) // SoA: 4 for rotation (xywz) and 3 for translation (xyz) + (NumBones * 4); // skeletonpose entries value + contrib
if(bWithAdditive)
size *= 2; // additive float buf and additive value entries
U8* memory = fastBufferAllocator.Alloc(size, 16); // try allow simd :(
// .... SIMD
}

void
AnimationMixerAccumulater<SkeletonPose>::AccumulateCurrent(const ComputedValue<SkeletonPose> *pCurrentValues, U32 numCurrentValues,
                                                           ComputedValue<SkeletonPose> &finalValue, Ptr<Skeleton> &pSkeleton, U32 numBones, const Float *Contribution, Memory::FastBufferAllocator& outer)
{
    memset(&finalValue, 0, sizeof(ComputedValue<SkeletonPose>));
    finalValue.AdditiveContribState = SkeletonPoseContributionState::ALL_ONE;
    finalValue.InputContribution = kDefaultContribution;
    finalValue.Skl = pSkeleton;
    finalValue.NumBones = (U32)pSkeleton->GetEntries().size();
}

void
AnimationMixerAccumulater<SkeletonPose>::AccumulateFinal(const ComputedValue<SkeletonPose>* finalPriorityValues, U32 numFinalValues,
                                                         ComputedValue<SkeletonPose>& outValue, Ptr<Skeleton>& pSkeleton, U32 numBones)
{
    
}

// MIX COMPUTED SKELETON POSE
template<> inline void
AnimationMixer_ComputeValue<SkeletonPose>(AnimationMixerBase* Mixer, ComputedValue<SkeletonPose>* pOutPose, Float Time, const Float *Contribution)
{
    
}
 // use contrib state enum
 if(CompareFloatEpsilon(runningContrib, (Float)output.NumBones, 0.000001f))
 output.ContribState = SkeletonPoseContributionState::ALL_ONE; // approx. help optimise and speed up
 else
 output.ContribState = CompareFloatEpsilon(runningContrib, 0.0f) ? SkeletonPoseContributionState::ALL_ZERO : SkeletonPoseContributionState::DEFAULT;
 
 };
 
 template<>
 struct PerformMix<SkeletonPose> : PerformMixBase<SkeletonPose>
 {
 
 static inline SkeletonPose CatmullRom(const SkeletonPose& p0, const SkeletonPose& p1, const SkeletonPose& p2, const SkeletonPose& p3, Float t)
 {
 return {};
 }
 
 static inline void PrepareValue(ComputedValue<SkeletonPose>& out, const SkeletonPose& value)
 {
 
 }
 
 static inline void OutputValue(ComputedValue<SkeletonPose>& out, Bool bAdditive, const SkeletonPose& value, Float contribution)
 {
 
 }
 
 static inline SkeletonPose Blend(const SkeletonPose& start, const SkeletonPose& end, Float t)
 {
 return {};
 }
 
 static inline SkeletonPose BlendAdditive(const SkeletonPose& cur, const SkeletonPose& adding, Float mix)
 {
 return {};
 }
 
 };
 
 */
