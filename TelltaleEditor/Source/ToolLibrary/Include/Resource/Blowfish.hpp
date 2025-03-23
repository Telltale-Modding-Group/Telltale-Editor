#pragma once

#include <Core/Config.hpp>

// Singleton blowfish to manage encryption and decryption key (for each game). managed by main thread.
class Blowfish
{
public:
    
    static void Initialise(Bool modified, const U8* key, U32 keyLength); // determined by scripts. keylength max is 56
    
    static void Shutdown();
    
    inline static Blowfish* GetInstance()
    {
        return Instance;
    }
    
    // Encrypt the given buffer. THREAD SAFE between game Switches (job scheduler is reset)
    void Encrypt(U8* Buffer, U32 BufferLength);
    
    // Decrypt the given buffer. THREAD SAFE between game Switches (job scheduler is reset)
    void Decrypt(U8* Buffer, U32 BufferLength);
    
    // Instance
    struct Cipher
    {
        
        U32 S[4][256];
        U32 P[18];
        
        void Init(U8* Key, U32 KeyLength, Bool Modified); // init
        
        void Crypt(Bool IsEncrypt, Bool modified, U8* Buffer, U32 BufferLength); // performs the (en/de)crypt
        
        void _Do(Bool enc, Bool modified, U32& lhs, U32& rhs); // enc=true encrypt, else decrypt. crypts a block.
        
        U32 _DoRounds(U32);
        
    };
    
private:
    
    Bool _Modified; // use modified version of the encryption. endian swap on index 118 and some other changes. (newer games)
    U32 _KeyLength; // length in bytes of encryption key
    U8 _Key[56];
    
    Blowfish(Bool modified, const U8* key, U32 keylen);
    
    static Blowfish* Instance;
    
};
