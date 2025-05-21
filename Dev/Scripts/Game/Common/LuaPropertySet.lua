-- PropertySet class implementation.

-- Serialiser, V0 (oldest)
function SerialisePropertySet0(metaStream, propInstance, isWrite)
	-- start by calling default meta serialise (serialise members, so flags and parent list)
	if not MetaSerialiseDefault(metaStream, propInstance, isWrite) then
		return false;
	end

	MetaStreamBeginBlock(metaStream, isWrite) -- rest is inside a block in the binary

	if isWrite then
        local typeMap = {}
		local numTypeMap = {}
		local tot = 0
		local children = MetaGetChildrenNames(propInstance)
		for _, child in pairs(children) do
			local tp = MetaGetClass(MetaGetChild(propInstance, child))
			if typeMap[tp] == nil then
				typeMap[tp] = {}
				numTypeMap[tp] = 0
				tot = tot + 1
			end
			table.insert(typeMap[tp], child)
			numTypeMap[tp] = numTypeMap[tp] + 1
		end
		MetaStreamWriteInt(metaStream, tot)
		for typeName, keys in pairs(typeMap) do
			MetaStreamWriteSymbol(metaStream, typeName)
			MetaStreamWriteInt(metaStream, numTypeMap[typeName])
			for _, val in pairs(keys) do
				MetaStreamWriteSymbol(metaStream, val)
				if not MetaSerialise(metaStream, MetaGetChild(propInstance, val), true, "Property: " .. val) then return false end
			end
		end
    else
		local numTypes = MetaStreamReadInt(metaStream)
		for i=1,numTypes do
            local typeSymbol = MetaStreamReadSymbol(metaStream)
			local propType, propTypeVersionIndex = MetaStreamFindClass(metaStream, typeSymbol) -- read class symbol
            if propType == nil then
                local typeNameString = SymbolTableFind(typeSymbol)
                if string.len(typeNameString) == 0 then
                    typeNameString = typeSymbol
                end
                TTE_Log("Unregistered or unknown type in property set: " .. typeNameString)
                return false
            end
			local numOfThatType = MetaStreamReadInt(metaStream)
			for j=1, numOfThatType do
				local key = SymbolTableFind(MetaStreamReadSymbol(metaStream))
				local inst_of_type = MetaCreateInstance(propType, propTypeVersionIndex, key, propInstance)
				if not MetaSerialise(metaStream, inst_of_type, isWrite, key) then
					return false
				end
			end
		end
	end

	MetaStreamEndBlock(metaStream, isWrite)
	return true
end