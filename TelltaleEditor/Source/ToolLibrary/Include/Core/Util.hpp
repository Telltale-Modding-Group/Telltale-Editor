#pragma once

#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <cstdarg>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <cmath>
#include <atomic>
#include <type_traits>
#include <utility>

class ToolContext; // forward declaration. used a lot. see context.hpp
class DataStream; // See DataStream.hpp

// ===================================================================         TYPES
// ===================================================================

using U8 = uint8_t;
using I8 = int8_t;
using U16 = uint16_t;
using I16 = int16_t;
using U32 = uint32_t;
using I32 = int32_t;
using U64 = uint64_t;
using I64 = int64_t;

using Float = float;

using String = std::string;
using CString = const char *;

using Bool = bool;

// ================================================ THREAD UTIL ================================================

/**
 * @brief Sleeps the current thread for @p milliseconds time
 *
 * @param milliseconds duration in milliseconds
 */
void ThreadSleep(U64 milliseconds);

/**
 * @brief Sets the name of the current thread to @p tName
 *
 * @param tName name
 */
void SetThreadName(const String &tName);

// ================================================ MEMORY UTIL ================================================

template<typename T>
using Ptr = std::shared_ptr<T>; // Easier to write than std::shared_ptr.

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Useful alias for data stream pointer, which deallocates automagically when finished with.
using DataStreamRef = Ptr<DataStream>;

template <typename T> // checks if the weak ptr has no reference at all, even to an Ptr that has been reset.
inline bool IsWeakPtrUnbound(const WeakPtr<T>& weak)
{
    // Compare the weak pointer to a default-constructed weak pointer
    return !(weak.owner_before(WeakPtr<T>()) || WeakPtr<T>().owner_before(weak));
}

// helper to call object destructor
template<typename T>
inline void DestroyObject(T& val)
{
    val.~T();
}

class JobThread;

namespace Memory // All memory helpers
{
    
    // 1MB
#define FAST_BUFFER_SIZE 0x100000
    
    // TLS memory for temp fast alloc. Freed automatically at destructor
    class FastBufferAllocator
    {
    public:
        
        FastBufferAllocator();
        ~FastBufferAllocator();
        
        Bool IsInFastBuffer(const void* p);
        U8* Alloc(U64 size, U32 align);
        void Reset();
        
    private:
        
        U32 _InitialMarker = 0;
        U32 _IsMainThread = 0;
        JobThread* _Worker = 0;
        
    };
    
    void DumpTrackedMemory(); // if in debug mode, prints all tracked memory allocations.
    
    U8* _DebugAllocateTracked(U64 _Nbytes, U32 tag, CString filename, U32 number, CString objName);
    void _DebugDeallocateTracked(U8* Ptr);
    
    void Initialise();
    void Shutdown();
    
}


#ifdef DEBUG // use tracked memory

#define TTE_ALLOC(_Nbytes, _MemoryTag) Memory::_DebugAllocateTracked(_Nbytes, _MemoryTag, (CString) __FILE__, (U32) __LINE__, nullptr)

#define TTE_FREE(ptr) Memory::_DebugDeallocateTracked((U8*)ptr)

#define TTE_NEW(_Type, _MemoryTag, ...) new (Memory::_DebugAllocateTracked(sizeof(_Type), _MemoryTag, (CString)  \
__FILE__, (U32) __LINE__, #_Type)) _Type(__VA_ARGS__)

#define TTE_DEL(_Inst) { if(_Inst) { DestroyObject(*_Inst); TTE_FREE((U8*)_Inst); } }

#else

// Release. Dont need to track any allocations.

#define TTE_NEW(_Type, _MemoryTag, ...) new _Type(__VA_ARGS__)

#define TTE_DEL(_Inst) delete _Inst

// Should initialise to zero
#define TTE_ALLOC(_NBytes, _MemoryTag) new U8[_NBytes]()

#define TTE_FREE(_ByteArray) delete[] ((U8*)_ByteArray)

#endif

template<typename T> inline void _TTEDeleter(T* _Instance)
{
    TTE_DEL(_Instance);
}

inline void _TTEFree(U8* _Instance)
{
    TTE_FREE(_Instance);
}

// create managed shared ptr
#define TTE_NEW_PTR(_Type, _MemoryTag, ...) Ptr<_Type>(TTE_NEW(_Type, _MemoryTag, __VA_ARGS__), &_TTEDeleter<_Type>)

#define TTE_ALLOC_PTR(_NBytes, _MemoryTag) Ptr<U8>(TTE_ALLOC(_NBytes, _MemoryTag), &_TTEFree)

#define TTE_PROXY_PTR(_RawPtr, _WantedType) Ptr<_WantedType>(_RawPtr, &NullDeleter)


// ================================================ TIME UTIL ================================================

// Gets a current timestamp.
inline U64 GetTimeStamp()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// Gets the time difference in *seconds* between start and end.
inline Float GetTimeStampDifference(U64 start, U64 end)
{
    return static_cast<Float>(end - start) / 1'000'000.0f;
}

inline String GetFormatedTime(Float secs) 
{
    std::ostringstream stream;
    
    if (secs >= 1.0f)
        stream << std::fixed << std::setprecision(3) << secs << " s";
    else if (secs >= 0.001f)
        stream << std::fixed << std::setprecision(3) << secs * 1000.0f << " ms";
    else if (secs >= 0.000001f)
        stream << std::fixed << std::setprecision(3) << secs * 1000000.0f << " µs";
    else
        stream << std::fixed << std::setprecision(3) << secs * 1000000000.0f << " ns";
    
    return stream.str();
}

// Helper. If T must be default constructible use this. Get wil return nullptr if not.
template<typename T, Bool Value = std::is_default_constructible<T>::value>
class OptionalDefaultConstructible
{
    
    T _Value;
    
public:
    
    inline T* Get()
    {
        return &_Value;
    }
    
};

template<typename T>
class OptionalDefaultConstructible<T, false>
{
public:
    
    inline T* Get()
    {
        return nullptr;
    }
    
};

// ================================================ COLLECTION UTIL ================================================

template< typename T >
typename std::vector<T>::iterator VectorInsertSorted(std::vector<T> & vec, T&& item )
{
    return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), std::move(item));
}

// Helper class. std::priority_queue normally does not let us access by finding elements. Little hack to bypass and get internal vector container.
template <typename T> class hacked_priority_queue : public std::priority_queue<T>
{ // Not applying library convention, see this as an 'extension' to std::
public:
    std::vector<T> &get_container() { return this->c; }
    
    const std::vector<T> &get_container() const { return this->c; }
    
    auto get_cmp() { return this->comp; }
};

// ================================================ STRING UTIL ================================================

inline String StringTrim(const String& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, last - first + 1);
}

// Checks if a string starts with the other string prefix
inline Bool StringStartsWith(const String& str, const String& prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

// Gets the file name without path or extension
inline String FileGetName(const String& filename)
{
    size_t slashPos = filename.find_last_of("/\\");
    size_t dotPos = filename.find_last_of('.');
    
    size_t nameStart = (slashPos == String::npos) ? 0 : slashPos + 1;
    
    if (dotPos != String::npos && dotPos > nameStart)
    {
        return filename.substr(nameStart, dotPos - nameStart);
    }
    else
    {
        return filename.substr(nameStart);
    }
}

// Sets or replaces the file name (excluding extension and path)
inline String FileSetName(const String& filename, const String& newName)
{
    size_t slashPos = filename.find_last_of("/\\");
    size_t dotPos = filename.find_last_of('.');
    
    size_t nameStart = (slashPos == String::npos) ? 0 : slashPos + 1;
    bool hasExt = dotPos != String::npos && (dotPos > nameStart);
    
    String path = (slashPos == String::npos) ? "" : filename.substr(0, nameStart);
    String ext = hasExt ? filename.substr(dotPos) : "";
    
    return path + newName + ext;
}

// Sets or replaces the file extension of a string
inline String FileSetExtension(const String& filename, const String& newExt)
{
    size_t dotPos = filename.find_last_of('.');
    size_t slashPos = filename.find_last_of("/\\");
    
    if (dotPos != String::npos && (slashPos == String::npos || dotPos > slashPos))
    {
        return filename.substr(0, dotPos + 1) + newExt;
    }
    else
    {
        return filename + "." + newExt;
    }
}

// Gets the file extension from a string (without the dot)
inline String FileGetExtension(const String& filename)
{
    size_t dotPos = filename.find_last_of('.');
    size_t slashPos = filename.find_last_of("/\\");
    
    // Valid extension only if the dot is after the last slash
    if (dotPos != String::npos && (slashPos == String::npos || dotPos > slashPos))
    {
        return filename.substr(dotPos + 1);
    }
    return "";
}

// Gets the path portion of a filename (excluding the filename and trailing slash)
inline String FileGetPath(const String& filename)
{
    size_t slashPos = filename.find_last_of("/\\");
    
    if (slashPos != String::npos)
    {
        return filename.substr(0, slashPos);
    }
    return "";
}

inline String FileGetFilename(const String& filepath)
{
    size_t slashPos = filepath.find_last_of("/\\");
    return (slashPos == String::npos) ? filepath : filepath.substr(slashPos + 1);
}

inline String FileSetFilename(const String& filepath, const String& newFilename)
{
    size_t slashPos = filepath.find_last_of("/\\");
    String path = (slashPos == String::npos) ? "" : filepath.substr(0, slashPos + 1);
    return path + newFilename;
}

// Checks if a string ends with the other string suffix
inline Bool StringEndsWith(const String& str, const String& suffix, Bool bCaseSensitive = true)
{
    if (str.length() < suffix.length())
        return false;
    
    if (bCaseSensitive)
    {
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    else
    {
        size_t offset = str.length() - suffix.length();
        for (size_t i = 0; i < suffix.length(); ++i)
        {
            // Compare each character in a case-insensitive way
            if (std::tolower(static_cast<U8>(str[offset + i])) !=
                std::tolower(static_cast<U8>(suffix[i])))
            {
                return false;
            }
        }
        return true;
    }
}

inline void StringParseFunctionSignature(const String& line, std::string& outReturnType, std::vector<String>& outArgs)
{
    size_t openParen = line.find('(');
    size_t closeParen = line.find(')', openParen);
    
    if (openParen == std::string::npos || closeParen == std::string::npos || openParen == 0)
        return;
    
    size_t lastSpaceBeforeName = line.rfind(' ', openParen);
    if (lastSpaceBeforeName == std::string::npos)
        return;
    
    outReturnType = line.substr(0, lastSpaceBeforeName);
    outReturnType = StringTrim(outReturnType);
    
    String args = line.substr(openParen + 1, closeParen - openParen - 1);
    std::istringstream argStream(args);
    String arg;
    outArgs.clear();
    while (std::getline(argStream, arg, ','))
    {
        arg = StringTrim(arg);
        if (!arg.empty())
            outArgs.push_back(arg);
    }
}

// AI definitely did not write this function.
inline String StringWrapText(const String& input, const String& prefix, size_t maxWordsPerLine = 30, size_t maxCharsPerLine = 200)
{
    std::istringstream iss(input);
    std::ostringstream oss;
    
    String word;
    size_t wordCount = 0;
    size_t charCount = 0;
    
    while (iss >> word)
    {
        // If adding the next word would exceed limits, break line
        if ((wordCount >= maxWordsPerLine) || (charCount + word.length() > maxCharsPerLine))
        {
            oss << prefix;
            wordCount = 0;
            charCount = 0;
        }
        else if (wordCount > 0)
        {
            oss << ' '; // space before word (not before first word)
            ++charCount;
        }
        
        oss << word;
        ++wordCount;
        charCount += word.length();
    }
    
    return oss.str();
}

inline String ToLower(const String& input)
{
    String result = input;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

inline void StringReplace(String& str, const String& from, const String& to, Bool ignoreCase = false)
{
    if (from.empty()) return;
    
    String searchStr = str;
    String target = from;
    if (ignoreCase)
    {
        searchStr = ToLower(searchStr);
        target = ToLower(target);
    }
    
    size_t pos = 0;
    while ((pos = searchStr.find(target, pos)) != String::npos) {
        str.replace(pos, from.length(), to);
        searchStr.replace(pos, from.length(), to);
        pos += to.length();
    }
}

inline Bool StringContains(const String& str, const String& substr, Bool ignoreCase = false)
{
    if (substr.empty()) return false;

    String haystack = str;
    String needle = substr;
    if (ignoreCase)
    {
        haystack = ToLower(haystack);
        needle = ToLower(needle);
    }

    return haystack.find(needle) != String::npos;
}

inline String StringToSnake(const String& input)
{
    String result;
    result.reserve(input.size());
    for (char c : input)
    {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            result.push_back('_');
        }
        else
        {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }

    return result;
}


// Radius must be 2 to 36
String StringFromInteger(I64 original_value,U32 radix, Bool is_negative); // defined in Config.cpp

// Removes 'class ' 'struct ' 'std::' and 'enum ' stuff. Used by telltale. Tests game caps if they strip. Defined in Config.cpp
String MakeTypeName(String fullName);

// ================================================== STRING MASK HELPER ==================================================

/// A string mask to help find resources.
class StringMask : public String {
public:
    
    // MUST HAVE NO MEMBERS.
    
    enum MaskMode {
        MASKMODE_SIMPLE_MATCH = 0,          // Exact match required (except for wildcards)
        MASKMODE_ANY_SUBSTRING = 1,         // Pattern can match anywhere within `str`
        MASKMODE_ANY_ENDING = 2,            // Pattern can match any suffix of `str`
        MASKMODE_ANY_ENDING_NO_DIRECTORY = 3 // Similar to MASKMODE_ANY_ENDING but ignores directory separators
    };
    
    inline StringMask(const String& mask) : String(mask) {}
    
    inline StringMask(CString mask) : String(mask) {}
    
    /**
     * @brief Checks if a given string matches a search mask with wildcard patterns.
     *
     * @param testString The string to check against the search mask.
     * @param searchMask A semicolon-separated list of patterns (e.g., "*.dlog;*.chore;!private_*").
     *                   - Patterns prefixed with '!' indicate exclusion (negation).
     * @param mode The matching mode from StringMask::MaskMode.
     *             - MASKMODE_SIMPLE_MATCH: Exact match with wildcards.
     *             - MASKMODE_ANY_SUBSTRING: Pattern can appear anywhere.
     *             - MASKMODE_ANY_ENDING: Pattern can match the end.
     *             - MASKMODE_ANY_ENDING_NO_DIRECTORY: Ignores directories, only matches filenames.
     * @param excluded (Optional) If provided, set to `true` if `testString` is explicitly excluded.
     *
     * @return `true` if `testString` matches any pattern, `false` if it is excluded.
     */
    static Bool MatchSearchMask(
                                CString testString,
                                CString searchMask,
                                StringMask::MaskMode mode,
                                Bool* excluded = nullptr);
    
    static Bool MaskCompare(CString pattern, CString str, CString end, MaskMode mode); // Defined in the resource registry.
    
    inline Bool operator==(const String& rhs) const
    {
        return MatchSearchMask(rhs.c_str(), this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(const String& rhs) const
    {
        return !MatchSearchMask(rhs.c_str(), this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator==(CString rhs) const
    {
        return MatchSearchMask(rhs, this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
    inline Bool operator!=(CString rhs) const
    {
        return !MatchSearchMask(rhs, this->c_str(), MASKMODE_SIMPLE_MATCH);
    }
    
};

inline Bool operator==(const String& lhs, const StringMask& mask) // otherway round just in case
{
    return mask == lhs;
}

static_assert(sizeof(String) == sizeof(StringMask), "String and StringMask must have same size and be castable.");

// ================================================ FLOAT UTIL ================================================

// Check if a and b arguments are very close to each other
inline Bool CompareFloatFuzzy(Float a, Float b, Float epsilon = 0.000001f)
{
    return std::fabs(a - b) < epsilon;
}

inline Float ClampFloat(Float val, Float min, Float max)
{
    return fmaxf(min, fminf(max, val));
}

struct Placeholder {};

// ================================================ MISC UTIL ================================================

#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

// padding bytes needed to align _Off to align _Algn
#define TTE_PADDING(_Off, _Algn) (((_Algn) - ((_Off) % (_Algn))) % (_Algn))

// rounds the U32 argument up to the nearest whole power of two. https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
#define TTE_ROUND_UPOW2_U32(dstVar, srcVar) { dstVar = srcVar - 1; dstVar |= dstVar >> 1; dstVar |= dstVar >> 2; \
dstVar |= dstVar >> 4; dstVar |= dstVar >> 8; dstVar |= dstVar >> 16; dstVar++; }

#define ENDIAN_SWAP_U32(_data) ((((_data ^ (_data >> 16 | (_data << 16))) & 0xFF00FFFF) >> 8) ^ (_data >> 8 | _data << 24))

#define COERCE(_Ptr, _WantedT) (*(_WantedT*)(_Ptr))

#define MACRO_COMMA ,

#define NULLABLE

#define NONNULL

struct WeakPtrHash
{

    template <typename T>
    std::size_t operator()(const std::weak_ptr<T>& wp) const
    {
        auto sp = wp.lock();
        return std::hash<std::shared_ptr<T>>{}(sp);
    }

};

struct WeakPtrEqual
{

    template <typename T>
    Bool operator()(const std::weak_ptr<T>& a, const std::weak_ptr<T>& b) const
    {
        return !a.owner_before(b) && !b.owner_before(a);
    }

};

class Ticker
{
public:

    inline Ticker(I32 threshold) : _Threshold(MAX(1, threshold)), _Ticks(0) {}

    inline Bool Tick()
    {
        _Ticks = (_Ticks + 1) % _Threshold;
        return _Ticks == 0;
    }

private:

    I32 _Threshold;
    I32 _Ticks;

};

// ================================================ WEAK REF SLOTS ================================================

// fun test! find where a hidden sequence here is from. hint: crack 180 software. good luck. bits 5 & 37 used for signal state.
#define WEAK_SIGNAL_SANITY 0xEDFD425C'CB42FDCD
#define WEAK_SIGNAL_SANITY_NSTATE_MASK 0xFFFFFFDF'FFFFFDFF
#define WEAK_SIGNAL_SANITY_SSTATE_MASK 0x00000020'00000020

// must be alive until all weak refs expire & master expire!!
// fast listenable signal if master expires. can be shared by masters/weaks
class WeakSlotSignal
{
    
    friend class ToolContext;
    
    U64 _Stat = 0; // 2bits is the stat, rest are sanity bits to ensure it doesnt change (corruption checks).
    
    inline WeakSlotSignal() {}
    inline WeakSlotSignal(WeakSlotSignal&&) : _Stat(0) {}
    inline WeakSlotSignal(const WeakSlotSignal&) : _Stat(0) {}
    WeakSlotSignal& operator=(WeakSlotSignal&&) { _Stat = 0; }
    WeakSlotSignal& operator=(const WeakSlotSignal&) { _Stat = 0; }
    
public:
    
    inline Bool Expired() const
    {
        return _Stat != 0 && ((_Stat & WEAK_SIGNAL_SANITY_SSTATE_MASK) == WEAK_SIGNAL_SANITY_SSTATE_MASK);
    }
    
    // reset signal to unset, allowing you to check if another master with this signal has expired
    inline void Unset()
    {
        _Stat &= WEAK_SIGNAL_SANITY_NSTATE_MASK;
    }
    
    // ensures status is 0 or sanity bit-and'ed with the initial sanity above.
    ~WeakSlotSignal();
    
};

// master holder to the weak slot. unique ptr, not shared. weak is sharable.
class WeakSlotMaster
{
    // can copy but wont copy ref, ie x = y, x will be detached and will be empty.
    
    friend class ToolContext;
    friend class WeakSlotRef;
    
    ToolContext* _Context = 0;
    WeakSlotSignal* _Signal = 0;
    U32 _WeakSlot = 0;
    
    WeakSlotMaster(WeakSlotMaster&&) = default;
    WeakSlotMaster& operator=(WeakSlotMaster&&) = default;
    
    WeakSlotMaster& operator=(const WeakSlotMaster&);
    
    inline WeakSlotMaster(const WeakSlotMaster&) : _WeakSlot(0)
    {
    }
    
    ~WeakSlotMaster();
    
public:
    
    inline WeakSlotMaster() {}
    
};

// weak reference to slot. up to 32767 wk refs
class WeakSlotRef
{
    
    friend class WeakSlotMaster;
    friend class ToolContext;
    
    ToolContext* _Context = 0;
    U32 _WeakSlot = 0;
    
    WeakSlotRef(WeakSlotRef&&) = default;
    WeakSlotRef& operator=(WeakSlotRef&&) = default;
    
    WeakSlotRef& operator=(const WeakSlotRef& rhs);
    
    inline WeakSlotRef(const WeakSlotMaster& rhs)
    {
        *this = rhs;
    }
    
    ~WeakSlotRef();
    
    inline WeakSlotRef(U32 s, ToolContext* c) : _WeakSlot(s), _Context(c) {}
    
public:
    
    inline WeakSlotRef() {}
    
};
