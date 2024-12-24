#include <Core/Symbol.hpp>
#include <Resource/DataStream.hpp>

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
}

DataStreamRef DataStreamManager::CreateTempStream() { return CreateFileStream(ResourceURL(ResourceScheme::FILE, FileNewTemp())); }

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

// transfer blocked.
Bool DataStreamManager::Transfer(DataStreamRef &src, DataStreamRef &dst, U64 Nbytes)
{
    if (src && dst && Nbytes)
    {

        U8 *Tmp = TTE_ALLOC(MAX(0x10000, Nbytes), MEMORY_TAG_TEMPORARY);
        U64 Nblocks = (Nbytes + 0xFFFF) / 0x10000;

        Bool Result = true;

        for (U64 i = 0; i < Nblocks; i++)
        {

            Result = src->Read(Tmp, 0x10000);
            if (!Result)
                break;

            Result = dst->Write(Tmp, 0x10000);
            if (!Result)
                break;
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

    // remove leading/trailing whitespace 'trim'
    _Path.erase(0, _Path.find_first_not_of(" \t\n\r/"));
    _Path.erase(_Path.find_last_not_of(" \t\n\r/") + 1);
}

// ===================================================================         MEMORY DATA STREAM
// ===================================================================

DataStreamMemory::DataStreamMemory(const ResourceURL &url, U64 pageSize) : DataStream(url)
{
    _PageSize = MAX(0x100, pageSize); // page size of less than 256 causes probably more overhead from vector allocs.
    _PageIdx = _PagePos = _Size = 0;
}

// perform a chunked read in blocks of pagesize.
Bool DataStreamMemory::Read(U8 *OutputBuffer, U64 Nbytes)
{
    if (Nbytes + _PagePos + (_PageIdx * _PageSize) > _Size)
    {
        TTE_ASSERT(false, "Cannot read 0x%llx bytes from current offset in cache memory stream, not enough remaining bytes", Nbytes);
        return false;
    }

    // first read bytes from current page
    U64 remInCurrentPage = _PageSize - _PagePos;

    if (remInCurrentPage >= Nbytes)
    {
        if (!_ReadPage(_PageIdx, OutputBuffer, Nbytes))
            return false;
        _PagePos += Nbytes;
        return true; // enough bytes were read from the current page.
    }
    else
    {
        // read remaining bytes from the current page
        if (!_ReadPage(_PageIdx, OutputBuffer, remInCurrentPage))
            return false;
        Nbytes -= remInCurrentPage; // decrease pending bytes to be read
        OutputBuffer += remInCurrentPage;
    }

    // next loop through and read pages totally
    while (Nbytes >= _PageSize)
    {
        if (!_ReadPage(++_PageIdx, OutputBuffer, _PageSize))
            return false;
        OutputBuffer += _PageSize;
        Nbytes -= _PageSize;
    }

    // finally read any remainding bytes
    if (Nbytes)
    {
        if (!_ReadPage(++_PageIdx, OutputBuffer, Nbytes))
            return false;
        _PagePos = Nbytes;
    }
    else
        _PagePos = 0;

    return true;
}

Bool DataStreamMemory::_ReadPage(U64 index, U8 *OutputBuffer, U64 Nbytes)
{
    memcpy(OutputBuffer, _PageTable[_PageIdx], Nbytes);
    return true;
}

// split data into first remaining chunk in current page, chunks for middle full sized pages, then final remainder page.
Bool DataStreamMemory::Write(const U8 *InputBuffer, U64 Nbytes)
{
    _EnsureCap(_Size + Nbytes);

    // first read bytes from current page
    U64 remInCurrentPage = _PageSize - _PagePos;

    _Size += Nbytes; // update size here

    if (remInCurrentPage >= Nbytes)
    {
        memcpy(_PageTable[_PageIdx] + _PagePos, InputBuffer, Nbytes);
        return true; // enough bytes to write to the single current page.
    }
    else
    {
        // write remaining bytes to the current page
        memcpy(_PageTable[_PageIdx] + _PagePos, InputBuffer, remInCurrentPage);
        Nbytes -= remInCurrentPage; // decrease pending bytes
        InputBuffer += remInCurrentPage;
    }

    // next loop through and read pages totally
    while (Nbytes >= _PageSize)
    {
        memcpy(_PageTable[++_PageIdx], InputBuffer, _PageSize);
        InputBuffer += _PageSize;
        Nbytes -= _PageSize;
    }

    // finally read any remainding bytes
    if (Nbytes)
    {
        memcpy(_PageTable[++_PageIdx], InputBuffer, Nbytes);
        _PagePos = Nbytes;
    }
    else
        _PagePos = 0;

    return true;
}

DataStreamMemory::~DataStreamMemory()
{
    _PageSize = _PageIdx = _PagePos = 0;
    for (auto ptr : _PageTable)
        TTE_FREE(ptr);
    _PageTable.clear();
}

void DataStreamMemory::SetPosition(U64 newPosition)
{
    if (newPosition > _PageTable.size() * _PageSize) // ensure backend capacity
        _EnsureCap(newPosition);

    _PageIdx = newPosition / _PageSize;
    _PagePos = newPosition % _PageSize;
}

void DataStreamMemory::_EnsureCap(U64 bytes)
{ // bytes is target size

    if (bytes <= _PageTable.size() * _PageSize)
        return;

    U64 numExtraPages = 0;

    if (_PageTable.size() == 0)
        numExtraPages = (bytes / _PageSize) + 1; // + 1 to ensure remainder, if not needed still provides more space.
    else
        numExtraPages = MAX(0, ((bytes + _PageSize - 1) / _PageSize) - _PageTable.size()); // ensure any remainder adds new page

    if (numExtraPages > 0)
    {

        // To save allocations, let us allocate one big buffer and add each pointer offset
        U8 *pMemory = TTE_ALLOC(numExtraPages * _PageSize, MEMORY_TAG_DATASTREAM);

        // minimise vector allocs and push each pointer offset to the page table.
        _PageTable.reserve(_PageTable.size() + numExtraPages);
        for (U64 i = 0; i < numExtraPages; i++)
            _PageTable.push_back(pMemory + (i * _PageSize));
    }
}

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
    TTE_ASSERT(_Off + Nbytes < _Size, "Trying to read too many bytes from sub stream");
    if (_Prnt->GetPosition() != _BaseOff + _Off)
        _Prnt->SetPosition(_BaseOff + _Off);
    _Prnt->Read(OutputBuffer, Nbytes);
    _Off += Nbytes;
    return true;
}

Bool DataStreamSubStream::Write(const U8 *InputBuffer, U64 Nbytes)
{
    TTE_ASSERT(_Off + Nbytes < _Size, "Trying to write too many bytes to sub stream");
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
}

void DataStreamSubStream::SetPosition(U64 pos)
{
    TTE_ASSERT(pos < _Size, "Trying to seek to invalid position in buffer stream");
    _Off = pos;
}
