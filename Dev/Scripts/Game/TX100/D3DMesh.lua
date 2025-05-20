-- TX100 Mesh

require("ToolLibrary/Game/Common/D3DMesh.lua")

function RegisterTXD3DMesh(vendor, hTexture)

    local indexBuffer = NewClass("class D3DIndexBuffer", 0)
    indexBuffer.Serialiser = "SerialiseD3DIndexBuffer0"
    indexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
    indexBuffer.Members[2] = NewMember("mFormat", kMetaInt)      -- 101 for U16, else U32 [??]
    indexBuffer.Members[3] = NewMember("mNumIndicies", kMetaInt)
    indexBuffer.Members[4] = NewMember("mIndexByteSize", kMetaInt)
    indexBuffer.Members[5] = NewMember("_IndexBufferData", kMetaClassInternalBinaryBuffer)
    indexBuffer.Members[5].Flags = kMetaMemberVersionDisable
    MetaRegisterClass(indexBuffer)

    local triangleSet = NewClass("class D3DMesh::TriangleSet", 0)
    triangleSet.Serialiser = "SerialiseTXTriangleSet"
    triangleSet.Members[1] = NewMember("mBonePaletteIndex", kMetaInt)
    triangleSet.Members[2] = NewMember("mGeometryFormat", kMetaInt)
    triangleSet.Members[3] = NewMember("mMinVertIndex", kMetaInt)
    triangleSet.Members[4] = NewMember("mMaxVertIndex", kMetaInt)
    triangleSet.Members[5] = NewMember("mStartIndex", kMetaInt)
    triangleSet.Members[6] = NewMember("mNumPrimitives", kMetaInt)
    triangleSet.Members[7] = NewMember("mhDiffuseMap", hTexture)
    triangleSet.Members[8] = NewMember("mhDetailMap", hTexture)
    MetaRegisterClass(triangleSet)

    local arrayTriangleSet, _ = RegisterTXCollection("class DCArray<class D3DMesh::TriangleSet>", nil,
        triangleSet)

    local meshEntry = NewClass("struct D3DMesh::PaletteEntry", 0)
    meshEntry.Members[1] = NewMember("mBoneName", kMetaClassString)
    meshEntry.Members[2] = NewMember("mSkeletonIndex", kMetaInt)
    MetaRegisterClass(meshEntry)

    local arrayMeshPalette, _ = RegisterTXCollection("class DCArray<struct D3DMesh::PaletteEntry>", nil,
        meshEntry)
    local arrayArrayMeshPalette, _ = RegisterTXCollection(
        "class DCArray<class DCArray<struct D3DMesh::PaletteEntry> >", nil, arrayMeshPalette)

    local vertexBuffer = NewClass("class D3DVertexBuffer", 0)
    vertexBuffer.Serialiser = "SerialiseTXD3DVertexBuffer"
    vertexBuffer.Members[1] = NewMember("mbLocked", kMetaBool)
    vertexBuffer.Members[2] = NewMember("mNumVerts", kMetaInt)
    vertexBuffer.Members[3] = NewMember("mVertFormat", kMetaUnsignedLong)
    vertexBuffer.Members[4] = NewMember("mVertSize", kMetaInt)
    vertexBuffer.Members[5] = NewMember("_VertexBufferData", kMetaClassInternalBinaryBuffer)
    vertexBuffer.Members[5].Flags = kMetaMemberVersionDisable
    MetaRegisterClass(vertexBuffer)

    local mesh = NewClass("class D3DMesh", 0)
    mesh.Extension = "d3dmesh"
    mesh.Serialiser = "SerialiseD3DMesh0"
    mesh.Members[1] = NewMember("mName", kMetaClassString)
    mesh.Members[2] = NewMember("mbDeformable", kMetaBool)
    mesh.Members[3] = NewMember("mTriangleSets", arrayTriangleSet)
    mesh.Members[4] = NewMember("mBonePalettes", arrayArrayMeshPalette)
    mesh.Members[5] = NewMember("_IndexBuffer0", indexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[6] = NewMember("_VertexBuffer0", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[7] = NewMember("_VertexBuffer1", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[8] = NewMember("_VertexBuffer2", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[9] = NewMember("_VertexBuffer3", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[10] = NewMember("_VertexBuffer4", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[11] = NewMember("_VertexBuffer5", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[12] = NewMember("_VertexBuffer6", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[13] = NewMember("_VertexBuffer7", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    mesh.Members[14] = NewMember("_VertexBuffer8", vertexBuffer, kMetaMemberVersionDisable + kMetaMemberSerialiseDisable)
    MetaRegisterClass(mesh)

    return mesh
end

function SerialiseTXTriangleSet(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    -- for some reason they write the shader and pixel name twice. its already written from the default serialise. oh well!
    MetaStreamReadString(stream)
    return true
end

-- TODO
function SerialiseTXD3DVertexBuffer(stream, inst, write)
    if not MetaSerialiseDefault(stream, inst, write) then
        return false
    end
    if write then
        -- fuck write for now, only read
    else
        local vertSize = MetaGetClassValue(MetaGetMember(inst, "mVertSize"))
        local numVerts = MetaGetClassValue(MetaGetMember(inst, "mNumVerts"))
        local vertType = MetaGetClassValue(MetaGetMember(inst, "mVertFormat"))
        local totalVerticesByteSize = 0
        if vertType == 0 then
            totalVerticesByteSize = numVerts * vertSize -- normal
        else
            TTE_Log("Vertex buffer format is non-zero: not supported in Texas Hold'em")
            return false
        end
        MetaStreamReadBuffer(stream, MetaGetMember(inst, "_VertexBufferData"), totalVerticesByteSize)
    end
    return true
end