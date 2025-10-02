-- Telltale Editor generated Lua documentation. Using v1.0.0


--- Creates a concrete directory location in the resource system, essentially mounting it to a physical folder on the users computer device, Physical path is relative to the working directory or
--- absolute if you prefer.
--- @param name nil
--- @param path nil
--- @return nil
function ResourceCreateConcreteDirectoryLocation(name, path)
end

--- Creates a concrete archive location to mount into the resource system. The first argument is here for compatibilty! You should not use this function unless using it for debugging and
--- personal tools inside TTE (ie don't use in a telltale game). ArchiveID should be the value of location + name + a '/' at the end. It is default done
--- this way overriding what you pass in. The name is the archive file name so should end in .ttarch2 or .ttarch. The location is the existing (or this does nothing)
--- resource location the archive exists inside of. Techically you can also pass in the cache mode as a fourth argument but prefer to use the set cache mode function.
--- @param --[[ignored]] archiveID nil
--- @param name nil
--- @param location nil
--- @return nil
function ResourceCreateConcreteArchiveLocation(--[[ignored]] archiveID, name, location)
end

--- Loads a resource. Must be a telltale file. If loaded, returns the previous loaded instance. This returns a *weak* reference, so when the object is unloaded then it is invalidated.
--- @param address nil
--- @return resource
function Load(address)
end

--- Loads a resource. Must be a telltale file. If loaded, returns the previous loaded instance. This returns a *weak* reference, so when the object is unloaded then it is invalidated.
--- @param address nil
--- @return resource
function LoadAsync(address)
end

--- Loads a resource. Must be a telltale file. If loaded, returns the previous loaded instance. This returns a *weak* reference, so when the object is unloaded then it is invalidated.
--- @param address nil
--- @return resource
function LoadAsyncAndWait(address)
end

--- Copies a resource. If the file is meta described, then the source must be loaded and the new resource will be loaded into the cache with destName. Else copies resource
--- files.
--- @param sourceName nil
--- @param destName nil
--- @return bool
function ResourceCopy(sourceName, destName)
end

--- Returns if the given archive is currently active, ie is loaded. Pass in the archive name.
--- @param archiveName nil
--- @return bool
function ResourceArchiveIsActive(archiveName)
end

--- Gets the resource name string from the given resource address / URL.
--- @param address nil
--- @return string
function ResourceAddressGetResourceName(address)
end

--- Yes its spelt wrong by Telltale. Finds all archive locations. Mask must be non-null, eg *.ttarch2.
--- @param mask nil
--- @return nil
function ResourceAchiveFind(mask)
end

--- TTE Uses a different preloading system but this would normally advance to the next preloading batch of files. Don't use.
--- @param count nil
--- @return nil
function ResourceAdvancePreloadBatch(count)
end

--- Does nothing, here for compatibility reasons. This function was used externally only for episode download management.
--- @param archiveID nil
--- @param mode nil
--- @return nil
function ResourceArchiveSetCacheMode(archiveID, mode)
end

--- Does nothing, here for compatibility reasons. This function was used externally only for episode download management.
--- @param archiveID nil
--- @return nil
function ResourceArchiveWaitForCaching(archiveID)
end

--- Creates a logical location with the given name. Returns false if it already existed. It must start and end with <> angle brackets.
--- @param name nil
--- @return nil
function ResourceCreateLogicalLocation(name)
end

--- Deletes the resource from the loaded resources, essentially unloading it. If not saved, changes could be lost. Returns if it was deleted. May return false if the resource is set
--- as non purgable.
--- @param resource nil
--- @return bool
function ResourceDelete(resource)
end

--- Returns if the given resource exists. If you pass in an actual loaded resource (eg propertySet) then this will return true if its valid. If you pass in a URL
--- or resource name then it returns if it exists somewhere in the resource system.
--- @param resource nil
--- @return bool
function ResourceExists(resource)
end

--- Returns if the given logical location exists.
--- @param locationStr nil
--- @return bool
function ResourceExistsLogicalLocation(locationStr)
end

--- Does nothing, here for compatibility reasons
--- @return nil
function ResourceGetLoadingCall()
end

--- Gets the string name of the given resource. The resource is a currently loaded resource such as a propertySet.
--- @param resource nil
--- @return string
function ResourceGetName(resource)
end

--- Gets all of the resource names that match the given mask in the whole resource system. Returns tabled indexed 1 to N. Prefer to use the get names version which
--- ensures no symbol hash strings are returned
--- @param mask nil
--- @return table
function ResourceGetSymbols(mask)
end

--- Gets all of the resource names that match the given mask in the whole resource system. Returns tabled indexed 1 to N.
--- @param mask nil
--- @return table
function ResourceGetNames(mask)
end

--- Gets the resource address of the given resource name with the current resource sets applied. Will include its resource location.
--- @param resourceName nil
--- @return string
function ResourceGetURL(resourceName)
end

--- Returns the URL as a full physical concrete path. You can pass in a folder or a filename address.
--- @param urlString nil
--- @return string
function ResourceGetURLAsLocal(urlString)
end

--- Does not do anything, only exists for compatibility reasons.
--- @return nil
function ResourceInitializeWritebackCache()
end

--- Does not do anything, only exists for compatibility reasons.
--- @return nil
function ResourceInitializeLegacySubproject()
end

--- Does not do anything, only exists for compatibility reasons.
--- @return nil
function ResourceInitializeSubprojects()
end

--- Returns true if the given resource is currently loaded in the resource system.
--- @param resourceName nil
--- @return bool
function ResourceIsLoaded(resourceName)
end

--- Gets all resource names inside the given resource location. You also need to specify a mask for file names. Use * for all files. This versions gets them as symbols
--- which are not gaurunteed to be strings. Prefer to use the get names version.
--- @param locationName nil
--- @param mask nil
--- @return table
function ResourceLocationGetSymbols(locationName, mask)
end

--- Gets all resource names inside the given resource location. You also need to specify a mask for file names. Use * for all files.
--- @param locationName nil
--- @param mask nil
--- @return table
function ResourceLocationGetNames(locationName, mask)
end

--- Injects a concrete resource location into the resource system. The name will be equal to the physical path. Returns if succeeds, can fail if it already exists.
--- @param physicalPath nil
--- @return bool
function ResourceLocationInjectIntoResourceSystem(physicalPath)
end

--- Does not do anything, only exists for compatibility reasons.
--- @return nil
function ResourceOpenUser()
end

--- Prints to the console all assets that are currently loaded (ie referenced).
--- @return nil
function ResourceReportReferencedAssets()
end

--- Returns the concrete location ID for the given resource address.
--- @param urlStr nil
--- @return string
function ResourceResolveAddressToConcreteLocationID(urlStr)
end

--- Resolves the URL to a concrete URL with the currently active resource sets. Returns nil if invalid url is passed in.
--- @param urlStr nil
--- @return string
function ResourceResolveURLToConcrete(urlStr)
end

--- Does not do anything, here for compatibility.
--- @return nil
function ResourceRetryFailedResDescs()
end

--- Does not do anything, here for compatibility. Used in old games.
--- @param file nil
--- @return nil
function ResourceSaveManifest(file)
end

--- Does not do anything, here for compatibility. Used in old games.
--- @param file nil
--- @return nil
function ResourceLoadManifest(file)
end

--- Changes the priority of the given resource set.
--- @param setName nil
--- @param priority nil
--- @return nil
function ResourceSetChangePriority(setName, priority)
end

--- Creates a resource set at runtime. All arguments apart from the name are optional and default to false / 0.
--- @param name nil
--- @param priority nil
--- @param isDynamic nil
--- @param isBootable nil
--- @param isSticky nil
--- @return nil
function ResourceSetCreate(name, priority, isDynamic, isBootable, isSticky)
end

--- Sets the default resource location to create resources in.
--- @param locationName nil
--- @return nil
function ResourceSetDefaultLocation(locationName)
end

--- Destroys the given resource set, removing it and disabling it. See the disable function. Force is optional and default false.
--- @param setName nil
--- @param force nil
--- @return bool
function ResourceSetDestroy(setName, force)
end

--- Disables the resource set, optionally specifying to force that it gets disabled. Force is default true and optional. Returns if it was disabled.
--- @param setName nil
--- @param force nil
--- @return bool
function ResourceSetDisable(setName, force)
end

--- Enables the given resource set, meaning all resource names that try to get loaded will be looked into the given set. Optionally specify an override priority (default uses priority in
--- the resource set description). Higher priority resource sets get searched first, meaning they override other files. Priorities can be negative or positive, any integer.
--- @param setName nil
--- @param priorityOverride nil
--- @return nil
function ResourceSetEnable(setName, priorityOverride)
end

--- Returns if the given resource set is currently applied in the resource system, such that it maps resource names to its concrete resource locations.
--- @param setName nil
--- @return bool
function ResourceSetEnabled(setName)
end

--- Returns if the resource set exists in the resource system.
--- @param setName nil
--- @return bool
function ResourceSetExists(setName)
end

--- Gets a table of all resource set names, indexed 1 to N. The first argument must be nil or a string but is ignored. The last three arguments are all
--- optional and default to false. They specify how to filter the resource sets.
--- @param unused nil
--- @param onlyDynamic nil
--- @param onlyBootable nil
--- @param onlySticky nil
--- @return table
function ResourceSetGetAll(unused, onlyDynamic, onlyBootable, onlySticky)
end

--- Returns if the given resource set is sticky. These tends to be localisation resource sets.
--- @param setName nil
--- @return bool
function ResourceSetIsSticky(setName)
end

--- Returns if the given resource set is bootable. These can be booted from.
--- @param setName nil
--- @return bool
function ResourceSetIsBootable(setName)
end

--- Returns if the given resource set is dynamic. These are dynamic.
--- @param setName nil
--- @return bool
function ResourceSetIsDynamic(setName)
end

--- Does nothing and provided for compatibility.
--- @param string nil
--- @return nil
function ResourceSetLoadingCall(string)
end

--- Maps a resource location to another concrete one for the given resource set. Turn it off and back on again to have effect if already enabled.
--- @param setName nil
--- @param destLocation nil
--- @param srcLocation nil
--- @return nil
function ResourceSetMapLocation(setName, destLocation, srcLocation)
end

--- Sets if the resource is non purgable or default (default not). This means it won't be unloaded until set off again.
--- @param resourceName nil
--- @param bOnOff nil
--- @return nil
function ResourceSetNonPurgable(resourceName, bOnOff)
end

--- Reconfigures resource set priorities. The argument should be a table of resource set name to priority override integer. If the priority is a boolean then it is treated as 'should
--- this resource set be turned on or off', and the priority is left unchanged. By setting it to a number if will override the priority of it and enable it.
--- Using a priority value of 0 acts as disabling it too.
--- @param sets nil
--- @return nil
function ResourceSetReconfigure(sets)
end

--- Returns if the given resource name exists inside the given resource set in any one of its mapped locations.
--- @param setName nil
--- @param resourceName nil
--- @return bool
function ResourceSetResourceExists(setName, resourceName)
end

--- Gets the concrete location ID which the resource name in the given resource set is inside. For example, could return the name of the texture mesh archive if its in
--- that archive for the given resource set. Returns nil if not found.
--- @param setName nil
--- @param resourceName nil
--- @return string
function ResourceSetGetLocationID(setName, resourceName)
end

--- Unused and here for compatibility
--- @return nil
function ResourceSetResourceGetURL()
end

--- Save the resource. It must be loaded and have an associated location to know where to save it.
--- @param resName nil
--- @return bool
function Save(resName)
end

--- Revert the object to reflect what is loaded on disk. Pass in the resource name and not path. It must be loaded else returns false.
--- @param resName nil
--- @return bool
function Revert(resName)
end

--- Create a brand new resource. Path is a resource address, so include the optional (default master loc mount point) logical identifier, eg logical://<User>/SaveGame.file
--- @param resAddr nil
--- @return bool
function Create(resAddr)
end

--- Registers a resource set description to the attached resource registry
--- @param set nil
--- @return nil
function RegisterSetDescription(set)
end

--- Prints all resource locations. Must have an attached resource registry.
--- @return nil
function ResourcePrintLocations()
end

--- Adds build information dates. Ignored.
--- @param info nil
--- @return nil
function GameEngine_AddBuildVersionInfo(info)
end

--- Removes the file extension from the string
--- @param filename nil
--- @return string
function FileStripExtension(filename)
end

--- Because Telltale employees can't spell this exists.
--- @param filename nil
--- @return string
function FileStripExtention(filename)
end

--- Sets the file extension of the given filename
--- @param filename nil
--- @param extension nil
--- @return string
function FileSetExtension(filename, extension)
end

--- Because Telltale employees are dyslexic this exists! <3
--- @param filename nil
--- @param extension nil
--- @return string
function FileSetExtention(filename, extension)
end

--- Does nothing, not used in TTE or the runtime engine.
--- @param filename nil
--- @return nil
function FileMakeWriteable(filename)
end

--- Does nothing, not used in TTE or the runtime engine.
--- @param filename nil
--- @return nil
function FileMakeReadOnly(filename)
end

--- Does nothing, not used in TTE or the runtime engine.
--- @return nil
function FileClearLastErrorCorruptSaveFile()
end

--- Does nothing, not used in TTE or the runtime engine.
--- @return nil
function FileIsLastErrorCorruptSaveFile()
end

--- Get the file name, removing any path and extensions.
--- @param filename nil
--- @return string
function FileGetFileName(filename)
end

--- Removes the file extension without the dot of the given filename.
--- @param filename nil
--- @return string
function FileGetExtension(filename)
end

--- Find first file. Returns nil when done. This is a legacy API function and is only here for compatibility!
--- @param strMask nil
--- @param strPath nil
--- @return string
function FileFindFirst(strMask, strPath)
end

--- Find next file. Returns nil when done. This is a legacy API function and is only here for compatibility! The mask should be the same as the previous calls.
--- @param strMask nil
--- @return string
function FileFindNext(strMask)
end

--- Checks if the global pathname (from C:/ or / in unix) exists.
--- @param filename nil
--- @return bool
function FileExistsGlobal(filename)
end

--- Checks whether the file exists in the resource system. Pass in the full specified address or just the file name.
--- @param fileaddr nil
--- @return bool
function FileExists(fileaddr)
end

--- Deletes the file in the mounted resource system. Pass resource address or just file name.
--- @param addr nil
--- @return nil
function FileDelete(addr)
end

--- Copies file source to file destination. Pass in the resource addresses with schemes (default looks in master)> the destination address must be concrete so it should be logical pointing to
--- a concrete directory.
--- @param srcAddr nil
--- @param dstAddr nil
--- @return nil
function FileCopy(srcAddr, dstAddr)
end

--- Get the number of elements in a container.
--- @param container nil
--- @return int
function ContainerGetNumElements(container)
end

--- Remove the item at index from the container
--- @param container nil
--- @param index nil
--- @return nil
function ContainerRemoveElement(container, index)
end

--- Get the element name at the given index in the container. For keyed containers such as Map<K,V> then this will return the key as a string, for non keyed will
--- return the index as a string - 0 based as a string.
--- @param container nil
--- @param index nil
--- @return string
function ContainerGetElementName(container, index)
end

--- Sets an element in the container by either direct index. If a keyed container such as Map<K,V> then provide the key value there instead. Element is the value.
--- @param container nil
--- @param index_or_key nil
--- @param element nil
--- @return nil
function ContainerSetElement(container, index_or_key, element)
end

--- Insert at an item at the end of the container. Specify the key value if its a map type (3rd argument), if not a default value is used (0, empty
--- string etc).
--- @param container nil
--- @param element nil
--- @param --[[optional]] key nil
--- @return nil
function ContainerInsertElement(container, element, --[[optional]] key)
end

--- Emplace and return an element at the end of the container. This is part of TTE only. Please note this returns the meta class instance and not the lua type
--- equivalent. For other use like Telltale API, use set element (new games) or just insert element.
--- @param container nil
--- @return obj
function ContainerEmplaceElement(container)
end

--- Get the item in the container. If the container is keyed then pass in the key, else the 0-based index. This returns the element as a lua type if possible
--- (see documentation on coercable types). If not, this will return the meta instance for that element. This is for values such as compound classes such as skeleton entry for example,
--- which aren't intrinsically convertible like strings.
--- @param container nil
--- @param index_or_key nil
--- @return obj
function ContainerGetElement(container, index_or_key)
end

--- Clear the container
--- @param container nil
--- @return nil
function ContainerClear(container)
end

--- Allocate backend memory for the container to reduce memory allocations until size is bigger than capacity
--- @param container nil
--- @param capacity nil
--- @return nil
function ContainerReserve(container, capacity)
end

--- Remove a luaFunction that was registered for changes to a key in keysFromPropertySet when modified. If keysFromPropertySet is nil, then the callback is removed for each key in propertySet. Optional
--- 4th parameter to search parents (default true).
--- @param propertySet nil
--- @param keysFromPropertySet nil
--- @param luaFuncName nil
--- @return nil
function PropertyRemoveMultiKeyCallback(propertySet, keysFromPropertySet, luaFuncName)
end

--- Remove the luaFunction registered to be called whenever the key in the given property set is modified.
--- @param propertySet nil
--- @param key nil
--- @param luaFuncName nil
--- @return nil
function PropertyRemoveKeyCallback(propertySet, key, luaFuncName)
end

--- Add a luaFunction to be called whenever a key in keysFromPropertySet is modified. If keysFromPropertySet is nil, then the callback will occur for any key modified in propertySet. The function
--- gets called with the key and the new data for the key. Optional 4th argument to use all non-local parent keys when traversing keys from. It is default true. The
--- keys from property set is not cached in any way and the keys with adding callbacks are taken at this call. Keys from can also be nil meaning the callback
--- is added to every single value in the property set at the time of this call.
--- @param propertySet nil
--- @param keysFromPropertySet nil
--- @param luaFuncName nil
--- @return nil
function PropertyAddMultiKeyCallback(propertySet, keysFromPropertySet, luaFuncName)
end

--- Adds a callback to be called every time the key is modified. Pass in the lua function name for compatibility, while in most new games the function can also be
--- the function lua object. The function gets called with the key and the new data for the key.
--- @param propertySet nil
--- @param key nil
--- @param luaFuncName nil
--- @return nil
function PropertyAddKeyCallback(propertySet, key, luaFuncName)
end

--- Sets the property set as modified, calling every local key in the property set's callback.
--- @param propertySet nil
--- @return nil
function PropertySetModified(propertySet)
end

--- Does nothing (in the engine either).
--- @param propertySet nil
--- @return nil
function PropertyClearKeyCallbacks(propertySet)
end

--- Returns the input property set if the property set is a runtime property set, else nil.
--- @param propertySet nil
--- @return propertySet
function PropertyIsRuntime(propertySet)
end

--- Returns the number of keys in the property set. Set the second argument to true to count global parent keys as well.
--- @param propertySet nil
--- @param countGlobals nil
--- @return int
function PropertyNumKeys(propertySet, countGlobals)
end

--- Moves the given parent property to the front of the parent array. Does not need to contain it, if it does it is moved, else added to the front. This
--- means that it will be searched first for any non local keys.
--- @param propertySet nil
--- @param parentName nil
--- @return nil
function PropertyMoveGlobalToFront(propertySet, parentName)
end

--- Clears all local property keys.
--- @param propertySet nil
--- @return nil
function PropertyClearKeys(propertySet)
end

--- Gets all global parent property file names.
--- @param propertySet nil
--- @return table
function PropertyGetGlobals(propertySet)
end

--- Tests if the given property is a container type, such as an array or map.
--- @param propertySet nil
--- @param keyName nil
--- @return bool
function PropertyIsContainer(propertySet, keyName)
end

--- Returns true if the given property exists, false otherwise. Optional 3rd parameter to check parents, default true.
--- @param prop nil
--- @param keyName nil
--- @return bool
function PropertyExists(prop, keyName)
end

--- Does this property set use this parent? Optional 3rd parameter to check parents, default true
--- @param propertySet nil
--- @param parentPropertySet nil
--- @return bool
function PropertyHasGlobal(propertySet, parentPropertySet)
end

--- Is the key 'local' to the property set? This means that it exists in the property set and not through one of the parents.
--- @param propertySet nil
--- @param key nil
--- @return bool
function PropertyIsLocal(propertySet, key)
end

--- Make the key 'local' to the property set. See IsLocal.
--- @param propertySet nil
--- @param key nil
--- @return bool
function PropertyMakeLocal(propertySet, key)
end

--- Add a global parent to a property set. Parent property set should be a symbol or string filename.
--- @param propertySet nil
--- @param parentPropertySet nil
--- @return nil
function PropertyAddGlobal(propertySet, parentPropertySet)
end

--- Add a value to the property set. Valid type names are: "bool", "Color", etc. Optional 4th argument is the property value to assign. Don't include and 'class ' etc specifiers,
--- for compatibility with all games (you can if you really want)
--- @param propertySet nil
--- @param keyName nil
--- @param typeName nil
--- @param --[[optional]] value nil
--- @return nil
function PropertyCreate(propertySet, keyName, typeName, --[[optional]] value)
end

--- Set a value in a property set. The key name must exist.
--- @param propertySet nil
--- @param keyName nil
--- @param value nil
--- @return nil
function PropertySet(propertySet, keyName, value)
end

--- Get a value from the property set
--- @param propertySet nil
--- @param keyName nil
--- @return value
function PropertyGet(propertySet, keyName)
end

--- Flags this property set to ignore runtime changes (so they aren't saved in save games.
--- @param propertySet nil
--- @return nil
function PropertyDontSaveInSaveGames(propertySet)
end

--- Import all the keys in property set "keysFromPropertySet" from property set"sourcePropertySet" to property set "destPropertySet". 'keysfromPropertySet' is the filter.
--- @param destPropertySet nil
--- @param sourcePropertySet nil
--- @param keysFromPropertySet nil
--- @return nil
function PropertyImportKeyValues(destPropertySet, sourcePropertySet, keysFromPropertySet)
end

--- Print the keys of the property set
--- @param propertySet nil
--- @return nil
function PropertyPrintKeys(propertySet)
end

--- Returns the type name of the property value type
--- @param propertySet nil
--- @param key nil
--- @return string
function PropertyGetKeyType(propertySet, key)
end

--- Remove a value from the property set
--- @param propertySet nil
--- @param keyName nil
--- @return nil
function PropertyRemove(propertySet, keyName)
end

--- Remove a global parent from a property set. Optional 3rd parameter being 1 to keep any local keys overriden by the global parent (default discards)
--- @param propertySet nil
--- @param parentPropertySet nil
--- @return nil
function PropertyRemoveGlobal(propertySet, parentPropertySet)
end

--- Get the property set a key is introduced from
--- @param propertySet nil
--- @param keyName nil
--- @return propertySet
function PropertyGetKeyPropertySet(propertySet, keyName)
end

--- Get the property set a value is retrieved from. This is different to PropertyGetKeyPropertySet, where it finds the prop where the key is introduced from, while this finds the local'est'
--- property set (closest to input set) there is an overriding value in for the key.
--- @param propertySet nil
--- @param keyName nil
--- @return propertySet
function PropertyGetValuePropertySet(propertySet, keyName)
end

--- Get all the keys in a property set.
--- @param propertySet nil
--- @return table
function PropertyKeys(propertySet)
end

--- Telltale Property Keys
--- @type string
kLightWrapAround = ""

--- Telltale Property Keys
--- @type string
kLightKeyLight = ""

--- Telltale Property Keys
--- @type string
kBokehAberrationOffsetsY = ""

--- Telltale Property Keys
--- @type string
kLightIntensityDiffuse = ""

--- Telltale Property Keys
--- @type string
kLightIntensity = ""

--- Telltale Property Keys
--- @type string
kBokehBrightnessThreshold = ""

--- Telltale Property Keys
--- @type string
kEmptySymbol = ""

--- Telltale Property Keys
--- @type string
kLightColour = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFogNearPlane = ""

--- Telltale Property Keys
--- @type string
kLightIntensitySpecular = ""

--- Telltale Property Keys
--- @type string
kCellBlendMode = ""

--- Telltale Property Keys
--- @type string
kAnimationConstraintsPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAlphaAntialiasing = ""

--- Telltale Property Keys
--- @type string
kLightColourDark = ""

--- Telltale Property Keys
--- @type string
kLightMaxDistance = ""

--- Telltale Property Keys
--- @type string
kLightMinDistance = ""

--- Telltale Property Keys
--- @type string
kSceneGlowSigmaScale = ""

--- Telltale Property Keys
--- @type string
kDynamicOnLightMap = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapDOFEnabled = ""

--- Telltale Property Keys
--- @type string
kLightShadowMax = ""

--- Telltale Property Keys
--- @type string
kTextLeading = ""

--- Telltale Property Keys
--- @type string
kCellBlendWeight = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldFallOffFar = ""

--- Telltale Property Keys
--- @type string
kLightTurnedOn = ""

--- Telltale Property Keys
--- @type string
kLightShadowDistanceFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneAmbientColor = ""

--- Telltale Property Keys
--- @type string
kSceneWindDirection = ""

--- Telltale Property Keys
--- @type string
kLightShadowCascades = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyNPRLineFalloffBiasOverride = ""

--- Telltale Property Keys
--- @type string
kSceneRenderLayer = ""

--- Telltale Property Keys
--- @type string
kTextSkew = ""

--- Telltale Property Keys
--- @type string
kLightShadowBias = ""

--- Telltale Property Keys
--- @type string
kNprSpecularIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteTintEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXBloomIntensity = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureShearOrigin = ""

--- Telltale Property Keys
--- @type string
kSceneFXNoiseScale = ""

--- Telltale Property Keys
--- @type string
kLightDimmer = ""

--- Telltale Property Keys
--- @type string
kSceneFogColor = ""

--- Telltale Property Keys
--- @type string
kSceneShadowColor = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvBakeEnabled = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureScale = ""

--- Telltale Property Keys
--- @type string
kMoviePlayerPropName = ""

--- Telltale Property Keys
--- @type string
kTextMaxLinesToDisplay = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOHemisphereBias = ""

--- Telltale Property Keys
--- @type string
kLightColorCorrection = ""

--- Telltale Property Keys
--- @type string
kSceneVignettePropName = ""

--- Telltale Property Keys
--- @type string
kLightToonPriority = ""

--- Telltale Property Keys
--- @type string
kTextPropName = ""

--- Telltale Property Keys
--- @type string
kLightToonOpacity = ""

--- Telltale Property Keys
--- @type string
kSceneFogFarPlane = ""

--- Telltale Property Keys
--- @type string
kSceneFootstepWalkBoxes = ""

--- Telltale Property Keys
--- @type string
kLightType = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldFallOffNear = ""

--- Telltale Property Keys
--- @type string
kLightSpotAlpha = ""

--- Telltale Property Keys
--- @type string
kLightGroups = ""

--- Telltale Property Keys
--- @type string
kSceneWindGustSeparationExponent = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFFOVAdjustEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFDebug = ""

--- Telltale Property Keys
--- @type string
kLightSpotInnerRadius = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesThickness = ""

--- Telltale Property Keys
--- @type string
kSceneTimeScale = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFNearMax = ""

--- Telltale Property Keys
--- @type string
kLightSpotOuterRadius = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvReflectionIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneAudioReverbDefinition = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarScale = ""

--- Telltale Property Keys
--- @type string
kCellLightBlendMask = ""

--- Telltale Property Keys
--- @type string
kLightSpotTexture = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarPlaneMaxScale = ""

--- Telltale Property Keys
--- @type string
kLightSpotAlphaMode = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvSaturation = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOMaxRadiusPercent = ""

--- Telltale Property Keys
--- @type string
kLightRenderLayer = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureTranslate = ""

--- Telltale Property Keys
--- @type string
kSceneRenderPriority = ""

--- Telltale Property Keys
--- @type string
kCell0Colour = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesDepthMagnitude = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureShear = ""

--- Telltale Property Keys
--- @type string
kUIDialogPropName = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldMaxNear = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureRotate = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRenderAfterAntiAliasing = ""

--- Telltale Property Keys
--- @type string
kSceneFXColorOpacity = ""

--- Telltale Property Keys
--- @type string
kLightSpotTextureRotateOrigin = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarPivots = ""

--- Telltale Property Keys
--- @type string
kLightAmbientOcclusion = ""

--- Telltale Property Keys
--- @type string
kMaterialParticlePropName = ""

--- Telltale Property Keys
--- @type string
kLightRimWrapAround = ""

--- Telltale Property Keys
--- @type string
kSceneHBAODebug = ""

--- Telltale Property Keys
--- @type string
kLightRimIntensity = ""

--- Telltale Property Keys
--- @type string
kTextDialogFile = ""

--- Telltale Property Keys
--- @type string
kTextPercentToDisplay = ""

--- Telltale Property Keys
--- @type string
kLightRimOcclusion = ""

--- Telltale Property Keys
--- @type string
kCell1Colour = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBrushNearDetailBias = ""

--- Telltale Property Keys
--- @type string
kCell2Colour = ""

--- Telltale Property Keys
--- @type string
kTextReferenceScreenSize = ""

--- Telltale Property Keys
--- @type string
kSceneAudioPlayerOrigin = ""

--- Telltale Property Keys
--- @type string
kCell3Colour = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFFarFalloff = ""

--- Telltale Property Keys
--- @type string
kLightStatic = ""

--- Telltale Property Keys
--- @type string
kSceneActiveCamera = ""

--- Telltale Property Keys
--- @type string
kAudioReverbOverride = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneAudioReverb = ""

--- Telltale Property Keys
--- @type string
kSceneExcludeFromSaveGames = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvTint = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFilmicPivot = ""

--- Telltale Property Keys
--- @type string
kSceneWalkBoxes = ""

--- Telltale Property Keys
--- @type string
kTextWidth = ""

--- Telltale Property Keys
--- @type string
kSceneInputEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneAudioListener = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOPropName = ""

--- Telltale Property Keys
--- @type string
kSceneAudioMaster = ""

--- Telltale Property Keys
--- @type string
kSelectablePropName = ""

--- Telltale Property Keys
--- @type string
kEnlightenAdaptiveProbeVolumePropName = ""

--- Telltale Property Keys
--- @type string
kFxDelayMotionBlur = ""

--- Telltale Property Keys
--- @type string
kFXLevelsBlackPoint = ""

--- Telltale Property Keys
--- @type string
kSceneAudioSnapshotSuite = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneSceneExportsToContentScene = ""

--- Telltale Property Keys
--- @type string
kSceneAudioEventBanks = ""

--- Telltale Property Keys
--- @type string
kScenePreloadable = ""

--- Telltale Property Keys
--- @type string
kSoundSnapshotPropName = ""

--- Telltale Property Keys
--- @type string
kScenePreloadShaders = ""

--- Telltale Property Keys
--- @type string
kSceneAfterEffectsEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneGenerateNPRLines = ""

--- Telltale Property Keys
--- @type string
kSceneNPRLinesFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneBrushPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFXColorEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneNPRLinesBias = ""

--- Telltale Property Keys
--- @type string
kSceneNPRLinesAlphaFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarFilmicShoulderIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneNPRLinesAlphaBias = ""

--- Telltale Property Keys
--- @type string
kSceneGlowClearColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRenderAfterPostEffects = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBrushScale = ""

--- Telltale Property Keys
--- @type string
kSceneFXAntiAliasing = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvIntensity = ""

--- Telltale Property Keys
--- @type string
kTextKeyFormatString = ""

--- Telltale Property Keys
--- @type string
kSceneFXTAAWeight = ""

--- Telltale Property Keys
--- @type string
kSceneFXColorEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneViewportPropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXColorTint = ""

--- Telltale Property Keys
--- @type string
kSceneFXSharpShadowsEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsBlackPoint = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushDOFEnable = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsIntensityHDR = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsWhitePoint = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAllowNPRLines = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsBlackPointHDR = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFFarMax = ""

--- Telltale Property Keys
--- @type string
kSceneFXLevelsWhitePointHDR = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyConstantAlpha = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapType = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapEnabled = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurInRadius = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapIntensity = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapWhitePoint = ""

--- Telltale Property Keys
--- @type string
kEnvironmentPropName = ""

--- Telltale Property Keys
--- @type string
kDecalPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyColorCorrection = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapBlackPoint = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFilmicShoulderIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFilmicToeIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFilmicSign = ""

--- Telltale Property Keys
--- @type string
kCullObjects = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarWhitePoint = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarBlackPoint = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarFilmicPivot = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyD3DMesh = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarFilmicToeIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapFarFilmicSign = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBEnabled = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyOverrideToonOutlineColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDisableLightBake = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBDOFEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvReflectionTexture = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBBlackPoints = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyGlobalScale = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushNearTexture = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBWhitePoints = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBPivots = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowLightBleedReduction = ""

--- Telltale Property Keys
--- @type string
kSceneWindGustSpeed = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBShoulderIntensities = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBToeIntensities = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBSigns = ""

--- Telltale Property Keys
--- @type string
kExcludeAgents = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFFar = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowAutoDepthBounds = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarBlackPoints = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarScaleBoost = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarWhitePoints = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarShoulderIntensities = ""

--- Telltale Property Keys
--- @type string
kLogicGroupLogicItemDlgNodePropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarToeIntensities = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLODBias = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvGroup = ""

--- Telltale Property Keys
--- @type string
kSceneFXTonemapRGBFarSigns = ""

--- Telltale Property Keys
--- @type string
kSceneFXBloomThreshold = ""

--- Telltale Property Keys
--- @type string
kTextOffset = ""

--- Telltale Property Keys
--- @type string
kSceneFXAmbientOcclusionEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXAmbientOcclusionIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneFXAmbientOcclusionFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneFXAmbientOcclusionRadius = ""

--- Telltale Property Keys
--- @type string
kSceneFXAmbientOcclusionLightmap = ""

--- Telltale Property Keys
--- @type string
kUseBokeh = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFNear = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFAutoFocusEnabled = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFogFarPlane = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldFar = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFNearFalloff = ""

--- Telltale Property Keys
--- @type string
kFootsteps2PropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFVignetteMax = ""

--- Telltale Property Keys
--- @type string
kPathToPropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXDOFCoverageBoost = ""

--- Telltale Property Keys
--- @type string
kSceneFXForceLinearDepthOffset = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarTexture = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteDOFEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteTint = ""

--- Telltale Property Keys
--- @type string
kAudioPlayerOriginOverride = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteCenter = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyCastShadowsAsStatic = ""

--- Telltale Property Keys
--- @type string
kSceneFXVignetteCorners = ""

--- Telltale Property Keys
--- @type string
kIKAttachPropName = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneHBAORadius = ""

--- Telltale Property Keys
--- @type string
kSceneViewportScissorTop = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOOcclusionScale = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOLuminanceScale = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOMaxDistance = ""

--- Telltale Property Keys
--- @type string
kSceneHBAODistanceFalloff = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyColorWrite = ""

--- Telltale Property Keys
--- @type string
kSceneHDRLightmapsIntensity = ""

--- Telltale Property Keys
--- @type string
kSceneHBAOBlurSharpness = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurMovementThresholdEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyShadowUseLowLOD = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRecieveShadows = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesDepthFadeNear = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesDepthFadeFar = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesDepthExponent = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAlphaMultiply = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesLightDirection = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesLightMagnitude = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesLightExponent = ""

--- Telltale Property Keys
--- @type string
kSceneScreenSpaceLinesDebug = ""

--- Telltale Property Keys
--- @type string
kSceneFogEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushNearOutlineScale = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowCascadeSplitBias = ""

--- Telltale Property Keys
--- @type string
kSceneFogAlpha = ""

--- Telltale Property Keys
--- @type string
kSceneFogNearPlane = ""

--- Telltale Property Keys
--- @type string
kSkeletonArmWidth = ""

--- Telltale Property Keys
--- @type string
kSceneWindSpeed = ""

--- Telltale Property Keys
--- @type string
kSceneHDRPaperWhiteNits = ""

--- Telltale Property Keys
--- @type string
kHLSPlayerPropName = ""

--- Telltale Property Keys
--- @type string
kSceneGraphicBlackThreshold = ""

--- Telltale Property Keys
--- @type string
kSceneGraphicBlackSoftness = ""

--- Telltale Property Keys
--- @type string
kSceneGraphicBlackAlpha = ""

--- Telltale Property Keys
--- @type string
kSceneGraphicBlackNear = ""

--- Telltale Property Keys
--- @type string
kSceneGraphicBlackFar = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurTint = ""

--- Telltale Property Keys
--- @type string
kSceneWindIdleStrength = ""

--- Telltale Property Keys
--- @type string
kSkeletonInstanceNodeName = ""

--- Telltale Property Keys
--- @type string
kSceneWindIdleSpacialFrequency = ""

--- Telltale Property Keys
--- @type string
kSceneWindGustStrength = ""

--- Telltale Property Keys
--- @type string
kSceneWindGustSpacialFrequency = ""

--- Telltale Property Keys
--- @type string
kSceneWindGustIdleStrengthMultiplier = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAmbientOcclusionLightmap = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvReflectionEnabled = ""

--- Telltale Property Keys
--- @type string
kFXColourEnabled = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvReflectionIntensity = ""

--- Telltale Property Keys
--- @type string
kSkeletonLegWidth = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvReflectionTint = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvReflectionIntensityShadow = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvEnabled = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldDebug = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvProbeData = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvIntensity = ""

--- Telltale Property Keys
--- @type string
kIKPropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvBackgroundColor = ""

--- Telltale Property Keys
--- @type string
kEnlightenCubemapTextureEnvPropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvProbeResolutionXZ = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvProbeResolutionY = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowMomentBias = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowDepthBias = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowPositionOffsetBias = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowMinDistance = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowMaxDistance = ""

--- Telltale Property Keys
--- @type string
kText2PropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvDynamicShadowMaxDistance = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvShadowMaxUpdates = ""

--- Telltale Property Keys
--- @type string
kTextColor = ""

--- Telltale Property Keys
--- @type string
kSceneLightShadowTraceMaxDistance = ""

--- Telltale Property Keys
--- @type string
kEnlightenSystemPropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightStaticShadowBoundsMin = ""

--- Telltale Property Keys
--- @type string
kSceneLightStaticShadowBoundsMax = ""

--- Telltale Property Keys
--- @type string
kSceneEnvLightShadowGoboTexture = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineEnable = ""

--- Telltale Property Keys
--- @type string
kClipPlaneFar = ""

--- Telltale Property Keys
--- @type string
kTextKerning = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineFilterEnable = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineSize = ""

--- Telltale Property Keys
--- @type string
kPostMaterialScenePropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineThreshold = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineColorThreshold = ""

--- Telltale Property Keys
--- @type string
kTextBackgroundColor = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushOutlineFalloff = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushNearScale = ""

--- Telltale Property Keys
--- @type string
kEnvironmentLightPropName = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushNearDetail = ""

--- Telltale Property Keys
--- @type string
kAudioReverbEventSnapshotOverride = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarDetail = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarPlane = ""

--- Telltale Property Keys
--- @type string
kSceneFXBrushFarPlaneFalloff = ""

--- Telltale Property Keys
--- @type string
kEnvironmentTilePropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDepthTestFunc = ""

--- Telltale Property Keys
--- @type string
kSceneFrameBufferScaleOverride = ""

--- Telltale Property Keys
--- @type string
kSceneFrameBufferScaleFactor = ""

--- Telltale Property Keys
--- @type string
kSceneSpecularMultiplierEnabled = ""

--- Telltale Property Keys
--- @type string
kTextBackground = ""

--- Telltale Property Keys
--- @type string
kSceneSpecularColorMultiplier = ""

--- Telltale Property Keys
--- @type string
kSceneSpecularIntensityMultiplier = ""

--- Telltale Property Keys
--- @type string
kVfxGroupPropName = ""

--- Telltale Property Keys
--- @type string
kSceneSpecularExponentMultiplier = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyD3DMeshList = ""

--- Telltale Property Keys
--- @type string
kSceneHDRLightmapsEnabled = ""

--- Telltale Property Keys
--- @type string
kCameraPropName = ""

--- Telltale Property Keys
--- @type string
kSceneCameraCutPositionThreshold = ""

--- Telltale Property Keys
--- @type string
kSceneCameraCutRotationThreshold = ""

--- Telltale Property Keys
--- @type string
kSceneViewportScissorLeft = ""

--- Telltale Property Keys
--- @type string
kSceneViewportScissorRight = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyMaskWrite = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldMaxFar = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLODScale = ""

--- Telltale Property Keys
--- @type string
kSceneViewportScissorBottom = ""

--- Telltale Property Keys
--- @type string
kSceneScenePhysicsData = ""

--- Telltale Property Keys
--- @type string
kTextFont = ""

--- Telltale Property Keys
--- @type string
kTextScale = ""

--- Telltale Property Keys
--- @type string
kTextNonProportionalScale = ""

--- Telltale Property Keys
--- @type string
kLightPropName = ""

--- Telltale Property Keys
--- @type string
kTextString = ""

--- Telltale Property Keys
--- @type string
kTextConfine = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldBlurStrength = ""

--- Telltale Property Keys
--- @type string
kTextDialogTextResource = ""

--- Telltale Property Keys
--- @type string
kTextAlignmentHorizontal = ""

--- Telltale Property Keys
--- @type string
kTextAlignmentVertical = ""

--- Telltale Property Keys
--- @type string
kTextShadowColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightmapScale = ""

--- Telltale Property Keys
--- @type string
kTextShadowHeight = ""

--- Telltale Property Keys
--- @type string
kBokehBrightnessDeltaThreshold = ""

--- Telltale Property Keys
--- @type string
kTextMinWidth = ""

--- Telltale Property Keys
--- @type string
kTextMinHeight = ""

--- Telltale Property Keys
--- @type string
kTextExtrudeX = ""

--- Telltale Property Keys
--- @type string
kSoundEventEmitterPropName = ""

--- Telltale Property Keys
--- @type string
kAnimationAbsoluteNode = ""

--- Telltale Property Keys
--- @type string
kTextExtrudeY = ""

--- Telltale Property Keys
--- @type string
kTextPlaybackSpeed = ""

--- Telltale Property Keys
--- @type string
kEnlightenProbeVolumePropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvEnable = ""

--- Telltale Property Keys
--- @type string
kTextRenderLayer = ""

--- Telltale Property Keys
--- @type string
kCinematicLightRigPropName = ""

--- Telltale Property Keys
--- @type string
kTextDialog2File = ""

--- Telltale Property Keys
--- @type string
kTextDialog2NodeName = ""

--- Telltale Property Keys
--- @type string
kCursorPropName = ""

--- Telltale Property Keys
--- @type string
kTextKeyAgentName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRenderLayer = ""

--- Telltale Property Keys
--- @type string
kTextKeyName = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldCoverageBoost = ""

--- Telltale Property Keys
--- @type string
kTextWorldSpaceZ = ""

--- Telltale Property Keys
--- @type string
kFootstepsPropName = ""

--- Telltale Property Keys
--- @type string
kTextAlphaMultiply = ""

--- Telltale Property Keys
--- @type string
kTextBackgroundAlphaMultiply = ""

--- Telltale Property Keys
--- @type string
kSkeletonPropName = ""

--- Telltale Property Keys
--- @type string
kSkeletonFile = ""

--- Telltale Property Keys
--- @type string
kBokehBlurThreshold = ""

--- Telltale Property Keys
--- @type string
kSkeletonUseProceduralJointCorners = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyForceLinearDepthWrite = ""

--- Telltale Property Keys
--- @type string
kClipPlaneNear = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldNear = ""

--- Telltale Property Keys
--- @type string
kCameraPush = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightmapUVGenerationType = ""

--- Telltale Property Keys
--- @type string
kAspectRatio = ""

--- Telltale Property Keys
--- @type string
kFieldOfView = ""

--- Telltale Property Keys
--- @type string
kFieldOfViewScale = ""

--- Telltale Property Keys
--- @type string
kCameraBlendEnable = ""

--- Telltale Property Keys
--- @type string
kExposure = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDepthTest = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldEnabled = ""

--- Telltale Property Keys
--- @type string
kUseHighQualityDOF = ""

--- Telltale Property Keys
--- @type string
kDepthOfFieldType = ""

--- Telltale Property Keys
--- @type string
kBokehPatternTexture = ""

--- Telltale Property Keys
--- @type string
kContextMenuPropName = ""

--- Telltale Property Keys
--- @type string
kBokehMinSize = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRenderCull = ""

--- Telltale Property Keys
--- @type string
kBokehMaxSize = ""

--- Telltale Property Keys
--- @type string
kEnlightenCubemapPropName = ""

--- Telltale Property Keys
--- @type string
kBokehFalloff = ""

--- Telltale Property Keys
--- @type string
kBokehAberrationOffsetsX = ""

--- Telltale Property Keys
--- @type string
kMaxBokehBufferAmount = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFogEnabled = ""

--- Telltale Property Keys
--- @type string
kExcludeFromAutoDirector = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyNPRLineBias = ""

--- Telltale Property Keys
--- @type string
kFXColourTint = ""

--- Telltale Property Keys
--- @type string
kFXColourOpacity = ""

--- Telltale Property Keys
--- @type string
kFXLevelsEnabled = ""

--- Telltale Property Keys
--- @type string
kFXLevelsWhitePoint = ""

--- Telltale Property Keys
--- @type string
kDialogPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeySceneLightmapData = ""

--- Telltale Property Keys
--- @type string
kFXLevelsIntensity = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeySceneEnlightenData = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurEnabled = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurOutRadius = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurTintIntensity = ""

--- Telltale Property Keys
--- @type string
kFxRadialBlurScale = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurIntensity = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurMovementThreshold = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurRotationThresholdEnabled = ""

--- Telltale Property Keys
--- @type string
kWalkAnimatorPropName = ""

--- Telltale Property Keys
--- @type string
kFxMotionBlurRotationThreshold = ""

--- Telltale Property Keys
--- @type string
kAudioListenerOverride = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKey3DAlpha = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAutoFocusEnable = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyAxisScale = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBakeAsStatic = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBrushFarDetailBias = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBrushForceEnable = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyBrushScaleByVertex = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyCameraFacing = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightCinematicRig = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyCameraFacingType = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyCastShadow = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeySSLineEnable = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyCubeBakeEnable = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDepthWrite = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyShadowReceiveGroup = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDepthWriteAlpha = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDiffuseColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyDoMotionBlur = ""

--- Telltale Property Keys
--- @type string
kSoundAmbientInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyEmissionColor = ""

--- Telltale Property Keys
--- @type string
kLookAtBlockingPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFogColor = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyFogOverride = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyVisibleThresholdScale = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyForceAsAlpha = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvNode = ""

--- Telltale Property Keys
--- @type string
kCinematicLightPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvReflectionEnable = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvShadowCastEnable = ""

--- Telltale Property Keys
--- @type string
kEnlightenPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightEnvShadowCastGroups = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyLightingGroups = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyMaskTest = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyMaterialTime = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyNPRLineFalloff = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRecieveShadowsDecal = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRecieveShadowsDoublesided = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRecieveShadowsIntensity = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRenderToonOutline = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyRimBumpScale = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyShadowCastGroup = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyShadowVisibleThresholdScale = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyStatic = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyTextureOverrides = ""

--- Telltale Property Keys
--- @type string
kRenderablePropKeyToonOutlineColor = ""

--- Telltale Property Keys
--- @type string
kAnimationRelativeNode = ""

--- Telltale Property Keys
--- @type string
kScenePropName = ""

--- Telltale Property Keys
--- @type string
kMaterialLegacyPropName = ""

--- Telltale Property Keys
--- @type string
kAnimationPropName = ""

--- Telltale Property Keys
--- @type string
k3dSoundPropName = ""

--- Telltale Property Keys
--- @type string
kLightProbePropName = ""

--- Telltale Property Keys
--- @type string
kAgentStatePropName = ""

--- Telltale Property Keys
--- @type string
kViewportPropName = ""

--- Telltale Property Keys
--- @type string
kSceneTonemapPropName = ""

--- Telltale Property Keys
--- @type string
kChorecorderPropName = ""

--- Telltale Property Keys
--- @type string
kRenderablePropName = ""

--- Telltale Property Keys
--- @type string
kCinematicLightBlockingPropName = ""

--- Telltale Property Keys
--- @type string
kEnlightenAutoProbeVolumePropName = ""

--- Telltale Property Keys
--- @type string
kEnvironmentLightGroupPropName = ""

--- Telltale Property Keys
--- @type string
kSoundReverbInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kLipSyncPropName = ""

--- Telltale Property Keys
--- @type string
kNavCamPropName = ""

--- Telltale Property Keys
--- @type string
kParticleAffectorPropName = ""

--- Telltale Property Keys
--- @type string
kParticleEmitterBasePropName = ""

--- Telltale Property Keys
--- @type string
kParticleEmitterMaterialPropName = ""

--- Telltale Property Keys
--- @type string
kParticleEmitterPropName = ""

--- Telltale Property Keys
--- @type string
kPhysicsObjectPropName = ""

--- Telltale Property Keys
--- @type string
kRolloverPropName = ""

--- Telltale Property Keys
--- @type string
kSceneAutofocusPropName = ""

--- Telltale Property Keys
--- @type string
kSceneBrushOutlinePropName = ""

--- Telltale Property Keys
--- @type string
kStyleIdleTransitionsPropName = ""

--- Telltale Property Keys
--- @type string
kSceneDOFPropName = ""

--- Telltale Property Keys
--- @type string
kSoundEventPreloadInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvPropName = ""

--- Telltale Property Keys
--- @type string
kSceneLightEnvAdvancedPropName = ""

--- Telltale Property Keys
--- @type string
kSceneSSLinesPropName = ""

--- Telltale Property Keys
--- @type string
kSceneTonemapRGBPropName = ""

--- Telltale Property Keys
--- @type string
kSoundListenerInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kSoundMusicInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kSoundSfxInterfacePropName = ""

--- Telltale Property Keys
--- @type string
kStylePropName = ""

--- Telltale Property Keys
--- @type string
kTriggerPropName = ""

--- Telltale Property Keys
--- @type string
kVoiceSpeakerPropName = ""

--- Telltale Property Keys
--- @type string
kProceduralLookAtPropName = ""

--- Telltale Property Keys
--- @type string
kMaterialDecalPropName = ""

--- Telltale Property Keys
--- @type string
kMaterialPostPropName = ""

--- Telltale Property Keys
--- @type string
kMaterialPropName = ""

--- Telltale Property Keys
--- @type string
kParticleIKPropName = ""

--- Telltale Property Keys
--- @type string
kSound3dParametersPropName = ""

--- Telltale Property Keys
--- @type string
kSoundDataPropName = ""

--- Telltale Property Keys
--- @type string
kRuntimeVisibilityKey = ""

--- Returns if currently being called from a worker thread.
--- @return bool
function IsIsolated()
end

--- This function is only available to mod scripts! Prints to the console all of the resource sets in the resource system, Must have a resource system attached!
--- @return nil
function TTE_PrintResourceSets()
end

--- This function is only available to mod scripts! Mounts a game data archive from the current game snapshot into the resource system, so that all of the files inside that
--- archive can be found. This supports .ttarch2/ttarch/iso/pk2. Physical path is the path on your local machine. Location ID should be in the format <XXX>/. Please note that the archive must
--- be from the current game snapshot! Otherwise it will fail to read due to incorrect encryptino and expected format.
--- @param locationID nil
--- @param physPath nil
--- @return nil
function TTE_MountArchive(locationID, physPath)
end

--- This function is only available to mod scripts! Mounts the resource system (like creating a concrete directory location) to the given physical path under the name locationID. Location ID should
--- be in the format <XXX>/. Physical path can be absolute or relative to your working directory. The last argument can be set to true to use the old telltale engine
--- resource system which had no resource set descriptions. This means that it will recurse all directories and add them all. Set this to true to just easily get all resources
--- quickly for debugging, so that they are all in resource system ready to be used.
--- @param locationID nil
--- @param physPath nil
--- @param forceLegacy nil
--- @return nil
function TTE_MountSystem(locationID, physPath, forceLegacy)
end

--- This function is only available to mod scripts! Switches the editor context to a new game snapshot. Note that this can only be called from a mod script at startup
--- or when no files are being read or processed. If it is called at one of those times, an error is thrown to the log and it does nothing.
--- @param gameID nil
--- @param platformName nil
--- @param vendorName nil
--- @return nil
function TTE_Switch(gameID, platformName, vendorName)
end

--- This function is only available to mod scripts! Opens and reads a meta stream, returning the base class instance that the file contains. This returns a strong reference to the
--- instance. If one argument is passed in, filename is resource address in the resource system where it is located. If two are passed in, the first must be the file
--- name string and the second is the archive (any version) which the file is located in.
--- @param address_or_file nil
--- @param --[[optional]] archive nil
--- @return instance
function TTE_OpenMetaStream(address_or_file, --[[optional]] archive)
end

--- This function is only available to mod scripts! Writes the given instance to the file located at the given resource address. Returns if it was successfully written.
--- @param address nil
--- @param instance nil
--- @return bool
function TTE_SaveMetaStream(address, instance)
end

--- This function is only available to mod scripts! Opens a TTARCH file which is located at the given file path on your computer (or relative, like all file paths, to
--- the executable). Returns the archive instance, a strong reference although this is not a class instance. You can then use this to list files and open files from it. Note
--- that this archive MUST match the version of the currently selected game,ie the archive must be from the game that the editor context is currently working with.
--- @param filepath nil
--- @return arc
function TTE_OpenTTArchive(filepath)
end

--- This function is only available to mod scripts! Opens a TTARCH2 file which is located at the given file path on your computer (or relative). Returns the archive instance, a
--- strong reference. This archive, like others, must be from the currently active game such that the encryption key matches.
--- @param filepath nil
--- @return arc
function TTE_OpenTTArchive2(filepath)
end

--- This function is only available to mod scripts! Returns a table (indices 1 to N) of all the file names in the given archive, which was previously opened with open
--- TTArchive or TTArchive2.
--- @param archive nil
--- @return table
function TTE_ArchiveListFiles(archive)
end

--- Performs a blowfish encryption on the given buffer class instance (must be kMetaClassInternalBinaryBuffer class). The second argument must be less than or equal to the size of the buffer, how
--- many bytes are to be encrypted or decrypted. The last argument is if we want to encrypt (true) or decrypt (false). The key used is the current game key. Note
--- that blowfish encrypts in blocks of eight, ie the last remaining bytes aren't encrypted. All blowfish is like this, however, so don't worry.
--- @param bufferMetaInstance nil
--- @param size nil
--- @param isEncrypt nil
--- @return nil
function TTE_Blowfish(bufferMetaInstance, size, isEncrypt)
end

--- Converts the container into a lua table and returns it. For keyed container, they are the table keys. For non keyed, they are 0 based indices.
--- @param container nil
--- @return table
function TTE_TableToContainer(container)
end

--- Inserts all key-value mappings from the input table into the container. For non keyed containers (eg arrays) keys are ignored and its added at the back.
--- @param container nil
--- @param table nil
--- @return table
function TTE_ContainerToTable(container, table)
end

--- Dumps to the logger all memory which has not been freed yet. This will contain a lot of script stuff which won't matter as the script engine requires memory which
--- s tracked, however can be useful for tracking specific script object allocations.
--- @return nil
function TTE_DumpMemoryLeaks()
end

--- Returns the information about the currently active game. The returned table contains keys string 'Name', string 'ID', and string 'Vendor'
--- @return table
function TTE_GetActiveSnapshot()
end

--- If the first parameter is false, debug builds will throw an assert and break in the debugger
--- @param bool nil
--- @param errorMessage nil
--- @return nil
function TTE_Assert(bool, errorMessage)
end

--- Logs to the editor logger, the string argument.
--- @param valueStr nil
--- @return nil
function TTE_Log(valueStr)
end

--- Gets the platform name which the Telltale Editor is running on. This will return strings 'Windows' or 'MacOS', 'Linux' and etc for others in the future.
--- @return string
function TTE_GetPlatform()
end

--- This is used in the Games.lua script to register a game to the Meta system on initialisation. It takes in a table which must have keys string Name, string ID,
--- bool ModifiedEncryption, string DefaultMetaVersion,string LuaVersion, int/table ArchiveVersion, string/table Key, bool IsArchive2, table Platforms, table Vendors and pushed capabilities. Archive
--- version and key are either one static key/version or a table of snapshot ID to it. This is just 'Platform/Vendor' if there are more than just the default (empty string)
--- vendor. Else its just the platform name. See existing examples. If vendors are specified, then DefaultVendor must be as well as a string. The 'CommonSelector' key is also useful. It
--- should specify a function *name* which takes in the platform, vendor and common class type and returns the class name and version number of the common class to create for
--- that snapshot and class type. Optional, defaults to the normal class name and version 0. If specified, return just nil and 0 to use default class name as well. Else
--- do special checks dependent on that snapshot.
--- @param gameInfoTable nil
--- @return nil
function MetaRegisterGame(gameInfoTable)
end

--- Push a game capability to the game table. Call before registering the game table.
--- @param gameTable nil
--- @param cap nil
--- @return nil
function MetaPushGameCapability(gameTable, cap)
end

--- Push an executable hash which maps to the given platform/vendor pair. This is used for the Editor when user selects the mount points of their installation such that we can
--- detect their installation game snapshot.
--- @param gameTable nil
--- @param hashStr nil
--- @param platform nil
--- @param vendor nil
--- @return nil
function MetaPushExecutableHash(gameTable, hashStr, platform, vendor)
end

--- This associates file name masks to folders. When extracting files in the Telltale Editor sometimes the user can select to extract to sub-folders. This function associates masks to sub-folders which
--- the game engines. For example, a typical mapping is any files matching 'module_*.prop' should go into 'Properties/Primitives/'. You should use forward slashes and end with one, although if not it
--- will be changed for you. This should only be called during initialisation.
--- @param gameID nil
--- @param mask nil
--- @param folder nil
--- @return nil
function MetaAssociateFolderExtension(gameID, mask, folder)
end

--- This registers intrinsic types required for the meta system for the current game. This should only be called a game meta classes registration script (eg WD100.lua).
--- @return nil
function MetaRegisterIntrinsics()
end

--- This registers a class to the meta system, passing in the information table. This must only be called in the initialisation. Table must contain the Name string, VersionIndex number, optional
--- flags number, optional serialiser script name and optional members table. See example scripts from games for more information on its use with examples. You can register multiple classes with the
--- same name as long as they are not exactly the same, ie the version hashes are different.
--- @param table nil
--- @return nil
function MetaRegisterClass(table)
end

--- This registers a collection class to the meta system, only to be used in initialisation. Pass in the same table information as would be used in the normal register class.
--- The table is the information about the collection class. The second argument is the key type value table, (previously defined and must have been passed into register class). This can
--- be nil for non keyed collections, such as most arrays, but must be a previoulsy registered table otherwise (eg for Map classes). If this is a static array (SArray) class,
--- then this should be an integer being the number of elements. The third argument must always be non nil and a table, which is the previously registered value type in
--- the collection. There is a special case where key or value type table can be strings. If you pass them as a string, this means they will be resolved once
--- all classes have been registered. This allows forward declaration of a class inside a collection before its fully defined. Instead of passing in the type table you pass in the
--- string name of the class it should be. Optionally you can end the string with a semicolon followed by the version index, eg 'class Hello;1', such that version index 1
--- is used in this example. By default version index 0 is used.
--- @param table nil
--- @param keyTypeTable nil
--- @param valueTypeTable nil
--- @return nil
function MetaRegisterCollection(table, keyTypeTable, valueTypeTable)
end

--- Gets the unique class ID for that table. Must be previously registered using the collection or class register function.
--- @param table nil
--- @return number
function MetaGetClassID(table)
end

--- Gets the version hash for that class table. Must be previously registered using the collection or class register function.
--- @param table nil
--- @return number
function MetaGetVersionCRC(table)
end

--- Gets the hexadecimal string hash of the given class table. Must be previously registered using the collection or class register function. Returned as a string as Lua numbers internally are
--- floating points which cannot represent the 64 bit hash properly.
--- @param table nil
--- @return string
function MetaGetClassHash(table)
end

--- This queries if the given flagToTest exists in the the first flags argument.
--- @param flags nil
--- @param flagToTest nil
--- @return bool
function MetaFlagQuery(flags, flagToTest)
end

--- CRC32 hashes the given byte and returns the 32 bit hash. Pass in an optional initial hash (else zero). The byte is a number, in which the bottom 8 bits
--- are hashed, so make sure its in the range of 0 to 255 inclusive. Used for version hashing.
--- @param initialHash nil
--- @param byte nil
--- @return number
function MetaHashByte(initialHash, byte)
end

--- CRC32 hashes the given number and returns the 32 bit hash. Pass in an optional initial hash (else zero). Only the bottom 32 bits are hashed, ie the value of
--- the number, which must be from 0 to about 4 billion. Used for version hashing.
--- @param initialHash nil
--- @param int nil
--- @return number
function MetaHashInt(initialHash, int)
end

--- Very similar to MetaHashHexString, however the value string must be of length 16 and the value is endian flipped such that it works for little endian machines.
--- @param initialHash nil
--- @param valuestring nil
--- @return number
function MetaHashLong(initialHash, valuestring)
end

--- CRC32 hashes the given hexadecimal string buffer and returns the 32 bit hash. Pass in an optional initial hash (else zero). The string must be a set of adjacent hexadecimal
--- digits (padded), and its length must be a multiple of 2. For example '00' is valid but '0' is not. '00010203' hashes the buffer with bytes 0, 1, 2 and
--- 3 in that order. Used for version hashing.
--- @param initialHash nil
--- @param valuestring nil
--- @return number
function MetaHashHexString(initialHash, valuestring)
end

--- This hashes the given string, CRC32, with an optional initial hash (else set to 0).
--- @param initialHash nil
--- @param value nil
--- @return number
function MetaHashString(initialHash, value)
end

--- Sets the function (pass the name not the function) which generates the version hash for the given class table (the function should return a number, see hashing functions below, and
--- take in the class table). This should be set first, then call register intrinsics, then the rest of the classes. See existing game scripts for examples.
--- @param functionName nil
--- @return nil
function MetaSetVersionFn(functionName)
end

--- This function simply dumps all of the currently registered meta types to the log, along with their version CRC hashes and version numbers.
--- @return nil
function MetaDumpVersions()
end

--- Registers a duplicate of the pre-registered class, source. The only difference is the new name and version number as specified by the last two arguments. Returns the table of the
--- type.
--- @param source nil
--- @param typename nil
--- @param versionNumber nil
--- @return table
function MetaRegisterDuplicate(source, typename, versionNumber)
end

--- Returns the flags of a given class.
--- @param typename nil
--- @param versionNumber nil
--- @return number
function MetaGetClassFlags(typename, versionNumber)
end

--- Returns the flags of the given member of the given class.
--- @param typename nil
--- @param versionNumber nil
--- @param memberName nil
--- @return number
function MetaGetMemberFlags(typename, versionNumber, memberName)
end

--- Returns the class information of a given class instance. For a similar function to find the class information about a class serialised inside a meta stream, use MetaStreamFindClass.
--- @param instance nil
--- @return typename, versionNumber
function MetaGetClass(instance)
end

--- Returns the class name and version number of the member specified of the class specified.
--- @param typename nil
--- @param versionNumber nil
--- @param member nil
--- @return typename, versionNumber
function MetaGetMemberClass(typename, versionNumber, member)
end

--- Obtains a weak reference (instance) to the given member variable in the given class instance in passed. Pass in the name of the member variable as the second argument.
--- @param instance nil
--- @param member nil
--- @return instance
function MetaGetMember(instance, member)
end

--- Returns a table of all the class names. Returns table indexed 1 to N. Does not take into account version numbers.
--- @return table
function MetaGetClasses()
end

--- Similar to the get class function. Sets the value argument. Ensure that the type matches what you are setting it to, as it won't be casted.
--- @param instance nil
--- @param value nil
--- @return nil
function MetaSetClassValue(instance, value)
end

--- If the instance is an intrinsic type, it is converted to the equivalent Lua type. Strings, booleans, floats and doubles translate as normal. Integers, unsigned or signed, casted all to
--- the same internal Lua number type, so this can be used for all integer types. Symbols are converted to symbol strings. Integers of width 64, signed and unsigned, are always
--- converted to symbol strings as well, as the Lua number type cannot hold them to the correct value. If it is not one of those types, a normal weak reference
--- is returned as it is a compound, normal class type.
--- @param instance nil
--- @return value
function MetaGetClassValue(instance)
end

--- Given the member in the given class, this function returns a table of string enum or flag name to enum or flag integer value. The returned table is not a
--- table of tables like when registering enums or flags, however is a table where the keys are the enum or flag names and the values are the integer values. Note
--- that this works if the member is an enum or a flag. Returns nil if the member is not an enum or flag member.
--- @param typename nil
--- @param versionNumber nil
--- @param member nil
--- @return table
function MetaGetEnumFlags(typename, versionNumber, member)
end

--- Returns a table of the member variable names in the given class. Indexed from 1 to N.
--- @param typename nil
--- @param versionNumber nil
--- @return table
function MetaGetMemberNames(typename, versionNumber)
end

--- Creates an instance of the class specified by the first two arguments. The third argument is the name you want to give to the returned child instance. You can then
--- access this returned instance later by finding the child instance by this unique name. Pass in a valid instance as the parent instance, which the returned instance's lifetime depends totally
--- on.
--- @param typename nil
--- @param versionNumber nil
--- @param name nil
--- @param parent nil
--- @return instance
function MetaCreateInstance(typename, versionNumber, name, parent)
end

--- Creates a new instance, copying everything from the 1st argument into the returned instance, leaving the first one unchanged. Rest of arguments are the same as CreateInstance. Parent and instance
--- cannot be the same.
--- @param instance nil
--- @param name nil
--- @param parent nil
--- @return instance
function MetaCopyInstance(instance, name, parent)
end

--- Creates a new instance, moving everything from the 1st argument into the returned instance, leaving the first one alive but empty like it was just created. Rest of arguments are
--- the same as CreateInstance. Parent and instance cannot be the same.
--- @param instance nil
--- @param name nil
--- @param parent nil
--- @return instane
function MetaMoveInstance(instance, name, parent)
end

--- Returns true if the given instance (1 argument) else is a collection class. If two arguments, pass in the typename and version number to test.
--- @param instance_Or_typename nil
--- @param --[[optional]] versionNumber nil
--- @return bool
function MetaIsCollection(instance_Or_typename, --[[optional]] versionNumber)
end

--- Converts the given instance to a string. If the class of the instance has a defined to string meta operation, then that is called. All intrinsic types such as integer
--- are converted to string by default. Higher level typed, is used defined typed, return a list of the member names and the instance values formatted nicely in a string.
--- @param instance nil
--- @return string
function MetaToString(instance)
end

--- Same as less than but returns true only if they are equal using the comparison meta operation.
--- @param instance1 nil
--- @param instance2 nil
--- @return bool
function MetaEquals(instance1, instance2)
end

--- Compares the two instances, using their less than meta operation. For intrinsic types such as floats and integers, this does the usual. Else this will return false for other types
--- without a defined comparison operation.
--- @param instanceL nil
--- @param instanceR nil
--- @return bool
function MetaLessThan(instanceL, instanceR)
end

--- Returns the .VERS file name for the given class. Optionally pass in to use the alternative file name (without struct etc), which is generally not used but is done by
--- Telltale at runtime but not saved on disk.
--- @param className nil
--- @param classVersionNumber nil
--- @param --[[optional]] bAltFileName nil
--- @return string
function MetaGetSerialisedVersionInfoFileName(className, classVersionNumber, --[[optional]] bAltFileName)
end

--- Returns a weak reference to the child instance associated with the given parent under the given name.
--- @param instance nil
--- @param name nil
--- @return instance
function MetaGetChild(instance, name)
end

--- Returns the number of named children that the given instance holds. This does not include any instances returned with GetMember, but only MetaCreateInstance.
--- @param instance nil
--- @return number
function MetaGetNumChildren(instance)
end

--- Returns a table (indexed from 1 to N) of all of the children names in the given instance. Each value in the table is a string. Some children names may
--- be hash strings (length 16, upper case hex) if the symbol could not be resolved. Internally child names are stored as symbols. Try to use global symbols, and use SymbolCompare
--- to compare, so reduce this risk.
--- @param instance nil
--- @return table
function MetaGetChildrenNames(instance)
end

--- Releases the given child (identified by name) of the given instance. Use this to remove it from the internal map such that it will get destroyed after (unless a lower
--- level C++ object still obtains a reference to it). Calls to get child after this will return nil.
--- @param instance nil
--- @param name nil
--- @return nil
function MetaReleaseChild(instance, name)
end

--- Returns true if the given instance of a class is alive. Returns false if the instance is invalid or the parent of the instance is no longer alive - meaning
--- you cannot access the instance anymore as it does not exist.
--- @param instance nil
--- @return bool
function MetaInstanceAlive(instance)
end

--- Returns if asynchronous serialisation is possible for the current serialisation
--- @param stream nil
--- @return bool
function MetaStreamAsyncSerialiseEnabled(stream)
end

--- Reads a signed 4 byte integer from the given meta stream and returns it.
--- @param stream nil
--- @return number
function MetaStreamReadInt(stream)
end

--- Reads a signed 1 byte integer from the given meta stream and returns it.
--- @param stream nil
--- @return number
function MetaStreamReadByte(stream)
end

--- Reads a boolean from the meta stream.
--- @param stream nil
--- @return bool
function MetaStreamReadBool(stream)
end

--- Reads a signed 2 byte integer from the given meta stream and returns it.
--- @param stream nil
--- @return number
function MetaStreamReadShort(stream)
end

--- Reads a string, reading 4 bytes as the length and then that many bytes as the ASCII string data.
--- @param stream nil
--- @return string
function MetaStreamReadString(stream)
end

--- Reads an unsigned 64 bit integer as a symbol, returning it as a hexadecimal string of length 16.
--- @param stream nil
--- @return string
function MetaStreamReadSymbol(stream)
end

--- Advances by the given number of bytes in the current meta stream. Used in read mode to skip over data which we don't need or know the correct value already.
--- @param stream nil
--- @param nBytes nil
--- @return nil
function MetaStreamAdvance(stream, nBytes)
end

--- Writes zeros to the meta stream in the current section.
--- @param stream nil
--- @param N nil
--- @return nil
function MetaStreamWriteZeros(stream, N)
end

--- Writes the argument to the stream argument as a 4 byte signed integer.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteInt(stream, value)
end

--- Writes the float argument to the stream argument.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteFloat(stream, value)
end

--- Reads a float from the meta stream
--- @param stream nil
--- @param value nil
--- @return float
function MetaStreamReadFloat(stream, value)
end

--- Writes the argument to the stream argument as a 1 byte signed integer.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteByte(stream, value)
end

--- Writes the argument to the stream argument as a 2 byte signed integer.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteShort(stream, value)
end

--- Returns the name of the currently processing file. Pass in the meta stream.
--- @param stream nil
--- @return str
function MetaStreamGetFileName(stream)
end

--- Writes the length of the stream as a 4 byte signed integer followed by the string ASCII data.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteString(stream, value)
end

--- Writes a boolean to the meta stream given.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteBool(stream, value)
end

--- Writes the symbol argument. If it is a 16 byte hex string hash with <>, the value is represents is written, else the string is hashed on the lower case
--- version. Note that SymbolXXX can be used to create and manage symbols.
--- @param stream nil
--- @param value nil
--- @return nil
function MetaStreamWriteSymbol(stream, value)
end

--- Sets the current section to write to in stream as the main one, this is the default one and is by default where all writes go to. When calling any
--- of the SetXXXSection functions, note that block sizes are restricted to their section that they are called in. If you call start block in the main section and then an
--- end block call in the async section, it will error as the main block is waiting for to call end block, not the async one.
--- @param stream nil
--- @return nil
function MetaStreamSetMainSection(stream)
end

--- Sets the current section in the stream as the async section. This is where buffers in meshes and textures normally store their data. Do not use this in games which
--- don't use meta stream versions of MSV5 or 6.
--- @param stream nil
--- @return nil
function MetaStreamSetAsyncSection(stream)
end

--- Sets the current section in the stream as the debug section. This is where buffers in meshes and textures normally store their data. Do not use this in games which
--- don't use meta stream versions of MSV5 or 6.
--- @param stream nil
--- @return nil
function MetaStreamSetDebugSection(stream)
end

--- Starts a block size in the stream, in the current section. Pass in if you are writing or reading to the stream currently. You must have a call to end
--- block some time after this call to write the size as the difference between the offsets in the file when begin and end offset are called.
--- @param stream nil
--- @param isWrite nil
--- @return nil
function MetaStreamBeginBlock(stream, isWrite)
end

--- Ends a block in the stream in the current section. BeginBlock must have previously been called in the current section in the meta stream. Pass in if you are reading
--- or writing to the meta stream currently.
--- @param stream nil
--- @param isWrite nil
--- @return nil
function MetaStreamEndBlock(stream, isWrite)
end

--- Returns the size of the buffer instance (kMetaClassInternalBinaryBuffer class). This size is set when you read a buffer from a meta stream - the argument is saved along side the
--- buffer internal.
--- @param bufferInstance nil
--- @return number
function MetaGetBufferSize(bufferInstance)
end

--- Reads size bytes from the stream into the buffer instance. The buffer instance must be of the class kMetaClassInternalBinaryBuffer, so must be a member variable of another class. This type
--- is never serialised and is used so you can hold a memory block in which it is freed after the class instance is destroyed. You can declare member variables with
--- this type and then use it to store the buffer.
--- @param stream nil
--- @param bufferInstance nil
--- @param size nil
--- @return nil
function MetaStreamReadBuffer(stream, bufferInstance, size)
end

--- See above for information about bufferInstance. Does not write the size. Writes the buffer in its entirety, size bytes.
--- @param stream nil
--- @param bufferInstance nil
--- @return nil
function MetaStreamWriteBuffer(stream, bufferInstance)
end

--- Reads size bytes from the stream into the buffer instance. The buffer instance must be of the class kMetaClassInternalDataStreamCache, so must be a member variable of another class. This type
--- is very similar to the binary buffer type above, however it stores a cache of the range of bytes from the current position in the stream to the current position
--- plus the size parameter. What this means is that even if the file is finished reading from it will stay open as long as the instance is alive. This is
--- useful for large data blocks which we wouldn't want in memory such as archives of files. By default the size is -1 and doesn't need to be passed in, suggesting
--- that all remaining bytes should be cached. After this call the meta stream will have effectively read all these bytes so the next read bytes will be after this.
--- @param stream nil
--- @param cacheInstance nil
--- @param --[[optional]] size nil
--- @return nil
function MetaStreamReadCache(stream, cacheInstance, --[[optional]] size)
end

--- See above for information about cacheInstance. Does not write the size. Writes the cached bytes in its entirety to the stream.
--- @param stream nil
--- @param cacheInstance nil
--- @return nil
function MetaStreamWriteCached(stream, cacheInstance)
end

--- Writes a DDS file header with the given information. It must include the same information as returned by the read version. See MetaStreamReadDDS for information about the table values.
--- @param stream nil
--- @param table nil
--- @return nil
function MetaStreamWriteDDS(stream, table)
end

--- Dumps the binary buffer out to a file name. Pass in the offset in the buffer and number of bytes to dump.
--- @param binaryBuffer nil
--- @param offset nil
--- @param size nil
--- @param outFileName nil
--- @return nil
function MetaStreamDumpBuffer(binaryBuffer, offset, size, outFileName)
end

--- Gets the number of bytes that would be written or have been read from the given DDS header table that either you contruct or was returned in the read function
--- above. This is the number of bytes read or would be written.
--- @param table nil
--- @return number
function MetaStreamGetDDSHeaderSize(table)
end

--- Reads a DDS file header returning a table with its information. See Gitbook documentation in the Meta System page for the table values.
--- @param stream nil
--- @return table
function MetaStreamReadDDS(stream)
end

--- Returns the size of the cached data stream instance (kMetaClassInternalDataStreamCache class). This size is set when you read a buffer from a meta stream - its held internally.
--- @param cacheInstance nil
--- @return number
function MetaGetCachedSize(cacheInstance)
end

--- Finds the class in the meta system associated with the given type symbol for the given meta stream. The symbol is either a symbol (16 byte hex hash string with
--- <>) or a string which will be hashed. Examples of use are passing in a symbol read from the meta stream when the meta stream contains a class determined by
--- its symbol also stored in the meta stream. Returns the class type name and version number associated. The class is found by searching the meta stream header for the type
--- name symbol, if it is found then the returned class is one with the same version hash as in the header of the meta stream. If it is not found
--- in the header, then it may still be valid as intrinsic types are container types are not in headers. In those cases, the type with version number zero is returned
--- - as those types don't have different versions and if they do they are never used.
--- @param stream nil
--- @param typeSymbol nil
--- @return typename, versionNumber
function MetaStreamFindClass(stream, typeSymbol)
end

--- Default serialises the given class instance to/from the given stream in the given serialisation mode. See documentation for more information about this function. Returns if success or failed.
--- @param stream nil
--- @param instance nil
--- @param isWrite nil
--- @return bool
function MetaSerialiseDefault(stream, instance, isWrite)
end

--- Serialises the given instance to/from the given stream in the given serialisation mode. This will decide whether to call the custom serialisation, if there is one, or else to call
--- the default serialiser. Use this to serialise any class instance to/from the given stream, as opposed to the default serialiser which should be called in a custom serialiser if needed.
--- Optionally specify a debug name to put into any debug outputs when reading.
--- @param stream nil
--- @param instance nil
--- @param isWrite nil
--- @param --[[optional]] debugName nil
--- @return bool
function MetaSerialise(stream, instance, isWrite, --[[optional]] debugName)
end

--- Searches the global symbol table for the given symbol hash (argument must be a symbol). If it is found, the string version is returned, else an empty string.
--- @param symbolString nil
--- @return string
function SymbolTableFind(symbolString)
end

--- Registers a string such that a call to find after with the symbol whose hash is equal to the arguments hash, will return the argument.
--- @param string nil
--- @return nil
function SymbolTableRegister(string)
end

--- Clears the global symbol table. Do not use this function unless there is an explicit reason to do so: you will loose all string versions of hashes.
--- @return nil
function SymbolTableClear()
end

--- Compares if the symbol hash of left and right are equal to each other. Either or both of them can be an un-hashed string, or any of them can be
--- a hashed string (ie a 16 byte hex hash string with <>).
--- @param left nil
--- @param right nil
--- @return bool
function SymbolCompare(left, right)
end

--- Creates a symbol, by hashing the string argument in lower case. If the string argument is itself a string 16 byte hex hash with <>, it will still be hashed
--- so be careful.
--- @param string nil
--- @return symbol
function SymbolCreate(string)
end

--- Intrinsic signed 8-bit integer
--- @type number
kMetaInt8 = 0

--- Class is attachable
--- @type number
kMetaClassAttachable = 0

--- Class is not blocked
--- @type number
kMetaClassNonBlocked = 0

--- Container flag
--- @type number
kMetaClassContainer = 0

--- Alias for signed 64-bit integer
--- @type number
kMeta__Int64 = 0

--- Class is intrinsic
--- @type number
kMetaClassIntrinsic = 0

--- Can be serialised asynchronously
--- @type number
kMetaClassAllowAsync = 0

--- Is an enum wrapper class. Should have one member (normally mVal), as integer value.
--- @type number
kMetaClassEnumWrapper = 0

--- Alias for signed 8-bit integer
--- @type number
kMetaChar = 0

--- Proxy class disables member blocking
--- @type number
kMetaClassProxy = 0

--- Member is a flag
--- @type number
kMetaMemberFlag = 0

--- Class is abstract
--- @type number
kMetaClassAbstract = 0

--- Member is an enum
--- @type number
kMetaMemberEnum = 0

--- Member is excluded from disk
--- @type number
kMetaMemberSerialiseDisable = 0

--- Member is a base class
--- @type number
kMetaMemberBaseClass = 0

--- Internal data stream cache class
--- @type number
kMetaClassInternalDataStreamCache = 0

--- Member is excluded from memory
--- @type number
kMetaMemberMemoryDisable = 0

--- Excluded from version hash
--- @type number
kMetaMemberVersionDisable = 0

--- Intrinsic 64-bit floating point
--- @type number
kMetaDouble = 0

--- Intrinsic unsigned 32-bit integer
--- @type number
kMetaUInt32 = 0

--- Intrinsic signed 32-bit integer
--- @type number
kMetaInt = 0

--- Intrinsic signed 16-bit integer
--- @type number
kMetaInt16 = 0

--- Alias for unsigned 32-bit integer
--- @type number
kMetaUInt = 0

--- Alias for signed 32-bit integer
--- @type number
kMetaInt32 = 0

--- Intrinsic unsigned 64-bit integer
--- @type number
kMetaUInt64 = 0

--- Intrinsic signed 64-bit integer
--- @type number
kMetaInt64 = 0

--- Alias for unsigned 64-bit integer
--- @type number
kMeta__UnsignedInt64 = 0

--- Alias for unsigned 32-bit integer
--- @type number
kMetaUnsignedInt = 0

--- Alias for signed 32-bit integer (Windows)
--- @type number
kMetaLong = 0

--- Intrinsic 32-bit floating point
--- @type number
kMetaFloat = 0

--- Intrinsic boolean type
--- @type number
kMetaBool = 0

--- Alias for signed 8-bit integer
--- @type number
kMetaSignedChar = 0

--- Intrinsic unsigned 8-bit integer
--- @type number
kMetaUnsignedInt8 = 0

--- Intrinsic String type
--- @type number
kMetaString = 0

--- Intrinsic unsigned 16-bit integer
--- @type number
kMetaUnsignedInt16 = 0

--- Alias for 64-bit float (double)
--- @type number
kMetaLongDoubler = 0

--- Alias for unsigned 8-bit integer
--- @type number
kMetaUnsignedChar = 0

--- Alias for unsigned 32-bit integer
--- @type number
kMetaUnsignedLong = 0

--- Game capabilities
--- @type number
kGameCapRawClassNames = 0

--- Alias for unsigned 16-bit integer
--- @type number
kMetaUnsignedShort = 0

--- Alias for signed 64-bit integer
--- @type number
kMetaLongLong = 0

--- Alias for signed 16-bit integer
--- @type number
kMetaShort = 0

--- Wide character (typically UTF-16)
--- @type number
kMetaWideChar = 0

--- Alias for unsigned 64-bit integer
--- @type number
kMetaUnsignedLongLong = 0

--- Internal binary buffer class
--- @type number
kMetaClassInternalBinaryBuffer = 0

--- Intrinsic Symbol type
--- @type number
kMetaSymbol = 0

--- Intrinsic Symbol type with class prefix
--- @type number
kMetaClassSymbol = 0

--- Intrinsic String type with class prefix
--- @type number
kMetaClassString = 0

--- Game capabilities
--- @type number
kGameCapSeparateAnimationTransform = 0

--- Game capabilities
--- @type number
kGameCapNoScriptEncryption = 0

--- Game capabilities
--- @type number
kGameCapAllowTransitionMaps = 0

--- Game capabilities
--- @type number
kGameCapUsesLocationInfo = 0

--- Game capabilities
--- @type number
kGameCapUsesLenc = 0

--- Registers data which describes how the inspector view should render this module. Data table is a table of UI element to a table of that elements info. Class, PropKey and
--- Default are required keys. In each entry, 'UI' is a required table which describes how to render it. It should contain InputType which is a string of the valid kPropRenderXXX
--- types. Or 'handle:xxx' where xxx is the *extension* of the file name to have a handle set as. Note in this case that the handle property must be a string
--- or symbol. Along with InputType, SubPath is a string. By default set it to 'this', else a chain of variables to get to the input type variable, eg this.mHandle if
--- the type is a handle and a string is required if mHandle is a string in the Handle class specifying the file name. There can be as many this.sub.paths.as.needed .
--- @param moduleID nil
--- @param moduleImage nil
--- @param dataTable nil
--- @return nil
function RegisterModuleUI(moduleID, moduleImage, dataTable)
end

--- Set the common mesh name
--- @param state nil
--- @param name nil
--- @return nil
function CommonMeshSetName(state, name)
end

--- Set whether the mesh is deformable or not
--- @param state nil
--- @param deformable nil
--- @return nil
function CommonMeshSetDeformable(state, deformable)
end

--- Push a new vertex buffer to the common mesh
--- @param state nil
--- @param numVerts nil
--- @param vertexStride nil
--- @param binaryBuffer nil
--- @return nil
function CommonMeshPushVertexBuffer(state, numVerts, vertexStride, binaryBuffer)
end

--- Set the common mesh's index buffer
--- @param state nil
--- @param numIndices nil
--- @param formatIsUnsignedShort nil
--- @param binaryBuffer nil
--- @return nil
function CommonMeshSetIndexBuffer(state, numIndices, formatIsUnsignedShort, binaryBuffer)
end

--- Sequential attributes and buffers will be assigned to this new vertex state.
--- @param state nil
--- @return nil
function CommonMeshAdvanceVertexState(state)
end

--- Decompress vertices from a telltale mesh
--- @param state nil
--- @param binaryBuffer nil
--- @param numVerts nil
--- @param compressedType nil
--- @return nil
function CommonMeshDecompressVertices(state, binaryBuffer, numVerts, compressedType)
end

--- Push a level-of-detail to the given vertex state index. All sequential calls will append to this LOD.
--- @param state nil
--- @param vertexStateIndex nil
--- @return nil
function CommonMeshPushLOD(state, vertexStateIndex)
end

--- Set current LOD bounding information
--- @param state nil
--- @param boundingBoxInst nil
--- @param --[[optional]] boundingSphereInst nil
--- @return nil
function CommonMeshSetLODBounds(state, boundingBoxInst, --[[optional]] boundingSphereInst)
end

--- Push a mesh batch. Specify if its a shadow batch.
--- @param state nil
--- @param isShadowBatch nil
--- @return nil
function CommonMeshPushBatch(state, isShadowBatch)
end

--- Set current batch bounding information
--- @param state nil
--- @param isShadowBatch nil
--- @param boundingBoxInst nil
--- @param --[[optional]] boundingSphereInst nil
--- @return nil
function CommonMeshSetBatchBounds(state, isShadowBatch, boundingBoxInst, --[[optional]] boundingSphereInst)
end

--- Set batch parameter information
--- @param state nil
--- @param bIsShadowBatch nil
--- @param minVert nil
--- @param maxVert nil
--- @param startIndexBufferIndex nil
--- @param numPrimitives nil
--- @param numIndices nil
--- @param baseIndex nil
--- @param matIndex nil
--- @return nil
function CommonMeshSetBatchParameters(state, bIsShadowBatch, minVert, maxVert, startIndexBufferIndex, numPrimitives, numIndices, baseIndex, matIndex)
end

--- Adds a new vertex attribute to the current vertex state
--- @param state nil
--- @param attribType nil
--- @param vertexBufferIndex nil
--- @param attribFormat nil
--- @return nil
function CommonMeshAddVertexAttribute(state, attribType, vertexBufferIndex, attribFormat)
end

--- Push material information to the mesh
--- @param state nil
--- @param diffuseTexName nil
--- @return nil
function CommonMeshPushMaterial(state, diffuseTexName)
end

--- Resolve legacy mesh bone palettes. This will modify the bone indices buffer given. Palette table is table of tables. paletteTable[batch/whatever index] = table_u32_array[actual serialised buffer data
--- index => bone name]. Vertex index table maps 'whatever indices' (above) to an actual vertex buffer range. Each value in table must be have Min and Max keys as int
--- indices Four bone skinning is a bool: true for 4 bone influence, else 3. Index divisor is what each bone index is divided by. Used for old Vec4 indices for
--- each row (into matrix, div 4) for old games. Use 1 to ignore. This function only accepts bone index buffer in the format of 4x ubytes! (or float unorm bytes
--- x4)
--- @param state nil
--- @param buffer nil
--- @param palette_table nil
--- @param vertIndexTab nil
--- @param fourBoneSkinning nil
--- @param indexDivisor nil
--- @param meshName nil
--- @return nil
function CommonMeshResolveBonePalettes(state, buffer, palette_table, vertIndexTab, fourBoneSkinning, indexDivisor, meshName)
end

--- Sets the name of the common scene
--- @param state nil
--- @param name nil
--- @return nil
function CommonSceneSetName(state, name)
end

--- Sets if the common scene is initially hidden
--- @param state nil
--- @param bHidden nil
--- @return nil
function CommonSceneSetHidden(state, bHidden)
end

--- Pushes an agent to the common scene
--- @param state nil
--- @param name nil
--- @param props nil
--- @param --[[optional]] initialTransform nil
--- @return nil
function CommonScenePushAgent(state, name, props, --[[optional]] initialTransform)
end

--- Sets the name of the common texture
--- @param state nil
--- @param name nil
--- @return nil
function CommonTextureSetName(state, name)
end

--- Sets the format of the texture data
--- @param state nil
--- @param texFormat nil
--- @return nil
function CommonTextureSetFormat(state, texFormat)
end

--- Sets the texture dimensions
--- @param state nil
--- @param width nil
--- @param height nil
--- @param mipMapCount nil
--- @param depth nil
--- @param arraySize nil
--- @return nil
function CommonTextureSetDimensions(state, width, height, mipMapCount, depth, arraySize)
end

--- Pushes an ordered image inside the texture. This must be done in an ordered fashion! See the DDS file format on how to push.
--- @param state nil
--- @param width nil
--- @param height nil
--- @param rowPitch nil
--- @param slicePitch nil
--- @param binaryBuffer nil
--- @return nil
function CommonTexturePushOrderedImage(state, width, height, rowPitch, slicePitch, binaryBuffer)
end

--- Resolve/decompress a texture format into a standard RGBA image such that it fits as a standard texture format.
--- @param state nil
--- @param binaryBuffer nil
--- @param resolvableFormat nil
--- @param width nil
--- @param height nil
--- @return nil
function CommonTextureResolveRGBA(state, binaryBuffer, resolvableFormat, width, height)
end

--- Calculates the pitch of the texture given the texture format, in bytes.
--- @param texFormat nil
--- @param width nil
--- @param height nil
--- @return int
function CommonTextureCalculatePitch(texFormat, width, height)
end

--- Calculates the slice pitch of the texture given the texture format, in bytes.
--- @param texFormat nil
--- @param width nil
--- @param height nil
--- @return int
function CommonTextureCalculateSlicePitch(texFormat, width, height)
end

--- Set name of input mapper
--- @param state nil
--- @param name nil
--- @return nil
function CommonInputMapperSetName(state, name)
end

--- Push a mapping to the input mapper common class
--- @param state nil
--- @param code nil
--- @param type nil
--- @param scriptFunction nil
--- @param controllerIndexOverride nil
--- @return nil
function CommonInputMapperPushMapping(state, code, type, scriptFunction, controllerIndexOverride)
end

--- Set common animation name
--- @param state nil
--- @param name nil
--- @return nil
function CommonAnimationSetName(state, name)
end

--- Set common animation length
--- @param state nil
--- @param length nil
--- @return nil
function CommonAnimationSetLength(state, length)
end

--- Push compressed position keys to the animation as an animated value
--- @param state nil
--- @param name nil
--- @param minTime nil
--- @param maxTime nil
--- @param flags nil
--- @param type nil
--- @param format nil
--- @param format_options --[[see examples / Animation.cpp]] nil
--- @return nil
function CommonAnimationPushCompressedVector3Keys(state, name, minTime, maxTime, flags, type, format, format_options --[[see examples / Animation.cpp]])
end

--- Push compressed position keys to the animation as an animated value
--- @param state nil
--- @param name nil
--- @param minTime nil
--- @param maxTime nil
--- @param flags nil
--- @param type nil
--- @param format nil
--- @param format_options --[[see examples / Animation.cpp]] nil
--- @return nil
function CommonAnimationPushCompressedQuatKeys(state, name, minTime, maxTime, flags, type, format, format_options --[[see examples / Animation.cpp]])
end

--- Pyush compressed quaternion rotation keys to the animation as an animated value.
--- @param state nil
--- @param name nil
--- @param keyframedClassNameDecayed nil
--- @param minVal nil
--- @param maxVal nil
--- @param sampleContainer nil
--- @param flags nil
--- @param type nil
--- @return nil
function CommonAnimationPushKeyframedValues(state, name, keyframedClassNameDecayed, minVal, maxVal, sampleContainer, flags, type)
end

--- Pushes skeleton entry information to the common skeleton
--- @param state nil
--- @param entryInfoTable nil
--- @return nil
function CommonSkeletonPushEntry(state, entryInfoTable)
end

--- Vertex attribute formats
--- @type number
kCommonMeshUByte4Norm = 0

--- Vertex attribute formats
--- @type number
kCommonMeshInt2 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshFloat1 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshInt1 = 0

--- Module property render instructions
--- @type string
kPropRenderString = ""

--- Module property render instructions
--- @type string
kPropRenderVector2 = ""

--- Animation value types
--- @type number
kAnimationValueTypeAudioSurroundDir = 0

--- Vertex attribute formats
--- @type number
kCommonMeshByte2 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshFloat2 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshFloat3 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshByte4 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshFloat4 = 0

--- Mesh compressed format involving unsigned approximated normed normal vector3 values
--- @type number
kCommonMeshCompressedFormatUNormNormalAprox = 0

--- Vertex attribute formats
--- @type number
kCommonMeshInt3 = 0

--- Surface formats
--- @type number
kCommonTextureFormatRGBA8 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshFormatUnknown = 0

--- Vertex attribute formats
--- @type number
kCommonMeshInt4 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUInt1 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUInt2 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUInt3 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUInt4 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUByte2 = 0

--- Animation value types
--- @type number
kAnimationValueTypeContribution = 0

--- Module property render instructions
--- @type string
kPropRenderFloat = ""

--- Vertex attribute formats
--- @type number
kCommonMeshUByte4 = 0

--- BGR'X' resolvable texture format. 'X' is unused and set to opaque.
--- @type number
kCommonTextureResolvableFormatBGRX = 0

--- Surface formats
--- @type number
kCommonTextureFormatBGRA8 = 0

--- Vertex attribute formats
--- @type number
kCommonMeshUByte2Norm = 0

--- Vertex attribute formats
--- @type number
kCommonMeshByte2Norm = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureScaleOriginV = 0

--- Vertex attribute formats
--- @type number
kCommonMeshByte4Norm = 0

--- Primitive types
--- @type number
kCommonMeshTriangleList = 0

--- Primitive types
--- @type number
kCommonMeshLineList = 0

--- Surface formats
--- @type number
kCommonTextureFormatDXT1 = 0

--- Surface formats
--- @type number
kCommonTextureFormatDXT3 = 0

--- Animation value types
--- @type number
kAnimationValueTypeSkeletal = 0

--- Surface formats
--- @type number
kCommonTextureFormatDXT5 = 0

--- Surface formats
--- @type number
kCommonTextureFormatDepth32FStencil8 = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributePosition = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeNormal = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeBinormal = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeTangent = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeBlendWeight = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeBlendIndex = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeColour = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeUVDiffuse = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeUVLightMap = 0

--- Vertex attributes
--- @type number
kCommonMeshAttributeUnknown = 0

--- Module property render instructions
--- @type string
kPropRenderVector4 = ""

--- Module property render instructions
--- @type string
kPropRenderVector3 = ""

--- Module property render instructions
--- @type string
kPropRenderBool = ""

--- Module property render instructions
--- @type string
kPropRenderColour = ""

--- Module property render instructions
--- @type string
kPropRenderEnum = ""

--- Mesh compressed format involving unsigned normed UV values
--- @type number
kCommonMeshCompressedFormatUNormUV = 0

--- Mesh compressed format involving signed normed normal vector3 values
--- @type number
kCommonMeshCompressedFormatSNormNormal = 0

--- Trigger when an event begins
--- @type number
kCommonInputMapperTypeBegin = 0

--- Trigger when an event ends
--- @type number
kCommonInputMapperTypeEnd = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioReverbDry = 0

--- Trigger on mouse move event
--- @type number
kCommonInputMapperTypeMouseMove = 0

--- Trigger forced(?)
--- @type number
kCommonInputMapperTypeForce = 0

--- Animation value types
--- @type number
kAnimationValueTypeAdditiveMask = 0

--- Trigger on event begin or end
--- @type number
kCommonInputMapperTypeBeginOrEnd = 0

--- Legacy normed compressed vector3 keys (0).
--- @type number
kCompressedVector3KeysFormatLegacy0 = 0

--- Legacy normed quaternion compressed keys (0)
--- @type number
kCompressedQuatKeysFormatLegacy0 = 0

--- Animation value types
--- @type number
kAnimationValueTypeTime = 0

--- Animation value types
--- @type number
kAnimationValueTypeMover = 0

--- Animation value types
--- @type number
kAnimationValueTypeProperty = 0

--- Animation value types
--- @type number
kAnimationValueTypeTargetedMover = 0

--- Animation value types
--- @type number
kAnimationValueTypeSkeletonPose = 0

--- Animation value types
--- @type number
kAnimationValueTypeSkeletonRootAnim = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureMoveU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureMoveV = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureScaleU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureScaleV = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureRotate = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureOverride = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureVisibility = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureShearU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureShearV = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureShearOriginU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureShearOriginV = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureScaleOriginU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureRotateOriginU = 0

--- Animation value types
--- @type number
kAnimationValueTypeTextureRotateOriginV = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioPan = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioPitch = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioLowPassFilter = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioHighPassFilter = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioReverbWet = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioFMODParameter = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioSurroundExtent = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioLowFreqSend = 0

--- Animation value types
--- @type number
kAnimationValueTypeAudioLanguageResourceVolume = 0

--- Animation value types
--- @type number
kAnimationValueTypeVertexNormal = 0

--- Animation value types
--- @type number
kAnimationValueTypeVertexPosition = 0

--- Animation value types
--- @type number
kAnimationValueTypeAutoAct = 0

--- Animation value types
--- @type number
kAnimationValueTypeExplicitCompoundValue = 0

