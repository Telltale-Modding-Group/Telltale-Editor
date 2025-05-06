#pragma once

#include <Scripting/ScriptManager.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Common/Mesh.hpp>
#include <Common/Scene.hpp>
#include <Common/Texture.hpp>
#include <Common/InputMapper.hpp>
#include <Common/Animation.hpp>
#include <Common/Skeleton.hpp>
#include <Common/PropertySet.hpp>

/**
 Creates function collection to register all common classes.
 */
inline LuaFunctionCollection CreateScriptAPI()
{
    LuaFunctionCollection Col{};
    
    RegisterRenderConstants(Col);
    
    Mesh::RegisterScriptAPI(Col);
    Scene::RegisterScriptAPI(Col);
    RenderTexture::RegisterScriptAPI(Col);
    InputMapper::RegisterScriptAPI(Col);
    Animation::RegisterScriptAPI(Col);
    Skeleton::RegisterScriptAPI(Col);
    PropertySet::RegisterScriptAPI(Col);
    
    return Col;
}
