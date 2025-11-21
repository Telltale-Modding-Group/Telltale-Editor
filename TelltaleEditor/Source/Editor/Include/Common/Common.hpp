#pragma once

#include <Scripting/ScriptManager.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Resource/PropertySet.hpp>
#include <Common/Mesh.hpp>
#include <Common/Scene.hpp>
#include <Common/Texture.hpp>
#include <Common/InputMapper.hpp>
#include <Common/Animation.hpp>
#include <Common/Skeleton.hpp>
#include <Common/Chore.hpp>
#include <Common/Procedural.hpp>

// Use TelltaleEditor::GetCommonClassInfo()
inline void RegisterCommonClassInfo()
{
    CommonClassInfo::Make<Mesh::MeshInstance>(CommonClass::MESH, "kCommonClassMesh");
    CommonClassInfo::Make<RenderTexture>(CommonClass::TEXTURE, "kCommonClassTexture");
    CommonClassInfo::Make<Scene>(CommonClass::SCENE, "kCommonClassScene");
    CommonClassInfo::Make<InputMapper>(CommonClass::INPUT_MAPPER, "kCommonClassInputMapper");
    CommonClassInfo::Make<Skeleton>(CommonClass::SKELETON, "kCommonClassSkeleton");
    CommonClassInfo::Make<Animation>(CommonClass::ANIMATION, "kCommonClassAnimation");
    CommonClassInfo::Make<Chore>(CommonClass::ANIMATION, "kCommonClassChore");
    CommonClassInfo::Make<Procedural_LookAt>(CommonClass::PROCEDURAL_LOOKAT, "kCommonClassProceduralLookAt");
    CommonClassInfo::RegisterCommonClass(CommonClassInfo{CommonClass::PROPERTY_SET, "kCommonClassPropertySet",
                                         "PropertySet", "Handle<PropertySet>", "prop", nullptr});
}

extern void luaModuleUI(LuaFunctionCollection& Col); // defined in editor ui cpp

/**
 Creates function collection to register all common classes. PropertySet is done by ToolLibrary as it is more fundamental.
 */
inline LuaFunctionCollection CreateScriptAPI()
{
    LuaFunctionCollection Col{};
    
    RegisterRenderConstants(Col);
    CommonClassInfo::RegisterConstants(Col);
    luaModuleUI(Col);

    Mesh::RegisterScriptAPI(Col);
    Scene::RegisterScriptAPI(Col);
    RenderTexture::RegisterScriptAPI(Col);
    InputMapper::RegisterScriptAPI(Col);
    Animation::RegisterScriptAPI(Col);
    Skeleton::RegisterScriptAPI(Col);
    Chore::RegisterScriptAPI(Col);
    Procedural_LookAt::RegisterScriptAPI(Col);
    
    return Col;
}
