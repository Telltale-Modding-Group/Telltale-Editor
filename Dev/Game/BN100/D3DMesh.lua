-- Mesh class implementation

function RegisterBoneD3DMesh(platform, vendor, bb, hTexture, arrayDCInt, MetaCol, MetaCI)

	local indexBuffer = NewClass("class D3DIndexBuffer", 0) -- custom seri todo
    indexBuffer.Serialiser = "SerialiseBoneD3DIndexBuffer"
	indexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
	indexBuffer.Members[2] = NewMember("mFormat", kMetaInt)
	indexBuffer.Members[3] = NewMember("mNumIndicies", kMetaInt) -- spelling mate
	indexBuffer.Members[4] = NewMember("mIndexByteSize", kMetaInt)
    indexBuffer.Members[5] = NewMember("_IndexBufferData", kMetaClassInternalBinaryBuffer)
    indexBuffer.Members[5].Flags = kMetaMemberVersionDisable
	MetaRegisterClass(indexBuffer)

	local triangleSet = NewClass("class D3DMesh::TriangleSet", 0) -- TODO custom serialiser
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

	arrayTriangleSet, _ = RegisterBoneCollection(MetaCI, "class DCArray<class D3DMesh::TriangleSet>", nil, triangleSet)

	local meshEntry = NewClass("struct D3DMesh::PaletteEntry", 0)
	meshEntry.Members[1] = NewMember("mBoneName", kMetaClassString)
	meshEntry.Members[2] = NewMember("mSkeletonIndex", kMetaInt)
	MetaRegisterClass(meshEntry)

	arrayMeshPalette, _ = RegisterBoneCollection(MetaCI, "class DCArray<struct D3DMesh::PaletteEntry>", nil, meshEntry)
	arrayArrayMeshPalette, _ = RegisterBoneCollection(MetaCI, "class DCArray<class DCArray<struct D3DMesh::PaletteEntry> >", nil, arrayMeshPalette)

	local vertexBuffer = NewClass("class D3DVertexBuffer", 0) -- custom ser todo
	vertexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
	vertexBuffer.Members[2] = NewMember("mNumVerts", kMetaInt)
	vertexBuffer.Members[3] = NewMember("mVertSize", kMetaInt)
	vertexBuffer.Members[4] = NewMember("mType", kMetaInt)
	vertexBuffer.Members[5] = NewMember("mbStoreCompressed", kMetaBool)
    vertexBuffer.Members[6] = NewMember("_VertexBufferData", kMetaClassInternalBinaryBuffer)
    vertexBuffer.Members[6].Flags = kMetaMemberVersionDisable
	MetaRegisterClass(vertexBuffer)

	local mesh = NewClass("class D3DMesh", 0) -- todo custom serialisr
	mesh.Extension = "d3dmesh"
	mesh.Members[1] = NewMember("mName", kMetaClassString)
	mesh.Members[2] = NewMember("mbDeformable", kMetaBool)
	mesh.Members[3] = NewMember("mTriangleSets", arrayTriangleSet)
	mesh.Members[4] = NewMember("mBonePalettes", arrayArrayMeshPalette)
	mesh.Members[5] = NewMember("mbLightmaps", kMetaBool)
	MetaRegisterClass(mesh)

    return mesh
end

function SerialiseBoneD3DIndexBuffer(stream, inst, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if write then
        MetaStreamWriteBuffer(MetaGetMember(inst, "_IndexBufferData"))
    else
        indexBufferBytes = 2 -- format 101 means U16
        if MetaGetClassValue(MetaGetMember(inst, "mFormat")) ~= 101 then
            indexBufferBytes = 4 -- else U32s
        end
        MetaStreamReadBuffer(MetaGetMember(inst, "_IndexBufferData"), indexBufferBytes * MetaGetClassValue(MetaGetMember(inst, "mNumIndicies")))
    end
    return true
end

function SerialiseBoneD3DVertexBuffer(stream, inst, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if write then
        
    else
        
    end
    return true
end
