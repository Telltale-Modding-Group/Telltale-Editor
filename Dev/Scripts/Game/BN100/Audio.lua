-- Voice and audio

function SerialiseVoxBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end

    local bEncrypted = not write
    local encMember = MetaGetMember(instance, "mbEncrypted")
    if bEncrypted and encMember then
        bEncrypted = MetaGetClassValue(encMember)
    end

    local data = MetaGetMember(instance, "_mPacketData")
    local dataSize = MetaGetClassValue(MetaGetMember(instance, "mAllPacketsSize"))

    if write then
        TTE_Assert(dataSize == MetaGetBufferSize(data), "Voice data not correctly updated. Buffer needs updating")
        MetaStreamWriteBuffer(stream, data)
    else
        MetaStreamReadBuffer(stream, data, dataSize)
    end

    if bEncrypted then TTE_Log("Is encrypted " .. MetaStreamGetFileName(stream)) end

    return true
end

function SerialiseAudBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end

    if write then

        local nChannels = MetaGetClassValue(MetaGetMember(instance, "_mNumChannels"))
        local sampleSize = MetaGetClassValue(MetaGetMember(instance, "_mSampleSizeBytes"))
        local bytesPerSecond = MetaGetClassValue(MetaGetMember(instance, "mBytesPerSecond"))

        MetaStreamWriteShort(stream, nChannels)
        MetaStreamWriteShort(stream, (sampleSize * 8) / nChannels)
        MetaStreamWriteInt(stream, (bytesPerSecond / nChannels) / sampleSize) -- sample rate
        MetaStreamWriteInt(stream, bytesPerSecond)
        MetaStreamWriteShort(stream, sampleSize)
        MetaStreamWriteShort(stream, 1)
        MetaStreamWriteInt(stream, 328160)
        MetaStreamWriteInt(stream, bytesPerSecond)

        MetaStreamWriteBuffer(stream, MetaGetMember(instance, "_mVorbisAudio"))

    else

        local nChannels = MetaStreamReadShort(stream)
        local bitDepth = MetaStreamReadShort(stream)
        local sampleRate  = MetaStreamReadInt(stream)
        local bytesPerSecond = MetaStreamReadInt(stream)
        local sampleSizeBytes = MetaStreamReadShort(stream) -- size of one sample
        local always1 = MetaStreamReadShort(stream)
        local always328160 = MetaStreamReadInt(stream)
        local bytesPerSecondCopy = MetaStreamReadInt(stream)

        TTE_Assert(always1 == 1 and always328160 == 328160 and bytesPerSecond == bytesPerSecondCopy, "AudioData needs checking! Send this file to us")

        MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mVorbisAudio"), -1) -- read remaining

        MetaSetClassValue(MetaGetMember(instance, "_mNumChannels"), nChannels)
        MetaSetClassValue(MetaGetMember(instance, "_mSampleSizeBytes"), sampleSizeBytes) -- rest can be determined by bytes per second and these two

    end

    return true
end