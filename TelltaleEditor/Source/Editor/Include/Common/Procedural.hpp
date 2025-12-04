#pragma once

#include <Core/Config.hpp>
#include <Meta/Meta.hpp>
#include <Common/MetaOperations.hpp>


///
/// Procedural Look At, Eyes, and more. These are runtime generated animations for character look ats between each other. They are rarely stored as individual files, but rather
/// are normally embedded into chores.
///

class Procedural_LookAt : public HandleableRegistered<Procedural_LookAt>, public MetaOperationsBucket_ChoreResource
{
public:
    
    struct Constraint
    {
        Float MaxLeftRightAngle = 30.0f;
        Float MinLeftRightAngle = -30.0f;
        Float MaxUpAngle = 35.0f;
        Float MinUpAngle = - 20.f;
        Float LeftRightFixedOffset = 0.0f;
        Float UpDownFixedOffset = 0.0f; // or Offsset
    };
    
    enum Flag
    {
        USE_PRIVATE_NODE = 1,
        ROTATE_HOST_NODE = 2,
    };
    
    enum ComputeStage
    {
        IDLE_LOOK_ATS = 0,
        DIALOG_LOOK_ATS = 1,
        FINAL = 2,
    };
    
    static constexpr CString ClassHandle = "Handle<Procedural_LookAt>";
    static constexpr CString Class = "Procedural_LookAt";
    static constexpr CString Extension = "look";
    
    inline Procedural_LookAt(Ptr<ResourceRegistry> reg) : HandleableRegistered<Procedural_LookAt>(std::move(reg)) {}

    Procedural_LookAt(const Procedural_LookAt& rhs);
    Procedural_LookAt(Procedural_LookAt&& rhs);
    
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    virtual void FinaliseNormalisationAsync() override;
    
    virtual void GetRenderParameters(Vector3& bgColourOut, CString& iconName) const override;
    
    virtual void AddToChore(const Ptr<Chore>& pChore, ChoreResource& resource) override;

    virtual void Attach(const Ptr<Chore>& pChore, ChoreResource& resource) override;
    
    virtual Float GetLength() const override;
    
    inline virtual CommonClass GetCommonClassType() override
    {
        return CommonClass::PROCEDURAL_LOOKAT;
    }

    void SetHostNode(String node);
    void SetTargetNode(String node);
    void SetTargetAgent(String agent);
    void SetTargetOffset(Vector3 offset);
    
private:

    Meta::ClassInstance _LookAtProperties;
    
    String _HostNode;
    String _TargetNode;
    String _TargetAgent;
    Flags _Flags;
    Vector3 _TargetOffset;

    //AnimOrChore _XAxisChore, _YAxisChore;
    //std::vector<Constraint> _Constaints;
    //ComputeStage _Stage;
    
    //Float _MaxAngleIncrement; // LEGACY
    
    friend class ProceduralAPI;
    
};

// RUNTIME DATA, attached as obj data to obj owner PlaybackController
struct Procedural_LookAt_InstanceData
{
    
};
