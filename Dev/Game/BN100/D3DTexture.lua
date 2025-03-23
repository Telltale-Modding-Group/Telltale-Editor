-- Texture class implementation

function RegisterBoneD3DTexture(vendor, platform)
    local tex = NewClass("class D3DTexture", 0)
    tex.Extension = "d3dtx"
    tex.Serialiser = "SerialiseD3DTexture_Bone"
    tex.Normaliser = "NormaliseD3DTexture_Bone"
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

function NormaliseD3DTexture_Bone(instance, state) 
    local name = MetaGetClassValue(MetaGetMember(instance, "mName"))
    local width = MetaGetClassValue(MetaGetMember(instance, "mWidth"))
    local height = MetaGetClassValue(MetaGetMember(instance, "mHeight"))
    local numMips = MetaGetClassValue(MetaGetMember(instance, "mNumMipLevels"))
    local data = MetaGetMember(instance, "_TextureData")

    CommonTextureSetName(state, name)
    CommonTextureSetDimensions(state, width, height, 1)

    local format = MetaGetClassValue(MetaGetMember(instance, "mD3DFormat"))
    local actualFormat = 0
    local rowPitch = 0
    if format == 22 then -- DXT9 B8G8R8X8
        actualFormat = kCommonTextureFormatRGBA8
        CommonTextureResolveRGBA(state, data, kCommonTextureResolvableFormatBGRX, width, height) -- sets alpha to 0xff (BGR + extra byte => RGBA opaque)
        rowPitch = 4 * width
    elseif format == 894720068 then -- 'DXT5' ASCII
        actualFormat = kCommonTextureFormatDXT5
        rowPitch = math.max(1, math.floor(width / 4)) * 8
    elseif format == 861165636 then -- 'DXT3' ASCII
        actualFormat = kCommonTextureFormatDXT3
        rowPitch = math.max(1, math.floor(width / 4)) * 16
    elseif format == 827611204 then -- 'DXT1' ASCII (LE)
        actualFormat = kCommonTextureFormatDXT1
        rowPitch = math.max(1, math.floor(width / 4)) * 16
    else
        TTE_Assert(false, "Texture " .. name .. " format is unknown: " .. tostring(format))
    end

    CommonTextureSetFormat(state, actualFormat)

    local slicePitch = MetaGetBufferSize(data)

    -- TODO multiple mips. need to push in order.
    CommonTexturePushOrderedImage(state, width, height, rowPitch, slicePitch, actualFormat, data)

    TTE_Log("Created common texture " .. name)
    return true
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