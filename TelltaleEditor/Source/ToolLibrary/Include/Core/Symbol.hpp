#pragma once

#include <Core/Config.hpp>
#include <vector>
#include <map>

// Perform ECMA-182 Poly CRC64
U64 CRC64(const U8 *Buffer, U32 BufferLength, U64 InitialCRC64 = 0);

// Perform ECMA-182 Poly CRC64 on the lower case version of the buffer. Stores in the global runtime symbol table.
U64 CRC64LowerCase(const U8 *Buffer, U32 BufferLength, U64 InitialCRC64 = 0);

// Perform CRC32
U32 CRC32(const U8 *Buffer, U32 BufferLength, U32 InitialCRC32 = 0);

// A Symbol is a hash locator for a file name. The lower case hash is always used.
class Symbol
{
  public:
    // Default
    Symbol() = default;

    // Constructs and hashes the string as the value.
    inline Symbol(const String &value) { _Hash = CRC64LowerCase((const U8 *)value.c_str(), (U32)value.length()); }
    
    // Constructs with C string
    inline Symbol(CString s) : Symbol(String(s)) {}

    // Constructs from a hash
    inline Symbol(U64 hash) : _Hash(hash) {}

    // Appends a string to this symbol, as though the whole string of this symbol and the argument was hashed in one call.
    inline Symbol &operator+=(const String &value)
    {
        _Hash = CRC64LowerCase((const U8 *)value.c_str(), (U32)value.length(), _Hash);
        return *this;
    }

    // Gets the internal CRC64 Hash.
    inline U64 GetCRC64() const { return _Hash; }

    // Comparison operator for use in containers and sorting
    inline const Bool operator<(const Symbol &rhs) const { return _Hash < rhs._Hash; }

    // Comparison operator for equality
    inline const Bool operator==(const Symbol &rhs) const { return _Hash == rhs._Hash; }

    // Comparison operator for inequality
    inline const Bool operator!=(const Symbol &rhs) const { return _Hash != rhs._Hash; }

    // Implicit converter to U64: U64 hash = <symbol_variable>;
    inline operator U64() const { return _Hash; }

  private:
    U64 _Hash; // Internal hash
};

// Global operator: symbol + string = symbol
inline Symbol operator+(const Symbol &sym, const String &value)
{
    return CRC64LowerCase((const U8 *)value.c_str(), (U32)value.length(), sym.GetCRC64());
}

inline bool CompareCaseInsensitive(const String &lhs, const String &rhs) { return Symbol(lhs) == Symbol(rhs); }

extern const U64 CRC64_Table[256];
extern const U32 CRC32_Table[256];

// .SYMTAB FILES. SymbolTable keeps a memory of the strings that symbols represent. Serialised format is just text file of strings one each line.
class SymbolTable {
public:
    
    SymbolTable() = default;
    ~SymbolTable() = default;
    
    String Find(Symbol); // Finds a symbol
    
    void Register(const String&); // Registers a symbol to the table
    
    void Clear(); // Clears the symbol table. Strongly advised not to do this, as old game files may not write back correctly.
    
    void SerialiseOut(DataStreamRef&); // Writes all the symbols to the given data stream, such that reading them back in produces the same table.
    
    void SerialiseIn(DataStreamRef&); // Reads in the serialised version. Adds to whats currently in here, does not clear.
    
private:
    
    std::vector<String> _Table{};
    std::map<U64, U32> _SortedHashed{}; // hash => index into _Table
    
};

extern SymbolTable RuntimeSymbols; // runtime loaded symbols
