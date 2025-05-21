-- Mesh class implementation

require("ToolLibrary/Game/Common/D3DMesh.lua")

function RegisterBoneD3DMesh(platform, vendor, bb, hTexture, arrayDCInt, MetaCol, MetaCI)

    local indexBuffer = NewClass("class D3DIndexBuffer", 0)
    indexBuffer.Serialiser = "SerialiseD3DIndexBuffer0"
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
    mesh.Serialiser = "SerialiseD3DMesh0"
    mesh.Normaliser = "NormaliseD3DMesh0"
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
