-- Voice and audio

function SerialiseVoxBone1(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then return false end

    bEncrypted = not write
    encMember = MetaGetMember(instance, "mbEncrypted")
    if bEncrypted and not encMember == nil then
        bEncrypted = MetaGetClassValue(encMember)
    end

    data = MetaGetMember(instance, "_mPacketData")
    dataSize = MetaGetClassValue(MetaGetMember(instance, "mAllPacketsSize"))

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

        nChannels = MetaGetClassValue(MetaGetMember(instance, "_mNumChannels"))
        sampleSize = MetaGetClassValue(MetaGetMember(instance, "_mSampleSizeBytes"))
        bytesPerSecond = MetaGetClassValue(MetaGetMember(instance, "mBytesPerSecond"))

        MetaStreamWriteShort(stream, MetaGetClassValue(MetaGetMember(instance, "_mNumChannels")))
        MetaStreamWriteShort(stream, (sampleSize * 8) / nChannels)
        MetaStreamWriteInt(stream, (bytesPerSecond / nChannels) / sampleSize) -- sample rate
        MetaStreamWriteInt(stream, bytesPerSecond)
        MetaStreamWriteShort(stream, sampleSize)
        MetaStreamWriteShort(stream, 1)
        MetaStreamWriteInt(stream, 328160)
        MetaStreamWriteInt(stream, bytesPerSecond)

        MetaStreamWriteBuffer(stream, MetaGetMember(instance, "_mVorbisAudio"))

    else

        nChannels = MetaStreamReadShort(stream)
        bitDepth = MetaStreamReadShort(stream)
        sampleRate  = MetaStreamReadInt(stream)
        bytesPerSecond = MetaStreamReadInt(stream)
        sampleSizeBytes = MetaStreamReadShort(stream) -- size of one sample
        always1 = MetaStreamReadShort(stream)
        always328160 = MetaStreamReadInt(stream)
        bytesPerSecondCopy = MetaStreamReadInt(stream)

        TTE_Assert(always1 == 1 and always328160 == 328160 and bytesPerSecond == bytesPerSecondCopy, "AudioData needs checking! Send this file to us")

        MetaStreamReadBuffer(stream, MetaGetMember(instance, "_mVorbisAudio"), -1) -- read remaining

        MetaSetClassValue(MetaGetMember(instance, "_mNumChannels"), nChannels)
        MetaSetClassValue(MetaGetMember(instance, "_mSampleSizeBytes"), sampleSizeBytes) -- rest can be determined by bytes per second and these two

    end

    return true
end