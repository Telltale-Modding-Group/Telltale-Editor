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
    return std::chrono::duration_cast<std::chrono::microseconds>(
                                                                 std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

// Gets the time difference in *seconds* between start and end.
inline Float GetTimeStampDifference(U64 start, U64 end)
{
    return static_cast<Float>(end - start) / 1'000'000.0f;
}

inline String GetFormatedTime(Float secs) {
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
        outReturnType = StringTrim(arg);
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
    if (from.empty()) return str;
    
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

// Removes 'class ' 'struct ' 'std::' and 'enum ' stuff. Used by telltale
inline String MakeTypeName(String fullName)
{
    StringReplace(fullName, "class ", "");
    StringReplace(fullName, "struct ", "");
    StringReplace(fullName, "enum ", "");
    StringReplace(fullName, "std::", "");
    StringReplace(fullName, " ", "");
    return fullName;
}

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
