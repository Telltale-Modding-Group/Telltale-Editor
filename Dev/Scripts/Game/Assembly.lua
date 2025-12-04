-- Taken from the executables. This is an in-executable script used to assemble resources, written by Telltale Games

_resourceRegistry = {}                                        

function AssembleResourcesStart()
end

function AssembleResourcesEnd()
end

-- Backwards Compatibility - Remove
function UnRegisterResourceDescription(name)
	UnRegisterResourceLocationConfiguration(name)
end

function CreateSetDescription(name)
	return CreateResourceLocationConfiguration(name)
end

function CreateDevSetDescriptionFromBaseSet(name)
	return CreateDevConfigFromBaseConfig(name)
end

function RegisterSetDescription(name)
	RegisterResourceLocationConfiguration(name)
end

function RegisterDescriptionWithResourceAssembler(set)
	print("Using legacy description file '" .. _currentDirectory .. _currentFile .. "'. Please remove and update _Project directory")
end     

-- Main functions

function UnRegisterResourceLocationConfiguration(name)
	if name and name ~= "" and _resourceRegistry.name then
		_resourceRegistry.name = nil
	end
end    

function CreateResourceLocationConfiguration(name)
	if not name or name == "" then
		print("cannot create set descriptions, missing name")
	end
	local set = {}
	set.name = name
	set.setName = name	
	set.descriptionFilenameOverride = nil	
	set.logicalName = "<" .. name .. ">"
	set.logicalDestination = nil
	set.priority = 0
	set.localDir = _currentDirectory
	set.enableMode = "bootable"
	set.version = "trunk"
	set.descriptionPriority = 0	
	
	set.gameDataName = name .. " Game Data"	
	set.gameDataPriority = 0
	set.gameDataEnableMode = "constant"
	set.localDirIncludeBase = true
	set.localDirRecurse = true
	set.localDirIncludeOnly = nil
	set.localDirExclude	= {"_dev/"} 
	set.gameDataArchives = {}
	
	return set
end      

function CreateDevConfigFromBaseConfig(baseSet)	
	local set = CreateSetDescription(baseSet.name .. " Dev")
	set.setName = "Dev"
	set.logicalDestination = nil
	set.priority = -9999
	set.enableMode = "localization"	
	set.gameDataName = "Dev"
	set.gameDataEnableMode = "localization"
	set.logicalName = baseSet.logicalName
	set.localDir = baseSet.localDir
	set.localDirIncludeBase = false
	set.localDirIncludeOnly = {"_dev/"}
	set.localDirExclude = nil	
	set.localDirRecurse = true
	set.includeOnlyRecurses = true
	return set
end

function RegisterResourceLocationConfiguration(set)
	local name = set.name
	if name and name ~= "" then	
		-- If in registration mode, regiser the collection with the assembly code
		set._currentDirectory = _currentDirectory
		if rawget(_G, "_registerCollections") then		
			-- Capture in our resource set registery
			local registryEntry = _resourceRegistry[name]
			if not registryEntry or registryEntry.descriptionPriority < set.descriptionPriority then
				_resourceRegistry[name] = set		
				--Print("Registered set " .. name)
			else			
				Print("Cannot register location configuration " .. name .. " from " .. _currentDirectory .. _currentFile .. " at priority " .. set.descriptionPriority )
				Print("The existing resource location configuration from " .. registryEntry.localDir .. " is registered at higher priority " .. registryEntry.descriptionPriority  )						
			end
		end
	end
end     

local pattern_cache = {}
local function GetPattern(ending)
	local cached = pattern_cache[ending]
	if not cached then
		cached = "/" .. ending:gsub("%a", function(letter)
			return "[" .. string.lower(letter) .. string.upper(letter) .. "]"
		end) .. "$"
		pattern_cache[ending] = cached
	end
	return cached
end

function AssembleResources()    
	-- Run start for any overrides
	AssembleResourcesStart()
	
	-- Apply all at the end for dependencies
	local autoapplylist = {}
	
	for name, set in pairs(_resourceRegistry) do			
		--Print("Assembling entry " .. name)
										
		-- Created named destination which maps into main	
		ResourceCreateLogicalLocation( set.logicalName  )

		-- Create the set used for mapping this data		
		local hasMainSet = true		
		if set.enableMode == "constant" then
			ResourceSetCreate(set.setName, set.priority, false, false, false, set._currentDirectory)
		elseif set.enableMode == "bootable" then
			ResourceSetCreate(set.setName, set.priority, true, true, false, set._currentDirectory)
		elseif set.enableMode == "localization" then
			ResourceSetCreate(set.setName, set.priority, true, false, true, set._currentDirectory)
		else
			hasMainSet = false
		end
		
		-- Create the main set and map this logical location to the parent if specified
		if hasMainSet and set.logicalDestination and set.logicalDestination ~= "" then							
			ResourceSetMapLocation(set.setName, set.logicalDestination, set.logicalName)
		end
			
		local logicalBaseDir = set.logicalName .. "/"
		
		-- Attempt to create a map for the base directory children				
		if set.gameDataEnableMode == "bootable" then
			ResourceSetCreate(set.gameDataName, set.gameDataPriority, true, true, false, set._currentDirectory)
		elseif set.gameDataEnableMode == "localization" then
			ResourceSetCreate(set.gameDataName, set.gameDataPriority, true, false, true, set._currentDirectory)
		else
			-- constant and other cases
			ResourceSetCreate(set.gameDataName, set.gameDataPriority, false, false, false, set._currentDirectory)
		end
		
		local function ProcessArchives()		
			-- Attempt to create a map for archives
			if set.gameDataArchives and #set.gameDataArchives ~= 0 then					
				-- Mount the archives into the logical	
				for i,archiveName in ipairs(set.gameDataArchives) do
					local location = ResourceResolveAddressToConcreteLocationID(archiveName)
					local resource = ResourceAddressGetResourceName(archiveName)
					if location and location ~= "" then
						local archiveID = location .. resource .. "/"				
						local cacheMode = "none"
						if string.find(string.lower(resource),"_mem") then
							cacheMode = "mem"
						elseif string.find(string.lower(resource),"_sync") then
							cacheMode = "hddsync"
						elseif not string.find(string.lower(resource),"_disc") then
							cacheMode = "hdd"
						end
						ResourceCreateConcreteArchiveLocation(archiveID,resource,location,cacheMode)
						ResourceSetMapLocation(set.gameDataName, set.logicalName, archiveID)
					end
				end		
			end
		end
		
		local function MatchDirectory(path, directory)
			return path:match(GetPattern(directory))
		end
		
		
		local function ProcessDir()
			if set.localDir then
				-- Mount directories
				local function ProcessDir(strPath)
					local excludeFromSet = false
					
					-- If we have an include list, only include those and their subdirectories
					local thisDirIncludeOnly = false
					if set.localDirIncludeOnly then
						excludeFromSet = true
						for i,include in pairs( set.localDirIncludeOnly ) do
							if MatchDirectory(strPath, include) then
								thisDirIncludeOnly = true
								excludeFromSet = false
								break
							end
						end
					end
					
					-- Now check the exclude list
					local matchedExcludeFilter = false
					if set.localDirExclude then
						for i,excludes in pairs( set.localDirExclude ) do
							if MatchDirectory(strPath, excludes) then
								excludeFromSet = truei
								matchedExcludeFilter = true
								break
							end
						end
					end
					
					local logicalPath = string.gsub(strPath, set.localDir, logicalBaseDir, 1)
					
					-- Now check if this is a basename
					if logicalPath == logicalBaseDir and not set.localDirIncludeBase then
						excludeFromSet = true
					end

					if not excludeFromSet then 
						if ResourceCreateConcreteDirectoryLocation(logicalPath,strPath) then
							ResourceSetMapLocation(set.gameDataName, set.logicalName, logicalPath)
						else
							print("Attempting to create concrete location " .. logicalPath .. " from directory " .. strPath .. " but it already has been defined.  Ignoring this assignment. Please fix!")
						end
					end
					
					-- Recursive - only in tool for now
					if IsToolBuildRaw() and set.localDirRecurse and not matchedExcludeFilter then
						-- Include all children of an included directory implicitly by removing the include filter for children.
						local saved_localDirIncludeOnly = nil
						if thisDirIncludeOnly and set.includeOnlyRecurses then
							saved_localDirIncludeOnly = set.localDirIncludeOnly
							set.localDirIncludeOnly = nil
						end
						local dirs = DirectoryGetSubdirectories("*", strPath)
						for _, dir in ipairs(dirs) do
							ProcessDir(dir)
						end
						-- Revert to previous setting if we've preserved the local directory.
						if thisDirIncludeOnly and set.includeOnlyRecurses then
							set.localDirIncludeOnly = saved_localDirIncludeOnly
						end
					end
				end
				-- Handle directory			
				if set.localDir and set.localDir ~= "" then
					ProcessDir(set.localDir, nil)		
				end
			end		
		end
		
		-- Different in tool then regular
		if IsToolBuild() then
			ProcessArchives()					
			ProcessDir()
		else
			ProcessDir()
			ProcessArchives()					
		end
		
		
		-- always autoapply	in non tool		
		-- otherwise, only apply 
		if not IsToolBuild() or not set.version or set.version == "trunk" then		
			table.insert(autoapplylist,set.gameDataName)		
				
			if hasMainSet and set.enableMode == "constant" then
				table.insert(autoapplylist,set.setName)			
			end		
		end
		
		-- Let the engine know about it
		RegisterResourceDescriptionWithEngine( set )
	end
	
	-- Do this after for any dependencies
	local configureMap = {}
	for i,setName in ipairs(autoapplylist) do
		configureMap[setName] = true
	end

	--[[
	-- Writeback cache
	if IsToolBuildRaw() and not ResourceSetExists( "Writeback Cache" ) then
		ResourceInitializeWritebackCache()
		if ResourceSetExists( "Writeback Cache" ) then
			configureMap["Writeback Cache"] = true
		end
	end]]

	ResourceSetReconfigure( configureMap )

	AssembleResourcesEnd()
end      