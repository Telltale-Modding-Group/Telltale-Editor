-- Texture class implementation

function RegisterBoneD3DTexture(vendor, platform)
    local tex = NewClass("class D3DTexture", 0)
	tex.Extension = "d3dtx"
    tex.Serialiser = "SerialiseD3DTexture_Bone"
	tex.Members[1] = NewMember("mName", kMetaClassString)
	tex.Members[2] = NewMember("mImportName", kMetaClassString)
	tex.Members[3] = NewMember("mbHasTextureData", kMetaBool)
	tex.Members[4] = NewMember("mbIsMipMapped", kMetaBool)
	tex.Members[5] = NewMember("mbIsFiltered", kMetaBool)
	tex.Members[6] = NewMember("mNumMipLevels", kMetaUnsignedInt)
	tex.Members[7] = NewMember("mD3DFormat", kMetaUnsignedInt)
	tex.Members[8] = NewMember("mWidth", kMetaUnsignedInt)
	tex.Members[9] = NewMember("mHeight", kMetaUnsignedInt)
	tex.Members[10] = NewMember("mType", kMetaInt)
	tex.Members[11] = NewMember("mbAlphaHDR", kMetaBool)

    tex.Members[12] = NewMember("_TextureData", kMetaClassInternalBinaryBuffer)
    tex.Members[12].Flags = kMetaMemberVersionDisable
	MetaRegisterClass(tex)
    return tex
end

function SerialiseD3DTexture_Bone(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if not MetaGetClassValue(MetaGetMember(instance, "mbHasTextureData")) then
        return true -- if we don't have any texture data, don't read into buffer
    end
    bufferInst = MetaGetMember(instance, "_TextureData")
    if write then
        ddsTexSize = MetaGetBufferSize(bufferInst) -- write buffer size
        MetaStreamWriteInt(stream, ddsTexSize)
        MetaStreamWriteBuffer(stream, bufferInst) -- write buffer
    else
        ddsTexSize = MetaStreamReadInt(stream) -- read buffer size
        MetaStreamReadBuffer(stream, bufferInst, ddsTexSize)
    end
    return true
end