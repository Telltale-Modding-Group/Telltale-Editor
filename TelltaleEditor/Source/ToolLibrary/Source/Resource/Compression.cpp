#include <Resource/Compression.hpp>

#include <zlib/zlib.h>

namespace Compression
{
    
    OodleLZ_Compress OodleCompressFn = nullptr;
    OodleLZ_Decompress OodleDecompressFn = nullptr;
    
    U64 Compress(U8* src, U64 srcSize, U8* dst, U64 dstSize, Type type)
    {
        if (type == ZLIB)
        {
            z_stream strm{};
            U32 ret;
            
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            
            ret = deflateInit2_(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, -15, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY, "1.2.8", sizeof(strm));
            if (ret != Z_OK)
                return 0;
            
            strm.avail_in = (uInt)srcSize;
            strm.next_in = src;
            strm.avail_out = (uInt)dstSize;
            strm.next_out = dst;
            
            ret = deflate(&strm, Z_FINISH);
            if (ret != Z_STREAM_END)
            {
                // not enough output space likely.
                deflateEnd(&strm);
                return 0;
            }
            
            U64 compressedSize = dstSize - strm.avail_out;
            deflateEnd(&strm);
            
            return compressedSize;
        }
        else if (type == OODLE)
        {
            TTE_ASSERT(OodleCompressFn != nullptr, "Oodle compressor not found");
            
            U64 num = (U64)OodleCompressFn(6, (const void*)src, (U32)srcSize, (void*)dst, 7, 0, 0, 0);
            
            return num;
        }
        return 0;
    }
    
    Bool Decompress(U8* src, U64 srcSize, U8* dst, U64 dstSize, Type type)
    {
        if (type == ZLIB)
        {
            z_stream strm{};
            U32 ret;
            
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            
            ret = inflateInit2_(&strm, -15, "1.2.8", sizeof(strm));
            if (ret != Z_OK)
                return false;
            
            strm.avail_in = (uInt)srcSize;
            strm.next_in = src;
            strm.avail_out = (uInt)dstSize;
            strm.next_out = dst;
            
            ret = inflate(&strm, Z_FINISH);
            if (ret != Z_STREAM_END)
            {
                inflateEnd(&strm);
                return false;
            }
            
            inflateEnd(&strm);
            return true;
        }
        else if (type == OODLE)
        {
            TTE_ASSERT(OodleDecompressFn != nullptr, "Oodle decompressor not found");
            
            OodleDecompressFn((void*)src, (U32)srcSize, dst, (U32)dstSize, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 3);
            
            return true;
        }
        return false;
    }

    
}
