-- Mesh class implementation

function RegisterBoneD3DMesh(platform, vendor, bb, hTexture, arrayDCInt, MetaCol, MetaCI)

    local indexBuffer = NewClass("class D3DIndexBuffer", 0)
    indexBuffer.Serialiser = "SerialiseBoneD3DIndexBuffer"
    indexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
    indexBuffer.Members[2] = NewMember("mFormat", kMetaInt) -- 101 for U16, else U32
    indexBuffer.Members[3] = NewMember("mNumIndicies", kMetaInt) -- spelling mate [DONT FIX! needs to be like this for hash]
    indexBuffer.Members[4] = NewMember("mIndexByteSize", kMetaInt)
    indexBuffer.Members[5] = NewMember("_IndexBufferData", kMetaClassInternalBinaryBuffer)
    indexBuffer.Members[5].Flags = kMetaMemberVersionDisable
    MetaRegisterClass(indexBuffer)

    local triangleSet = NewClass("class D3DMesh::TriangleSet", 0)
    triangleSet.Serialiser = "SerialiseBoneTriangleSet"
    triangleSet.Members[1] = NewMember("mVertexShaderName", kMetaClassString)
    triangleSet.Members[2] = NewMember("mPixelShaderName", kMetaClassString)
    triangleSet.Members[3] = NewMember("mBonePaletteIndex", kMetaInt)
    triangleSet.Members[4] = NewMember("mGeometryFormat", kMetaInt)
    triangleSet.Members[5] = NewMember("mMinVertIndex", kMetaInt)
    triangleSet.Members[6] = NewMember("mMaxVertIndex", kMetaInt)
    triangleSet.Members[7] = NewMember("mStartIndex", kMetaInt)
    triangleSet.Members[8] = NewMember("mNumPrimitives", kMetaInt)
    triangleSet.Members[9] = NewMember("mLightingGroup", kMetaClassString)
    triangleSet.Members[10] = NewMember("mBoundingBox", bb)
    triangleSet.Members[11] = NewMember("mhDiffuseMap", hTexture)
    triangleSet.Members[12] = NewMember("mhDetailMap", hTexture)
    triangleSet.Members[13] = NewMember("mhLightMap", hTexture)
    triangleSet.Members[14] = NewMember("mhBumpMap", hTexture)
    triangleSet.Members[15] = NewMember("mbHasPixelShader_RemoveMe", kMetaBool)
    triangleSet.Members[16] = NewMember("mTriStrips", arrayDCInt)
    triangleSet.Members[17] = NewMember("mNumTotalIndices", kMetaInt)
    triangleSet.Members[18] = NewMember("mbDoubleSided", kMetaBool)
    triangleSet.Members[19] = NewMember("mfBumpHeight", kMetaFloat)
    triangleSet.Members[20] = NewMember("mhEnvMap", hTexture)
    triangleSet.Members[21] = NewMember("mfEccentricity", kMetaFloat)
    triangleSet.Members[22] = NewMember("mSpecularColor", MetaCol)
    triangleSet.Members[23] = NewMember("mAmbientColor", MetaCol)
    triangleSet.Members[24] = NewMember("mbSelfIlluminated", kMetaBool)
    triangleSet.Members[25] = NewMember("mAlphaMode", kMetaInt)
    triangleSet.Members[26] = NewMember("mfReflectivity", kMetaFloat)
    MetaRegisterClass(triangleSet)

    local arrayTriangleSet, _ = RegisterBoneCollection(MetaCI, "class DCArray<class D3DMesh::TriangleSet>", nil, triangleSet)

    local meshEntry = NewClass("struct D3DMesh::PaletteEntry", 0)
    meshEntry.Members[1] = NewMember("mBoneName", kMetaClassString)
    meshEntry.Members[2] = NewMember("mSkeletonIndex", kMetaInt)
    MetaRegisterClass(meshEntry)

    local arrayMeshPalette, _ = RegisterBoneCollection(MetaCI, "class DCArray<struct D3DMesh::PaletteEntry>", nil, meshEntry)
    local arrayArrayMeshPalette, _ = RegisterBoneCollection(MetaCI, "class DCArray<class DCArray<struct D3DMesh::PaletteEntry> >", nil, arrayMeshPalette)

    local vertexBuffer = NewClass("class D3DVertexBuffer", 0)
    vertexBuffer.Serialiser = "SerialiseBoneD3DVertexBuffer"
    vertexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
    vertexBuffer.Members[2] = NewMember("mNumVerts", kMetaInt)
    vertexBuffer.Members[3] = NewMember("mVertSize", kMetaInt)
    -- mType values: 0(??) 1(Vec3, 3x floats), 2(compressed vec3s (from u16s), -1.0 to 1.0 in each, third is not stored, Xproduct), 3(Vec2, 2x floats),
    -- 4(compressed vec2s, from u16s, 0.0 to 1., each from each byte. unsigned)
    vertexBuffer.Members[4] = NewMember("mType", kMetaInt)
    vertexBuffer.Members[5] = NewMember("mbStoreCompressed", kMetaBool) -- if it should be compressed when writing. can still not be compressed if true. see mType
    vertexBuffer.Members[6] = NewMember("_VertexBufferData", kMetaClassInternalBinaryBuffer)
    vertexBuffer.Members[6].Flags = kMetaMemberVersionDisable
    MetaRegisterClass(vertexBuffer)

    local mesh = NewClass("class D3DMesh", 0)
    mesh.Extension = "d3dmesh"
    mesh.Serialiser = "SerialiseBoneD3DMesh"
    mesh.Normaliser = "NormaliseBoneD3DMesh"
    mesh.Members[1] = NewMember("mName", kMetaClassString)
    mesh.Members[2] = NewMember("mbDeformable", kMetaBool)
    mesh.Members[3] = NewMember("mTriangleSets", arrayTriangleSet)
    mesh.Members[4] = NewMember("mBonePalettes", arrayArrayMeshPalette)
    mesh.Members[5] = NewMember("mbLightmaps", kMetaBool)
    mesh.Members[6] = NewMember("_IndexBuffer0", indexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[7] = NewMember("_VertexBuffer0", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[8] = NewMember("_VertexBuffer1", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[9] = NewMember("_VertexBuffer2", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    -- VERTEX BUFFER BELOW (NUMBER 3, 4TH ONE) IS ENDIAN FLIPPED. FUCK KNOWS!!
    mesh.Members[10] = NewMember("_VertexBuffer3", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[11] = NewMember("_VertexBuffer4", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[12] = NewMember("_VertexBuffer5", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[13] = NewMember("_VertexBuffer6", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[14] = NewMember("_VertexBuffer7", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[15] = NewMember("_VertexBuffer8", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    MetaRegisterClass(mesh)

    return mesh
end

function SerialiseBoneD3DMesh(stream, inst, write)
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
function NormaliseBoneD3DMesh(inst, state)

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

function SerialiseBoneTriangleSet(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    -- for some reason they write the shader and pixel name twice. its already written from the default serialise. oh well!
    MetaStreamReadString(stream)
    if MetaGetClassValue(MetaGetMember(inst, "mbHasPixelShader_RemoveMe")) then
        MetaStreamReadString(stream)
    end
    return true
end

function SerialiseBoneD3DIndexBuffer(stream, inst, write)
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

function SerialiseBoneD3DVertexBuffer(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    if write then
        -- fuck write, only read
    else
        local isCompressed = MetaGetClassValue(MetaGetMember(inst, "mbStoreCompressed"))
        local vertSize = MetaGetClassValue(MetaGetMember(inst, "mVertSize"))
        local numVerts = MetaGetClassValue(MetaGetMember(inst, "mNumVerts"))
        local vertType = MetaGetClassValue(MetaGetMember(inst, "mType"))
        local totalVerticesByteSize = 0
        if isCompressed and (vertType == 2 or vertType == 4) then
            totalVerticesByteSize = 2 * numVerts
        else
            totalVerticesByteSize = vertSize * numVerts
        end
        MetaStreamReadBuffer(stream, MetaGetMember(inst, "_VertexBufferData"), totalVerticesByteSize)
    end
    return true
end
