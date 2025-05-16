-- Special one off case of CSI3 PS2

function SerialiseCSI3ArchiveFolder(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if write then
        return false
    end
    numFiles = MetaStreamReadShort(stream)
    MetaSetClassValue(MetaGetMember(instance, "_mNumFiles"), numFiles)
    buffer = MetaGetMember(instance, "_mFileInfoBuf")
    MetaStreamReadBuffer(stream, buffer, 12 * numFiles) -- name CRC32 hash, offset, size. name is a u32 simple hash.
    return true
end

function SerialiseCSI3Archive(stream, instance, write)
    if not MetaSerialiseDefault(stream, instance, write) then
        return false
    end
    if write then
        MetaStreamWriteCached(stream, MetaGetMember(instance, "_mCachedPayload"))
    else
        MetaStreamReadCache(stream, MetaGetMember(instance, "_mCachedPayload"), -1)
    end
    return true
end

function RegisterCSI3TTArchive(MetaCI)

    mapArchiveFolders = RegisterCSI3Collection(MetaCI, "HashMap<String,TTArchiveFolder>", 0, kMetaString, "TTArchiveFolder")

    -- NOT USED ANYMORE. USE BUFFERS ARE FASTER AND NO LUA ACCESS NEEDED ANYWAY (REDUNDANT TYPE BELOW, ALTHOUGH I THINK IT MAY EXIST)
    local archiveEntry = { VersionIndex = 0 } -- guessed, just here so we can use it as a container
    archiveEntry.Name = "TTArchiveFileTemplate"
    archiveEntry.Flags = kMetaClassIntrinsic
    archiveEntry.Members = {}
    archiveEntry.Members[1] = { Name = "mName", Class = kMetaUnsignedInt } -- yes, they don't store file names :( but folder names they do!?
    archiveEntry.Members[2] = { Name = "mOffset", Class = kMetaUnsignedInt } -- offset after default serialise finishes in root archive
    archiveEntry.Members[3] = { Name = "mSize", Class = kMetaUnsignedInt }
    MetaRegisterClass(archiveEntry) 

    local archiveFolder = { VersionIndex = 0 }
    archiveFolder.Serialiser = "SerialiseCSI3ArchiveFolder"
    archiveFolder.Name = "TTArchiveFolder"
    archiveFolder.Members = {}
    archiveFolder.Members[1] = { Name = "mName", Class = kMetaString }
    archiveFolder.Members[2] = { Name = "mFolders", Class = mapArchiveFolders }
    archiveFolder.Members[3] = { Name = "_mFileInfoBuf", Class = kMetaClassInternalBinaryBuffer, Flags = kMetaMemberSerialiseDisable + kMetaMemberVersionDisable }    
    archiveFolder.Members[4] = { Name = "_mNumFiles", Class = kMetaInt, Flags = kMetaMemberSerialiseDisable + kMetaMemberVersionDisable }
    MetaRegisterClass(archiveFolder)

    local archive = { VersionIndex = 0 }
    archive.Name = "TTArchive"
    archive.Serialiser = "SerialiseCSI3Archive"
    archive.VersionOverride = "E07F2132" -- Just CANNOT get this version CRC to match. i have the correct members, im 100% sure. MAYBE having serialiser effects?
    archive.Members = {}
    archive.Members[1] = { Name = "Baseclass_TTArchiveFolder", Class = archiveFolder }
    archive.Members[2] = { Name = "mDataSize", Class = kMetaUnsignedInt }
    archive.Members[3] = { Name = "_mCachedPayload", Class = kMetaClassInternalDataStreamCache, Flags = kMetaMemberSerialiseDisable + kMetaMemberVersionDisable }
    MetaRegisterClass(archive)

end