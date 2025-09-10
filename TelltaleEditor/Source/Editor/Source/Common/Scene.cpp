#include <Common/Scene.hpp>
#include <Renderer/RenderContext.hpp>
#include <Core/Callbacks.hpp>
#include <Symbols.hpp>

#include <cfloat>

class SceneAPI
{
public:
    
    static Scene* Task(LuaManager& man)
    {
        return (Scene*)man.ToPointer(1);
    }
    
    static U32 luaSceneSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        pScene->SetName(man.ToString(2));
        return 0;
    }
    
    static U32 luaSceneSetHidden(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        if(man.ToBool(2))
            pScene->_Flags.Add(SceneFlags::HIDDEN);
        else
            pScene->_Flags.Remove(SceneFlags::HIDDEN);
        return 0;
    }
    
    // state,agentname, props (will be copied), initial transform
    static U32 luaScenePushAgent(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() >= 3, "Incorrect number of arguments");
        Scene* pScene = Task(man);
        String agent = man.ToString(2);
        if(pScene->ExistsAgent(agent))
        {
            TTE_LOG("WARNING: When normalising scene %s the agent %s already exists! Ignoring...", pScene->Name.c_str(), agent.c_str());
            return 0;
        }
        Meta::ClassInstance props = Meta::CopyInstance(Meta::AcquireScriptInstance(man, 3));
        Transform trans{};
        if (man.GetTop() >= 4)
        {   
            if(man.Type(4) == LuaType::TABLE)
            {
                Meta::ExtractCoercableLuaValue(trans, man, 4);
            }
            else
            {
                Meta::ClassInstance inst = Meta::AcquireScriptInstance(man, 4);
                Meta::ExtractCoercableInstance(trans, inst);
            }
        }
        pScene->AddAgent(agent, {}, std::move(props), trans); // modules setup at runtime
        return 0;
    }
    
};

void Scene::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    PUSH_FUNC(Col, "CommonSceneSetName", &SceneAPI::luaSceneSetName, "nil CommonSceneSetName(state, name)", "Sets the name of the common scene");
    PUSH_FUNC(Col, "CommonSceneSetHidden", &SceneAPI::luaSceneSetHidden,"nil CommonSceneSetHidden(state, bHidden)", "Sets if the common scene is initially hidden");
    PUSH_FUNC(Col, "CommonScenePushAgent", &SceneAPI::luaScenePushAgent, "nil CommonScenePushAgent(state, name, props, --[[optional]] initialTransform)", "Pushes an agent to the common scene");
}

void Scene::FinaliseNormalisationAsync()
{
    
}

void Scene::_SetupAgent(std::map<Symbol, Ptr<SceneAgent>, SceneAgentComparator>::iterator agent)
{
    Ptr<SceneAgent> pAgent = agent->second;
    
    // create node
    Ptr<Node> pNode;
    pAgent->AgentNode = pNode = TTE_NEW_PTR(Node, MEMORY_TAG_OBJECT_DATA);
    pNode->LocalTransform = pNode->GlobalTransform = pAgent->InitialTransform;
    pNode->AgentName = pAgent->Name;
    pNode->Name = pAgent->Name;
    pNode->AttachedScene = this;
    pAgent->OwningScene = this;
    
    // SETUP RUNTIME PROPERTIES
    
    if(!PropertySet::ExistsKey(pAgent->Props, kRuntimeVisibilityKey, true, GetRegistry()))
    {
        PropertySet::Set<Bool>(pAgent->Props, kRuntimeVisibilityKey, true, "bool", GetRegistry());
    }

    // DOUBLE CHECK ALL MODULES ARE CHECKED
    SceneModuleTypes modules{};
    Ptr<ResourceRegistry> reg = GetRegistry();
    if (reg)
        SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_CollectAgentModules{ modules, *pAgent.get(), reg });
    for(auto it = modules.begin(); it != modules.end(); it++)
        AddAgentModule(pAgent->Name, *it);
    
    // ADD CALLBACKS
    
    PropertySet::AddCallback(pAgent->Props, kRuntimeVisibilityKey, ALLOCATE_METHOD_CALLBACK_1(pAgent, SetVisible, SceneAgent, Bool));
    
    // 1 (Sync1) prepare non dependent mesh agents
    
    SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::PRE_RENDERABLE,
                                                     SceneModuleUtil::_SetupAgentSubset{*this, *pAgent});
    
    // 2 (Sync2) prepare depent modules on mesh
    
    SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::POST_RENDERABLE,
                                                     SceneModuleUtil::_SetupAgentSubset{*this, *pAgent});
}

void SceneAgent::SetVisible(Bool bVisible)
{
    if(OwningScene && CompareCaseInsensitive(OwningScene->GetName(), Name))
    {
        if(!bVisible)
            OwningScene->_Flags.Add(SceneFlags::HIDDEN);
        else
            OwningScene->_Flags.Remove(SceneFlags::HIDDEN);
    }
}

void Scene::_SetupAgentsModules()
{
    if(_Flags.Test(SceneFlags::RUNNING))
        return;
    for(auto agent = _Agents.begin(); agent != _Agents.end(); agent++)
    {
        _SetupAgent(agent);
    }
}

Meta::ClassInstance Scene::GetAgentProps(const Symbol & sym)
{
    for(auto& agent: _Agents)
    {
        if(agent.first == sym)
        {
            return agent.second->Props;
        }
    }
    return {};
}

void Scene::AddAgentModule(const String& Name, SceneModuleType module)
{
    Symbol sym{Name};
    for(auto& agent: _Agents)
    {
        if(agent.second->NameSymbol == sym)
        {
            if(agent.second->ModuleIndices[(U32)module] == -1)
            {
                I32 index = -1;
                SceneModuleUtil::PerformRecursiveModuleOperation<SceneModuleUtil::_AddModuleRecurser>(
                    SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_AddModuleRecurser{*this, module, index});
                if(index == -1)
                {
                    TTE_ASSERT(false, "Invalid module enum!");
                    return;
                }
                agent.second->ModuleIndices[(U32)module] = index;
            }
            break;
        }
    }
}

void Scene::RemoveAgentModule(const Symbol& sym, SceneModuleType module)
{
    I32 toRemove = -1;
    
    // First pass: Find the agent and remove the module
    for (auto it = _Agents.begin(); it != _Agents.end(); it++)
    {
        const Ptr<SceneAgent>& agent = it->second;
        if (agent->NameSymbol == sym)
        {
            if (agent->ModuleIndices[(U32)module] != -1)
            {
                toRemove = agent->ModuleIndices[(U32)module];
                Bool bResult = false;
                
                SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_RemoveModuleRecurser{*this, module, toRemove, bResult});
                
                if(!bResult)
                {
                    TTE_ASSERT(false, "Invalid module type");
                    return;
                }
                
                // Clear the module index for the current agent (or mark it as invalid)
                agent->ModuleIndices[(U32)module] = -1;
            }
            break;
        }
    }
    
    if (toRemove != -1)
    {
        for (auto& agent : _Agents)
        {
            if (agent.second->ModuleIndices[(U32)module] > toRemove)
            {
                agent.second->ModuleIndices[(U32)module] = agent.second->ModuleIndices[(U32)module] - 1;
            }
        }
    }
}

void Scene::RemoveAgent(const Symbol& Name)
{
    auto it = _Agents.find(Name);
    if(it != _Agents.end())
    {
        // On remove

        for (I32 mod = 0; mod < (I32)SceneModuleType::NUM; mod++)
        {
            if (it->second->ModuleIndices[mod] != -1)
            {
                RemoveAgentModule(Name, (SceneModuleType)mod);
            }
        }
        _Agents.erase(it);
    }
}

Bool Scene::ExistsAgent(const Symbol& sym)
{
    for(auto& agent: _Agents)
        if(agent.second->NameSymbol == sym)
            return true;
    return false;
}

SceneModuleTypes Scene::GetAgentModules(const Symbol& Name)
{
    for (auto& agent : _Agents)
    {
        if (agent.second->NameSymbol == Name)
        {
            SceneModuleTypes m{};
            for(I32 mod  = 0; mod < (I32)SceneModuleType::NUM; mod++)
            {
                if(agent.second->ModuleIndices[mod] != -1)
                    m.Set((SceneModuleType)mod, true);
            }
            return m;
        }
    }
    return {};
}

Bool Scene::HasAgentModule(const Symbol& sym, SceneModuleType module)
{
    for(auto& agent: _Agents)
        if(agent.second->NameSymbol == sym)
            return agent.second->ModuleIndices[(U32)module] != -1;
    return false;
}

static Bool RayIntersectBB(
    const Vector3& rayOrigin,
    const Vector3& rayDir,
    const Vector3& boxMin, 
    const Vector3& boxMax,
    Float& t)
{
    Float tmin = (boxMin.x - rayOrigin.x) / rayDir.x;
    Float tmax = (boxMax.x - rayOrigin.x) / rayDir.x;

    if (tmin > tmax) std::swap(tmin, tmax);

    Float tymin = (boxMin.y - rayOrigin.y) / rayDir.y;
    Float tymax = (boxMax.y - rayOrigin.y) / rayDir.y;

    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    Float tzmin = (boxMin.z - rayOrigin.z) / rayDir.z;
    Float tzmax = (boxMax.z - rayOrigin.z) / rayDir.z;

    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    if (tmax < 0)
        return false;

    t = (tmin >= 0) ? tmin : tmax;
    return true;
}

static Bool RayIntersectSphere(
    const Vector3& rayOrigin,
    const Vector3& rayDir,
    const Vector3& sphereCenter,
    Float sphereRadius, Float& t)
{
    Vector3 oc = rayOrigin - sphereCenter;

    Float a = Vector3::Dot(rayDir, rayDir);
    Float b = 2.0f * Vector3::Dot(oc, rayDir);
    Float c = Vector3::Dot(oc, oc) - sphereRadius * sphereRadius;

    Float discriminant = b * b - 4 * a * c;
    if (discriminant < 0.0f)
        return false;

    Float sqrtDisc = sqrtf(discriminant);
    Float t0 = (-b - sqrtDisc) / (2.0f * a);
    Float t1 = (-b + sqrtDisc) / (2.0f * a);

    t = (t0 >= 0.0f) ? t0 : ((t1 >= 0.0f) ? t1 : -1.0f);
    if (t < 0.0f)
    {
        t = 0.0f;
        return false;
    }

    return true;
}

String Scene::GetAgentAtScreenPosition(Camera& cam, U32 screenX, U32 screenY, Bool bBySelectable)
{
    TTE_ASSERT(!bBySelectable, "Not supported by selectable"); // IMPL SELECTABLE MODULE

    Float ndcX = (2.0f * screenX) / cam._ScreenWidth - 1.0f;
    Float ndcY = 1.0f - (2.0f * screenY) / cam._ScreenHeight;
    Vector4 nearPointClip(ndcX, ndcY, 0.0f, 1.0f);
    Vector4 farPointClip(ndcX, ndcY, 1.0f, 1.0f);

    Matrix4 invViewProj = (cam.GetProjectionMatrix() * cam.GetViewMatrix()).Inverse();

    Vector4 nearWorld = nearPointClip * invViewProj;
    Vector4 farWorld = farPointClip * invViewProj;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    Vector3 origin = Vector3(nearWorld.x, nearWorld.y, nearWorld.z);
    Vector3 direction = (Vector3(farWorld.x, farWorld.y, farWorld.z) - origin);
    direction.Normalize();

    Float tmin = FLT_MAX;
    String agent{};
    for (const auto& renderable : _Renderables)
    {
        Transform agentTransform = GetNodeWorldTransform(renderable.AgentNode);
        Matrix4 agentMat = MatrixTransformation(agentTransform._Rot, agentTransform._Trans);
        for (const auto& mesh : renderable.Renderable.MeshList)
        {
            Float tSphere = 0.0f;
            Vector3 worldCenter = Vector4(mesh->BSphere._Center, 1.0f) * agentMat;

            if (RayIntersectSphere(origin, direction, worldCenter, mesh->BSphere._Radius, tSphere))
            {
                Float tBox = 0.0f;
                Vector3 boxMin = (Vector4(mesh->BBox._Min, 1.0f)) * agentMat;
                Vector3 boxMax = (Vector4(mesh->BBox._Max, 1.0f)) * agentMat;

                Bool intersects = RayIntersectBB(origin, direction, boxMin, boxMax, tBox);

                if (intersects && tBox < tmin)
                {
                    tmin = tBox;
                    agent = renderable.AgentNode->AgentName;
                }
                else if (!intersects && tSphere < tmin)
                {
                    // fallback to sphere if box miss
                    tmin = tSphere;
                    agent = renderable.AgentNode->AgentName;
                }
            }
        }
    }
    return agent;
}

void Scene::AddAgent(const String& Name, SceneModuleTypes modules, Meta::ClassInstance props, Transform init)
{
    TTE_ASSERT(!ExistsAgent(Name), "Agent %s already exists in %s", Name.c_str(), Name.c_str());
    Ptr<SceneAgent> pAgent = TTE_NEW_PTR(SceneAgent, MEMORY_TAG_SCENE_DATA, Name);
    auto agentIt = _Agents.insert({Symbol(Name), pAgent});
    auto& agentPtr = agentIt.first->second;
    agentPtr->OwningScene = this;
    agentPtr->InitialTransform = init;
    agentPtr->Props = props;
    if(!agentPtr->Props)
    {
        U32 clazz = Meta::FindClass("class PropertySet", 0);
        TTE_ASSERT(clazz!=0, "Property set class not found!");
        agentPtr->Props = Meta::CreateInstance(clazz);
    }
    else
    {
        TTE_ASSERT(Meta::GetClass(props.GetClassID()).Flags & Meta::_CLASS_PROP, "At Scene::AddAgent -> properties class mismatch!");
    }
    Ptr<ResourceRegistry> reg = GetRegistry();
    if(reg)
        SceneModuleUtil::PerformRecursiveModuleOperation(SceneModuleUtil::ModuleRange::ALL, SceneModuleUtil::_CollectAgentModules{modules, *pAgent.get(), reg});
    for(auto it = modules.begin(); it != modules.end(); it++)
    {
        AddAgentModule(Name, *it);
    }
    if(_Flags.Test(SceneFlags::RUNNING))
    {
        _SetupAgent(agentIt.first);
    }
}
// NODES

Bool Scene::_ValidateNodeAttachment(Ptr<Node> node, Ptr<Node> potentialChild)
{
    return !TestChildNode(node, potentialChild);
}

void Scene::_UpdateListenerAttachments(Ptr<Node> node)
{
    Ptr<NodeListener> listener = node->Listeners;
    while (listener)
    {
        listener->OnAttachmentChanged();
        listener = listener->Next;
    }

    auto child = node->FirstChild.lock();
    while (child)
    {
        _UpdateListenerAttachments(child);
        child = child->NextSibling.lock();
    }
}

void Scene::_InvalidateNode(Ptr<Node> node, Node* pParent, Bool bAllowStaticUpdate)
{
    node->Fl.Remove(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID);

    Ptr<NodeListener> listener = node->Listeners;
    while (listener)
    {
        listener->OnTransformChanged(nullptr);
        listener = listener->Next;
    }

    Node* pParentTile = node->Fl.Test(NodeFlags::NODE_ENVIRONMENT_TILE) ? node.get() : nullptr;

    auto pChild = node->FirstChild.lock();
    while (pChild)
    {
        if (bAllowStaticUpdate || _ValidateTransformUpdate(pChild, pParentTile))
            _InvalidateNode(pChild, pParentTile, bAllowStaticUpdate);
        pChild = pChild->NextSibling.lock();
    }
}

Transform Scene::GetNodeWorldTransform(Ptr<Node> node)
{
    if (!node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
        _CalculateGlobalNodeTransform(node);
    return node->GlobalTransform;
}

void Scene::_CalculateGlobalNodeTransform(Ptr<Node> node)
{
    auto parent = node->Parent.lock();
    if (parent)
    {
        if (!parent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(parent);
        node->GlobalTransform = parent->GlobalTransform * node->LocalTransform;
        node->GlobalTransform.Normalise();
    }
    else
    {
        node->GlobalTransform = node->LocalTransform;
    }
    node->Fl.Add(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID);
}

Transform Scene::NodeLocalToNode(const Ptr<Node>& node, Transform nodeLocalSpaceTransform, const String& max)
{
    Transform result = nodeLocalSpaceTransform;
    auto current = node->Parent.lock();

    while (current)
    {
        result = current->LocalTransform * result;
        result.Normalise();
        if (max.length() != 0 && CompareCaseInsensitive(current->Name, max))
            break;
        current = current->Parent.lock();
    }

    return result;
}

void Scene::UpdateNodeLocalTransform(Ptr<Node> node, Transform trans, Bool bAllowSt)
{
    if (!bAllowSt && !_ValidateTransformUpdate(node, nullptr))
        return;

    node->LocalTransform = trans;
    node->LocalTransform.Normalise();

    _InvalidateNode(node, nullptr, bAllowSt);
}

void Scene::UpdateNodeWorldTransform(Ptr<Node> node, Transform trans, Bool bAllowSt)
{
    if (!bAllowSt && !_ValidateTransformUpdate(node, nullptr))
        return;

    auto parent = node->Parent.lock();
    if (parent)
    {
        if (!parent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(parent);
        node->LocalTransform = trans / parent->GlobalTransform;
    }
    else
    {
        node->LocalTransform = trans;
    }

    node->LocalTransform.Normalise();
    _InvalidateNode(node, nullptr, bAllowSt);
}

Bool Scene::_ValidateTransformUpdate(Ptr<Node> node, Node* pTileNode)
{
    return pTileNode || !node->StaticListeners;
}

void NodeListener::SetStatic(Bool bStatic)
{
    if(Listening)
    {
        Bool bAlreadyStatic = ListenerFlags.Test(NodeListenerFlags::NODE_LISTENER_STATIC);
        if(bStatic != bAlreadyStatic)
        {
            if(bStatic)
            {
                Listening->StaticListeners++;
                ListenerFlags.Add(NodeListenerFlags::NODE_LISTENER_STATIC);
            }
            else
            {
                Listening->StaticListeners--;
                ListenerFlags.Remove(NodeListenerFlags::NODE_LISTENER_STATIC);
            }
        }
    }
}

NodeListener::~NodeListener()
{
    AdandonNode();
}

Bool NodeListener::IsStatic()
{
    return ListenerFlags.Test(NodeListenerFlags::NODE_LISTENER_STATIC);
}

void NodeListener::AdandonNode()
{
    if(Listening)
        Scene::RemoveNodeListener(Listening, this);
    ListenerFlags = 0;
}

void Scene::AddNodeListener(Ptr<Node> node, Ptr<NodeListener> pListener)
{
    pListener->AdandonNode();
    pListener->Listening = node;
    pListener->Next = node->Listeners;
    node->Listeners = pListener;
    if(pListener->IsStatic())
        node->StaticListeners++;
}

void Scene::RemoveNodeListener(Ptr<Node> node, NodeListener *pListener)
{
    NodeListener* pCurrent = node->Listeners.get();
    NodeListener* pPrev = nullptr;
    while(pCurrent)
    {
        if(pCurrent == pListener)
        {
            if(pPrev)
                pPrev->Next = std::move(pCurrent->Next);
            else
                node->Listeners = std::move(pCurrent->Next);
            if(pCurrent->IsStatic())
                node->StaticListeners--;
            pListener->ListenerFlags = 0;
            pListener->Listening.reset();
            return;
        }
        pPrev = pCurrent;
        pCurrent = pCurrent->Next.get();
    }
}

Ptr<Node> Scene::FindChildNode(Ptr<Node> node, Symbol name)
{
    if (Symbol(node->Name) == name)
        return node;

    auto pChild = node->FirstChild.lock();
    Ptr<Node> result{};
    if (!pChild || !(result = FindChildNode(pChild, name)))
    {
        pChild = pChild ? pChild->NextSibling.lock() : nullptr;
        if (pChild)
            return FindChildNode(pChild, name);
    }
    return result;
}

Bool Scene::TestChildNode(Ptr<Node> node, Ptr<Node> potentialChild)
{
    if (node == potentialChild)
        return true;

    auto pChild = node->FirstChild.lock();
    while (pChild)
    {
        if (pChild == potentialChild)
            return true;
        if (TestChildNode(pChild, potentialChild))
            return true;
        pChild = pChild->NextSibling.lock();
    }
    return false;
}

Bool Scene::TestParentNode(Ptr<Node> pNode, Ptr<Node> potentialParent)
{
    while (pNode)
    {
        if (pNode == potentialParent)
            return true;
        pNode = pNode->Parent.lock();
    }
    return false;
}

void Scene::AttachNode(Ptr<Node> node, Ptr<Node> parent, Bool bPreserveWorldPosition, Bool bInitialAttach)
{
    if (node != parent && (bInitialAttach || _ValidateTransformUpdate(node, nullptr)))
    {
        if (node->Parent.lock())
            UnAttachNode(node, bPreserveWorldPosition, true);

        auto grandParent = parent->Parent.lock();
        if (!grandParent || _ValidateNodeAttachment(grandParent, node))
        {
            node->Parent = parent;
            node->NextSibling = parent->FirstChild;
            if (auto first = parent->FirstChild.lock())
                first->PrevSibling = node;
            parent->FirstChild = node;

            _InvalidateNode(node, nullptr, bInitialAttach || bPreserveWorldPosition);

            if (bPreserveWorldPosition)
            {
                if (!node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
                    _CalculateGlobalNodeTransform(node);
                UpdateNodeLocalTransform(node, node->LocalTransform, true);
            }

            _UpdateListenerAttachments(node);
        }
    }
}

void Scene::UnAttachNode(Ptr<Node> node, Bool bPreserveWorldPosition, Bool bForceUnattach)
{
    auto parent = node->Parent.lock();
    if (node && parent && (bForceUnattach || _ValidateTransformUpdate(node, nullptr)))
    {
        if (bPreserveWorldPosition && !node->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
            _CalculateGlobalNodeTransform(node);

        auto prev = node->PrevSibling.lock();
        auto next = node->NextSibling.lock();

        if (prev)
        {
            prev->NextSibling = next;
            if (next)
                next->PrevSibling = prev;
        }
        else
        {
            parent->FirstChild = next;
            if (next)
                next->PrevSibling.reset();
        }

        node->NextSibling.reset();
        node->PrevSibling.reset();

        if (bPreserveWorldPosition)
        {
            if (auto pParent = node->Parent.lock())
            {
                if (!pParent->Fl.Test(NodeFlags::NODE_GLOBAL_TRANSFORM_VALID))
                    _CalculateGlobalNodeTransform(pParent);
                node->LocalTransform = node->GlobalTransform / pParent->GlobalTransform;
            }
            _InvalidateNode(node, nullptr, true);
        }
        else
        {
            _InvalidateNode(node, nullptr, bForceUnattach);
        }

        node->Parent.reset();
        _UpdateListenerAttachments(node);
    }
}

void Scene::UnAttachAllChildren(Ptr<Node> node, Bool bAttachChildrenToParent)
{
    if (!node->Parent.lock())
        bAttachChildrenToParent = false;

    auto child = node->FirstChild.lock();
    while (child)
    {
        auto next = child->NextSibling.lock();
        UnAttachNode(child, true, true);
        if (bAttachChildrenToParent)
            AttachNode(child, node->Parent.lock(), true, true);
        child = next;
    }
}

Scene::~Scene()
{
    _Agents.clear();
}

Node::~Node()
{
    if(Name == "BabyThorn")
    {
        int x;
    }
}

// OBJECT CACHE MANAGER

void ObjOwner::_AddObjData(Ptr<ObjDataBase> pObj)
{
    pObj->Next = Objects;
    Objects = std::move(pObj);
}

void ObjOwner::_RemoveObjData(const String& obj, const std::type_info& ti)
{
    Ptr<ObjDataBase> prev{};
    Ptr<ObjDataBase> cur = Objects;
    while(cur)
    {
        if(cur->Type == ti && CompareCaseInsensitive(obj, cur->Name))
        {
            if(prev)
                prev->Next = cur->Next;
            else
                Objects = cur->Next;
            return;
        }
        prev = cur;
        cur = cur->Next;
    }
}

void ObjOwner::_GetObjData(Ptr<ObjDataBase>& pOut, const String& obj, const std::type_info& ti)
{
    Ptr<ObjDataBase> cur = Objects;
    while(cur)
    {
        if(cur->Type == ti && CompareCaseInsensitive(obj, cur->Name))
        {
            pOut = cur;
            return;
        }
        cur = cur->Next;
    }
}

void ObjOwner::FreeOwnedObjects()
{
    while (Objects)
    {
        Ptr<ObjDataBase> next = Objects->Next;
        Objects->Next.reset(); // Break the link to the next node
        Objects = next;        // Move to next
    }
}

ObjOwner::~ObjOwner()
{
    FreeOwnedObjects();
}
