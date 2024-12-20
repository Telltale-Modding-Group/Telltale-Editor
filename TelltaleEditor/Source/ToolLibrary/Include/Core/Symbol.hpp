#pragma once

#include <Core/Config.hpp>

// Perform ECMA-182 Poly CRC64
U64 CRC64(const U8* Buffer, U32 BufferLength, U64 InitialCRC64 = 0);

// Perform ECMA-182 Poly CRC64 on the lower case version of the buffer
U64 CRC64LowerCase(const U8* Buffer, U32 BufferLength, U64 InitialCRC64 = 0);

// Perform CRC32
U32 CRC32(const U8* Buffer, U32 BufferLength, U32 InitialCRC32 = 0);

// A Symbol is a hash locator for a file name. The lower case hash is always used.
class Symbol {
public:
    
    // Default
    Symbol() = default;
    
    // Constructs and hashes the string as the value.
    inline Symbol(const String& value) {
        _Hash = CRC64LowerCase((const U8*)value.c_str(), (U32)value.length());
    }
    
    // Constructs from a hash
    inline Symbol(U64 hash) : _Hash(hash) {}
    
    // Appends a string to this symbol, as though the whole string of this symbol and the argument was hashed in one call.
    inline Symbol& operator+=(const String& value) {
        _Hash = CRC64LowerCase((const U8*)value.c_str(), (U32)value.length(), _Hash);
        return *this;
    }
    
    // Gets the internal CRC64 Hash.
    inline U64 GetCRC64() const {
        return _Hash;
    }
    
    // Comparison operator for use in containers and sorting
    inline const Bool operator<(const Symbol& rhs) const {
        return _Hash < rhs._Hash;
    }
    
    // Comparison operator for equality
    inline const Bool operator==(const Symbol& rhs) const {
        return _Hash == rhs._Hash;
    }
    
    // Comparison operator for inequality
    inline const Bool operator!=(const Symbol& rhs) const {
        return _Hash != rhs._Hash;
    }
    
    // Implicit converter to U64: U64 hash = <symbol_variable>;
    inline operator U64() const {
        return _Hash;
    }
    
private:
    
    U64 _Hash; // Internal hash
    
};

// Global operator: symbol + string = symbol
inline Symbol operator+(const Symbol& sym, const String& value){
    return CRC64LowerCase((const U8*)value.c_str(), (U32)value.length(), sym.GetCRC64());
}

inline bool CompareCaseInsensitive(const String& lhs, const String& rhs){
    return Symbol(lhs) == Symbol(rhs);
}

extern const U64 CRC64_Table[256];
extern const U32 CRC32_Table[256];
