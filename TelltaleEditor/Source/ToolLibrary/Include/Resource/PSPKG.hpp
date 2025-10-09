#pragma once

#include <Core/BitSet.hpp>
#include <Resource/DataStream.hpp>

#include <vector>
#include <set>

class LuaManager;
class ToolContext;

class PlaystationPKG
{
public:
    
    static constexpr CString Extension = ".pkg";
    
    struct PackageKey
    {
        String Name;
        char Key[16];
    };
    
    Bool SerialiseIn(DataStreamRef& in, const String& key);
    
    void GetFiles(std::set<String>& result); // get file names
    
    DataStreamRef Find(const Symbol& resourceName, String& outResource);
    
    String GetContentID() const;
    
    static const std::vector<CString> GetAvailableKeys();
    
    inline void Reset()
    {
        // reset to empty
        _Entries.clear();
        _ContentID = "";
        _Cached = {};
        _InputBaseOffset = 0;
    }
    
private:
    
    friend class ToolContext;
    friend class ResourceRegistry;
    
    enum Flag
    {
        DEBUG_PKG = 1,
        DISTRIBUTION_PKG = 2,
        TYPE_PS3 = 4,
        TYPE_PSP_VITA = 8,
    };
    
    enum class EntryType
    {
        REGULAR_FILE = 3,
        // FOLDER = 4,
        EXECUTABLE = 1,
        EDAT = 2,
    };
    
    struct Entry
    {
        String Name;
        U64 Size;
        U64 Offset;
        EntryType Type = EntryType::REGULAR_FILE;
    };
    
    String _ContentID;
    Flags _Flags;
    DataStreamRef _Cached;
    std::vector<Entry> _Entries;
    U8 _Key[16];
    U8 _IV[16];
    U64 _InputBaseOffset = 0;
    
    static void _RegisterKeys(ToolContext* context);
    static std::vector<PackageKey> _PkgKeys;
    
};
