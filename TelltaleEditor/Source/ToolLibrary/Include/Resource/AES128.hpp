#pragma once

#include <Core/Config.hpp>

// simple TTE AES128 implementation
namespace AES128
{

    // streaming crypt. pass in the init vector (16 bytes) as well as the data offset pData is in the total encrypted block.
    void CryptCTR(void* pData, U32 nData, const void* key, const void* iv, U32 dataOffset);

    void Encrypt(void* pData, U32 nData, const void* key);

    void Decrypt(void* pData, U32 nData, const void* key);

}
