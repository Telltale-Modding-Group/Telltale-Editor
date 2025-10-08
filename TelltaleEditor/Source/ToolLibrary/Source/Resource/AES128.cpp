#include <Resource/AES128.hpp>

// SIMPLE AES128 IMPL. NOTE THAT NO SIMD IS USED BECAUSE OF PLATFORM DEPENDENCE. MAYBE A FORK COULD USE _MM_ FOR X64 / ARM intrin
// Note COLUMN MAJOR IS USED HERE FOR THE STATE MATRIX

// Inverse S-box
static const U8 _SBox_Inverse[256] =
{
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38,
    0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
    0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D,
    0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2,
    0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA,
    0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A,
    0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
    0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA,
    0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,
    0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
    0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20,
    0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31,
    0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
    0x60, 0x51, 0x7F, 0
};

static const U8 _SBox[256] =
{
  0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
  0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
  0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
  0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
  0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
  0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
  0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
  0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
  0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
  0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
  0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
  0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
  0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
  0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
  0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
  0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
  0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
  0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
  0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
  0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
  0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
  0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
  0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
  0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
  0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
  0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
  0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
  0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
  0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
  0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
  0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
  0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};


static const U8 _RCON[11] = { // ROUND CONSTANT
  0x00, // not used
  0x01, 0x02, 0x04, 0x08,
  0x10, 0x20, 0x40, 0x80,
  0x1B, 0x36
};

static inline void _Rotate32(U8 *word)
{
    U8 tmp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = tmp;
}

static inline void _Sub32(U8 *state)
{
    for (U32 i = 0; i < 4; i++)
    {
        state[i] = _SBox[state[i]];
    }
}

static inline void _Sub(U8 *state)
{
    for (U32 i = 0; i < 16; i++)
    {
        state[i] = _SBox[state[i]];
    }
}

static inline void _InverseSub(U8 *state)
{
    for (U32 i = 0; i < 16; i++)
    {
        state[i] = _SBox_Inverse[state[i]];
    }
}

static void _KeyExpand(const U8 *key, U8 *roundKeys)
{
    I32 i;
    U8 temp[4]{};

    // 1: normal
    for (i = 0; i < 16; i++)
    {
        roundKeys[i] = key[i];
    }

    I32 bytesGenerated = 16;
    I32 rconIndex = 1;

    while (bytesGenerated < 16*11)
    {
        for (i = 0; i < 4; i++) 
        {
            temp[i] = roundKeys[bytesGenerated - 4 + i];
        }

        // every 16 apply key sched
        if (bytesGenerated % 16 == 0)
        {
            _Rotate32(temp);
            _Sub32(temp);
            temp[0] ^= _RCON[rconIndex++];
        }

        // XOR
        for (i = 0; i < 4; i++)
        {
            roundKeys[bytesGenerated] = roundKeys[bytesGenerated - 16] ^ temp[i];
            bytesGenerated++;
        }
    }
}

static inline void _AddRoundKey(U8 *state, const U8 *roundKey) 
{
    for (U32 i = 0; i < 16; i++)
    {
        state[i] ^= roundKey[i];
    }
}

// state index
#define ST_IND(Row, Col) state[Row + (Col*4)]

static inline void _InverseShiftRows(U8 *state)
{
    U8 temp{};

    // row 1: rot right
    temp = ST_IND(1, 3);
    ST_IND(1, 3) = ST_IND(1, 2);
    ST_IND(1, 2) = ST_IND(1, 1);
    ST_IND(1, 1) = ST_IND(1, 0);
    ST_IND(1, 0) = temp;

    // row 2: rot right by 2
    temp = ST_IND(2, 0);
    ST_IND(2, 0) = ST_IND(2, 2);
    ST_IND(2, 2) = temp;
    temp = ST_IND(2, 1);
    ST_IND(2, 1) = ST_IND(2, 3);
    ST_IND(2, 3) = temp;

    // row 3: rot right by 3 / left by 1
    temp = ST_IND(3, 0);
    ST_IND(3, 0) = ST_IND(3, 1);
    ST_IND(3, 1) = ST_IND(3, 2);
    ST_IND(3, 2) = ST_IND(3, 3);
    ST_IND(3, 3) = temp;
}

// opposite of inverse.
static inline void _ShiftRows(U8* state)
{
    U8 temp{};
    
    // row 1: rot left by 1
    temp = ST_IND(1, 0);
    ST_IND(1, 0) = ST_IND(1, 1);
    ST_IND(1, 1) = ST_IND(1, 2);
    ST_IND(1, 2) = ST_IND(1, 3);
    ST_IND(1, 3) = temp;
    
    // row 2: rot left by 2 / right by 2
    temp = ST_IND(2, 0);
    ST_IND(2, 0) = ST_IND(2, 2);
    ST_IND(2, 2) = temp;
    temp = ST_IND(2, 1);
    ST_IND(2, 1) = ST_IND(2, 3);
    ST_IND(2, 3) = temp;
    
    // row 3: rot left by 3, right by 1
    temp = ST_IND(3, 3);
    ST_IND(3, 3) = ST_IND(3, 2);
    ST_IND(3, 2) = ST_IND(3, 1);
    ST_IND(3, 1) = ST_IND(3, 0);
    ST_IND(3, 0) = temp;
}

// GF(2^8) SPACE OPERATION

static inline U8 _GF_MulArb(U8 a, U8 b)
{
    U8 result = 0;
    while (b > 0) 
    {
        result ^= ((b & 1) * a);
        U8 carry = (a & 0x80) != 0;
        a <<= 1;
        a ^= (carry * 0x1B); // reduce
        b >>= 1;
    }
    return result;
}

static inline U8 _GF_Mul2(U8 byte)
{
    if (byte & 0x80) 
    {
        // reduce
        return (byte << 1) ^ 0x1B;
    }
    else
    {
        return byte << 1;
    }
}

static inline U8 _GF_Mul3(U8 byte)
{
    return _GF_Mul2(byte) ^ byte; // remember in GF(2^8): x + y = x ^ y because in mod 2 space theres no xor carry
}

static void _MixCol(U8 *col)
{
    U8 s0 = col[0];
    U8 s1 = col[1];
    U8 s2 = col[2];
    U8 s3 = col[3];

    // AES GF(2^8) matrix
    col[0] = _GF_Mul2(s0) ^ _GF_Mul3(s1) ^ s2 ^ s3;
    col[1] = s0 ^ _GF_Mul2(s1) ^ _GF_Mul3(s2) ^ s3;
    col[2] = s0 ^ s1 ^ _GF_Mul2(s2) ^ _GF_Mul3(s3);
    col[3] = _GF_Mul3(s0) ^ s1 ^ s2 ^ _GF_Mul2(s3);
}

static inline void _MixState(U8 *state)
{
    for (I32 i = 0; i < 4; i++)
    {
        _MixCol(&state[i << 2]);
    }
}

static void _InvMixCol(U8* col)
{
    U8 a[4]{}; // inverse AES GF(2^8) matrix
    a[0] = _GF_MulArb(col[0], 0x0e) ^ _GF_MulArb(col[1], 0x0b) ^ _GF_MulArb(col[2], 0x0d) ^ _GF_MulArb(col[3], 0x09);
    a[1] = _GF_MulArb(col[0], 0x09) ^ _GF_MulArb(col[1], 0x0e) ^ _GF_MulArb(col[2], 0x0b) ^ _GF_MulArb(col[3], 0x0d);
    a[2] = _GF_MulArb(col[0], 0x0d) ^ _GF_MulArb(col[1], 0x09) ^ _GF_MulArb(col[2], 0x0e) ^ _GF_MulArb(col[3], 0x0b);
    a[3] = _GF_MulArb(col[0], 0x0b) ^ _GF_MulArb(col[1], 0x0d) ^ _GF_MulArb(col[2], 0x09) ^ _GF_MulArb(col[3], 0x0e);

    for (I32 i = 0; i < 4; i++)
        col[i] = a[i];
}

static inline void _InvMixState(U8 *state)
{
    for (I32 i = 0; i < 4; i++)
    {
        _InvMixCol(&state[i << 2]);
    }
}

static void _EncryptBlock(const U8* rkey, const void* datain, void* dataout)
{
    U8 state[16]{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    memcpy(state, datain, 16);
    _AddRoundKey(state, rkey + 0);
    for(U32 i = 1; i <= 9; i++)
    {
        _Sub(state);
        _ShiftRows(state);
        _MixState(state);
        _AddRoundKey(state, rkey + (i << 4));
    }
    _Sub(state);
    _ShiftRows(state);
    _AddRoundKey(state, rkey + 160);
    memcpy(dataout, state, 16);
}

void AES128::Encrypt(void *pData, U32 nData, const void *key)
{
    if(pData && nData && key)
    {
        U8 rkey[16 * 11] {};
        _KeyExpand((const U8*)key, rkey);
        U32 nBlocks = nData >> 4;
        for(U32 block = 0; block < nBlocks; block++)
        {
            _EncryptBlock(rkey, (U8*)pData + (block << 4), (U8*)pData + (block << 4));
        }
    }
    else
    {
        TTE_LOG("Input arguments invalid");
    }
}

void AES128::Decrypt(void *pData, U32 nData, const void *key)
{
    if(pData && nData && key)
    {
        U8 rkey[16 * 11] {};
        _KeyExpand((const U8*)key, rkey);
        U32 nBlocks = nData >> 4;
        for(U32 block = 0; block < nBlocks; block++)
        {
            U8 state[16]{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            memcpy(state, (const U8*)pData + (block << 4), 16);
            _AddRoundKey(state, rkey + 0);
            for(U32 i = 1; i <= 9; i++)
            {
                _InverseShiftRows(state);
                _InverseSub(state);
                _AddRoundKey(state, rkey + (i << 4));
                _InvMixState(state);
            }
            _InverseShiftRows(state);
            _InverseSub(state);
            _AddRoundKey(state, rkey + 160);
            memcpy((U8*)pData + (block << 4), state, 16);
        }
    }
    else
    {
        TTE_LOG("Input arguments invalid");
    }
}

void AES128::CryptCTR(void *pData, U32 nData, const void *key, const void *iv, U32 dataOffset)
{
    if(pData && nData && key && iv)
    {
        U8 rkey[176] {};
        U8 counter[16]{};
        U8 keystream[16]{};
        
        _KeyExpand((const U8*)key, rkey);
        memcpy(counter, iv, 16);
        
        // add data offset block to counter (add in big endian, nothing special)
        dataOffset >>= 4; // turn into block num
        for(I32 i = 15; i >= 8; i--)
        {
            U16 sum = counter[i] + (dataOffset & 0xFF);
            counter[i] = sum & 0xFF;
            dataOffset = (dataOffset >> 8) + (sum >> 8); // add sum (its basically a 9 bit int, closest up is 16)
        }
        
        U32 nBlocks = nData >> 4;
        for(U32 block = 0; block < nBlocks; block++)
        {
            // AES NORMAL
            _EncryptBlock(rkey, counter, keystream);
            
            // APPLY TO DATA
            for(U32 i = 0; i < 16; i++)
                ((U8*)pData)[(block << 4) + i] ^= keystream[i];
            
            // INCREMENT BE 16
            for(I32 i = 15; i >= 0; i--)
            {
                if(++counter[i])
                    break;
            }
            
        }
        
        // REM
        if(nData & 0xF)
        {
            _EncryptBlock(rkey, counter, keystream);
            for(U32 i = 0; i < (nData & 0xF); i++)
            {
                ((U8*)pData)[(nBlocks << 4) + i] ^= keystream[i];
            }
        }
    }
    else
    {
        TTE_LOG("Input arguments invalid");
    }
}
