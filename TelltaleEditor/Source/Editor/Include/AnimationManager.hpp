#pragma once

#include <Core/Config.hpp>
#include <Core/Context.hpp>

#include <Common/Animation.hpp>
#include <Common/Skeleton.hpp>
#include <Common/Scene.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Symbols.hpp>

#include <unordered_map>

// ANIMATION MANAGER => CORE TYPES AND PROVIDES ANIMATION FUNCTIONALITY: BOTH ANMS AND CHORES

class PlaybackController
{
    
    enum class Flag
    {
        PAUSED = 0x1,
        ACTIVE = 0x2,
        LOOPING = 0x4,
        MIRRORED = 0x8, // play it mirrored
    };
    
public:
    
    PlaybackController(String name, Scene* scene, Float length);
    
    ~PlaybackController();
    
    PlaybackController(PlaybackController&&) = default;
    
    // Get name of controller
    const String& GetName();
    
    // Pause it. No updates will occur
    void Pause(Bool bPaused);
    
    // Set the priority of it
    void SetPriority(I32 P);
    
    // Test if paused
    Bool IsPaused() const;
    
    // Test if active, ie running
    Bool IsActive() const;
    
    // Get priority
    I32 GetPriority() const;
    
    // Get current time into
    Float GetTime() const;
    
    // Advance time. Mostly done internally.
    void Advance(Float elapsedTimeSeconds);
    
    // Start the controller.
    void Play();
    
    // Stop the controller
    void Stop();
    
    // Get owning scene
    Scene* GetScene() const;
    
    // Get contribution of this controller.
    Float GetContribution() const;
    
    // Get time scale. Default 1.0
    Float GetTimeScale() const;
    
    Float GetAdditiveMix() const;
    
    Bool IsLooping() const;
    
    Bool IsMirrored() const;
    
    void SetAdditiveMix(Float mix);
    
    // Sets the time scale of this controller
    void SetTimeScale(Float tc);
    
    // Sets the contribution of whatever this controller is playing. Must be between 0 and 1.
    void SetContribution(Float c);
    
    void SetTime(Float t); // set time in animation. 0 to length.
    
    void SetTimeFractional(Float tp); // set time percent. 0 to 1.
    
    void SetLooping(Bool bLoop);
    
    void SetMirrored(Bool bMirrored);
    
    Float GetLength() const;
    
private:
    
    void _UpdateFlag(Bool bWanted, Flag f);
    
    String _Name;
    I32 _Priority = 0;
    Flags _ControllerFlags;
    Float _Time = 0.0f; // current time in animation
    Float _Length = 0.0f; // length
    Float _Contribution = 1.0f; // current contribution. can be used to fade.
    Float _TimeScale = 1.0f;
    Float _AdditiveMix = 1.0f;
    
    Ptr<PlaybackController> _Next;
    Scene* _Scene;
    
    friend class AnimationManager;
    friend class AnimationMixerBase;
    friend class AnimationValueInterface;
    
};

enum class AnimationManagerApplyMask
{
    SKELETON = 0x1,
    MOVER = 0x2,
    //MESH = 0x4,
    //PROPS = 0x8,
    ALL = 0xFFFF,
};

// Manages animation and performing them. This is attached as ObjData to nodes (ie per agent)
class AnimationManager
{
public:
    
    AnimationManager();
    ~AnimationManager();
    
    Ptr<PlaybackController> FindAnimation(const String& name); // find running animation by name
    
    /*
     Prepares an animation, returning the playback controller for it. The attached agent will be played on.
     Ensure to call Play to actually play it.
     */
    Ptr<PlaybackController> ApplyAnimation(Scene* scene, Ptr<Animation> pAnimation);
    
    // Called by scene to update animation. Set apply flags to ALL default.
    void UpdateAnimation(U64 frameNumber, Flags toApplyFlags, Float secsElapsed);
    
    // coerce legacy vec3s/quats into out transform keyframes
    static void CoerceLegacyKeyframes(Ptr<KeyframedValue<Transform>>& out, Ptr<KeyframedValue<Vector3>>& vec3s, Ptr<KeyframedValue<Quaternion>>& quats);
    
private:
    
    void _SetNode(Ptr<Node>& node);
    
    WeakPtr<Node> _AttachedNode;
    std::vector<Ptr<PlaybackController>> _Controllers;
    
    friend class Scene;
    
};

// Runtime skeleton instance which can be animated using
class SkeletonInstance
{
public:
    
    struct SklNode : Node
    {
        
        Transform CurrentTransform; // MESH bone transform (not actual node current transform)
        Transform RestTransform; // MESH LOCAL bone rest transform
        Transform CachedGlobalRestTransform; // global rest transform relative to root of skeleton
        Float BoneLength;
        Vector3 BoneDir; // local bone direction
        Vector3 BoneScaleAdjust;
        Quaternion BoneRotationAdjust;
        
    };
    
    // Additional node. These are created to create look at animations for example. They have a transform mixer and membership
    // map of which joints to move, eg the head a lot, torso a bit and eyes a bit.
    struct RuntimeSklNode : SklNode
    {
        std::unordered_map<String, Float> ResourceGroupMembership;
        Ptr<AnimationMixerBase> TransformMixer;
    };
    
    // Builds this skeleton instance from the skeleton file
    void Build(Handle<Skeleton> hSkeleton, const SceneAgent& agent);
    
    // Adds an animated value, typically done internally from the animation manager.
    void AddAnimatedValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimatedValue);
    
    SkeletonInstance() = default;
    ~SkeletonInstance();
    
    SklNode* GetNode(const String& name);
    
    RuntimeSklNode* GetAddRuntimeNode(const String& name, Bool bCreate);
    
    void SortRuntimeNodes();
    
    inline Ptr<Skeleton>& GetSkeleton()
    {
        return _Skeleton;
    }
    
    inline const Matrix4* GetCurrentPose() const
    {
        return _CurrentPose;
    }
    
private:
    
    void _UpdateAnimation(U64 frameNumber);
    
    void _UpdateNode(SklNode& node, const Transform& value, const Transform& additiveValue, Float vecContrib, Float quatContrib, Bool bAdditive);
    
    void _ApplySkeletonInstanceRestPose(Scene* pScene, Ptr<Node>& rootNode, I32 nodeIndex);
    
    void _ComputeSkeletonInstancePoseMatrices(Ptr<Node> node);
    
    Transform _ComputeGlobalRestTransform(const Ptr<Node>& node);
    
    void _UpdateSkeletonCachedRestPoses();
    
    Ptr<Node> _GetAgentNode(); // agent node
    
    Ptr<Skeleton> _Skeleton;
    Ptr<Node> _RootNode;
    std::vector<SklNode> _Nodes; // flat. CANNOT BE UPDATED UNLESS IN BUILD.
    std::vector<RuntimeSklNode> _RuntimeNodes;
    U64 _LastUpdatedFrame = UINT64_MAX;
    Matrix4* _CurrentPose = nullptr; // current frame pose final transforms for each bone
    
    Ptr<AnimationMixerBase> _RootMixer, _PoseMixer; // mix animation blends. pose is whole pose (normal), and root is movement.
    
    friend class Scene;
    friend class AnimationManager;
    
};

// Mover is a type of animated value. It manages animating movement of agents, the agent transforms.
class Mover
{
public:
    
    // Adds an animated value, typically done internally from the animation manager.
    void AddAnimatedValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimatedValue);
    
    void SetNode(Ptr<Node> pNode);
    
private:
    
    void _AddAnimatedTransformValue(Ptr<PlaybackController> pController, Ptr<KeyframedValue<Transform>> pAnimatedValue);
    
    Ptr<AnimationMixerBase>& _SelectMoverMixer(AnimationValueInterface* pInterface);
    
    friend class AnimationManager;
    
    void _Update(Float secsElapsed);
    
    Ptr<Node> _AgentNode;
    Transform CurrentAbsoluteTransform;
    Ptr<AnimationMixerBase> _AbsoluteNodeMixer, _RelativeNodeMixer;
    
};

#include <Common/AnimationTemplates.inl>
