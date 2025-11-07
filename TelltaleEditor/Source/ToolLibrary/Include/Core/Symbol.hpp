#pragma once

#include <Core/Config.hpp>
#include <vector>
#include <map>
#include <mutex>

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
    Symbol(CString s, Bool bRegisterSymbolMap = false, CString constantName = 0);
    
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

// Argument must be 16 characters long and a hex string, in big endian (ie normal reading format). Strict if it HAS to be a hex string. if not a hex string it returns normal symbol.
Symbol SymbolFromHexString(const String& str, Bool bStrict = false);

// Converts to a 16 byte hex string
String SymbolToHexString(Symbol sym);

inline bool CompareCaseInsensitive(const String &lhs, const String &rhs) { return Symbol(lhs) == Symbol(rhs); }

extern const U64 CRC64_Table[256];
extern const U32 CRC32_Table[256];

// .SYMTAB FILES. SymbolTable keeps a memory of the strings that symbols represent. Serialised format is just text file of strings one each line.
// If the line starts with 'CRC32:' then the CRC32 is registered instead (used for some other stuff)
class SymbolTable {
    
    static SymbolTable* _ActiveTables;
    
public:
    
    inline static String Find(Symbol sym) // global find across all tables
    {
        for(SymbolTable* table = _ActiveTables; table; table = table->_Next)
        {
            String result = table->FindLocal(sym);
            if(result.length())
                return std::move(result);
        }
        return ""; // not found
    }
    
    // Same as normal find, else 'Symbol<xxx>' with hash
    inline static String FindOrHashString(Symbol sym)
    {
        String val = Find(sym);
        if(val.length() == 0)
            val = SymbolToHexString(sym);
        return val;
    }

    inline static Bool IsHashString(const String& value)
    {
        return value.length() > 2 && StringStartsWith(value, "<") && StringEndsWith(value, ">");
    }
    
    // DO NOT CREATE NON PRIVATE INSTANCES OF THESE UNLESS THEY ARE GLOBALS AND ONLY GET DESTROYED AT PROGRAM END!
    SymbolTable(Bool bPrivate = false);
    ~SymbolTable() = default;
    
    void Register(const String&); // Registers a symbol to the table
    
    void Clear(); // Clears the symbol table. Strongly advised not to do this, as old game files may not write back correctly.
    
    void SerialiseOut(DataStreamRef&); // Writes all the symbols to the given data stream, such that reading them back in produces the same table.
    
    void SerialiseIn(DataStreamRef&); // Reads in the serialised version. Adds to whats currently in here, does not clear.
    
    String FindLocal(Symbol); // Finds a symbol

private:
    
    SymbolTable* _Next = nullptr;
    std::mutex _Lock{};
    std::vector<String> _Table{};
    std::map<U64, U32> _SortedHashed{}; // hash => index into _Table
    
};

inline SymbolTable& GetRuntimeSymbols() // runtime loaded symbols
{
    static SymbolTable symbols{};
    return symbols;
}

inline SymbolTable& GetGameSymbols() // game-specific runtime symbols
{
    static SymbolTable symbols{};
    return symbols;
}