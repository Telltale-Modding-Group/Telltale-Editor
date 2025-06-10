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

// Use TelltaleEditor::GetCommonClassInfo()
inline void RegisterCommonClassInfo()
{
    CommonClassInfo::Make<Mesh::MeshInstance>(CommonClass::MESH, "kCommonClassMesh");
    CommonClassInfo::Make<RenderTexture>(CommonClass::TEXTURE, "kCommonClassTexture");
    CommonClassInfo::Make<Scene>(CommonClass::SCENE, "kCommonClassScene");
    CommonClassInfo::Make<InputMapper>(CommonClass::INPUT_MAPPER, "kCommonClassInputMapper");
    CommonClassInfo::Make<Skeleton>(CommonClass::SKELETON, "kCommonClassSkeleton");
    CommonClassInfo::Make<Animation>(CommonClass::ANIMATION, "kCommonClassAnimation");
    CommonClassInfo::RegisterCommonClass(CommonClassInfo{CommonClass::PROPERTY_SET, "kCommonClassPropertySet", 
                                         "PropertySet", "Handle<PropertySet>", "prop", nullptr});
}

/**
 Creates function collection to register all common classes. PropertySet is done by ToolLibrary as it is more fundamental.
 */
inline LuaFunctionCollection CreateScriptAPI()
{
    LuaFunctionCollection Col{};
    
    RegisterRenderConstants(Col);
    CommonClassInfo::RegisterConstants(Col);

    Mesh::RegisterScriptAPI(Col);
    Scene::RegisterScriptAPI(Col);
    RenderTexture::RegisterScriptAPI(Col);
    InputMapper::RegisterScriptAPI(Col);
    Animation::RegisterScriptAPI(Col);
    Skeleton::RegisterScriptAPI(Col);
    
    return Col;
}
