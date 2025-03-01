#pragma once

#include <Scripting/ScriptManager.hpp>

#include <Common/Mesh.hpp>
#include <Common/Scene.hpp>


/**
 Creates function collection to register all common 
 */
inline LuaFunctionCollection CreateScriptAPI()
{
	LuaFunctionCollection Col{};
	
	Mesh::RegisterScriptAPI(Col);
	Scene::RegisterScriptAPI(Col);
	
	return Col;
}
