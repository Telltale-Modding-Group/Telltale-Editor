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
    CommonTextureSetDimensions(state, width, height, 1, 1, 1) -- 1 depth, 1 array size

    local format = MetaGetClassValue(MetaGetMember(instance, "mD3DFormat"))
    local actualFormat = 0
    if format == 22 then -- DXT9 B8G8R8X8
        actualFormat = kCommonTextureFormatRGBA8
        CommonTextureResolveRGBA(state, data, kCommonTextureResolvableFormatBGRX, width, height) -- sets alpha to 0xff (BGR + extra byte => RGBA opaque)
    elseif format == 894720068 then -- 'DXT5' ASCII
        actualFormat = kCommonTextureFormatDXT5
    elseif format == 861165636 then -- 'DXT3' ASCII
        actualFormat = kCommonTextureFormatDXT3
    elseif format == 827611204 then -- 'DXT1' ASCII (LE)
        actualFormat = kCommonTextureFormatDXT1
    else
        TTE_Assert(false, "Texture " .. name .. " format is unknown: " .. tostring(format))
    end -- any updates must update serialiser

    CommonTextureSetFormat(state, actualFormat)

    local rowPitch = CommonTextureCalculatePitch(actualFormat, width, height)
    local slicePitch = CommonTextureCalculateSlicePitch(actualFormat, width, height)

    -- TODO multiple mips. need to push in order.
    CommonTexturePushOrderedImage(state, width, height, rowPitch, slicePitch, data)

    return true
end

function SerialiseD3DTexture_Bone(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if not MetaGetClassValue(MetaGetMember(instance, "mbHasTextureData")) then
        return true -- if we don't have any texture data, don't read into buffer
    end
    local bufferInst = MetaGetMember(instance, "_TextureData")
    local format = MetaGetClassValue(MetaGetMember(instance, "mD3DFormat"))
    if write then
        local ddsTexSize = MetaGetBufferSize(bufferInst) -- write buffer size

        local dds = {} -- TODO
        TTE_Assert(false, "TODO!")

        MetaStreamWriteInt(stream, ddsTexSize + MetaStreamGetDDSHeaderSize(dds))
        MetaStreamWriteDDS(stream, dds)
        MetaStreamWriteBuffer(stream, bufferInst) -- write buffer
    else
        local ddsTexSize = MetaStreamReadInt(stream) -- read buffer size
        local headerInfo = MetaStreamReadDDS(stream) -- as far as we know, bone has nothing from this DDS header that isn't inferred from d3dtx header
        TTE_Assert(headerInfo["Format CC"] ~= "DX10", "DDS in DX9 contains DX10 information") -- should not, we are in DX9
        MetaStreamReadBuffer(stream, bufferInst, ddsTexSize - MetaStreamGetDDSHeaderSize(headerInfo))
    end
    return true
end