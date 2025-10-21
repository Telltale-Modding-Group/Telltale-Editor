#include <Resource/PSPKG.hpp>
#include <Resource/AES128.hpp>
#include <Core/Context.hpp>

std::vector<PlaystationPKG::PackageKey> PlaystationPKG::_PkgKeys{};

#define READ_XXX(var, z) in->Read((U8*)&var, z)
#define READ_INT32(var) in->Read((U8*)&var, 4); var = FLIP_ENDIAN32(var)
#define READ_INT64(var) in->Read((U8*)&var, 8); var = EndianSwap64(var)

const std::vector<CString> PlaystationPKG::GetAvailableKeys()
{
    std::vector<CString> keys{};
    keys.reserve(_PkgKeys.size());
    for(const auto& pk: _PkgKeys)
        keys.push_back(pk.Name.c_str());
    return keys;
}

void PlaystationPKG::_RegisterKeys(ToolContext* context)
{
    _PkgKeys.clear();
    LuaManager& man = context->GetLibraryLVM();
    
    String pskeysSrc = context->LoadLibraryStringResource("Scripts/PlaystationKeys.lua");
    if(pskeysSrc.length())
    {
        ScriptManager::RunText(man, pskeysSrc, "PlaystationKeys.lua", true);
    }
    else
    {
        TTE_LOG("WARNING: Could not load playstation keys lua. Will not be able to decrypt Playstation packages");
    }
    
    ScriptManager::GetGlobal(man, "RegisterPlaystationPKGKeys", true);
    LuaType t = man.Type(-1);
    if(t == LuaType::FUNCTION)
    {
        man.CallFunction(0, 1, true);
        if(man.Type(-1) == LuaType::TABLE)
        {
            ITERATE_TABLE(it, -1)
            {
                if(man.Type(it.KeyIndex()) == LuaType::STRING && man.Type(it.ValueIndex()) == LuaType::STRING)
                {
                    PackageKey key{};
                    String k = man.ToString(it.ValueIndex());
                    key.Name = man.ToString(it.KeyIndex());
                    if(k.length() == 32)
                    {
                        for(U32 i = 0; i < 16; i++)
                        {
                            char Lo = k[(i << 1) + 1];
                            char Hi = k[(i << 1) + 0];
                            key.Key[i] = Lo >= 'A' && Lo <= 'F' ? 10 + Lo - 'A' :
                                Lo >= 'a' && Lo <= 'f' ? 10 + Lo - 'a' : Lo >= '0' && Lo <= '9' ? Lo - '0' : 0;
                            key.Key[i] |= ((Hi >= 'A' && Hi <= 'F' ? 10 + Hi - 'A' :
                                Hi >= 'a' && Hi <= 'f' ? 10 + Hi - 'a' : Hi >= '0' && Hi <= '9' ? Hi - '0' : 0) << 4);
                        }
                        _PkgKeys.push_back(std::move(key));
                    }
                    else
                    {
                        TTE_LOG("WARNING: Package key %s is not 16 bytes hex format (32 chars)", key.Name.c_str());
                    }
                }
                // else quietly ignore
            }
        }
        else
        {
            TTE_LOG("ERROR: Playstation keys registrar doesn't return a table!");
        }
    }
    else
    {
        TTE_LOG("ERROR: Playstation keys registrar function not found!");
    }
    man.SetTop(0);
}

Bool PlaystationPKG::SerialiseIn(DataStreamRef &in, const String& kn)
{
    CString EncKey = nullptr;
    for(const auto& pk: _PkgKeys)
    {
        if(pk.Name.c_str() == kn.c_str())
        {
            EncKey = pk.Key;
            break;
        }
    }
    if(EncKey == nullptr)
    {
        TTE_LOG("ERROR: Cannot serialise playstation package. Encryption key not found or specified: '%s'", kn.c_str());
        return false;
    }
    
    U64 base = in->GetPosition();
    _Flags = 0;
    _ContentID = "";
    U8 Data[64]{};
    U32 datum{};
    
    READ_XXX(Data[0], 4);
    if(memcmp(Data, "\x7FPKG", 4))
    {
        TTE_LOG("Cannot serialise in PKG: invalid header magic. Is this a PS3 PKG distribution package?");
        return false;
    }
    
    READ_XXX(datum, 2);
    if(datum == 0x80)
    {
        _Flags += DISTRIBUTION_PKG;
    }
    else if(datum == 0)
    {
        _Flags += DEBUG_PKG;
    }
    else
    {
        // this might happen because user is on a BE machine.
        TTE_LOG("Cannot serialise in PKG: invalid revision. Is this file corrupt?");
        return false;
    }
    
    READ_XXX(datum, 2);
    if(datum == 0x100)
    {
        _Flags += TYPE_PS3;
    }
    else if(datum == 0x200)
    {
        _Flags += TYPE_PSP_VITA;
    }
    else
    {
        TTE_LOG("Cannot serialise in PKG: invalid type");
        return false;
    }
    
    U32 mdOffset{}; READ_INT32(mdOffset);
    U32 mdCount{}; READ_INT32(mdCount);
    U32 mdSize{}; READ_INT32(mdSize);
    
    U32 nFilesFolders{}; READ_INT32(nFilesFolders);
    U64 totalSize{}; READ_INT64(totalSize);
    U64 dataOffset{}; READ_INT64(dataOffset);
    U64 dataSize{}; READ_INT64(dataSize);
    
    READ_XXX(Data[0], 0x24);
    Data[0x24] = 0;
    _ContentID = (CString)Data;
    READ_XXX(Data[0], 0xC); // skip padding 0s
    
    READ_XXX(Data[0], 0x10); // SHA1 for debug bs
    
    U8 IV[0x10]{}; READ_XXX(IV[0], 0x10); // AES128 IV
    
    READ_XXX(Data[0], 0x40); // header digest hash
    
    if(_Flags.Test(TYPE_PSP_VITA))
    {
        READ_XXX(Data[0], 0x40); // extended header for psp/vita
    }
    
    // DATA
    _Entries.clear();
    in->SetPosition(dataOffset + base);
    U8* Temp = TTE_ALLOC(nFilesFolders * 0x20, MEMORY_TAG_TEMPORARY);
    READ_XXX(Temp[0], nFilesFolders * 0x20);
    AES128::CryptCTR(Temp, nFilesFolders * 0x20, EncKey, IV, 0);
    U8* Temp2 = nullptr;
    U32 TempBufSize = 0;
    
    // no aes round key cache but this doesn't need to be super fast
    for(U32 record = 0; record < nFilesFolders; record++)
    {
        U32 nameOff = COERCE(Temp + (record * 0x20) + 0, U32); nameOff = FLIP_ENDIAN32(nameOff);
        U32 nameSize = COERCE(Temp + (record * 0x20) + 4, U32); nameSize = FLIP_ENDIAN32(nameSize);
        U64 fileOff = COERCE(Temp + (record * 0x20) + 8, U64); fileOff = EndianSwap64(fileOff);
        U64 fileSize = COERCE(Temp + (record * 0x20) + 16, U64); fileSize = EndianSwap64(fileSize);
        U32 flags = COERCE(Temp + (record * 0x20) + 24, U32); flags = FLIP_ENDIAN32(flags);
        U32 pad = COERCE(Temp + (record * 0x20) + 28, U32);
        if((U64)nameOff >= dataSize || (U64)nameSize > 0x100 || fileOff >= dataSize || pad != 0)
        {
            TTE_LOG("ERROR: Corrupt data in encrypted Playstation PKG: most likely the encryption key (or CTR-IV) is wrong.");
            TTE_FREE(Temp);
            if(Temp2)
                TTE_FREE(Temp2);
            return false;
        }
        if((flags & 0xFFFF) > 4 || ((flags & 0xFFFF) == 0))
        {
            TTE_LOG("ERROR: Unknown record type in playstation package");
            TTE_FREE(Temp);
            TTE_FREE(Temp2);
            return false;
        }
        if((flags & 0xFFFF) == 4)
        {
            continue; // folder ignore it.
        }
        in->SetPosition(base + nameOff + dataOffset);
        if(!Temp2 || nameSize > TempBufSize)
        {
            TempBufSize = nameSize;
            if(Temp2)
                TTE_FREE(Temp2);
            Temp2 = TTE_ALLOC(TempBufSize, MEMORY_TAG_TEMPORARY);
        }
        READ_XXX(Temp2[0], nameSize);
        AES128::CryptCTR(Temp2, nameSize, EncKey, IV, nameOff);
        Entry entry{};
        entry.Name = String((CString)Temp2, nameSize);
        entry.Offset = fileOff;
        entry.Size = fileSize;
        entry.Type = (EntryType)(flags & 0xFFFF);
        _Entries.push_back(std::move(entry));
    }
    
    TTE_FREE(Temp);
    if(Temp2)
        TTE_FREE(Temp2);
    
    memcpy(_Key, EncKey, 16);
    memcpy(_IV, IV, 16);
    _InputBaseOffset = dataOffset;
    _Cached = in;
    return true;
}

DataStreamRef PlaystationPKG::Find(const Symbol &resourceName, String &outResource)
{
    for(const auto& e: _Entries)
    {
        String name = FileGetName(e.Name);
        if(resourceName == Symbol(name))
        {
            outResource = name;
            return DataStreamManager::GetInstance()->CreateAES128CTRDecryptingStream(
                        _Cached, e.Offset + _InputBaseOffset, _Key, _IV, (U32)e.Size, (U32)e.Offset);
        }
    }
    return {};
}

void PlaystationPKG::GetFiles(std::set<String> &result)
{
    for(const auto& e: _Entries)
    {
        result.insert(e.Name);
    }
}

String PlaystationPKG::GetContentID() const
{
    return _ContentID;
}
