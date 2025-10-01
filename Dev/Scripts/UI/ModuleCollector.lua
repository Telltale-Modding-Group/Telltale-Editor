require("ToolLibrary/UI/ModuleRenderable.lua")
require("ToolLibrary/UI/ModuleSkeleton.lua")
require("ToolLibrary/UI/ModuleCamera.lua")
require("ToolLibrary/UI/ModuleText.lua")
require("ToolLibrary/UI/ModuleScene.lua")

function ModuleCollector_RegisterUI(Ver)

	ModuleRenderable_RegisterUI(Ver)
	ModuleRenderable_RegisterUI(Ver)
	ModuleSkeleton_RegisterUI(Ver)
	ModuleCamera_RegisterUI(Ver)
	ModuleText_RegisterUI(Ver)
    ModuleScene_RegisterUI(Ver)

end