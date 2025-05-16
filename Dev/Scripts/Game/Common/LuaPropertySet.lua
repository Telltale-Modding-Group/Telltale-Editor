-- PropertySet class implementation.

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
				if not MetaSerialise(metaStream, inst_of_type, isWrite, key) then
					return false
				end
			end
		end
	end

	MetaStreamEndBlock(metaStream, isWrite)
	return true
end