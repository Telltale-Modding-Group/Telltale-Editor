-- Dialog Implementation

function SerialiseDialogBaseBone1(stream, instance, write)
    local arrayRefs = MetaGetMember(instance, "_mStyleRefs")
    local hasRefs = ContainerGetNumElements(arrayRefs) > 0
    if write and hasRefs then MetaSetClassValue(MetaGetMember(instance, "mbHasStyleGuides"), true) end

    if not MetaSerialiseDefault(stream, instance, write) then return false end

    if MetaGetClassValue(MetaGetMember(instance, "mbHasStyleGuides")) then
        if not MetaSerialise(stream, arrayRefs, write) then return false end
    end

    return true
end

function SerialiseDialogResourceBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    -- TODO. if write, purse unused references (see ida) (maybe do in normalisation??)
    if write then
        return false
    end

    local function ReadDialogBaseArray(num, memberName)
        if num == 0 then
            return true -- nothing
        end
        local ids = {}
        for i=1,num do
            ids[i] = MetaStreamReadInt(stream)
        end
        local array = MetaGetMember(instance, memberName)
        for i=1,num do
            local newBase = ContainerEmplaceElement(array) -- transient reference. newBase access only valid until another call to modify the container.
            if not MetaSerialise(stream, newBase, false) then
                return false
            end
            MetaSetClassValue(MetaGetMember(MetaGetMember(newBase, "Baseclass_DialogBase"), "_mActualID"), ids[i])
        end
        return true
    end

    local numDialogDialogs = MetaStreamReadInt(stream)
    local numDialogBranches = MetaStreamReadInt(stream)
    local numDialogItems = MetaStreamReadInt(stream)
    local numDialogExchanges = MetaStreamReadInt(stream)
    local numDialogLines = MetaStreamReadInt(stream)
    local numDialogTexts = MetaStreamReadInt(stream)

    if not ReadDialogBaseArray(numDialogDialogs, "_mDialogDialogs") then
        return false
    end
    if not ReadDialogBaseArray(numDialogBranches, "_mDialogBranches") then
        return false
    end
    if not ReadDialogBaseArray(numDialogItems, "_mDialogItems") then
        return false
    end
    if not ReadDialogBaseArray(numDialogExchanges, "_mDialogExchanges") then
        return false
    end
    if not ReadDialogBaseArray(numDialogLines, "_mDialogLines") then
        return false
    end
    if not ReadDialogBaseArray(numDialogTexts, "_mDialogTexts") then
        return false
    end

    return true
end