-- Provides exact game engine functionality for the LuaContainer function collection. GAME: TX100

-- int ContainerGetNumElements(container) => Get the number of elements in the container.
function ContainerGetNumElements(container)
	return _ContainerGetNumElements(container) -- call low level lib
end

-- nil ContainerRemoveElement(container,index) => Removes the item at index in the container
function ContainerRemoveElement(container, index)
	_ContainerRemoveElement(container, index)
end

-- nil ContainerInsertElement(container,element) => Insert an item at the end of the container
function ContainerInsertElement(container, element)
	_ContainerInsertElement(container, element)
end

-- obj ContainerGetElement(container,_index) => Get the item in the container
function ContainerGetElement(container, index)
	return _ContainerGetElement(container, index)
end