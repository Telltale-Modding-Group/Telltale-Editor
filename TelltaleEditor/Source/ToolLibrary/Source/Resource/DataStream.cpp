#include <Core/Symbol.hpp>
#include <Resource/DataStream.hpp>
#include <Resource/Blowfish.hpp>
#include <Meta/Meta.hpp>
#include <Core/Context.hpp>

#include <algorithm>
#include <stdexcept>
#include <deque>

// ===================================================================
// SCHEMES
// ===================================================================

String SchemeToString(ResourceScheme scheme)
{
    if (scheme == ResourceScheme::CACHE)
        return "cache";
    else if (scheme == ResourceScheme::SYMBOL)
        return "symbol";
    else if (scheme == ResourceScheme::FILE)
        return "file";
    else if (scheme == ResourceScheme::LOGICAL)
        return "logical";
    else
        return "";
}

ResourceScheme StringToScheme(const String &scheme)
{
    Symbol schemeSymbol = Symbol(scheme);
    if (schemeSymbol == Symbol("file"))
        return ResourceScheme::FILE;
    else if (schemeSymbol == Symbol("symbol"))
        return ResourceScheme::SYMBOL;
    else if (schemeSymbol == Symbol("logical"))
        return ResourceScheme::LOGICAL;
    else if (schemeSymbol == Symbol("cache"))
        return ResourceScheme::CACHE;
    else
    {
        TTE_LOG("Scheme is invalid: %s", scheme.c_str());
        return ResourceScheme::INVALID; // Default
    }
}

// ===================================================================
// MANAGER
// ===================================================================

DataStreamManager *DataStreamManager::Instance = nullptr;

void DataStreamManager::Shutdown()
{
    if (Instance)
    {
        TTE_DEL(Instance);
        Instance = nullptr;
    }
}

void DataStreamManager::Initialise()
{
    if (!Instance)
    {
        Instance = TTE_NEW(DataStreamManager, MEMORY_TAG_DATASTREAM);
    }
}

void DataStreamManager::WriteZeros(DataStreamRef& ref, U64 N)
{
    U8 Zeros[128]{0};
    if(N <= 128)
        ref->Write(Zeros, N);
    else
    {
        U8* Temp = AllocateAnonymousMemory(N);
        ref->Write(Temp, N);
        FreeAnonymousMemory(Temp, N);
    }
}

void DataStreamManager::WriteString(DataStreamRef &stream, const String &str)
{
    U32 sz = (U32)str.length();
    SerialiseDataU32(stream, nullptr, &sz, true);
    stream->Write((const U8*)str.c_str(), str.length());
}

String DataStreamManager::ReadString(DataStreamRef& str)
{
    U32 size{};
    SerialiseDataU32(str, 0, &size, false);
    if(size > 10000)
    {
        TTE_ASSERT(false, "String size is too large, serialised string is likely corrupted");
        return "";
    }
    String s{};
    s.resize(size, '0');
    str->Read((U8*)s.c_str(), (U64)size); // !!
    return s;
}

// Local deleter used to deallocate all data streams using TTE_DEL when pointers expire.
static void DataStreamDeleter(DataStream *Stream) { TTE_DEL(Stream); }

// create using a custom deleter, function above. tagged allocation for tracking. must be a file url.
DataStreamRef DataStreamManager::CreateFileStream(const ResourceURL &path)
{
    TTE_ASSERT(path.GetScheme() == ResourceScheme::FILE, "Resource URL must be a file path");
    
    DataStreamFile *pDSFile = TTE_NEW(DataStreamFile, MEMORY_TAG_DATASTREAM, path);
    return DataStreamRef(pDSFile, &DataStreamDeleter);
}

ResourceURL DataStreamManager::Resolve(const Symbol &unresolved)
{
    return ResourceURL(); // .
}

void DataStreamManager::Publicise(Ptr<DataStreamMemory> &stream)
{
    std::lock_guard<std::mutex> G{_CacheLock};
    for (auto &it : _Cache)
    {
        if (it.second == stream)
            return; // already exists in the cache
    }
    _Cache[stream->GetURL().GetRawPath()] = stream; // add to cache
}

void DataStreamManager::ReleaseCache(const String &path)
{
    std::lock_guard<std::mutex> G{_CacheLock};
    for (auto it = _Cache.begin(); it != _Cache.end(); it++)
    {
        if (CompareCaseInsensitive(it->first, path))
        {
            _Cache.erase(it);
            return; // found, erase and return
        }
    }
}

Ptr<DataStreamMemory> DataStreamManager::CreatePublicCache(const String &path, U32 pageSize)
{
    std::lock_guard<std::mutex> G{_CacheLock};
    Ptr<DataStreamMemory> pMemoryStream = _Cache[path] = CreatePrivateCache(path, pageSize);
    return pMemoryStream; // Create privately, make public, return it.
}

Ptr<DataStreamMemory> DataStreamManager::CreatePrivateCache(const String &path, U32 pageSize)
{
    // Allocate new instance with correct deleter.
    DataStreamMemory *pDSFile = TTE_NEW(DataStreamMemory, MEMORY_TAG_DATASTREAM, path, (U64)pageSize);
    return Ptr<DataStreamMemory>(pDSFile, &DataStreamDeleter);
}

Ptr<DataStreamSequentialStream> DataStreamManager::CreateSequentialStream(const String& url)
{
    DataStreamSequentialStream *pDS = TTE_NEW(DataStreamSequentialStream, MEMORY_TAG_DATASTREAM, url);
    return Ptr<DataStreamSequentialStream>(pDS, &DataStreamDeleter);
}

Ptr<DataStreamMemory> DataStreamManager::FindCache(const String &path)
{
    std::lock_guard<std::mutex> G{_CacheLock};
    for (auto it = _Cache.begin(); it != _Cache.end(); it++)
    {
        if (CompareCaseInsensitive(it->first, path))
        {
            return it->second; // return a new copy of the pointer (inc refs)
        }
    }
    return nullptr;
}

DataStreamRef DataStreamManager::CreateTempStream() { return CreateFileStream(ResourceURL(ResourceScheme::FILE, FileNewTemp())); }

DataStreamRef DataStreamManager::CreateContainerStream(const DataStreamRef& src)
{
    DataStreamContainer *pDS = TTE_NEW(DataStreamContainer, MEMORY_TAG_DATASTREAM, src);
    return DataStreamRef(pDS, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::CreateCachedStream(const DataStreamRef& src)
{
    if(!src || dynamic_cast<DataStreamBuffer*>(src.get()) || dynamic_cast<DataStreamMemory*>(src.get()))
        return src; // already in memory
    
    U8* Buffer = TTE_ALLOC(src->GetSize(), MEMORY_TAG_RUNTIME_BUFFER); // move ownership to new buffer
    
    src->SetPosition(0);
    
    if(!src->Read(Buffer, src->GetSize()))
    {
        TTE_LOG("At CreateCachedStream: could not read all bytes from source stream");
        TTE_FREE(Buffer);
        return nullptr;
    }
    
    DataStreamBuffer *pDS = TTE_NEW(DataStreamBuffer, MEMORY_TAG_DATASTREAM, src->GetURL(), src->GetSize(), Buffer, true);
    return DataStreamRef(pDS, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::Copy(DataStreamRef src, U64 srcOff, U64 n)
{
    TTE_ASSERT(src, "Source stream");
    DataStreamRef mem = CreatePrivateCache("");
    src->SetPosition(srcOff);
    Transfer(src, mem, n);
    return mem;
}

DataStreamRef DataStreamManager::CreateNullStream()
{
    DataStreamNull *pDS = TTE_NEW(DataStreamNull, MEMORY_TAG_DATASTREAM, ResourceURL());
    return DataStreamRef(pDS, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::CreateBufferStream(const ResourceURL &url, U64 size, U8 *b, Bool o)
{
    DataStreamBuffer *pDSFile = TTE_NEW(DataStreamBuffer, MEMORY_TAG_DATASTREAM, url, size, b, o);
    return DataStreamRef(pDSFile, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::CreateSubStream(const DataStreamRef &p, U64 o, U64 z)
{
    DataStreamSubStream *pDSFile = TTE_NEW(DataStreamSubStream, MEMORY_TAG_DATASTREAM, p, o, z);
    return DataStreamRef(pDSFile, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::CreateLegacyEncryptedStream(const DataStreamRef& src, U64 baseOffset, U16 blockSize, U8 rawf, U8 blowf)
{
    DataStreamLegacyEncrypted* pDS = TTE_NEW(DataStreamLegacyEncrypted, MEMORY_TAG_DATASTREAM, src, baseOffset, blockSize, rawf, blowf);
    return DataStreamRef(pDS, &DataStreamDeleter);
}

// transfer blocked.
Bool DataStreamManager::Transfer(DataStreamRef &src, DataStreamRef &dst, U64 Nbytes)
{
    if (src && dst && Nbytes)
    {
        
        U8 *Tmp = TTE_ALLOC(MIN(0x10000, Nbytes), MEMORY_TAG_TEMPORARY);
        U64 Nblocks = Nbytes / 0x10000;
        
        Bool Result = true;
        
        for (U64 i = 0; i < Nblocks; i++)
        {
            
            Result = src->Read(Tmp, 0x10000);
            if (!Result)
            {
                TTE_FREE(Tmp);
                return false;
            }
            
            Result = dst->Write(Tmp, 0x10000);
            if (!Result)
            {
                TTE_FREE(Tmp);
                return false;
            }
        }
        
        if(Nbytes & 0xFFFF) // transfer remaining bytes
        {
            Result = src->Read(Tmp, Nbytes & 0xFFFF);
            if (!Result)
            {
                TTE_FREE(Tmp);
                return false;
            }
            
            Result = dst->Write(Tmp, Nbytes & 0xFFFF);
            if (!Result)
            {
                TTE_FREE(Tmp);
                return false;
            }
        }
        
        TTE_FREE(Tmp);
        
        return Result;
    }
    else
        return false; // invalid input arguments.
}

// ===================================================================
// FILE DATA STREAM
// ===================================================================

Bool DataStreamFile::Read(U8 *OutputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Handle != FileNull(), "File handle is null. Cannot read");
    return FileRead(_Handle, OutputBuffer, Nbytes);
}

Bool DataStreamFile::Write(const U8 *InputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Handle != FileNull(), "File handle is null. Cannot write");
    _MaxOffset = MAX(_MaxOffset, (GetPosition() + Nbytes));
    return FileWrite(_Handle, InputBuffer, Nbytes);
}

U64 DataStreamFile::GetPosition() { return FilePos(_Handle); }

void DataStreamFile::SetPosition(U64 p) { FileSeek(_Handle, p); }

DataStreamFile::~DataStreamFile()
{
    if (_Handle != FileNull())
    {
        FileClose(_Handle, _MaxOffset);
        _Handle = FileNull();
    }
}

DataStreamFile::DataStreamFile(const ResourceURL &url) : DataStream(url), _Handle(FileNull()), _MaxOffset(0)
{
    // Attempt to open the file
    String fpath = url.GetRawPath(); // Get the raw path without the scheme, and pass it to the file system to try and find the file.
    std::filesystem::path p{fpath.c_str()};
    p = std::filesystem::absolute(p.parent_path());
    if(!std::filesystem::exists(p))
        std::filesystem::create_directories(p);
    _Handle = FileOpen(fpath.c_str());
}

U64 DataStreamFile::GetSize() { return _Handle == FileNull() ? 0 : FileSize(_Handle); }

// ===================================================================
// RESOURCE URL
// ===================================================================

DataStreamRef ResourceURL::Open()
{
    if (_Scheme == ResourceScheme::INVALID)
        return nullptr; // Invalid, so always return null.
    else if (_Scheme == ResourceScheme::FILE)
        return DataStreamManager::GetInstance()->CreateFileStream(*this); // Open the file
    else if (_Scheme == ResourceScheme::CACHE)
    {
        
        // First try and search cache for it.
        Ptr<DataStreamMemory> pMemoryStream = DataStreamManager::GetInstance()->FindCache(_Path);
        
        // If it does not exist, create the public cached one and return it.
        if (!pMemoryStream)
            pMemoryStream = DataStreamManager::GetInstance()->CreatePublicCache(_Path);
        
        return pMemoryStream;
    }
    else if (_Scheme == ResourceScheme::SYMBOL)
    {
        // Try and resolve it first. If the hex hash is valid and was resolved, yay! Update this current resource URL too.
        try
        {
            
            Symbol value = std::stoull(_Path, nullptr, 0x10); // to string
            
            ResourceURL resolvedURL = DataStreamManager::GetInstance()->Resolve(value);
            if (resolvedURL)
            {
                *this = resolvedURL;
                TTE_ASSERT(resolvedURL.GetScheme() != ResourceScheme::SYMBOL, "Unresolved"); // should not return a symbol, is resolved.
                return resolvedURL.Open();                                                   // Open it.
            }
            else
                return nullptr; // No errors, it just could not be resolved.
        }
        catch (const std::invalid_argument &e)
        {
            TTE_ASSERT(false, "Invalid symbol hex hash in resource path: %s", _Path.c_str());
            return nullptr;
        }
        catch (const std::out_of_range &e)
        {
            TTE_ASSERT(false, "Symbol hash is too large, therefore invaild, in resource path: %s", _Path.c_str());
            return nullptr;
        }
    }
    else
    {
        TTE_ASSERT(false, "Cannot open scheme type currently (%d)", (U32)_Scheme);
    }
    return nullptr;
}

String ResourceURL::GetPath() const { return SchemeToString(_Scheme) + ":" + _Path; }

ResourceURL::ResourceURL(const Symbol &symbol)
{
    ResourceURL resolved = DataStreamManager::GetInstance()->Resolve(symbol);
    *this = resolved;
    if (GetScheme() == ResourceScheme::INVALID)
    {
        _Scheme = ResourceScheme::SYMBOL;
        char tmp[32];
        sprintf(tmp, "%llX", symbol.GetCRC64());
        _Path = tmp;
    }
}

ResourceURL::ResourceURL(String path, Bool bAllowAngles)
{
    // Split from scheme.
    size_t pos = path.find_first_of(':');
    if (pos == std::string::npos)
    { // no ':', default file.
        _Path = std::move(path);
        _Scheme = ResourceScheme::FILE;
    }
    else
    {
        _Scheme = StringToScheme(path.substr(0, pos));
        _Path = path.substr(pos + 1);
    }
    _Normalise(bAllowAngles);
}

void ResourceURL::_Normalise(Bool angleBrackets)
{
    // replace backslashes with forward slashes
    std::replace(_Path.begin(), _Path.end(), '\\', '/');
    
    // paths cannot contain bad characters
    std::string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./-_<> ";
    _Path.erase(std::remove_if(_Path.begin(), _Path.end(), [&validChars](char c) { return validChars.find(c) == std::string::npos; }), _Path.end());
    if(!angleBrackets)
    {
        validChars = "<>"; // 'invalid' chars
        _Path.erase(std::remove_if(_Path.begin(), _Path.end(), [&validChars](char c) { return validChars.find(c) != std::string::npos; }), _Path.end());
    }
    
    // remove leading/trailing whitespace 'trim' and slashes
    _Path.erase(0, _Path.find_first_not_of(" \t\n\r\\")); // dont remove '/' prefix, macos absolute paths / linux
    _Path.erase(_Path.find_last_not_of(" \t\n\r\\/") + 1);
}

// ===================================================================         PAGED DATA STREAM
// ===================================================================

DataStreamDeferred::~DataStreamDeferred()
{
    _PageIdx = _PagePos = _Size = 0;
}

DataStreamDeferred::DataStreamDeferred(const ResourceURL &url, U64 pageSize) : DataStream(url), _PageSize(pageSize)
{
    _PageIdx = _PagePos = _Size = 0;
}

// perform a chunked read in blocks of pagesize.
Bool DataStreamDeferred::Read(U8 *OutputBuffer, U64 Nbytes)
{
    if(!Nbytes)
        return true; // edge case
    
    if (Nbytes + GetPosition() > GetSize())
    {
        TTE_ASSERT(false, "Cannot read 0x%llx bytes from current offset in deferred stream, not enough remaining bytes", Nbytes);
        return false;
    }
    
    // first read bytes from current page
    U64 remInCurrentPage = _PageSize - _PagePos;
    
    if (remInCurrentPage > Nbytes)
    {
        if (!_SerialisePage(_PageIdx, OutputBuffer, Nbytes, _PagePos, false))
            return false;
        _PagePos += Nbytes;
        return true; // enough bytes were read from the current page.
    }
    else if(remInCurrentPage > 0)
    {
        // read remaining bytes from the current page
        if (!_SerialisePage(_PageIdx, OutputBuffer, remInCurrentPage, _PagePos, false))
            return false;
        Nbytes -= remInCurrentPage; // decrease pending bytes to be read
        OutputBuffer += remInCurrentPage;
    }
    
    // next loop through and read pages totally
    while (Nbytes >= _PageSize)
    {
        if (!_SerialisePage(++_PageIdx, OutputBuffer, _PageSize, 0, false))
            return false;
        OutputBuffer += _PageSize;
        Nbytes -= _PageSize;
    }
    
    // finally read any remainding bytes
    if (Nbytes)
    {
        if (!_SerialisePage(++_PageIdx, OutputBuffer, Nbytes, 0, false))
            return false;
        _PagePos = Nbytes;
    }
    else
    {
        _PageIdx++;
        _PagePos = 0;
    }
    
    return true;
}

Bool DataStreamDeferred::Write(const U8 *InputBuffer, U64 Nbytes)
{
    if(!Nbytes)
        return true; // edge case: nothing to write
    
    // first read bytes from current page
    U64 remInCurrentPage = _PageSize - _PagePos;
    
    _Size = MAX(GetPosition() + Nbytes, _Size); // update size if needed
    
    if (remInCurrentPage > Nbytes)
    {
        // Write the bytes to the current page
        if(!_SerialisePage(_PageIdx, const_cast<U8*>(InputBuffer), Nbytes, _PagePos, true))
            return false;
        _PagePos += Nbytes;
        return true; // enough bytes to write to the single current page.
    }
    else if(remInCurrentPage)
    {
        // Write remaining bytes to the current page
        if(!_SerialisePage(_PageIdx, const_cast<U8*>(InputBuffer), remInCurrentPage, _PagePos, true))
            return false;
        // Decrease remaining bytes to be written
        Nbytes -= remInCurrentPage;
        InputBuffer += remInCurrentPage;
        _PagePos = 0;
    }
    
    // Write full pages if remaining bytes are >= _PageSize
    while (Nbytes >= _PageSize)
    {
        if(!_SerialisePage(++_PageIdx, const_cast<U8*>(InputBuffer), _PageSize, 0, true))
            return false;
        InputBuffer += _PageSize;
        Nbytes -= _PageSize;
    }
    
    // Finally, handle the remaining bytes that don't fill a full page
    if (Nbytes)
    {
        if(!_SerialisePage(++_PageIdx, const_cast<U8*>(InputBuffer), Nbytes, 0, true))
            return false;
        _PagePos = Nbytes;  // Update the position for the next write operation
    }
    else
    {
        _PagePos = 0;  // Reset position when no remaining bytes
        _PageIdx++;    // Move to next page for next write
    }
    
    return true;
}


// split data into first remaining chunk in current page, chunks for middle full sized pages, then final remainder page.
/*Bool DataStreamDeferred::Write(const U8 *InputBuffer, U64 Nbytes)
 {
 if(!Nbytes)
 return true; //edge case
 
 // first read bytes from current page
 U64 remInCurrentPage = _PageSize - _PagePos;
 
 _Size = MAX(GetPosition() + Nbytes, _Size); // update size if needed
 
 if (remInCurrentPage > Nbytes)
 {
 if(!_SerialisePage(_PageIdx, const_cast<U8*>(InputBuffer), Nbytes, _PagePos, true))
 return false;
 _PagePos += Nbytes;
 return true; // enough bytes to write to the single current page.
 }
 else if(remInCurrentPage)
 {
 if(!_SerialisePage(_PageIdx, const_cast<U8*>(InputBuffer), Nbytes, _PagePos, true))
 return false;
 // write remaining bytes to the current page
 Nbytes -= remInCurrentPage; // decrease pending bytes
 InputBuffer += remInCurrentPage;
 _PagePos = 0;
 }
 
 // next loop through and read pages totally
 while (Nbytes >= _PageSize)
 {
 if(!_SerialisePage(++_PageIdx, const_cast<U8*>(InputBuffer), _PageSize, 0, true))
 return false;
 InputBuffer += _PageSize;
 Nbytes -= _PageSize;
 }
 
 // finally read any remainding bytes
 if (Nbytes)
 {
 if(!_SerialisePage(++_PageIdx, const_cast<U8*>(InputBuffer), Nbytes, 0, true))
 return false;
 _PagePos = Nbytes;
 }
 else
 {
 _PagePos = 0;
 _PageIdx++;
 }
 
 return true;
 }*/

void DataStreamDeferred::SetPosition(U64 newPosition)
{
    _PageIdx = newPosition / _PageSize;
    _PagePos = newPosition % _PageSize;
}

// ===================================================================         MEMORY DATA STREAM
// ===================================================================

// memory page serialise. no special operations performed on the pages, just put into page table
Bool DataStreamMemory::_SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 off, Bool IsWrite)
{
    TTE_ASSERT(off < _PageSize, "Invalid size");
    if(IsWrite)
    {
        U8* Page;
        
        // first check if we need to insert empty pages between
        if(index > _PageTable.size())
        {
            U64 between = index - _PageTable.size();
            for(U64 i = 0; i < between; i++)
            {
                U8* Page = TTE_ALLOC(_PageSize, MEMORY_TAG_RUNTIME_BUFFER);
                memset(Page, 0, _PageSize);
                _PageTable.push_back(Page);
                
            }
        }
        
        // check if we need to push the new page
        if(index == _PageTable.size())
        {
            Page = TTE_ALLOC(_PageSize, MEMORY_TAG_RUNTIME_BUFFER);
            memset(Page, 0, _PageSize);
            _PageTable.push_back(Page);
        }
        else
        {
            Page = _PageTable[index]; // page already exists
        }
        
        // write to the page buffer
        memcpy(Page + off, Buffer, Nbytes);
    }
    else
    {
        if(index >= _PageTable.size())
            memset(Buffer, 0, Nbytes); // read zeros
        else
            memcpy(Buffer, _PageTable[index] + off, Nbytes);
    }
    return true;
}

DataStreamMemory::DataStreamMemory(const ResourceURL &url, U64 pageSize) : DataStreamDeferred(url, MAX(0x100, pageSize))
{
    ; // page size of less than 256 causes probably more overhead from vector allocs.
}

DataStreamMemory::~DataStreamMemory()
{
    for (auto ptr : _PageTable)
        TTE_FREE(ptr);
    _PageTable.clear();
}

// ===================================================================
// NULL DATA STREAM
// ===================================================================

DataStreamNull::DataStreamNull(const ResourceURL& url) : DataStream(url) {}

DataStreamNull::~DataStreamNull() {}

// ===================================================================
// BUFFER DATA STREAM
// ===================================================================

Bool DataStreamBuffer::Read(U8 *OutputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes <= _Size, "Trying to read too many bytes from buffer stream");
    memcpy(OutputBuffer, _Buffer + _Off, Nbytes);
    _Off += Nbytes;
    return true;
}

Bool DataStreamBuffer::Write(const U8 *InputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes <= _Size, "Trying to write too many bytes to buffer stream");
    memcpy(_Buffer + _Off, InputBuffer, Nbytes);
    _Off += Nbytes;
    return true;
}

DataStreamBuffer::~DataStreamBuffer()
{
    if (_Owns && _Buffer)
        TTE_FREE(_Buffer);
    _Buffer = nullptr;
    _Size = _Off = 0;
}

DataStreamBuffer::DataStreamBuffer(const ResourceURL &url, U64 sz, U8 *buf, Bool f) : DataStream(url)
{
    _Size = sz;
    _Off = 0;
    _Owns = buf ? f : true;
    if (buf == nullptr)
        _Buffer = TTE_ALLOC(sz, MEMORY_TAG_DATASTREAM);
    else
        _Buffer = buf;
}

void DataStreamBuffer::SetPosition(U64 pos)
{
    TTE_ASSERT(pos < _Size, "Trying to seek to invalid position in buffer stream");
    _Off = pos;
}

// ===================================================================         SUB DATA STREAM
// ===================================================================

Bool DataStreamSubStream::Read(U8 *OutputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes <= _Size, "Trying to read too many bytes from sub stream");
    if (_Prnt->GetPosition() != _BaseOff + _Off)
        _Prnt->SetPosition(_BaseOff + _Off);
    if(!_Prnt->Read(OutputBuffer, Nbytes))
        return false;
    _Off += Nbytes;
    return true;
}

Bool DataStreamSubStream::Write(const U8 *InputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes <= _Size, "Trying to write too many bytes to sub stream");
    if (_Prnt->GetPosition() != _BaseOff + _Off)
        _Prnt->SetPosition(_BaseOff + _Off);
    _Prnt->Write(InputBuffer, Nbytes);
    _Off += Nbytes;
    return true;
}

DataStreamSubStream::~DataStreamSubStream()
{
    _Off = _BaseOff = _Size = 0;
    _Prnt.reset();
}

DataStreamSubStream::DataStreamSubStream(const DataStreamRef &ref, U64 off, U64 sz) : DataStream(ref ? ref->GetURL() : ResourceURL())
{
    _Off = 0;
    _BaseOff = off;
    _Size = sz;
    _Prnt = ref;
}

void DataStreamSubStream::SetPosition(U64 pos)
{
    TTE_ASSERT(pos <= _Size, "Trying to seek to invalid position in buffer stream");
    _Off = pos;
}

// ===================================================================         LEGACY ENCRYPTED STREAM
// ===================================================================

// Delegate to _GetBlock
Bool DataStreamLegacyEncrypted::_SerialisePage(U64 index, U8* Buffer, U64 n, U64 off, Bool)
{
    _GetBlock((U32)index);
    // Now read it (writes disabled)
    memcpy(Buffer, _Rb + off, n);
    return true;
}

void DataStreamLegacyEncrypted::_GetBlock(U32 i)
{
    // in macos boneville executable, function that reads this is at 0x478F6 (check with IDA or your choice), altho thats part of MetaStream class
    
    U64 blockStartOffset = (U64)_PageSize * i; // offset in base stream
    
    TTE_ASSERT(blockStartOffset < GetSize(), "Trying too read too many bytes from encrypted stream"); // sanity
    
    U64 readSize = MIN(_PageSize, GetSize() - blockStartOffset); // the last page is left as is (raw)
    
    _Prnt->SetPosition(_BaseOff + blockStartOffset); // set position
    
    TTE_ASSERT(_Prnt->Read(_Rb, readSize), "Failed to read block from parent stream"); // do read
    
    if(readSize == _PageSize) // if they are not equal, then we are in the last block and its left as is.
    {
        
        if((i % _EncFreq) == 0) // if its a multiple or is the first block
        {
            Blowfish::GetInstance()->Decrypt(_Rb, (U32)_PageSize); // blowfish decrypt this block
        }
        else
        {
            if((i % _RawFreq) == 0) // if its a multiple
            {
                // this block is left as is
            }
            else
            {
                // most common case. all bits are flipped. i dont know why this was thought, as its probably the worst encryption.
                // at least maybe, idk, make the most common the blowfish? i mean, aren't archives encrypted anyway.. with the SAME encryption!?
                for(U64 i = 0; i < (_PageSize); i++)
                    _Rb[i] = _Rb[i] ^ 0xFF; // flip bits, ~
            }
        }
    }
}

Bool DataStreamLegacyEncrypted::Write(const U8 *InBuffer, U64 Nbytes)
{
    TTE_ASSERT(false, "Write mode not available for encrypted streams - don't use this wrapper class here.");
    return false;
}

DataStreamLegacyEncrypted::DataStreamLegacyEncrypted(const DataStreamRef& p, U64 o, U16 z, U8 r, U8 bf)
: DataStreamDeferred(p->GetURL(), (U64)z)
{
    TTE_ASSERT(bf != r, "Frequencies cannot be the same");
    // init members
    _Prnt = p;
    _BaseOff = o;
    _RawFreq = r;
    _EncFreq = bf;
    memset(_Rb, 0, 0x100);
}

// ===================================================================         APPEND STREAM
// ===================================================================

DataStreamAppendStream::DataStreamAppendStream(const ResourceURL& url, DataStreamRef* pRef, U32 N) : DataStream(url)
{
    _AppendStreams.reserve(N);
    for(U32 i = 0; i < N; i++)
    {
        _AppendStreams.push_back(pRef[i]);
    }
}

Bool DataStreamAppendStream::Write(const U8 *InputBuffer, U64 Nbytes)
{
    Bool result = true;
    
    for(auto& stream: _AppendStreams)
        if(!stream->Write(InputBuffer, Nbytes))
            result = false;
    
    return result;
}

// ===================================================================         SEQUENTIAL DATA STREAM
// ===================================================================

DataStreamSequentialStream::DataStreamSequentialStream(const ResourceURL& url) : DataStream(url) {}

U64 DataStreamSequentialStream::GetSize()
{
    U64 totalSize = _BytesRead;
    if(_StreamIndex < (U64)_OrderedStreams.size())
        totalSize += _OrderedStreams[_StreamIndex]->GetSize() - _StreamPos; // remaining from current
    for(U32 i = _StreamIndex + 1; i < (U64)_OrderedStreams.size(); i++)
    {
        totalSize += _OrderedStreams[i]->GetSize();
    }
    return totalSize;
}

U64 DataStreamSequentialStream::GetPosition()
{
    return _BytesRead;
}

void DataStreamSequentialStream::SetPosition(U64 pos)
{
    U64 running = 0;
    U32 streamIndex = 0;
    for(auto& stream: _OrderedStreams)
    {
        U64 ssize = stream->GetSize();
        if(pos >= running && pos <= running + ssize)
        {
            _BytesRead = pos;
            _StreamIndex = streamIndex;
            _StreamPos = pos - running;
            return;
        }
        streamIndex++;
    }
    TTE_ASSERT(false, "DataStreamSequentialStream::SetPosition => position 0x%llX too large", pos);
}

Bool DataStreamSequentialStream::Read(U8 *OutputBuffer, U64 Nbytes)
{
    if(!Nbytes)
        return true; // edge case
    
    if(_StreamIndex >= (U64)_OrderedStreams.size()) // finished
    {
        TTE_ASSERT(false, "Cannot read bytes from sequential stream: no data available");
        return false;
    }
    
    // first read bytes from current page
    U64 remInCurrentStream = _OrderedStreams[_StreamIndex]->GetSize() - _StreamPos;
    
    if (remInCurrentStream > Nbytes)
    {
        if(!_OrderedStreams[_StreamIndex]->Read(OutputBuffer, Nbytes))
            return false;
        _StreamPos += Nbytes;
        _BytesRead += Nbytes;
        return true; // enough bytes were read from the current stream
    }
    else if(remInCurrentStream > 0)
    {
        // read remaining bytes from the current page
        if(!_OrderedStreams[_StreamIndex]->Read(OutputBuffer, remInCurrentStream))
            return false;
        Nbytes -= remInCurrentStream; // decrease pending bytes to be read
        OutputBuffer += remInCurrentStream;
        _BytesRead += remInCurrentStream;
    }
    
    _StreamIndex++;
    _StreamPos = 0;
    
    // next loop through and streams fully
    while (Nbytes)
    {
        if(_StreamIndex >= (U64)_OrderedStreams.size()) // finished
        {
            TTE_ASSERT(false, "Cannot read bytes from sequential stream: no data available");
            return false;
        }
        U64 ssize = _OrderedStreams[_StreamIndex]->GetSize();
        if(Nbytes < ssize)
            break;
        if(!_OrderedStreams[_StreamIndex]->Read(OutputBuffer, ssize))
            return false;
        OutputBuffer += ssize;
        Nbytes -= ssize;
        _BytesRead += ssize;
        _StreamIndex++;
        _StreamPos = 0;
    }
    
    // finally read any remainding bytes
    if (Nbytes)
    {
        
        if(_StreamIndex >= (U64)_OrderedStreams.size()) // finished
        {
            TTE_ASSERT(false, "Cannot read bytes from sequential stream: no data available");
            return false;
        }
        
        if(!_OrderedStreams[_StreamIndex]->Read(OutputBuffer, Nbytes))
            return false;
        
        _StreamPos = Nbytes;
        _BytesRead += Nbytes;
    }
    else
    {
        _StreamPos = 0;
    }
    
    return true;
}

// ===================================================================         CONTAINER (COMPRESSABLE) DATA STREAM
// ===================================================================

Bool DataStreamContainer::_SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 pageOffset, Bool IsWrite)
{
    TTE_ASSERT(!IsWrite, "Cannot write to container stream directly"); // other functionality for this
    TTE_ASSERT(index < _PageOffsets.size() - 1, "Invalid container stream page index");
    
    if(_Compressed)
    {
        if(_CachedPageIndex != index)
        {
            // cache new page index
            U32 pageSize = (U32)(_PageOffsets[index+1] - _PageOffsets[index]);
            _Prnt->SetPosition(_DataOffsetStart + _PageOffsets[index]);
            if(!_Prnt->Read(_IntPage, pageSize))
            {
                TTE_LOG("Could not read page from container stream: index %lld", index);
                return false;
            }
            
            if(_Encrypted)
            {
                // decrypt
                Blowfish::GetInstance()->Decrypt(_IntPage, pageSize);
            }
            
            // decompress
            if(!Compression::Decompress(_IntPage, (U64)pageSize, _CachedPage, _PageSize, _Compression))
            {
                TTE_LOG("Decompression failed at data stream container");
                return false;
            }
            
            // update index
            _CachedPageIndex = index;
            
        }
        
        // read from page
        memcpy(Buffer, _CachedPage + pageOffset, Nbytes);
        
        return true;
    }
    else
    {
        _Prnt->SetPosition(_DataOffsetStart + (index * _PageSize) + pageOffset); // set pos and read normal
        return _Prnt->Read(Buffer, Nbytes);
    }
}

DataStreamContainer::DataStreamContainer(const DataStreamRef& p) : DataStreamDeferred(p->GetURL(), 0x10000), _CachedPage(nullptr),
_Valid(false), _Compressed(false), _Encrypted(false), _Compression(Compression::ZLIB),  _CachedPageIndex(0)
{
    _Prnt = p; // set parent
    U32 Magic = {}; // top 3 bytes should be TTC - telltale container
    SerialiseDataU32(_Prnt, 0, &Magic, false);
    
    if((Magic & 0xFFFFFF00) != 0x54544300)
    {
        TTE_LOG("When reading container: invalid format");
        return;
    }
    
    char containerVersion = (char)(Magic & 0xFF);
    
    if(containerVersion == 'N') // N: no compression/encrypted
    {
        SerialiseDataU64(_Prnt, 0, &_Size, false); // read the size of it
        _Valid = true;
        _Compressed = _Encrypted = false;
        _DataOffsetStart = _Prnt->GetPosition();
    }
    else if(containerVersion == 'e' || containerVersion == 'z') // lower case: read compression type (1 == oodle, 0 = zlib)
    {
        _DataOffsetStart = _Prnt->GetPosition() - 4; // for offsets, go back behind magic
        _Compressed = true;
        _Encrypted = containerVersion == 'e'; // e = encrypted & compressed. z = compressed
        U32 compressor = {};
        SerialiseDataU32(_Prnt, 0, &compressor, false);
        _Compression = compressor == 0 ? Compression::ZLIB : Compression::OODLE;
        if(compressor > 1) // 0: zlib, 1: oodle. if not oodle or zlib, error
        {
            TTE_LOG("When reading container: unknown compression type");
            return;
        }
    }
    else if(containerVersion == 'Z' || containerVersion == 'E')
    {
        _DataOffsetStart = _Prnt->GetPosition() - 4; // for offsets, go back behind magic
        _Compressed = true;
        _Encrypted = containerVersion == 'E'; // E = Encrypyed (+ zlib)
        _Compression = Compression::ZLIB; // older versions default to zlib
    }
    else
    {
        TTE_LOG("When reading container: unknown container version");
        return;
    }
    
    if(_Compressed) // read compressed info
    {
        
        U32 WindowSize = {};
        SerialiseDataU32(_Prnt, 0, &WindowSize, false); // size of each uncompressed page
        
        // set page size... its const but lets just
        *const_cast<U64*>(&_PageSize) = WindowSize;
        
        U32 NumPages = {};
        SerialiseDataU32(_Prnt, 0, &NumPages, false); // number of compressed pages. total uncompressed size = windowwsize * ++numpages
        
        _PageOffsets.resize(NumPages + 1); // + 1 for extra EOF
        // vector stores in a block, so serialise all
        if(!_Prnt->Read((U8*)_PageOffsets.data(), ((U64)NumPages + 1) << 3))
        {
            TTE_LOG("When reading container: could not read page offsets");
            return;
        }
        
        _Size = WindowSize * NumPages; // total uncompressed size
        
        _CachedPageIndex = (U64)-1; // init cache page to none yet
        _CachedPage = TTE_ALLOC(WindowSize, MEMORY_TAG_RUNTIME_BUFFER);
        _IntPage = TTE_ALLOC(WindowSize, MEMORY_TAG_RUNTIME_BUFFER);
        
    }
    
    _Valid = true; // DONE
}

#define CONTAINER_BULK_SIZE 16
#define MAX_FOOTPRINT_MB 512

struct _AsyncContainerContext
{
    
    std::mutex Input; // input data mutex
    DataStreamRef Src;
    volatile U32 InputIndex; // current index of next page(s) to be read from src
    
    std::timed_mutex Output; // output stream mutex: locks the following:
    DataStreamRef Dst;
    volatile U32 FlushIndex; // next page index to be flushed (always a multiple of BulkSize)
    std::vector<U32> FinalCompressedSizes;
    
    U32 NumSlaves; // number of threads, including master, actively running and compressing pages. between 1 and NUM_SCHEDULER_THREADS.
    U32 NumPages; // total number of pages to be compressed
    U32 MaxPendingFlushes; // maximum number of pages which can fit in to the stashed pending buffer for each thread
    U64 SrcSize; // total bytes in from src to read
    U64 SrcDataStart; // position in src to start reading SrcSize bytes from
    U64 DstContainerStart; // position in dst stream of magic bytes for container
    
    U8* WorkingBuffersIn[NUM_SCHEDULER_THREADS]; // inputs for each thread
    U8* WorkingBuffersOut[NUM_SCHEDULER_THREADS]; // output for each thread
    U8* OverflowWorkingOut[NUM_SCHEDULER_THREADS]; // if compression is larger than uncompressed size. size of each is 2*0x10000
    
    U8* StashedPending[NUM_SCHEDULER_THREADS];
    
    // information
    Compression::Type Compression;
    Bool Encrypt;
    // U32 BlockSize; we will always use 65536
    
};

struct ProcessedPageBulk
{
    U32 FirstPageIndex;
    U32 FlushSlot; // slot in stashed buffer
    U32 CompressedSizes[CONTAINER_BULK_SIZE];
    Ptr<U8> Overflow[CONTAINER_BULK_SIZE];
    // if compressed data is larger than window size (ie couldn't compress well) store it externally.
};

static void _U8Deleter(U8* F)
{
    TTE_FREE(F);
}

static Bool _DoPage(U8* In, U8* Out, _AsyncContainerContext* ctx, ProcessedPageBulk& blk, U32 blkIndex, U32 pgIndex)
{
    
    U64 compressedSize = Compression::Compress(In, 0x10000, Out, 0x10000, ctx->Compression);
    if(compressedSize == 0)
    {
        // try again with larger output buffer.
        U8* OutputBuffer = TTE_ALLOC(0x11000, MEMORY_TAG_TEMPORARY);
        compressedSize = Compression::Compress(In, 0x10000, OutputBuffer, 0x11000, ctx->Compression);
        if(compressedSize == 0)
        {
            TTE_FREE(OutputBuffer);
            TTE_LOG("Failed to compress page %d (output buffer is 2x input size) of container input. Failing...",
                    (blkIndex*CONTAINER_BULK_SIZE)+pgIndex);
            return false;
        }
        Out = OutputBuffer;
        blk.Overflow[pgIndex] = Ptr<U8>(Out, &_U8Deleter);
    }
    
    blk.CompressedSizes[pgIndex] = (U32)compressedSize;
    
    // encrypt if needed
    if(ctx->Encrypt)
        Blowfish::GetInstance()->Encrypt(Out, (U32)compressedSize);
    
    return true;
}

// slave async job: compresses pages needed
static Bool _AsyncCreateContainerSlave(const JobThread& thread, void* pCtx, void* _n)
{
    _AsyncContainerContext* ctx = (_AsyncContainerContext*)pCtx;
    U32 myIndex = (U32)((U64)_n & 0xFFFFllu); // worker slave index. give priority to myIndex 0 as no wait for the thread to start (direct call)
    
    std::deque<ProcessedPageBulk> PendingFlushesInfo{}; // data info about what is written in the pending file
    
    U32 PagesRead = 0; // number of pages we have read in so far (not neccesarily compressed).
    U32 BulkIndex = 0; // current index of compression in working buffers
    Bool Finished = false;
    
    while(!Finished || PendingFlushesInfo.size() > 0) // keep going until we can
    {
        
        ProcessedPageBulk blk{};
        
        Bool bThisPageNeedsPendingFlush = !Finished; // set false if we can flush already
        
        if(!Finished)
        {
            // read all pages we can
            ctx->Input.lock();
            // check first, have we finished? ie have we *read* all pages (ignoring the remainder pages, done first).
            if(ctx->InputIndex >= (ctx->NumPages - (ctx->NumPages % CONTAINER_BULK_SIZE)))
            {
                Finished = true;
                ctx->Input.unlock();
                continue;
            }
            else
            {
                // egde case: perfect multiple of bulk size: last page very likely (65535-1/65535) to have less bytes
                U64 skipEndBytes = 0;
                if(ctx->InputIndex + CONTAINER_BULK_SIZE == ctx->NumPages)
                {
                    skipEndBytes = 0x10000 - (ctx->SrcSize & 0xFFFF);
                }
                
                // do the read
                if(!ctx->Src->Read(ctx->WorkingBuffersIn[myIndex], (CONTAINER_BULK_SIZE * 0x10000) - skipEndBytes))
                {
                    TTE_LOG("Could not read from source stream while flushing container stream!");
                    ctx->Input.unlock();
                    return false;
                }
                
                // edge case: fill remaining with zeros.
                if(skipEndBytes)
                    memset(ctx->WorkingBuffersIn[myIndex] + (CONTAINER_BULK_SIZE * 0x10000) - skipEndBytes, 0, skipEndBytes);
                
                blk.FirstPageIndex = ctx->InputIndex;
                ctx->InputIndex += CONTAINER_BULK_SIZE;
            }
            ctx->Input.unlock();
            
            // process the data: compress and encrypt.
            for(U32 i = 0; i < CONTAINER_BULK_SIZE; i++)
            {
                if(!_DoPage(ctx->WorkingBuffersIn[myIndex] + i * 0x10000,
                            ctx->WorkingBuffersOut[myIndex] + i * 0x10000, ctx, blk, blk.FirstPageIndex / CONTAINER_BULK_SIZE, i))
                {
                    TTE_LOG("Failed to compress container page!");
                    return false;
                }
            }
        }
        
        if(PendingFlushesInfo.size()) // try and flush as many as we can out of the stashed pending buffer
        {
            // Check if we can flush
            Bool bLocked = true;
            
            if(ctx->Output.try_lock_for(std::chrono::milliseconds(1)))
            {
                // output it open, lock is held. (yay!)
            }
            else if((PendingFlushesInfo.size() >= ctx->MaxPendingFlushes) || Finished)
            {
                ctx->Output.lock(); // if we really need to lock it, force it here (ie we have a lot of pages finished) OR are finished reading.
            }
            else bLocked = false;
            
            if(bLocked)
            {
                // ensure order of output writes. write all the pages we want to flush until the order does not match.
                while(PendingFlushesInfo.size() && ctx->FlushIndex == PendingFlushesInfo.front().FirstPageIndex)
                {
                    ProcessedPageBulk pg = std::move(PendingFlushesInfo.front());
                    PendingFlushesInfo.pop_front();
                    
                    // transfer to output stream. we can use working buffer IN, as its not used at the moment.
                    for(U32 i = 0; i < CONTAINER_BULK_SIZE; i++)
                    {
                        U8* Buf = ctx->StashedPending[myIndex] + (pg.FlushSlot * 0x10000 * CONTAINER_BULK_SIZE) + i * 0x10000;
                        if(pg.Overflow[i])
                            Buf = pg.Overflow[i].get();
                        if(!ctx->Dst->Write(Buf, pg.CompressedSizes[i]))
                        {
                            TTE_LOG("Failed to write compressed page to output container stream!");
                            ctx->Output.unlock();
                            return false;
                        }
                    }
                    
                    // add compressed sizes to final
                    ctx->FinalCompressedSizes.insert(ctx->FinalCompressedSizes.end(), &pg.CompressedSizes[0], &pg.CompressedSizes[CONTAINER_BULK_SIZE]);
                    
                    ctx->FlushIndex +=CONTAINER_BULK_SIZE;
                }
                
                // we may actually be able to flush the data just processed already. check.
                if(blk.FirstPageIndex == ctx->FlushIndex)
                {
                    // yay! write the compressed chunks
                    for(U32 i = 0; i < CONTAINER_BULK_SIZE; i++)
                    {
                        U8* Buf = ctx->WorkingBuffersOut[myIndex] + i * 0x10000;
                        if(blk.Overflow[i])
                            Buf = blk.Overflow[i].get();
                        if(!ctx->Dst->Write(Buf, blk.CompressedSizes[i]))
                        {
                            TTE_LOG("Failed to write compressed page to output container stream (2)!");
                            ctx->Output.unlock();
                            return false;
                        }
                    }
                    
                    // add compressed sizes to final
                    ctx->FinalCompressedSizes.insert(ctx->FinalCompressedSizes.end(), &blk.CompressedSizes[0], &blk.CompressedSizes[CONTAINER_BULK_SIZE]);
                    
                    ctx->FlushIndex +=CONTAINER_BULK_SIZE;
                    
                    bThisPageNeedsPendingFlush = false;
                }
                
                // unlock
                ctx->Output.unlock();
            }
        }
        
        // finally, flush if needed
        if(bThisPageNeedsPendingFlush)
        {
            // find a slot, the lowest one. (common problem: finding lowest number not in array. use fact we know max number).
            constexpr U32 bitsetSize = ((MAX_FOOTPRINT_MB * 1024 * 1024)/(0x10000 * CONTAINER_BULK_SIZE)) >> 3; // = 64
            U8 localBitset[bitsetSize]{};
            I32 slot = -1;
            for(auto& it: PendingFlushesInfo)
            {
                TTE_ASSERT(it.FlushSlot < ctx->MaxPendingFlushes, "Invalid flush slot");
                localBitset[it.FlushSlot >> 3] |= (1u << (it.FlushSlot & 7));
            }
            // could use bitscan routines. compilers might optimise
            for(U32 i = 0; i < bitsetSize; i++)
            {
                if(localBitset[i] == 0xFF)
                    continue; // no free in this byte
                for(U32 j = 0; j < 7; j++)
                {
                    if((localBitset[i] & (1u << j)) == 0)
                    {
                        slot = (i << 3) + j;
                        break;
                    }
                }
                // slot is MSB
                slot = slot ? slot : (i << 3) + 7;
                break;
            }
            blk.FlushSlot = slot;
            
            // flush it all
            memcpy(ctx->StashedPending[myIndex] + (blk.FlushSlot * 0x10000 * CONTAINER_BULK_SIZE),
                   ctx->WorkingBuffersOut[myIndex], CONTAINER_BULK_SIZE * 0x10000);
            PendingFlushesInfo.push_back(std::move(blk));
        }
    }
    
    return true;
}

// master async job: dispatches more jobs to do each page
static Bool _AsyncCreateContainerMaster(const JobThread* pThread, void* pCtx, void* _Del)
{
    _AsyncContainerContext* ctx = (_AsyncContainerContext*)pCtx;
    Bool bResult = true;
    
    // 1. reserve and write offset bytes to dst to be done later
    ctx->FinalCompressedSizes.reserve(ctx->NumPages);
    U64 offsetsOffset = ctx->Dst->GetPosition();
    DataStreamManager::GetInstance()->WriteZeros(ctx->Dst, ((U64)ctx->NumPages + 1) << 3);
    U64 dstDataOffset = ctx->Dst->GetPosition() - ctx->DstContainerStart;
    
    // 2. set the maximum number of stashable pages per slave while waiting for their turn in the ordered final writes
    ctx->MaxPendingFlushes = MIN((MAX_FOOTPRINT_MB * 1024 * 1024 - (2 * 0x10000 * ctx->NumSlaves * CONTAINER_BULK_SIZE))
                                 / (0x10000 * ctx->NumSlaves  * CONTAINER_BULK_SIZE), ctx->NumPages / ctx->NumSlaves); // 512MB MAXIMUM in memory.
    
    // 3. allocate temp buffers
    U8* TempMemory = TTE_ALLOC(ctx->NumSlaves * 0x10000 * (2 + CONTAINER_BULK_SIZE * (2 + ctx->MaxPendingFlushes)),
                               MEMORY_TAG_TEMPORARY_ASYNC); // + 2 for in and out, then plus on stashed pending. +2 for overflow compress
    
    // 4. assign buffer regions
    U8* ToFreeBuffer = TempMemory;
    for(U32 i = 0; i < ctx->NumSlaves; i++)
    {
        ctx->WorkingBuffersIn[i] = TempMemory;
        TempMemory += CONTAINER_BULK_SIZE * 0x10000;
        ctx->WorkingBuffersOut[i] = TempMemory;
        TempMemory += CONTAINER_BULK_SIZE * 0x10000;
        ctx->OverflowWorkingOut[i] = TempMemory;
        TempMemory += 2 * 0x10000;
        ctx->StashedPending[i] = TempMemory;
        TempMemory += ctx->MaxPendingFlushes * 0x10000 * CONTAINER_BULK_SIZE;
    }
    
    JobHandle Handles[NUM_SCHEDULER_THREADS-1]{}; // job handles to wait after
    
    // 5. if we have any remaining pages, read them first here.
    U32 RemPages = ctx->NumPages % CONTAINER_BULK_SIZE;
    U32 NBulks = (ctx->NumPages - RemPages) / CONTAINER_BULK_SIZE; // number of bulks slaves will do
    U8* RemPagesTemp = nullptr;
    if(RemPages)
    {
        U64 skipEndBytes = 0x10000 - (ctx->SrcSize & 0xFFFF);
        ctx->Src->SetPosition(ctx->SrcDataStart + ((ctx->NumPages - RemPages)) * 0x10000); // seek to last remaining pages
        RemPagesTemp = TTE_ALLOC((RemPages + 1) * 0x10000, MEMORY_TAG_TEMPORARY);// add +1 for working buffer
        bResult = ctx->Src->Read(RemPagesTemp + 0x10000, RemPages * 0x10000 - skipEndBytes); // this must succeed
        ctx->Src->SetPosition(ctx->SrcDataStart); // reset pos
    }
    
    if(bResult)
    {
        ProcessedPageBulk remBlk{};
        
        // 5. kick off other slave jobs (not 0, thats here)
        JobDescriptor Descriptors[NUM_SCHEDULER_THREADS-1];
        for(U32 i = 1; i < ctx->NumSlaves; i++)
        {
            Descriptors[i-1].AsyncFunction = &_AsyncCreateContainerSlave;
            Descriptors[i-1].Priority = JOB_PRIORITY_NORMAL;
            Descriptors[i-1].UserArgA = ctx;
            Descriptors[i-1].UserArgB = (void*)((U64)i);
        }
        JobScheduler::Instance->PostAll(Descriptors, ctx->NumSlaves - 1, Handles);
        
        // 6. do compress and encrypt on remaining pages
        if(RemPages)
        {
            U8* WorkingBufferIn = RemPagesTemp;
            for(U32 i = 1; i <= RemPages; i++)
            {
                U8* WorkingBufferOut = RemPagesTemp + (i * 0x10000);
                if(!_DoPage(WorkingBufferIn, WorkingBufferOut, ctx, remBlk, NBulks, i - 1))
                {
                    TTE_LOG("Failed to compress container page!");
                    bResult = false;
                    break;
                }
            }
        }
        
        // 7. we already have this job running, make it slave 0
        if(bResult)
            bResult = _AsyncCreateContainerSlave(*pThread, ctx, nullptr); // index 0
        else
        {
            for(U32 i = 0; i < ctx->NumSlaves - 1; i++)
                JobScheduler::Instance->Cancel(Handles[i], false); // cancel if fail
        }
        
        // 8. wait for others to complete.
        JobScheduler::Instance->Wait(ctx->NumSlaves - 1, Handles);
        
        if(bResult)
        {
            
            // write remaining pages. first ensure we have read all bulks.
            TTE_ASSERT(ctx->Src->GetPosition() == ctx->SrcDataStart + NBulks * 0x10000 * CONTAINER_BULK_SIZE, "Internal error with container");
            
            for(U32 i = 1; i <= RemPages; i++)
            {
                U8* Buf = RemPagesTemp + (i * 0x10000);
                if(remBlk.Overflow[i-1])
                    Buf = remBlk.Overflow[i-1].get();
                bResult = ctx->Dst->Write(Buf, remBlk.CompressedSizes[i-1]);
                if(!bResult)
                    break;
                ctx->FinalCompressedSizes.push_back(remBlk.CompressedSizes[i-1]);
            }
            
            if(bResult)
            {
                
                // finally write compressed offsets.
                ctx->Dst->SetPosition(offsetsOffset);
                U64 runningOffset = dstDataOffset;
                for(auto size: ctx->FinalCompressedSizes)
                {
                    SerialiseDataU64(ctx->Dst, nullptr, &runningOffset, true);
                    runningOffset += (U64)size;
                }
                SerialiseDataU64(ctx->Dst, nullptr, &runningOffset, true);
                ctx->Dst->SetPosition(ctx->Dst->GetSize());
                
            }
        }
    }
    
    if(RemPagesTemp)
        TTE_FREE(RemPagesTemp);
    TTE_FREE(ToFreeBuffer); // free large buffer
    if(_Del)
        TTE_DEL(ctx); // done
    return bResult;
}

Bool _AsyncCreateContainerMasterDelegate(const JobThread& pThread, void* pCtx, void* _Del)
{
    return _AsyncCreateContainerMaster(&pThread, pCtx, _Del); // job thread can be 0 if just running from calling thread
}

// write telltale container using async jobs if needed
Bool DataStreamManager::FlushContainer(DataStreamRef& src, U64 n, DataStreamRef& dst, ContainerParams p, JobHandle& jobout)
{
    TTE_ASSERT(IsCallingFromMain(), "Flush container can only be called from the main thread!");
    TTE_ASSERT(JobScheduler::Instance, "Job scheduler is not initialised");
    
    if(!src || !dst || n > src->GetPosition() + src->GetSize())
        return false;
    
    if(p.Compression == Compression::END_LIBRARY && p.Encrypt)
        p.Compression = Compression::ZLIB; // must compress if encrypting
    
    if(p.Compression == Compression::OODLE && !GetToolContext()->GetActiveGame()->Fl.Test(Meta::RegGame::ENABLE_OODLE))
    {
        TTE_LOG("Cannot use oodle compression for the current game snapshot - it is not supported in the game. Please specify Zlib and retry!");
        return false;
    }
    
    if(p.Compression == Compression::END_LIBRARY)
    {
        // no compression, easy mode: magic, size, data. no async.
        dst->Write((const U8*)"NCTT", 4);
        SerialiseDataU64(dst, nullptr, &n, true);
        Transfer(src, dst, n);
    }
    else
    {
        
        U64 dstStart = dst->GetPosition();
        
        if(p.Compression == Compression::OODLE)
        {
            dst->Write(p.Encrypt ? (const U8*)"eCTT" : (const U8*)"zCTT", 4);
            U32 compressionLib = 1; // oodle
            SerialiseDataU32(dst, nullptr, &compressionLib, true);
        }
        else
        {
            dst->Write(p.Encrypt ? (const U8*)"ECTT" : (const U8*)"ZCTT", 4);
        }
        
        U32 windowSize = 0x10000; // default as all archive to 65536
        SerialiseDataU32(dst, nullptr, &windowSize, true);
        
        U32 nPages = (U32)((n + 0xFFFF) >> 16); // last page is padded with zeros
        SerialiseDataU32(dst, nullptr, &nPages, true);
        
        if(nPages < 32) // anything less than 32 pages we can on this thread, anything more we will need async.
        {
            _AsyncContainerContext ctx{};
            ctx.Compression = p.Compression;
            ctx.Src = src;
            ctx.Dst = dst;
            ctx.DstContainerStart = dstStart;
            ctx.Encrypt = p.Encrypt;
            ctx.SrcDataStart = src->GetPosition();
            ctx.NumSlaves = 1;
            ctx.SrcSize = n;
            ctx.NumPages = nPages;
            return _AsyncCreateContainerMaster(nullptr, &ctx, nullptr);
        }
        else
        {
            _AsyncContainerContext* pContext = TTE_NEW(_AsyncContainerContext, MEMORY_TAG_TEMPORARY_ASYNC);
            pContext->Compression = p.Compression;
            pContext->Src = src;
            pContext->SrcSize = n;
            pContext->Dst = dst;
            pContext->DstContainerStart = dstStart;
            pContext->SrcDataStart = src->GetPosition();
            pContext->Encrypt = p.Encrypt;
            // pages 32 to 1024 use 2 slaves, 1024 to 4096 use 4, else 8
            pContext->NumSlaves = nPages > 4096 ? 8 : nPages > 1024 ? 4 : 2;
            pContext->NumPages = nPages;
            
            JobDescriptor desc{};
            desc.AsyncFunction = &_AsyncCreateContainerMasterDelegate;
            desc.Priority = JOB_PRIORITY_COUNT;
            desc.UserArgA = pContext;
            desc.UserArgB = pContext; // if non zero we need to delete it after
            jobout = JobScheduler::Instance->Post(std::move(desc));
        }
        
    }
    
    return true;
}
