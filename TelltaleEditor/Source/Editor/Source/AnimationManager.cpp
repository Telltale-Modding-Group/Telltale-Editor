#include <AnimationManager.hpp>
#include <Common/Scene.hpp>

// ========================================= MOVER (AGENT TRANSFORM) ANIMATION =========================================

void Mover::SetNode(Ptr<Node> pNode)
{
    _AgentNode = std::move(pNode);
}

void Mover::_Update(Float secsElapsed)
{
    if(secsElapsed != 0.0f && _AgentNode)
    {
        // blah blah walk animator stuff eventually here AND blend graph stuff
        Transform relativeXform{};
        Transform absoluteXform{};
        Transform agentXform = _AgentNode->LocalTransform;
        if(_RelativeNodeMixer)
        {
            ComputedValue<Transform> computed{};
            _RelativeNodeMixer->ComputeValue(&computed, nullptr, kDefaultContribution);
            PerformMix<Quaternion>::Blend(relativeXform._Rot, computed.Value._Rot, computed.QuatContrib);
            PerformMix<Vector3>::Blend(relativeXform._Trans, computed.Value._Trans, computed.Vec3Contrib);
            relativeXform = relativeXform * computed.AdditiveValue;
            agentXform = agentXform * relativeXform;
        }
        if(_AbsoluteNodeMixer)
        {
            ComputedValue<Transform> computed{};
            _AbsoluteNodeMixer->ComputeValue(&computed, nullptr, kDefaultContribution);
            absoluteXform = agentXform;
            PerformMix<Quaternion>::Blend(absoluteXform._Rot, computed.Value._Rot, computed.QuatContrib);
            PerformMix<Vector3>::Blend(absoluteXform._Trans, computed.Value._Trans, computed.Vec3Contrib);
            absoluteXform = absoluteXform * computed.AdditiveValue;
            CurrentAbsoluteTransform = absoluteXform;
            agentXform = absoluteXform;
        }
        Scene::UpdateNodeLocalTransform(_AgentNode, agentXform, false);
    }
}

void Mover::AddAnimatedValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue)
{
    const Bool bSeparated = Meta::GetInternalState().GetActiveGame().Caps[GameCapability::SEPARATE_ANIMATION_TRANSFORM];
    Bool bNonTransform = false;
    Ptr<KeyframedValue<Vector3>> pVec3{};
    Ptr<KeyframedValue<Quaternion>> pQuat{};
    Ptr<KeyframedValue<Transform>> pTransform{};
    if(bSeparated)
    {
        pVec3 = std::dynamic_pointer_cast<KeyframedValue<Vector3>>(pAnimValue);
        pQuat = std::dynamic_pointer_cast<KeyframedValue<Quaternion>>(pAnimValue);
        bNonTransform = pVec3 == nullptr && pQuat == nullptr;
    }
    else
    {
        pTransform = std::dynamic_pointer_cast<KeyframedValue<Transform>>(pAnimValue);
        bNonTransform = pTransform == nullptr;
    }
    if(bNonTransform)
    {
        TTE_ASSERT(false, "The Animated value %s is not a transform! Only transforms (or if separated, quat/vec pairs) can be used for movers",
                   pAnimValue->GetName().c_str()); // or game cap flag is wrong for game.
        return;
    }
    else
    {
        if(pAnimValue->GetFlags().Test(AnimationValueFlags::MIXER_DIRTY))
            pAnimValue->CleanMixer();
        if(bSeparated)
        {
            Ptr<KeyframedValue<Transform>> coerced{};
            Ptr<AnimationMixerBase>& pMixer = _SelectMoverMixer(pAnimValue.get());
            auto it = pMixer->_ActiveValuesHead;
            Bool actives = true;
            for(;;)
            {
                if(!it)
                {
                    if(actives)
                    {
                        it = pMixer->_PassiveValuesHead;
                        actives = false;
                        continue;
                    }
                    else break;
                }
                coerced = std::dynamic_pointer_cast<KeyframedValue<Transform>>(it->AnimatedValue);
                // maybe skeleton pose compress keys??? everything should go to transforms.
                if(coerced && ((!coerced->Vector3Coerced && pVec3) || (!coerced->QuatCoerced && pQuat)))
                    break;
                coerced = {};
                it = it->Next;
            }
            
            if(!coerced)
            {
                coerced = TTE_NEW_PTR(KeyframedValue<Transform>, MEMORY_TAG_ANIMATION_DATA, pAnimValue->GetName());
                coerced->Vector3Coerced = coerced->QuatCoerced = false;
                _AddAnimatedTransformValue(pController, coerced);
            }
            
            coerced->_Flags += pAnimValue->_Flags;
            AnimationManager::CoerceLegacyKeyframes(coerced, pVec3, pQuat);
        }
        else
        {
            _AddAnimatedTransformValue(pController, pTransform);
        }
    }
}

Ptr<AnimationMixerBase>& Mover::_SelectMoverMixer(AnimationValueInterface* pAnimatedValue)
{
    if(pAnimatedValue->GetName() == kAnimationRelativeNode)
    {
        if(!_RelativeNodeMixer)
        {
            _RelativeNodeMixer = TTE_NEW_PTR(AnimationMixer<Transform>, MEMORY_TAG_ANIMATION_DATA, "Mover Relative Node Mixer");
            TTE_ATTACH_DBG_STR(_RelativeNodeMixer.get(), "Relative Node Animation Mixer Transform");
        }
        return _RelativeNodeMixer;
    }
    else if(pAnimatedValue->GetName() == kAnimationAbsoluteNode)
    {
        if(!_AbsoluteNodeMixer)
        {
            _AbsoluteNodeMixer = TTE_NEW_PTR(AnimationMixer<Transform>, MEMORY_TAG_ANIMATION_DATA, "Mover Absolute Node Mixer");
            TTE_ATTACH_DBG_STR(_AbsoluteNodeMixer.get(), "Absolute Node Animation Mixer Transform");
        }
        return _AbsoluteNodeMixer;
    }
    else
    {
        TTE_ASSERT(false, "Animated value '%s' was not a relative or absolute node!", pAnimatedValue->GetName().c_str());
    }
    return _AbsoluteNodeMixer; // !!
}

void Mover::_AddAnimatedTransformValue(Ptr<PlaybackController> pController, Ptr<KeyframedValue<Transform>> pAnimatedValue)
{
    _SelectMoverMixer(pAnimatedValue.get())->AddValue(pController, pAnimatedValue, kDefaultContribution);
}

// ========================================= SKELETAL ANIMATION =========================================

Float kDefaultContribution[256]{};

void SceneModule<SceneModuleType::SKELETON>::OnSetupAgent(SceneAgent *pAgentGettingCreated)
{
    if(Skl.GetObject().GetCRC64() == 0)
    {
        Meta::ClassInstance hSklInst = PropertySet::Get(pAgentGettingCreated->OwningScene->GetAgentProps(pAgentGettingCreated->NameSymbol), kSkeletonFileSymbol, true, pAgentGettingCreated->OwningScene->GetRegistry());
        if(hSklInst)
        {
            Meta::ExtractCoercableInstance(Skl, hSklInst);
        }
        else
        {
            TTE_LOG("WARNING: Agent %s inherits from skeleton module but has no skeleton file key. Ignoring skeleton.", pAgentGettingCreated->Name.c_str());
        }
    }
    if (Skl.GetObject().GetCRC64() != 0)
    {
        SkeletonInstance* pSkeletonInst = pAgentGettingCreated->AgentNode->CreateObjData<SkeletonInstance>("");
        pSkeletonInst->Build(Skl, *pAgentGettingCreated);
    }
    else
    {
        TTE_LOG("WARNING: Agent %s skeleton file could not be found", pAgentGettingCreated->Name.c_str());
    }
}

void SceneModule<SceneModuleType::SKELETON>::OnModuleRemove(SceneAgent* pAttachedAgent)
{
    pAttachedAgent->AgentNode->RemoveObjData<SkeletonInstance>("");
}

void SkeletonInstance::AddAnimatedValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimatedValue)
{
    Ptr<AnimationMixerBase> pSelectedMixer{};
    SklNode* pFoundNode=nullptr;
    RuntimeSklNode* pRTNode = nullptr;
    if(pAnimatedValue->GetType() != AnimationValueType::SKELETON_POSE)
    {
        pFoundNode = GetNode(pAnimatedValue->GetName());
        if(!pFoundNode)
        {
            RuntimeSklNode* pRTNode = GetAddRuntimeNode(pAnimatedValue->GetName(), false);
            if(pRTNode)
            {
                if(!pRTNode->TransformMixer)
                {
                    TTE_ASSERT(false, "Impl needed"); // find template froxm animated type (Make Createmixer from AVI?)
                }
                pSelectedMixer = pRTNode->TransformMixer;
            }
            else
            {
                TTE_LOG("Could not find appropriate mixer for %s!", pAnimatedValue->GetName().c_str());
                return;
            }
        }
        else if(pAnimatedValue->GetType() == AnimationValueType::SKELETON_ROOT_ANIMATION)
        {
            if(!_RootMixer)
            {
                _RootMixer = TTE_NEW_PTR(AnimationMixer<Transform>, MEMORY_TAG_ANIMATION_DATA, "RootNodeSkeletonPose");
                _RootMixer->_Type = AnimationValueType::SKELETON_ROOT_ANIMATION;
            }
            pSelectedMixer = _RootMixer;
        }
        // set homogeneous depending if panimatedvalue has the flag
    }
    if(pAnimatedValue->GetType() == AnimationValueType::SKELETON_POSE || !pSelectedMixer)
    {
        // get non homo names and make depending for each entry
        if(!_PoseMixer)
        {
            _PoseMixer = TTE_NEW_PTR(AnimationMixer<SkeletonPose>, MEMORY_TAG_ANIMATION_DATA, "SkeletonPose");
            _PoseMixer->_Type = AnimationValueType::SKELETON_POSE;
        }
        pSelectedMixer = _PoseMixer;
    }
    pSelectedMixer->AddValue(pController, pAnimatedValue, kDefaultContribution);
}

SkeletonInstance::RuntimeSklNode* SkeletonInstance::GetAddRuntimeNode(const String &name, Bool bCreate)
{
    for(auto& node: _RuntimeNodes)
    {
        if(CompareCaseInsensitive(node.Name, name))
        {
            return &node;
        }
    }
    if(bCreate)
    {
        RuntimeSklNode* pNode = &_RuntimeNodes.emplace_back();
        pNode->Name = name;
        pNode->AgentName = this->_RootNode->AgentName;
        return pNode;
    }
    return nullptr;
}

SkeletonInstance::SklNode* SkeletonInstance::GetNode(const String &name)
{
    for(auto& node: _Nodes)
    {
        if(CompareCaseInsensitive(node.Name, name))
        {
            return &node;
        }
    }
    return nullptr;
}

Ptr<Node> SkeletonInstance::_GetAgentNode()
{
    return _RootNode->Parent.lock();
}

void SkeletonInstance::_ApplySkeletonInstanceRestPose(Scene* pScene, Ptr<Node>& rootNode, I32 nodeIndex)
{
    if(nodeIndex < 0)
    {
        nodeIndex = 0;
        for(auto& node: _Nodes)
        {
            if(node.Parent.lock() == rootNode)
                _ApplySkeletonInstanceRestPose(pScene, rootNode, nodeIndex);
            nodeIndex++;
        }
    }
    else
    {
        SkeletonInstance::SklNode& node = _Nodes[nodeIndex];
        const SkeletonEntry& entry = _Skeleton->GetEntries()[nodeIndex];
        
        Transform local{entry.LocalRotation, entry.LocalPosition}; // relative to parent
        node.BoneLength = entry.LocalPosition.Magnitude();
        node.BoneDir = Vector3::Normalize(entry.LocalPosition) * node.BoneLength;
        if(node.BoneLength <= 0.00001f)
        {
            node.BoneScaleAdjust = Vector3::Identity;
            node.BoneRotationAdjust = Quaternion::kIdentity;
        }
        else
        {
            node.BoneScaleAdjust = Vector3(node.BoneLength);
            Vector3 animDir = Vector3::Normalize(entry.LocalPosition / entry.AnimTranslationScale);
            node.BoneRotationAdjust = Quaternion::Rotate(entry.LocalPosition, animDir);
        }

        Ptr<Node> nodePtr = TTE_PROXY_PTR(&node, Node);
        node.RestTransform = local;
        Scene::UpdateNodeLocalTransform(nodePtr, local, true);
        
        node.CurrentTransform._Rot = local._Rot;
        node.CurrentTransform._Trans = (local._Trans / node.BoneScaleAdjust) * node.BoneRotationAdjust.Conjugate();
        
        for (I32 i = 0; i < _Skeleton->GetEntries().size(); ++i)
        {
            if (_Skeleton->GetEntries()[i].ParentIndex == nodeIndex)
                _ApplySkeletonInstanceRestPose(pScene, rootNode, i);
        }
    }
}

void SkeletonInstance::Build(Handle<Skeleton> hSkeleton, const SceneAgent& agent)
{
    // TODO lua callbacks for scripting API.
    Ptr<Skeleton> skl = hSkeleton.GetObject(agent.OwningScene->GetRegistry(), true);
    _RootNode = TTE_NEW_PTR(Node, MEMORY_TAG_ANIMATION_DATA);
    _RootNode->AgentName = agent.Name;
    _RootNode->Name = kSkeletonInstanceNodeName;
    agent.OwningScene->AttachNode(_RootNode, agent.AgentNode, false, false);
    _Nodes.reserve(skl->GetEntries().size());
    _Nodes.clear();
    for(auto& entry: skl->GetEntries())
    {
        SklNode& instNode = _Nodes.emplace_back();
        instNode.AgentName = agent.Name;
        instNode.Name = entry.JointName;
    }
    U32 node = 0;
    for(auto& entry: skl->GetEntries())
    {
        SklNode& instNode = _Nodes[node];
        if(entry.ParentIndex <= -1)
            agent.OwningScene->AttachNode(TTE_PROXY_PTR(&instNode, Node), _RootNode, false, false);
        else
            agent.OwningScene->AttachNode(TTE_PROXY_PTR(&instNode, Node),
                                          TTE_PROXY_PTR(&_Nodes[entry.ParentIndex], Node), false, false);
        ++node;
    }
    _Skeleton = std::move(skl);
    _ApplySkeletonInstanceRestPose(agent.OwningScene, _RootNode, -1);
    _UpdateSkeletonCachedRestPoses();
    if(!_CurrentPose)
    {
        _CurrentPose = (Matrix4*)TTE_ALLOC(sizeof(Matrix4) * _Nodes.size(), MEMORY_TAG_TEMPORARY);
        memset(_CurrentPose, 0, sizeof(Matrix4) * _Nodes.size());
        for(U32 i = 0; i < _Skeleton->GetEntries().size(); i++)
            _CurrentPose[i] = Matrix4::Identity();
    }
}

void SkeletonInstance::_UpdateNode(SklNode &node, const Transform& value, const Transform& additiveValue, Float vecContrib, Float quatContrib, Bool bAdditive)
{
    Transform v = value;
    
    // Blend transforms
    Transform blended = node.CurrentTransform;
    PerformMix<Quaternion>::Blend(blended._Rot, v._Rot, quatContrib);
    PerformMix<Vector3>::Blend(blended._Trans, v._Trans, vecContrib);
    
    // Apply additive layer (if any)
    /*if (bAdditive)
     {
     blended._Trans += additiveValue._Trans * blended._Rot;
     blended._Rot = blended._Rot * additiveValue._Rot;
     }*/
    
    // Save blended transform
    node.CurrentTransform = blended;
    
    // Apply bone scale and rotation adjust
    Vector3 scaledTrans = blended._Trans * node.BoneScaleAdjust;
    Vector3 adjustedTrans = scaledTrans * node.BoneRotationAdjust;
    
    // Update scene node's local transform
    Transform finalTransform = Transform(blended._Rot, adjustedTrans);
    finalTransform.Normalise();
    Scene::UpdateNodeLocalTransform(TTE_PROXY_PTR(&node, Node), finalTransform, false);
}

void SkeletonInstance::_UpdateSkeletonCachedRestPoses()
{
    for(auto& node: _Nodes)
    {
        node.CachedGlobalRestTransform = _ComputeGlobalRestTransform(TTE_PROXY_PTR(&node, Node));
    }
}

Transform SkeletonInstance::_ComputeGlobalRestTransform(const Ptr<Node>& node)
{
    Transform result = ((SklNode*)node.get())->RestTransform;
    
    Ptr<Node> current = node->Parent.lock();
    
    while(current && current != _RootNode)
    {
        result = ((SklNode*)current.get())->RestTransform * result;
        current = current->Parent.lock();
    }
    
    return result;
}

void SkeletonInstance::_ComputeSkeletonInstancePoseMatrices(Ptr<Node> node)
{
    if(node != _RootNode)
    {
        SklNode& sklNode = *((SklNode*)node.get());
        U32 index = 0;
        for(auto& n: _Nodes)
        {
            if(n.Name == node->Name)
            {
                break;
            }
            index++;
        }
        TTE_ASSERT(index < _Nodes.size(), "Could not find skeleton node %s which should exist in instance", node->Name.c_str());
        Transform current = Scene::GetNodeWorldTransform(TTE_PROXY_PTR(&sklNode, Node)) / Scene::GetNodeWorldTransform(_RootNode);
        Transform finalBonePose = current / sklNode.CachedGlobalRestTransform;
        finalBonePose.Normalise();
        _CurrentPose[index] = MatrixTransformation(Vector3::Identity, finalBonePose._Rot, finalBonePose._Trans).Transpose(); // can now be sent off to GPU.
    }
    for(Ptr<Node> child = node->FirstChild.lock(); child; child = child->NextSibling.lock())
    {
        _ComputeSkeletonInstancePoseMatrices(child);
    }
}

void SkeletonInstance::_UpdateAnimation(U64 frameNumber)
{
    if(_LastUpdatedFrame != frameNumber)
    {
        Memory::FastBufferAllocator allocator{};
        // Update parents first ensure
        Ptr<Node> pAgentParentNode = _GetAgentNode()->Parent.lock(); // root node parent is agent node, so agent node parent
        if(pAgentParentNode)
        {
            // ensure they are updated first
            SkeletonInstance* pAttachedSkl = pAgentParentNode->GetObjDataByType<SkeletonInstance>();
            if(pAttachedSkl && pAttachedSkl->_LastUpdatedFrame < frameNumber)
                pAttachedSkl->_UpdateAnimation(frameNumber);
        }
        // Animate!
        if(_PoseMixer)
        {
            ComputedValue<SkeletonPose> animatedPose{};
            animatedPose.Skl = _Skeleton.get();
            animatedPose.NumEntries = (U32)_Skeleton->GetEntries().size();
            animatedPose.AllocateFromFastBuffer(allocator, true);
            _PoseMixer->ComputeValue(&animatedPose, nullptr, kDefaultContribution);
            U32 index = 0;
            Bool bAdditive = _PoseMixer->GetAdditive();
            for(auto& node: _Nodes)
            {
                _UpdateNode(node, animatedPose.Value.Entries[index], bAdditive ? animatedPose.AdditiveValue.Entries[index] : Transform{}, animatedPose.Contribution[index],
                            animatedPose.Contribution[index], bAdditive);
                index++;
            }
        }
        _ComputeSkeletonInstancePoseMatrices(_RootNode);
    }
    _LastUpdatedFrame = frameNumber;
}

SkeletonInstance::~SkeletonInstance()
{
    if(_CurrentPose)
    {
        TTE_FREE(_CurrentPose);
        _CurrentPose = nullptr;
    }
}

// ========================================= ANIMATION MANAGER =========================================

void AnimationManager::UpdateAnimation(U64 frameNumber, Flags toApplyFlags, Float elapsed)
{
    if(!_AttachedNode.expired())
    {
        Ptr<Node> pNode = _AttachedNode.lock();
        if(toApplyFlags.Test(AnimationManagerApplyMask::SKELETON))
        {
            SkeletonInstance* pSkl = pNode->GetObjDataByType<SkeletonInstance>();
            if(pSkl)
                pSkl->_UpdateAnimation(frameNumber);
        }
        if(toApplyFlags.Test(AnimationManagerApplyMask::MOVER))
        {
            Mover* pMover = pNode->GetObjDataByType<Mover>();
            if(pMover)
                pMover->_Update(elapsed);
        }
    }
}

Ptr<PlaybackController> AnimationManager::ApplyAnimation(Scene* scene, Ptr<Animation> pAnimation)
{
    if(_AttachedNode.expired())
    {
        TTE_LOG("Cannot play %s as node was not found", pAnimation->GetName().c_str());
        return nullptr;
    }
    
    Ptr<PlaybackController> controller = TTE_NEW_PTR(PlaybackController, MEMORY_TAG_ANIMATION_DATA, pAnimation->GetName(), scene, pAnimation->GetLength());
    Ptr<Node> node = _AttachedNode.lock();
    
    // 1. Find animatable attached types to node
    SkeletonInstance* pSkeleton = node->GetObjDataByType<SkeletonInstance>();
    Mover* pMover = nullptr;
    Bool bTriedFindingMover = false;
    // others todo style anim, prop key anim, mesh vertex anim, mover anim
    
    for(auto& animated: pAnimation->_Values)
    {
        if(animated->GetType() == AnimationValueType::SKELETAL ||
           animated->GetType() == AnimationValueType::SKELETON_POSE ||
           animated->GetType() == AnimationValueType::SKELETON_ROOT_ANIMATION)
        {
            TTE_ASSERT(animated->GetType() == AnimationValueType::SKELETON_POSE, "Animation value type needs to be checked"); // skeletal/root anim i havent seen files using yet.
            if(pSkeleton)
            {
                pSkeleton->AddAnimatedValue(controller, animated);
            }
            else
            {
                TTE_LOG("Skeleton instance not found for animation with skeletal animated values! On node %s",
                        node->Name.c_str());
            }
        }
        else if(animated->GetType() == AnimationValueType::MOVER)
        {
            if(!bTriedFindingMover)
            {
                pMover = node->GetObjData<Mover>("", true);
                bTriedFindingMover = true;
                pMover->SetNode(node);
            }
            pMover->AddAnimatedValue(controller, animated);
        }
        else
        {
            TTE_LOG("WARNING: Animation value type %s not supported yet", GetAnimationTypeDesc(animated->GetType()).Name);
        }
    }
    
    controller->_Length = pAnimation->_Length;
    
    return controller;
}

Ptr<PlaybackController> AnimationManager::FindAnimation(const String &name)
{
    for(auto& controller: _Controllers)
    {
        if(CompareCaseInsensitive(controller->GetName(), name))
        {
            return controller;
        }
    }
    return nullptr;
}

void AnimationManager::_SetNode(Ptr<Node>& node)
{
    Ptr<Node> cur{};
    if(_AttachedNode.expired() || ((cur = _AttachedNode.lock()) == nullptr) || cur == node)
    {
        if(cur != node) // likely could already have been assigned
        {
            _AttachedNode = node;
            node->AttachedScene->_AnimationMgrs.push_back(this);
        }
    }
    else
    {
        TTE_ASSERT(false, "AnimationManager is already attached to a different agent");
    }
}

AnimationManager::AnimationManager()
{

}

AnimationManager::~AnimationManager()
{
    if(!_AttachedNode.expired())
    {
        auto& managers = _AttachedNode.lock()->AttachedScene->_AnimationMgrs;
        for(auto it = managers.begin(); it != managers.end(); it++)
        {
            if(*it == this)
            {
                managers.erase(it);
                break;
            }
        }
    }
}

// ========================================= ANIMATION MIXER =========================================

AnimationMixerBase::AnimationMixerBase(String name) : AnimationValueInterface(name)
{
    _ActiveValuesHead = _ActiveValuesTail = _PassiveValuesHead = _PassiveValuesTail = {};
    _AdditivePriority = 9999999;
}

Ptr<AnimationMixerValueInfo> AnimationMixerBase::_FindMixerInfo(Ptr<PlaybackController> pController, Bool bAdditive)
{
    Ptr<AnimationMixerValueInfo> info = _PassiveValuesHead;
    while (info)
    {
        if (info->Controller == pController)
        {
            if(info->AnimatedValue->GetFlags().Test(AnimationValueFlags::MIXER_DIRTY))
                info->AnimatedValue->CleanMixer();
            if(info->AnimatedValue->GetFlags().Test(AnimationValueFlags::ADDITIVE) == bAdditive)
                return info;
        }
        info = info->Next;
    }
    info = _ActiveValuesHead;
    while (info)
    {
        if (info->Controller == pController)
        {
            if(info->AnimatedValue->GetFlags().Test(AnimationValueFlags::MIXER_DIRTY))
                info->AnimatedValue->CleanMixer();
            if(info->AnimatedValue->GetFlags().Test(AnimationValueFlags::ADDITIVE) == bAdditive)
                return info;
        }
        info = info->Next;
    }
    return {};
}

void AnimationMixerBase::AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float *cscale)
{
    Ptr<AnimationMixerValueInfo> info = TTE_NEW_PTR(AnimationMixerValueInfo, MEMORY_TAG_ANIMATION_DATA);
    info->Controller = pController;
    info->ContributionScale = cscale;
    info->AnimatedValue = pAnimValue;
    info->Owner = this;
    info->Next = _PassiveValuesHead;
    _PassiveValuesHead = info;
    if(!_PassiveValuesTail)
        _PassiveValuesTail = info;
    _Flags.Add(AnimationValueFlags::MIXER_DIRTY);
    _PassiveCount++;
}

void AnimationMixerBase::RemoveValue(Ptr<PlaybackController> pController)
{
    Ptr<AnimationMixerValueInfo> info = _PassiveValuesHead, prev = nullptr;
    
    while (info)
    {
        if (info->Controller == pController)
        {
            if (info == _PassiveValuesHead)
            {
                _PassiveValuesHead = info->Next;
                if (!_PassiveValuesHead)
                    _PassiveValuesTail.reset();
            }
            else
            {
                prev->Next = info->Next;
                if (info == _PassiveValuesTail)
                    _PassiveValuesTail = prev;
            }
            _PassiveCount--;
            info = (prev) ? prev->Next : _PassiveValuesHead;
            continue; // Don't advance prev
        }
        
        prev = info;
        info = info->Next;
    }
    
    info = _ActiveValuesHead;
    prev = nullptr;
    
    while (info)
    {
        if (info->Controller == pController)
        {
            if (info == _ActiveValuesHead)
            {
                _ActiveValuesHead = info->Next;
                if (!_ActiveValuesHead)
                    _ActiveValuesTail = nullptr;
            }
            else
            {
                prev->Next = info->Next;
                if (info == _ActiveValuesTail)
                    _ActiveValuesTail = prev;
            }
            _ActiveCount--;
            info = (prev) ? prev->Next : _ActiveValuesHead;
            continue; // Don't advance prev
        }
        
        prev = info;
        info = info->Next;
    }
    _Flags.Add(AnimationValueFlags::MIXER_DIRTY);
}

void AnimationValueInterface::CleanMixer() {} // do nothing here

void AnimationMixerBase::CleanMixer()
{
    _SortValues();
}

void AnimationMixerBase::_SortValues()
{
    _Flags.Remove(AnimationValueFlags::MIXER_DIRTY);
    
    // Step 1: Merge active + passive into one temporary list
    Ptr<AnimationMixerValueInfo> unsortedHead = _ActiveValuesHead ? _ActiveValuesHead : _PassiveValuesHead;
    if (_ActiveValuesTail)
        _ActiveValuesTail->Next = _PassiveValuesHead;
    _ActiveValuesHead = _ActiveValuesTail = nullptr;
    _PassiveValuesHead = _PassiveValuesTail = nullptr;
    _ActiveCount = _PassiveCount = 0;
    
    // Step 2: Loop through unified list and sort into active/passive
    Ptr<AnimationMixerValueInfo> info = unsortedHead;
    
    I32 minAdditive = 99999999;
    Bool additive = false;
    while (info)
    {
        auto next = info->Next;
        info->Next = nullptr;
        
        auto& animated = info->AnimatedValue;
        if (animated->GetFlags().Test(AnimationValueFlags::MIXER_DIRTY))
            animated->CleanMixer();
        
        Bool isPassive = animated->GetFlags().Test(AnimationValueFlags::DISABLED) || info->Controller->GetContribution() < 0.000001f;
        I32 curPriority = info->Controller->GetPriority();
        
        if (isPassive)
        {
            // Append to passive list
            if (!_PassiveValuesHead)
                _PassiveValuesHead = _PassiveValuesTail = info;
            else
            {
                _PassiveValuesTail->Next = info;
                _PassiveValuesTail = info;
            }
            _PassiveCount++;
        }
        else
        {
            // Insert into active list sorted by priority (descending)
            if (!_ActiveValuesHead || curPriority > _ActiveValuesHead->Controller->GetPriority())
            {
                info->Next = _ActiveValuesHead;
                _ActiveValuesHead = info;
                if (!_ActiveValuesTail)
                    _ActiveValuesTail = info;
            }
            else
            {
                Ptr<AnimationMixerValueInfo> prev = nullptr;
                Ptr<AnimationMixerValueInfo> current = _ActiveValuesHead;
                
                while (current && curPriority <= current->Controller->GetPriority())
                {
                    prev = current;
                    current = current->Next;
                }
                
                prev->Next = info;
                info->Next = current;
                
                if (!current)
                    _ActiveValuesTail = info;
            }
            
            if (animated->GetFlags().Test(AnimationValueFlags::ADDITIVE))
            {
                minAdditive = MIN(minAdditive, curPriority);
                additive = true;
            }
            
            _ActiveCount++;
        }
        
        info = next;
    }
    if(!_ActiveValuesHead)
        this->_Flags.Add(AnimationValueFlags::DISABLED);
    else
        this->_Flags.Remove(AnimationValueFlags::DISABLED);
    if(additive)
        this->_Flags.Add(AnimationValueFlags::ADDITIVE);
    else
        this->_Flags.Remove(AnimationValueFlags::ADDITIVE);
}

// ========================================= PBC =========================================

PlaybackController::PlaybackController(String name, Scene* scene, Float len) : _Name(std::move(name)), _Length(fmaxf(len, 0.0f))
{
    _Scene = scene;
    TTE_ASSERT(scene, "Scene must be specified");
    scene->_Controllers.insert(this);
}

PlaybackController::~PlaybackController()
{
    _Scene->_Controllers.erase(_Scene->_Controllers.find(this));
}

void PlaybackController::Play()
{
    if(!IsActive())
    {
        _ControllerFlags.Add(Flag::ACTIVE);
        _ControllerFlags.Remove(Flag::PAUSED);
        _Time = 0.0f;
        // Callbacks?
    }
}

void PlaybackController::Stop()
{
    if(IsActive())
    {
        _ControllerFlags.Remove(Flag::ACTIVE);
        _ControllerFlags.Remove(Flag::PAUSED);
        _Time = 0.0f;
        // Callbacks
    }
}

void PlaybackController::Pause(Bool bPaused)
{
    if(_ControllerFlags.Test(Flag::ACTIVE) && _ControllerFlags.Test(Flag::PAUSED) != bPaused)
    {
        // Callbacks?
        if(bPaused)
        {
            _ControllerFlags.Add(Flag::PAUSED);
        }
        else
        {
            _ControllerFlags.Remove(Flag::PAUSED);
        }
    }
}

void PlaybackController::Advance(Float elapsedTimeSeconds)
{
    if(IsActive() && !IsPaused())
    {
        // Fades?
        Float newTime = _Time + elapsedTimeSeconds * _TimeScale;
        if(newTime >= _Length)
        {
            if(IsLooping())
            {
                _Time = fmodf(newTime, _Length);
            }
            else
            {
                // On finish callbacks?
                _ControllerFlags.Remove(Flag::ACTIVE);
                _ControllerFlags.Remove(Flag::PAUSED);
                _Time = 0.0f;
            }
        }
        else
        {
            _Time = newTime;
        }
    }
}

void PlaybackController::SetTime(Float t)
{
    if(IsActive())
    {
        _Time = fmaxf(0.0f, fminf(_Length, t));
    }
}

void PlaybackController::SetTimeFractional(Float tp)
{
    SetTime(tp * _Length);
}

void PlaybackController::SetPriority(I32 P)
{
    _Priority = P;
}

Float PlaybackController::GetContribution() const
{
    return _Contribution;
}

Scene* PlaybackController::GetScene() const
{
    return _Scene;
}

const String& PlaybackController::GetName()
{
    return _Name;
}

Bool PlaybackController::IsActive() const
{
    return _ControllerFlags.Test(Flag::ACTIVE);
}

Bool PlaybackController::IsPaused() const
{
    return _ControllerFlags.Test(Flag::PAUSED);
}

I32 PlaybackController::GetPriority() const
{
    return _Priority;
}

Float PlaybackController::GetTime() const
{
    return _Time;
}

Float PlaybackController::GetLength() const
{
    return _Length;
}

Float PlaybackController::GetTimeScale() const
{
    return _TimeScale;
}

void PlaybackController::SetContribution(Float c)
{
    _Contribution = fmaxf(0.000001f, fminf(c, 1.0f));
}

void PlaybackController::SetTimeScale(Float tc)
{
    _TimeScale = fmaxf(tc, 0.000001f);
}

void PlaybackController::SetAdditiveMix(Float mix)
{
    _AdditiveMix = fmaxf(0.0f, fminf(1.0f, mix));
}

Float PlaybackController::GetAdditiveMix() const
{
    return _AdditiveMix;
}

Bool PlaybackController::IsLooping() const
{
    return _ControllerFlags.Test(Flag::LOOPING);
}

Bool PlaybackController::IsMirrored() const
{
    return _ControllerFlags.Test(Flag::MIRRORED);
}

void PlaybackController::_UpdateFlag(Bool bWanted, Flag f)
{
    if(_ControllerFlags.Test(f) != bWanted)
    {
        if(bWanted)
        {
            _ControllerFlags.Add(f);
        }
        else
        {
            _ControllerFlags.Remove(f);
        }
    }
}

void PlaybackController::SetMirrored(Bool bMirror)
{
    _UpdateFlag(bMirror, Flag::MIRRORED);
}

void PlaybackController::SetLooping(Bool bLoop)
{
    _UpdateFlag(bLoop, Flag::LOOPING);
}

// ========================================= SKELETON COMPOUND VALUE MIXER =========================================

SkeletonPoseCompoundValue::SkeletonPoseCompoundValue(String N) : AnimationMixerBase(N)
{
    _Type = AnimationValueType::SKELETON_POSE;
}

Bool SkeletonPoseCompoundValue::HasValue(const String &Name) const
{
    for(auto& v: _Values)
    {
        if(CompareCaseInsensitive(v.Value->GetName(), Name))
            return true;
    }
    for(auto& v: _AdditiveValues)
    {
        if(CompareCaseInsensitive(v.Value->GetName(), Name))
            return true;
    }
    return false;
}

void SkeletonPoseCompoundValue::GetNonHomogeneousNames(std::set<String> &names) const
{
    for(auto& v: _Values)
    {
        v.Value->GetNonHomogeneousNames(names);
    }
    for(auto& v: _AdditiveValues)
    {
        v.Value->GetNonHomogeneousNames(names);
    }
}

void SkeletonPoseCompoundValue::AddSkeletonValue(Ptr<AnimationValueInterface> pValue, Float contribution)
{
    if(contribution > 0.00001f && !HasValue(pValue->GetName()))
    {
        if(pValue->GetAdditive())
            _Flags.Add(AnimationValueFlags::ADDITIVE);
        Entry e{};
        Bool additive = pValue->GetAdditive();
        e.Value = std::move(pValue);
        e.Contribution = contribution;
        if(additive)
            _AdditiveValues.push_back(std::move(e));
        else
            _Values.push_back(std::move(e));
    }
}


void SkeletonPoseCompoundValue::AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float *cscale)
{
    const Bool bSeparated = Meta::GetInternalState().GetActiveGame().Caps[GameCapability::SEPARATE_ANIMATION_TRANSFORM];
    Bool bNonTransform = false;
    // this function delegates mixing to the skeleton pose mixer specialisation template.
    Ptr<KeyframedValue<Vector3>> pVec3{};
    Ptr<KeyframedValue<Quaternion>> pQuat{};
    Ptr<KeyframedValue<Transform>> pTransform{};
    if(bSeparated && pAnimValue->GetType() == AnimationValueType::SKELETON_POSE)
    {
        pVec3 = std::dynamic_pointer_cast<KeyframedValue<Vector3>>(pAnimValue);
        pQuat = std::dynamic_pointer_cast<KeyframedValue<Quaternion>>(pAnimValue);
        bNonTransform = pVec3 == nullptr && pQuat == nullptr;
    }
    else
    {
        pTransform = std::dynamic_pointer_cast<KeyframedValue<Transform>>(pAnimValue);
        bNonTransform = pTransform == nullptr;
    }
    if(bNonTransform)
    {
        TTE_ASSERT(false, "The Animated value %s is not a transform! Only transforms (or if separated, quat/vec pairs) can be used here",
                   pAnimValue->GetName().c_str()); // or game cap flag is wrong for game.
        return;
    }
    else
    {
        if(pAnimValue->GetFlags().Test(AnimationValueFlags::MIXER_DIRTY))
            pAnimValue->CleanMixer();
        if(bSeparated)
        {
            Ptr<KeyframedValue<Transform>> coerced{};
            for(auto& v: pAnimValue->GetAdditive() ? _AdditiveValues : _Values)
            {
                if(CompareCaseInsensitive(v.Value->GetName(), pAnimValue->GetName()))
                {
                    coerced = std::dynamic_pointer_cast<KeyframedValue<Transform>>(v.Value);
                    TTE_ASSERT(coerced, "Animation value inside skeleton pose compound is not a keyframed transform!");
                    // maybe skeleton pose compress keys??? everything should go to transforms.
                    if((!coerced->Vector3Coerced && pVec3) || (!coerced->QuatCoerced && pQuat))
                        break;
                    coerced = {};
                }
            }
            
            if(!coerced)
            {
                coerced = TTE_NEW_PTR(KeyframedValue<Transform>, MEMORY_TAG_ANIMATION_DATA, pAnimValue->GetName());
                coerced->Vector3Coerced = coerced->QuatCoerced = false;
                AddSkeletonValue(coerced, cscale[0]);
            }
            
            coerced->_Flags += pAnimValue->_Flags;
            AnimationManager::CoerceLegacyKeyframes(coerced, pVec3, pQuat);
        }
        else
        {
            AddSkeletonValue(pAnimValue, cscale[0]);
        }
    }
}

void SkeletonPoseCompoundValue::_ResolveSkeleton(const Skeleton* pSkeleton, Bool bMirrored)
{
    if(_ResolvedSkeletonSerial != pSkeleton->GetSerial() || _ResolvedSkeletonIsMirrored != bMirrored)
    {
        for(auto it = _Values.begin(); ; it++)
        {
            if(it == _Values.end())
                it = _AdditiveValues.begin();
            if(it == _AdditiveValues.end())
                break;
            auto& animated = *it;
            I32 index = -1;
            I32 curIndex = 0;
            for(const auto& skl: pSkeleton->GetEntries())
            {
                if(CompareCaseInsensitive(skl.JointName, animated.Value->GetName()))
                {
                    index = curIndex;
                    break;
                }
                curIndex++;
            }
            animated.BoneIndex = (U32)curIndex;
            if(bMirrored && curIndex > -1)
            {
                I32 mirror = pSkeleton->GetEntries()[curIndex].MirrorBoneIndex;
                if(mirror < 0)
                {
                    TTE_LOG("WARNING: Mirror bone index was not specified (likely older game)! Ignoring mirror.. (%s)", animated.Value->GetName().c_str());
                    mirror = curIndex;
                }
                animated.BoneIndex = (U32)mirror;
            }
        }
    }
}

void ComputedValue<SkeletonPose>::AllocateFromFastBuffer(Memory::FastBufferAllocator &fastBufferAllocator, Bool bWithAdditive)
{
    U32 numBones = (U32)Skl->GetEntries().size();
    U32 size = (U32)(sizeof(Transform) * numBones);
    if(bWithAdditive)
        size *= 2;
    size += (4*numBones);
    if(bWithAdditive)
        size += (4*numBones);
    U8* memory = fastBufferAllocator.Alloc((U64)size, 8);
    Value.Entries = (Transform*)memory;
    Contribution = (Float*)(memory + (numBones * sizeof(Transform)));
    if(bWithAdditive)
    {
        AdditiveValue.Entries = (Transform*)(memory + (numBones * (sizeof(Transform) + 4)));
        AdditiveMix = (Float*)(memory + (numBones * (2*sizeof(Transform) + 4)));
    }
    if(bWithAdditive)
    {
        for(U32 i = 0; i < numBones; i++)
        {
            new (Value.Entries + i) Transform();
            new (AdditiveValue.Entries + i) Transform();
        }
    }
    else
    {
        for(U32 i = 0; i < numBones; i++)
        {
            new (Value.Entries + i) Transform();
        }
    }
}

void AnimationMixerAccumulater<SkeletonPose>::AccumulateCurrent(const ComputedValue<SkeletonPose>* pCurrentValues, U32 numCurrentValues,
                                     ComputedValue<SkeletonPose>& finalValue, Skeleton* pSkeleton, U32 numBones, Float* Contributions, Memory::FastBufferAllocator& outer)
{
    Memory::FastBufferAllocator local{};
    Float* denominator = (Float*)local.Alloc(numBones * 4, 4);
    for(U32 i = 0; i < numBones; i++)
    {
        denominator[i] = 1.0f / fmaxf(Contributions[i], 0.000001f);
    }
    for(U32 i = 0; i < numCurrentValues; i++)
    {
        for(U32 b = 0; b < numBones; b++)
        {
            PerformMix<Transform>::Blend(finalValue.Value.Entries[b], pCurrentValues[i].Value.Entries[b], denominator[b]*pCurrentValues[i].Contribution[b]);
            finalValue.Contribution[b] = fmaxf(finalValue.Contribution[b], pCurrentValues[i].Contribution[b]);
        }
    }
    finalValue.AdditiveMix = nullptr;
    finalValue.AdditiveValue = {};
}

void AnimationMixerAccumulater<SkeletonPose>::AccumulateFinal(const ComputedValue<SkeletonPose>* finalPriorityValues, U32 numFinalValues,
                                   ComputedValue<SkeletonPose>& outValue, U32 numBones, Float* ComputedContribution)
{
    Memory::FastBufferAllocator mem{};
    U32 curIndex = numFinalValues - 1;
    Float* curContribution = (Float*)mem.Alloc(numBones*4, 4);
    memcpy(curContribution, finalPriorityValues[curIndex].Contribution, numBones*4);
    memcpy(outValue.Value.Entries, finalPriorityValues[curIndex].Value.Entries, numBones*sizeof(Transform));
    for(I32 i = (I32)curIndex-1; i >= 0; i--) // blend other lowert priority layers
    {
        for(U32 b = 0; b < numBones; b++)
        {
            curContribution[b] += finalPriorityValues[i].Contribution[b];
            PerformMix<Transform>::Blend(outValue.Value.Entries[b], finalPriorityValues[i].Value.Entries[b],
                                         finalPriorityValues[i].Contribution[b]/fmaxf(curContribution[b], 0.000001f));
        }
    }
    memcpy(outValue.Contribution, curContribution, 4*numBones);
}

template<>
void AnimationMixer_ComputeValue<SkeletonPose>(AnimationMixerBase* Mixer, ComputedValue<SkeletonPose>* pOutput, const Float *Contribution)
{
    if(Mixer->_Flags.Test(AnimationValueFlags::MIXER_DIRTY))
        Mixer->CleanMixer();
    
    Memory::FastBufferAllocator allocator{};
    
    if(!Mixer->_ActiveValuesHead)
    {
        return; // nothing
    }
    
    ComputedValue<SkeletonPose>* pThisPriorityComputedValues = (ComputedValue<SkeletonPose>*)allocator.Alloc(sizeof(ComputedValue<SkeletonPose>) * Mixer->_ActiveCount,
                                                                                       alignof(ComputedValue<SkeletonPose>));
    ComputedValue<SkeletonPose>* pFinalComputedValues = (ComputedValue<SkeletonPose>*)allocator.Alloc(sizeof(ComputedValue<SkeletonPose>) * Mixer->_ActiveCount,
                                                                                alignof(ComputedValue<SkeletonPose>));
    
    Ptr<AnimationMixerValueInfo> pCurrent = Mixer->_ActiveValuesHead;
    
    Bool bAdditive = true;
    I32 lastPriority = pCurrent->Controller->GetPriority();
    U32 numFinalValues = 0;
    U32 numBones = pOutput->NumEntries;
    
    Float* maxContrib = (Float*)allocator.Alloc(4*numBones, 4);
    Float* accumulatedContributionThisPriority = (Float*)allocator.Alloc(4*numBones, 4);
    U32 numAccumulatedThisPriority = 0;
    Float* minAdditiveThisPriority = (Float*)allocator.Alloc(4*numBones, 4);
    Float* additive = (Float*)allocator.Alloc(4*numBones, 4);
    memset(maxContrib, 0, 4*numBones);
    memset(accumulatedContributionThisPriority, 0, numBones);
    for(U32 i = 0;i < numBones; i++)
    {
        additive[i] = 1.0f;
        minAdditiveThisPriority[i] = 1.0f;
    }
    
    while(pCurrent)
    {
        I32 currentPriority = pCurrent->Controller->GetPriority();
        
        bAdditive = Mixer->_AdditivePriority > pCurrent->Controller->GetPriority();
        
        if(!pCurrent->Controller->IsPaused() && pCurrent->Controller->IsActive())
        {
            ComputedValue<SkeletonPose>* pThisComputed = pThisPriorityComputedValues + numAccumulatedThisPriority;
            new (pThisComputed) ComputedValue<SkeletonPose>();
            pThisComputed->NumEntries = numBones;
            pThisComputed->Skl = pOutput->Skl;
            pThisComputed->AllocateFromFastBuffer(allocator, true);
            pCurrent->AnimatedValue->ComputeValue(pThisComputed, pCurrent->Controller, kDefaultContribution);
            Bool bAlmost0 = true;
            for(U32 i = 0; i < numBones; i++)
            {
                if(pThisComputed->Contribution[i] >= 0.00001f)
                {
                    bAlmost0 = false;
                    break;
                }
            }
            if(!bAlmost0)
            {
                numAccumulatedThisPriority++;
                for(U32 i = 0; i < numBones; i++)
                {
                    accumulatedContributionThisPriority[i] += pThisComputed->Contribution[i];
                }
            }
            else
            {
                pThisComputed->~ComputedValue<SkeletonPose>();
            }
            if(bAdditive)
            {
                for(U32 i = 0; i < numBones; i++)
                {
                    minAdditiveThisPriority[i] = fminf(minAdditiveThisPriority[i], 1.0f + (Contribution[i] *
                                                                                           ((pCurrent->Controller->GetAdditiveMix() * pThisComputed->AdditiveMix[i]) - 1.0f)));
                    pOutput->AdditiveValue.Entries[i] = PerformMix<Transform>::BlendAdditive(pOutput->AdditiveValue.Entries[i],
                                                                                             pThisComputed->AdditiveValue.Entries[i], additive[i]);
                }
            }
        }
        
        if(lastPriority != currentPriority || pCurrent->Next == nullptr)
        {
            for(U32 i = 0;i < numBones; i++)
            {
                additive[i] *= minAdditiveThisPriority[i];
                minAdditiveThisPriority[i] = 1.0f;
            }
            if(numAccumulatedThisPriority > 0)
            {
                new (pFinalComputedValues + numFinalValues) ComputedValue<SkeletonPose>();
                pFinalComputedValues[numFinalValues].NumEntries = numBones;
                pFinalComputedValues[numFinalValues].Skl = pOutput->Skl;
                pFinalComputedValues[numFinalValues].AllocateFromFastBuffer(allocator, true);
                AnimationMixerAccumulater<SkeletonPose>::AccumulateCurrent(pThisPriorityComputedValues,
                                                                numAccumulatedThisPriority, pFinalComputedValues[numFinalValues], pOutput->Skl,
                                                                numBones, accumulatedContributionThisPriority, allocator);
                Bool bAlmost1 = true;
                for(U32 i = 0; i < numBones; i++)
                {
                    maxContrib[i] = fmaxf(maxContrib[i], pFinalComputedValues[numFinalValues].Contribution[i]);
                    if(bAlmost1 && maxContrib[i] <= 0.9999f)
                        bAlmost1 = false;
                }
                numFinalValues++;
                for(U32 i = 0; i < numAccumulatedThisPriority; i++)
                {
                    pThisPriorityComputedValues[i].~ComputedValue();
                }
                if(bAlmost1)
                    break; // no point continuing
            }
            memset(accumulatedContributionThisPriority, 0, 4*numBones);
            numAccumulatedThisPriority = 0;
        }
        lastPriority = currentPriority;
        pCurrent = pCurrent->Next;
    }
    if(numFinalValues > 0)
    {
        AnimationMixerAccumulater<SkeletonPose>::AccumulateFinal(pFinalComputedValues, numFinalValues, *pOutput, numBones, maxContrib);
    }
    else
    {
        memcpy(pOutput->Contribution, maxContrib, 4*numBones);
    }
    for(U32 i = 0; i < numFinalValues; i++)
    {
        pFinalComputedValues[i].~ComputedValue();
    }
    pOutput->AdditiveMix = additive;
}

// COMPUTE A SINGLE SKELETON POSE
void
SkeletonPoseCompoundValue::ComputeValue(void *Value, Ptr<PlaybackController> Controller, const Float *Contributions)
{
    ComputedValue<SkeletonPose>& output = *((ComputedValue<SkeletonPose>*)Value);
    const Bool bMirrored = Controller->IsMirrored();
    _ResolveSkeleton(output.Skl, bMirrored);
    Bool bAdditive = false;
    //Float runningContrib = 0.0f;
    for(auto it = _Values.begin(); ; it++)
    {
        if(it == _Values.end())
        {
            it = _AdditiveValues.begin();
            bAdditive = true;
        }
        if(it == _AdditiveValues.end())
            break;
        auto& animated = *it;
        if(animated.BoneIndex >= 0 && Contributions[animated.BoneIndex] > 0.00001f)
        {
            ComputedValue<Transform> computedBoneTransform{};
            PerformMix<Transform>::PrepareValue(computedBoneTransform, Transform{});
            animated.Value->ComputeValue(&computedBoneTransform, Controller, &Contributions[animated.BoneIndex]);
            Float maxContrib = fmaxf(computedBoneTransform.Vec3Contrib, computedBoneTransform.QuatContrib);
            //runningContrib += maxContrib;
            if(bMirrored)
                PerformMix<Transform>::Mirror(bAdditive ? computedBoneTransform.AdditiveValue : computedBoneTransform.Value);
            output.Contribution[animated.BoneIndex] = maxContrib;
            output.Value.Entries[animated.BoneIndex] = computedBoneTransform.Value;
            if(bAdditive)
                output.AdditiveValue.Entries[animated.BoneIndex] = computedBoneTransform.AdditiveValue;
        }
    }
}

const std::type_info& SkeletonPoseCompoundValue::GetValueType() const
{
    return typeid(SkeletonPose);
}

// ========================================= SKELETON POSE MIXER =========================================

const std::type_info& AnimationMixer<SkeletonPose>::GetValueType() const
{
    return typeid(SkeletonPose);
}

AnimationMixer<SkeletonPose>::AnimationMixer(String N) : AnimationMixerBase(N) {}

void AnimationMixer<SkeletonPose>::ComputeValue(void *Value, Ptr<PlaybackController>, const Float *Contribution)
{
    AnimationMixer_ComputeValue<SkeletonPose>(this, (ComputedValue<SkeletonPose>*)Value, Contribution);
}

void AnimationMixer<SkeletonPose>::AddValue(Ptr<PlaybackController> pController, Ptr<AnimationValueInterface> pAnimValue, const Float *cscale)
{
    Ptr<AnimationMixerValueInfo> pSkeletonPoseCompoundValue = _FindMixerInfo(pController, pAnimValue->GetAdditive());
    Ptr<SkeletonPoseCompoundValue> pCompound = nullptr;
    if(!pSkeletonPoseCompoundValue)
    {
        pCompound = TTE_NEW_PTR(SkeletonPoseCompoundValue, MEMORY_TAG_ANIMATION_DATA, GetName());
        AnimationMixerBase::AddValue(pController, pCompound, cscale);
    }
    else
    {
        pCompound = std::dynamic_pointer_cast<SkeletonPoseCompoundValue>(pSkeletonPoseCompoundValue->AnimatedValue);
    }
    pCompound->AddValue(pController, pAnimValue, cscale);
}

void AnimationMixer<SkeletonPose>::GetNonHomogeneousNames(std::set<String> &names) const
{
    for(Ptr<AnimationMixerValueInfo> it = _ActiveValuesHead; it; it = it->Next)
    {
        it->AnimatedValue->GetNonHomogeneousNames(names);
    }
    for(Ptr<AnimationMixerValueInfo> it = _PassiveValuesHead; it; it = it->Next)
    {
        it->AnimatedValue->GetNonHomogeneousNames(names);
    }
}

// =========================================  UTIL =========================================

void AnimationManager::CoerceLegacyKeyframes(Ptr<KeyframedValue<Transform>>& coerced, Ptr<KeyframedValue<Vector3>>& pVec3, Ptr<KeyframedValue<Quaternion>>& pQuat)
{
    TTE_ASSERT((pVec3 != nullptr) ^ (pQuat != nullptr), "Only one can be set");
    U32 N = pVec3 ? (U32)pVec3->GetSamples().size() : (U32)pQuat->GetSamples().size();
    if(pVec3)
    {
        TTE_ASSERT(!coerced->Vector3Coerced, "Vector3's already coerced");
        coerced->Vector3Coerced = true;
        coerced->_MinValue._Trans = pVec3->_MinValue;
        coerced->_MaxValue._Trans = pVec3->_MaxValue;
    }
    else
    {
        TTE_ASSERT(!coerced->QuatCoerced, "Quaternions already coerced");
        coerced->QuatCoerced = true;
        coerced->_MinValue._Rot = pQuat->_MinValue; // although quaternions have no 'maximum' or minimum. just here for neatness
        coerced->_MaxValue._Rot = pQuat->_MaxValue;
    }
    std::vector<Float> uniqueTimes{};
    uniqueTimes.reserve(pVec3 ? pVec3->_Samples.size() + coerced->_Samples.size() : pQuat->_Samples.size() + coerced->_Samples.size());
    
    for (const auto& sample : coerced->GetSamples())
        uniqueTimes.push_back(sample.Time);
    if (pVec3)
        for (const auto& sample : pVec3->GetSamples())
            uniqueTimes.push_back(sample.Time);
    if (pQuat)
        for (const auto& sample : pQuat->GetSamples())
            uniqueTimes.push_back(sample.Time);
    
    std::sort(uniqueTimes.begin(), uniqueTimes.end());
    uniqueTimes.erase(std::unique(uniqueTimes.begin(), uniqueTimes.end(),
                                  [](Float a, Float b)
                                  {
        return CompareFloatFuzzy(a, b);
    }), uniqueTimes.end());
    
    std::vector<KeyframedValue<Transform>::Sample> newSamples{};
    for (Float time : uniqueTimes)
    {
        auto& out = newSamples.emplace_back();
        out.Time = time;
        Flags combined{};
        
        // 1. COMBINE
        
        if (pVec3)
        {
            // NEW VEC
            ComputedValue<Vector3> computedVec{};
            pVec3->ComputeValueKeyframed(&computedVec, time, kDefaultContribution, combined, true);
            out.SampleFlags += combined;
            out.Value._Trans = computedVec.Value;
            
            // OLD QUAT
            ComputedValue<Transform> computedQuat{};
            coerced->ComputeValueKeyframed(&computedQuat, time, kDefaultContribution, combined, false); // no for quats. have no min max
            out.SampleFlags += combined;
            out.Value._Rot = computedQuat.Value._Rot;
        }
        
        if (pQuat)
        {
            // NEW QUAT
            ComputedValue<Quaternion> computedQuat{};
            pQuat->ComputeValueKeyframed(&computedQuat, time, kDefaultContribution, combined, false);
            out.SampleFlags += combined;
            out.Value._Rot = computedQuat.Value;
            
            // OLD VEC
            ComputedValue<Transform> computedVec{};
            coerced->ComputeValueKeyframed(&computedVec, time, kDefaultContribution, combined, true);
            out.SampleFlags += combined;
            out.Value._Trans = computedVec.Value._Trans;
        }
    }
    coerced->_Samples = std::move(newSamples);
}
