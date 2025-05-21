
function SerialiseD3DIndexBuffer0(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    if write then
        MetaStreamWriteBuffer(stream, MetaGetMember(inst, "_IndexBufferData"))
    else
        local indexBufferBytes = 2 -- format 101 means U16
        if MetaGetClassValue(MetaGetMember(inst, "mFormat")) ~= 101 then
            indexBufferBytes = 4 -- else U32s
        end
        MetaStreamReadBuffer(stream, MetaGetMember(inst, "_IndexBufferData"), indexBufferBytes * MetaGetClassValue(MetaGetMember(inst, "mNumIndicies")))
    end
    return true
end

function SerialiseD3DMesh0(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    if not write then
        local hasIndexBuffer = MetaStreamReadBool(stream)
        if hasIndexBuffer and not MetaSerialise(stream, MetaGetMember(inst, "_IndexBuffer0"), write, "Index Buffer") then
            return false
        end
        for i=0,8 do -- Nine vertex buffers
            local hasVertexBuffer = MetaStreamReadBool(stream)
            if hasVertexBuffer and not MetaSerialise(stream,
                MetaGetMember(inst, "_VertexBuffer" .. tostring(i)), write, "VertexBuffer" .. tostring(i)) then
                return false
            end
        end
        local bonePalettes = MetaGetMember(inst, "mBonePalettes")
        local numPalettes = ContainerGetNumElements(bonePalettes)
        for i=1,numPalettes do
            local palette = ContainerGetElement(bonePalettes, i-1)
            local numb = ContainerGetNumElements(palette)
            for j=1,numb do
                -- Never not -1, unused
                TTE_Assert(MetaGetClassValue(MetaGetMember(ContainerGetElement(palette, j - 1), "mSkeletonIndex")) == -1, "Skeleton index not -1!")
            end   
        end    
    end
    return true
end

-- Bone D3DMesh => common mesh. any accesses outside this functiom will return nil!
function NormaliseD3DMesh0(inst, state)

    local meshName = MetaGetClassValue(MetaGetMember(inst, "mName"))
    CommonMeshSetName(state, meshName)

    if MetaGetClassValue(MetaGetMember(inst, "mbDeformable")) then
        CommonMeshSetDeformable(state)
    end

    -- no concept of LODs. use one lod. each batch translates to a triangle set

    CommonMeshPushLOD(state, 0)

    local triangleSets = MetaGetMember(inst, "mTriangleSets")
    local numTriangleSets = ContainerGetNumElements(triangleSets)
    local bonePalettes = MetaGetMember(inst, "mBonePalettes")
    local resolveBoneTable = {} -- NOTE the max size is 18! only 18*4 vec4s extra could fit into the shaders.
    local boneIndexTable = {}

    for i=1,numTriangleSets do

        local triangleSet = ContainerGetElement(triangleSets, i-1)

        CommonMeshPushBatch(state, false) -- not a shadow batch (False)
        CommonMeshSetBatchBounds(state, false, MetaGetMember(triangleSet, "mBoundingBox"))

        local paletteIndex = MetaGetClassValue(MetaGetMember(triangleSet, "mBonePaletteIndex"))
        local minVert = MetaGetClassValue(MetaGetMember(triangleSet, "mMinVertIndex"))
        local maxVert = MetaGetClassValue(MetaGetMember(triangleSet, "mMaxVertIndex"))
        local startIndex = MetaGetClassValue(MetaGetMember(triangleSet, "mStartIndex"))
        local numPrim = MetaGetClassValue(MetaGetMember(triangleSet, "mNumPrimitives"))
        local numInd = MetaGetClassValue(MetaGetMember(triangleSet, "mNumTotalIndices"))

        local diffuseMaterial = MetaGetClassValue(MetaGetMember(MetaGetMember(triangleSet, "mhDiffuseMap"), "mHandle"))

        local bonePalette = ContainerGetElement(bonePalettes, paletteIndex)
        local numBoneMaps = ContainerGetNumElements(bonePalette)
        resolveBoneTable[i] = {}
        for j=1,numBoneMaps do -- j-1 below as 0 based in actual buffer
            resolveBoneTable[i][j-1] = MetaGetClassValue(MetaGetMember(ContainerGetElement(bonePalette, j - 1), "mBoneName"))
        end
        boneIndexTable[i] = {Min = minVert, Max = maxVert}

        CommonMeshPushMaterial(state, diffuseMaterial)
        CommonMeshSetBatchParameters(state, false, minVert, maxVert, startIndex, numPrim, numInd, 0, i - 1) -- base index not exist

    end

    local indexBuffer = MetaGetMember(inst, "_IndexBuffer0")
    CommonMeshSetIndexBuffer(state, MetaGetClassValue(MetaGetMember(indexBuffer, "mNumIndicies")), 
                                    MetaGetClassValue(MetaGetMember(indexBuffer, "mFormat")) == 101,
                                    MetaGetMember(indexBuffer, "_IndexBufferData"))

    -- In this game, they store the vertex information for each attrib in its own buffer, ie no interleaving.

    local function processBoneBuffer(state, bufferNum, index, expectedStride, attrib, format, isBoneIndices)
        local vertexBuffer = MetaGetMember(inst, "_VertexBuffer" .. tostring(bufferNum))
        local nVerts = MetaGetClassValue(MetaGetMember(vertexBuffer, "mNumVerts"))
        if nVerts > 0 then
            local stride = MetaGetClassValue(MetaGetMember(vertexBuffer, "mVertSize"))

            TTE_Assert(stride == expectedStride, "Vertex buffer " .. tostring(bufferNum) .. " stride is not "
            .. tostring(expectedStride) .. ": it's " .. tostring(stride) .. "!")

            local bufferCache = MetaGetMember(vertexBuffer, "_VertexBufferData")

            -- decompress if needed
            local type = MetaGetClassValue(MetaGetMember(vertexBuffer, "mType"))
            if MetaGetClassValue(MetaGetMember(vertexBuffer, "mbStoreCompressed")) and (type == 2 or type == 4) then
                local compressedFmt = kCommonMeshCompressedFormatSNormNormal
                if type == 4 then
                    compressedFmt = stride == 12 and kCommonMeshCompressedFormatSNormNormal or kCommonMeshCompressedFormatUNormUV
                    MetaSetClassValue(MetaGetMember(vertexBuffer, "mType"), stride == 12 and 1 or 3) -- decompressed, set to normal vec3s/vec2s
                else -- type 2, ie compressed normal
                    MetaSetClassValue(MetaGetMember(vertexBuffer, "mType"), 1) -- decompressed, set to normal vec3s
                end
                CommonMeshDecompressVertices(state, bufferCache, nVerts, compressedFmt)
            end
            if isBoneIndices then
                CommonMeshResolveBonePalettes(state, bufferCache, resolveBoneTable, boneIndexTable, false, 4, meshName)
            end
            CommonMeshPushVertexBuffer(state, nVerts, stride, bufferCache)
            CommonMeshAddVertexAttribute(state, attrib, index, format)
            return index + 1
        end
        return index
    end

    local pushedVertexBufferIndex = 0

    -- BUFFER 0: POSITION
    pushedVertexBufferIndex = processBoneBuffer(state, 0, pushedVertexBufferIndex, 12, kCommonMeshAttributePosition, kCommonMeshFloat3, false)

    -- BUFFER 1: NORMAL
    pushedVertexBufferIndex = processBoneBuffer(state, 1, pushedVertexBufferIndex, 12, kCommonMeshAttributeNormal, kCommonMeshFloat3, false)

    -- BUFFER 2: BONE WEIGHTS as F3
    pushedVertexBufferIndex = processBoneBuffer(state, 2, pushedVertexBufferIndex, 12, kCommonMeshAttributeBlendWeight, kCommonMeshFloat3, false)

    -- BUFFER 3: BONE INDICES. ENDIAN FLIPPED ON MACOS - CHECK. (4 bytes)
    pushedVertexBufferIndex = processBoneBuffer(state, 3, pushedVertexBufferIndex, 4, kCommonMeshAttributeBlendIndex, kCommonMeshUByte4, true)

    -- BUFFER 4: DIFFUSE UV (UV0).
    pushedVertexBufferIndex = processBoneBuffer(state, 4, pushedVertexBufferIndex, 8, kCommonMeshAttributeUVDiffuse, kCommonMeshFloat2, false)

    -- BUFFER 5: LIGHTMAP UV (UV1)
    pushedVertexBufferIndex = processBoneBuffer(state, 5, pushedVertexBufferIndex, 8, kCommonMeshAttributeUVLightMap, kCommonMeshFloat2, false)

    -- BUFFER 6: ?? UV2
    pushedVertexBufferIndex = processBoneBuffer(state, 6, pushedVertexBufferIndex, 0, kCommonMeshAttributeUnknown, kCommonMeshFormatUnknown, false)

    -- BUFFER 7: ?? UV3
    pushedVertexBufferIndex = processBoneBuffer(state, 7, pushedVertexBufferIndex, 0, kCommonMeshAttributeUnknown, kCommonMeshFormatUnknown, false)

    -- BUFFER 8: ?? TANGENT????
    pushedVertexBufferIndex = processBoneBuffer(state, 8, pushedVertexBufferIndex, 12, kCommonMeshAttributeTangent, kCommonMeshFloat3, false)

    return true
end