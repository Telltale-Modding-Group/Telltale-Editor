#pragma once

#include <Core/Config.hpp>

namespace Compression
{
 
    // compression type
    enum Type
    {
        ZLIB = 0,
        OODLE = 1,
        // END_LIBRARY = 2
    };
    
    typedef long long (*OodleLZ_Compress)(int algo, const void *pSrc, unsigned int srcLen, void* dst, long long max, void* a, void* b, void* c);

    typedef int (*OodleLZ_Decompress)(void* in, int insz, void* out, long long outsz, long long a, long long b, long long c, void* d, void* e, void* f, void* g, void* h, void* i, long long j);
    
    extern OodleLZ_Compress OodleCompressFn;
    extern OodleLZ_Decompress OodleDecompressFn;
    
    // initialise
    inline void Initialise()
    {
        if(OodleCompressFn == nullptr || OodleDecompressFn == nullptr)
        {
            // they are not set, set them
            void* c = nullptr; void* d = nullptr;
            
            OodleOpen(c, d); // platform open
            
            OodleCompressFn = (OodleLZ_Compress)c;
            OodleDecompressFn = (OodleLZ_Decompress)d;
        }
    }
    
    // Compresses by reading srcSize bytes from src and writing compressed bytes to dst. Returns number of written compressed bytes, or 0 if fail.
    U64 Compress(U8* src, U64 srcSize, U8* dst, U64 dstSize, Type type);
    
    // Decompresses by reading srcSize from src and writing decompressed bytes to dst. Returns if success. Uncompressed size must be known.
    Bool Decompress(U8* src, U64 srcSize, U8* dst, U64 dstSize, Type type);
    
}
