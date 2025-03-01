-- PropertySet class implementation.

require("ToolLibrary/Game/Common/LuaContainer.lua")

-- Serialiser, V0 (oldest)
function SerialisePropertySet_V0(metaStream, propInstance, isWrite)
	-- start by calling default meta serialise (serialise members, so flags and parent list)
	if not MetaSerialiseDefault(metaStream, propInstance, isWrite) then
		return false;
	end

	MetaStreamBeginBlock(metaStream, isWrite) -- rest is inside a block in the binary

	if isWrite then
        
    else
		local numTypes = MetaStreamReadInt(metaStream)
		for i=1,numTypes do
            local typeSymbol = MetaStreamReadSymbol(metaStream)
			local propType, propTypeVersionIndex = MetaStreamFindClass(metaStream, typeSymbol) -- read class symbol
            if propType == nil then
                typeNameString = SymbolTableFind(typeSymbol)
                if string.len(typeNameString) == 0 then
                    typeNameString = typeSymbol
                end
                TTE_Log("Unregistered or unknown type in property set: " .. typeNameString)
                return false
            end
			local numOfThatType = MetaStreamReadInt(metaStream)
			for j=1, numOfThatType do
				key = SymbolTableFind(MetaStreamReadSymbol(metaStream))
				inst_of_type = MetaCreateInstance(propType, propTypeVersionIndex, key, propInstance)
				if not MetaSerialise(metaStream, inst_of_type, isWrite) then
					return false
				end
			end
		end
	end

	MetaStreamEndBlock(metaStream, isWrite)
	return true
end

-- Sets a value. The value must exist already. If it doesn't, you should use PropertyCreate first.
-- Value can either be an existing class instance which will be copied, else can be the matching lua type.
function PropertySet(propertySet, key, value)
   local property = MetaGetChild(propertySet, key)
    if property == nil then
        -- find in parents todo!
        TTE_Log("At PropertySet: property key does not exist [Parent property sets not supported yet]")
        return
    end
    if type(value) == "table" then
        MetaCopyInstance(value, key, propertySet)
    else
        MetaSetClassValue(property, value)
    end
end

-- Returns a table of the property key names. They are resolved as best, if they couldn't be they are the symbol.
function PropertyGetKeys(propertySet)
   return MetaGetChildrenNames(propertySet) 
end

-- Same as PropertyGetKeys: alias
function PropertyKeys(propertySet)
    return PropertyGetKeys()
end

-- Creates a new property in the property set. Optionally pass in the value to initialise or copy from. Such that this is consistent with the telltale API,
-- the version number can be optionally passed in when resolving the type of the property, which defaults to 0.
function PropertyCreate(propertySet, key, typeName, --[[optional]] value, --[[typeVersionNumber]] typeVersionNumber)
    local property = nil
    if value and type(value) == "table" then
        property = MetaCopyInstance(value, key, propertySet)
    else
        property = MetaCreateInstance(typeName, typeVersionNumber or 0, key, propertySet)
        if value then
            MetaSetClassValue(property, value)
        end
    end
end

-- Clears all key-value pairs from the input property set
function PropertyClearKeys(propertySet)
    local keys = PropertyGetKeys(propertySet)
    for _, child in keys do
        MetaReleaseChild(propertySet, child)
    end
end

-- Clears all global keys in the given property set. This removes all parent property sets.
function PropertyClearGlobals(propertySet)
    -- parentList = MetaGetMember(propertySet, "mParentList")
    TTE_Log("Implementation required for PropertyClearGlobals()")
end

-- Tests wether the given property exists in the given property set.
function PropertyExists(propertySet, key)
    -- parent support !
    return MetaGetChild(propertySet, key) ~= nil
end

-- Finds a property value by its key.
function PropertyGet(propertySet, key)
    -- parent support !
    local value = MetaGetChild(propertySet, key)
    if value == nil then
        return nil
    end
    local tryLuaValue = MetaGetClassValue(value)
    if type(tryLuaValue) ~= "nil" then -- intrinsic type eg string, int, etc..
        return tryLuaValue
    end
    return value -- return weak reference, is a higher level class.
end

-- Returns a table of the parent property names
function PropertyGetGlobals(propertySet)
    local parents = MetaGetMember(propertySet, "mParentList")
    if not parents then
        return nil
    end
    -- parent list is container<handle<prop>>. handle for any game should have 1 member, so get that
    local globals = {}
    local numGlobals = ContainerGetNumElements(parents)
    for i=1,numGlobals do
        local globalHandle = ContainerGetElement(parents, i-1)
        globalHandle = MetaGetMember(globalHandle, "mHandle")
        if not globalHandle then
            TTE_Log("PropertyGetGlobal: Handle<T> type classes must have only one member 'mHandle' which is a symbol.")
            return nil
        end
        globals[i] = MetaGetClassValue(globalHandle)
    end
end

-- Pass in the file name or file name symbol. Returns true if it a parent of the given property set instance. Optionally search parents recursively
function PropertyHasGlobal(propertySet, parentName, --[[optional]] searchParents)
    local parents = MetaGetMember(propertySet, "mParentList")
    if not parents then
        return nil
    end
    -- parent list is container<handle<prop>>. handle for any game should have 1 member, so get that
    local numGlobals = ContainerGetNumElements(parents)
    for i=1,numGlobals do
        local globalHandle = ContainerGetElement(parents, i-1)
        globalHandle = MetaGetMember(globalHandle, "mHandle")
        if not globalHandle then
            TTE_Log("PropertyHasGlobal: Handle<T> type classes must have only one member 'mHandle' which is a symbol.")
            return nil
        end
        local parentNameToCheck = MetaGetClassValue(globalHandle)
        if SymbolCompare(parentNameToCheck, parentName) then
            return true
        end
        if searchParents then
            TTE_Log("Implementation required for loading parents")
            return false
        end
    end
    return false
end

-- Returns the property set, possibly the argument, which the key exists in. Will always return the highest level parent which it exists in.
function PropertyGetKeyPropertySet(propertySet, key)
    TTE_Log("Implementation required")
    return nil
end

-- Returns the property set the value at key exists in. Make the property local to make this return the input property set.
function PropertyGetValuePropertySet(propertySet, key)
    TTE_Log("Implementation required")
    return nil
end

-- See PropertyGetKeyType. This returns 2 values, the type name and version number
function PropertyGetKeyTypeAndVersion(propertySet, key)
    local prop = MetaGetChild(propertySet, key)
    local clazz, ver = MetaGetClass(prop)
    return clazz, ver
end

-- Returns just the typename of the key. To get the version number too, use the sister version. For compatibility with actual telltale scripts.
function PropertyGetKeyType(propertySet, key)
    local clazz, _ = PropertyGetKeyTypeAndVersion(propertySet, key)
    return clazz
end

-- Returns true if the given key is local to the given property set, ie it does not only exist in a parent of the argument.
function PropertyIsLocal(propertySet, key)
    local keys = PropertyGetKeys(propertySet)
    for _, k in keys do
        if SymbolCompare(key, k) then
            return true
        end
    end
    return false
end

-- Makes the given property key a local, meaning it exists in the argument and not just through a parent default.
function PropertyMakeLocal(propertySet, key)
    TTE_Log("Implementation required")
end

-- Moves the given parent, must exit, to the front of the parent list in the given property set.
function PropertyMoveGlobalToFront(propertySet, parentName)
    TTE_Log("Implementation required")
end

-- Returns the total number of property keys in the given property set, optionally searching parents as well.
function PropertyNumKeys(propertySet, --[[optional]] searchParents)
    if searchParents then
        TTE_Log("Implementation required") -- todo parent stuff
    end
    return MetaGetNumChildren(propertySet)
end

-- Removes a property key from the property key.
function PropertyRemove(propertySet, key)
    MetaReleaseChild(propertySet, key)
end

-- Removes a global (parent) property set. Pass in the name
function PropertyRemoveGlobal(propertySet, parentName)
    TTE_Log("Implementation required")
end

-- Adds a global (parent) property set. Pass in the name, and it will be found in currently loaded props, and loaded if required.
function PropertyAddGlobal(propertySet, parentName)
    TTE_Log("Implementation required")
end

-- Sets the key as modified. Does nothing internally and only acts as compatibility
function PropertySetModified(propertySet, key)
end

-- Returns a property set? Not sure. Here for compatibility.
function PropertySetIsRuntime(propertySet)
    return nil
end