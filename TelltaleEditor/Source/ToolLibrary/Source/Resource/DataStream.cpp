#include <Core/Symbol.hpp>
#include <Resource/DataStream.hpp>
#include <Resource/Blowfish.hpp>
#include <Meta/Meta.hpp>

#include <algorithm>
#include <stdexcept>

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

    // TODO. resource set descriptions, search archives and folders like engine, OR search data.ttarch if not found on fsys.

    return ResourceURL(); // Not found
}

void DataStreamManager::Publicise(std::shared_ptr<DataStreamMemory> &stream)
{
    for (auto &it : _Cache)
    {
        if (it.second == stream)
            return; // already exists in the cache
    }
    _Cache[stream->GetURL().GetRawPath()] = stream; // add to cache
}

void DataStreamManager::ReleaseCache(const String &path)
{
    for (auto it = _Cache.begin(); it != _Cache.end(); it++)
    {
        if (CompareCaseInsensitive(it->first, path))
        {
            _Cache.erase(it);
            return; // found, erase and return
        }
    }
}

std::shared_ptr<DataStreamMemory> DataStreamManager::CreatePublicCache(const String &path, U32 pageSize)
{
    std::shared_ptr<DataStreamMemory> pMemoryStream = _Cache[path] = CreatePrivateCache(path, pageSize);
    return pMemoryStream; // Create privately, make public, return it.
}

std::shared_ptr<DataStreamMemory> DataStreamManager::CreatePrivateCache(const String &path, U32 pageSize)
{
    // Allocate new instance with correct deleter.
    DataStreamMemory *pDSFile = TTE_NEW(DataStreamMemory, MEMORY_TAG_DATASTREAM, path, (U64)pageSize);
    return std::shared_ptr<DataStreamMemory>(pDSFile, &DataStreamDeleter);
}

std::shared_ptr<DataStreamMemory> DataStreamManager::FindCache(const String &path)
{
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

DataStreamRef DataStreamManager::CreateNullStream()
{
    DataStreamNull *pDS = TTE_NEW(DataStreamNull, MEMORY_TAG_DATASTREAM, ResourceURL());
    return DataStreamRef(pDS, &DataStreamDeleter);
}

DataStreamRef DataStreamManager::CreateBufferStream(const ResourceURL &url, U64 size, U8 *b)
{
    DataStreamBuffer *pDSFile = TTE_NEW(DataStreamBuffer, MEMORY_TAG_DATASTREAM, url, size, b);
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

        U8 *Tmp = TTE_ALLOC(MAX(0x10000, Nbytes), MEMORY_TAG_TEMPORARY);
        U64 Nblocks = Nbytes / 0x10000;

        Bool Result = true;

        for (U64 i = 0; i < Nblocks; i++)
        {

            Result = src->Read(Tmp, 0x10000);
            if (!Result)
                return false;

            Result = dst->Write(Tmp, 0x10000);
            if (!Result)
                return false;
        }
        
        if(Nbytes & 0xFFFF) // transfer remaining bytes
        {
            Result = src->Read(Tmp, Nbytes & 0xFFFF);
            if (!Result)
                return false;
            
            Result = dst->Write(Tmp, Nbytes & 0xFFFF);
            if (!Result)
                return false;
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
    return FileWrite(_Handle, InputBuffer, Nbytes);
}

U64 DataStreamFile::GetPosition() { return FilePos(_Handle); }

void DataStreamFile::SetPosition(U64 p) { FileSeek(_Handle, p); }

DataStreamFile::~DataStreamFile()
{
    if (_Handle != FileNull())
    {
        FileClose(_Handle);
        _Handle = FileNull();
    }
}

DataStreamFile::DataStreamFile(const ResourceURL &url) : DataStream(url), _Handle(FileNull())
{
    // Attempt to open the file
    String fpath = url.GetRawPath(); // Get the raw path without the scheme, and pass it to the file system to try and find the file.
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
        std::shared_ptr<DataStreamMemory> pMemoryStream = DataStreamManager::GetInstance()->FindCache(_Path);

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

ResourceURL::ResourceURL(const String &path)
{
    // Split from scheme.
    size_t pos = path.find_first_of(':');
    if (pos == std::string::npos)
    { // no ':', default file.
        _Path = path;
        _Scheme = ResourceScheme::FILE;
    }
    else
    {
        _Scheme = StringToScheme(path.substr(0, pos));
        _Path = path.substr(pos + 1);
    }
    _Normalise();
}

void ResourceURL::_Normalise()
{
    // replace backslashes with forward slashes
    std::replace(_Path.begin(), _Path.end(), '\\', '/');

    // paths cannot contain bad characters
    std::string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./-_";
    _Path.erase(std::remove_if(_Path.begin(), _Path.end(), [&validChars](char c) { return validChars.find(c) == std::string::npos; }), _Path.end());

    // remove leading/trailing whitespace 'trim' and slashes
    _Path.erase(0, _Path.find_first_not_of(" \t\n\r/"));
    _Path.erase(_Path.find_last_not_of(" \t\n\r/") + 1);
}

// ===================================================================         PAGED DATA STREAM
// ===================================================================

DataStreamDeferred::~DataStreamDeferred()
{
    _PageIdx = _PagePos = _Size = 0;
}

DataStreamDeferred::DataStreamDeferred(const ResourceURL &url, U64 pageSize) : DataStream(url), _PageSize(pageSize)
{
    _PageIdx = _PagePos = 0;
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
        _PagePos = 0;

    return true;
}

// split data into first remaining chunk in current page, chunks for middle full sized pages, then final remainder page.
Bool DataStreamDeferred::Write(const U8 *InputBuffer, U64 Nbytes)
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
        _PagePos = 0;
    
    return true;
}

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
                _PageTable.push_back(Page);
                
            }
        }
        
        // check if we need to push the new page
        if(index == _PageTable.size())
        {
            Page = TTE_ALLOC(_PageSize, MEMORY_TAG_RUNTIME_BUFFER);
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
            memcpy(Buffer, _PageTable[_PageIdx] + off, Nbytes);
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
    TTE_ASSERT(_Off + Nbytes < _Size, "Trying to read too many bytes from buffer stream");
    memcpy(OutputBuffer, _Buffer + _Off, Nbytes);
    _Off += Nbytes;
    return true;
}

Bool DataStreamBuffer::Write(const U8 *InputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes < _Size, "Trying to write too many bytes to buffer stream");
    memcpy(_Buffer + _Off, InputBuffer, Nbytes);
    _Off += Nbytes;
    return true;
}

DataStreamBuffer::~DataStreamBuffer()
{
    if (_Buffer)
        TTE_FREE(_Buffer);
    _Buffer = nullptr;
    _Size = _Off = 0;
}

DataStreamBuffer::DataStreamBuffer(const ResourceURL &url, U64 sz, U8 *buf) : DataStream(url)
{
    _Size = sz;
    _Off = 0;
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
    TTE_ASSERT(pos < _Size, "Trying to seek to invalid position in buffer stream");
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
