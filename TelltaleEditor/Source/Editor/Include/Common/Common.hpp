#pragma once

#include <Scripting/ScriptManager.hpp>
#include <Renderer/RenderAPI.hpp>

#include <Common/Mesh.hpp>
#include <Common/Scene.hpp>
#include <Common/Texture.hpp>


/**
 Creates function collection to register all common 
 */
inline LuaFunctionCollection CreateScriptAPI()
{
    LuaFunctionCollection Col{};
    
    RegisterRenderConstants(Col);
    
    Mesh::RegisterScriptAPI(Col);
    Scene::RegisterScriptAPI(Col);
    RenderTexture::RegisterScriptAPI(Col);
    
    return Col;
}
