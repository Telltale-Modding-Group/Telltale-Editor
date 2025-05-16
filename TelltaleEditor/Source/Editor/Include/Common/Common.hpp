#pragma once

#include <Scripting/ScriptManager.hpp>
#include <Renderer/RenderAPI.hpp>
#include <Resource/PropertySet.hpp>
#include <Common/Mesh.hpp>
#include <Common/Scene.hpp>
#include <Common/Texture.hpp>
#include <Common/InputMapper.hpp>
#include <Common/Animation.hpp>
#include <Common/Skeleton.hpp>

/**
 Creates function collection to register all common classes. PropertySet is done by ToolLibrary as it is more fundamental.
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
    
    return Col;
}
