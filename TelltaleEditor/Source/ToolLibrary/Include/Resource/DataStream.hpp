#pragma once

#include <Core/Config.hpp>
#include <Core/Symbol.hpp>
#include <Resource/Compression.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <map>
#include <memory>
#include <vector>

#define MEMORY_STREAM_DEFAULT_PAGESIZE 0x1000

// DATA STREAM AND URL MANAGEMENT. See DataStream class and URL class comments.

// Valid resource schemes for use in the ResourceURL class below.
enum class ResourceScheme
{
    INVALID = 0,
    FILE = 1,  //'file': a file on disk. The default scheme, relative to the executable directory if not absolute.
    CACHE = 2, //'cache': a file kept only in (volatile) memory. Not available when app is exited. Used for temporary stuff.
    // CACHE: warning. this will cache a reference to the file in memory. see DataStreamManager, as you must release it when finished!
    SYMBOL = 3, //'symbol': followed by the hexadecimal hash after the colon. Special scheme used to find from symbol.
};

String SchemeToString(ResourceScheme);
ResourceScheme StringToScheme(const String &scheme);

// Resource URL. Synonymous with a string although provides more functionality.
// A way of abstracting data stream *sources*, ie where they originate from, where they are stored.
// This could be a file on disk, an embedded file (archives or other files, eg sound banks and meta types), etc.
class ResourceURL
{
  public:
    // Constructs pointing nothing.
    ResourceURL() = default;

    // Constructs with the given path. Path can start with 'xxx:path/path' where xxx is the scheme string.
    ResourceURL(String path);

    // Constructs with the symbol scheme. The resource URL that points to that symbol. If GetScheme() is still symbol, it was not found.
    // You can still call open and another find attempt is made, so Open() may succeed if it now exists.
    // If it is not this, then it was found and you can open it.
    explicit ResourceURL(const Symbol &symbol);

    // Construct manually
    inline ResourceURL(ResourceScheme scheme, const String &path) : _Path(path), _Scheme(scheme) { _Normalise(); }

    // Returns the full path of the URL
    String GetPath() const;

    // Opens this URL, ie hold a new data stream to read/write from/to it. Pass in the mode. Returns NULL if it does not exist.
    DataStreamRef Open();

    // Returns the top-level scheme of this URL. By top-level we mean the master file scheme (ie the parent if its embedded)
    ResourceScheme GetScheme() const { return _Scheme; }

    // Returns the path of the URL, without the scheme prefix.
    inline String GetRawPath() const { return _Path; }

    // Implicit operator to string type.
    inline operator String() const { return GetPath(); }

    // equality operator
    inline bool operator==(const ResourceURL &rhs) const { return _Scheme == rhs._Scheme && Symbol(_Path) == Symbol(rhs._Path); }

    // inequality operator
    inline bool operator!=(const ResourceURL &rhs) const { return _Scheme != rhs._Scheme || Symbol(_Path) != Symbol(rhs._Path); }

    // Implicit bool converter, returns if valid.
    inline operator bool() const { return _Scheme != ResourceScheme::INVALID; }

  private:
    void _Normalise(); // remove platform specific stuff.

    String _Path;
    ResourceScheme _Scheme;
};

// A DataStream is a source or destination in which bytes are written to. This is a base class. Use ResourceURL and the manager to create.
class DataStream
{
  public:
    // Reads bytes into output buffer
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) = 0;

    // Writes bytes from input buffer
    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) = 0;

    // Returns the size in bytes of this data stream
    virtual U64 GetSize() = 0;

    virtual void SetPosition(U64 newPosition) = 0;

    virtual U64 GetPosition() = 0;

    // Gets the resource URL of this data stream.
    inline virtual const ResourceURL &GetURL() { return _ResourceLocation; }

    inline virtual ~DataStream() {} // Virtual destructor

  protected:
    // Creates new data stream from a URL. Opens it.
    inline DataStream(const ResourceURL &url) : _ResourceLocation(url) {}

    ResourceURL _ResourceLocation;

    friend class DataStreamManager; // Allow access to ctor
};

// File specific data stream
class DataStreamFile : public DataStream
{
  public:
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override;

    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override;

    virtual void SetPosition(U64 newPosition) override;

    virtual U64 GetPosition() override;

    virtual U64 GetSize() override;

    virtual ~DataStreamFile();

  protected:
    DataStreamFile(const ResourceURL &url);

    U64 _Handle;

    friend class DataStreamManager;
};

// Abstract class which provides read and write functionality for statically sized paged streams. Override _ReadPage
// Writes and reads in batches under the hood, in which the sub class of this can then encrypt / compress that how they like.
class DataStreamDeferred : public DataStream
{
public:
    
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override;
    
    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override;

    virtual void SetPosition(U64 newPosition) override;
    
    inline virtual U64 GetPosition() override { return _PageIdx * _PageSize + _PagePos; }
    
    virtual ~DataStreamDeferred();
    
    inline virtual U64 GetSize() override { return _Size; }
    
protected:
    
    const U64 _PageSize;                // Size of each page
    U64 _PagePos;                 // Position in current page.
    U64 _PageIdx;                 // Current page index.
    U64 _Size;                    // Total written bytes
    
    // Override by subclass. Reads or writes bytes to the specified page index.
    virtual Bool _SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 pageOffset, Bool IsWrite) = 0;
    
    DataStreamDeferred(const ResourceURL &url, U64 pagesize);
    
};

// Memory data stream. A writable and readable growable buffer internally.
class DataStreamMemory : public DataStreamDeferred
{
  public:

    virtual ~DataStreamMemory();

  protected:
    
    virtual Bool _SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 pageOffset, Bool IsWrite) override; // gets from _PageTable

    DataStreamMemory(const ResourceURL &url, U64 pageSize = MEMORY_STREAM_DEFAULT_PAGESIZE);

    std::vector<U8 *> _PageTable; // Ordered table array.

    friend class DataStreamManager;
};

// Buffer data stream. A writable and readable statically sized buffer internally.
class DataStreamBuffer : public DataStream
{
  public:
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override;

    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override;

    // Provide more functionality to move around the seek position. NOTE: a value larger than the current size will expand the memory.
    virtual U64 GetPosition() override { return _Off; }

    virtual void SetPosition(U64 newPosition) override;

    inline virtual U64 GetSize() override { return _Size; }

    virtual ~DataStreamBuffer();

  protected:
    DataStreamBuffer(const ResourceURL &url, U64 size, U8 *pBuffer, Bool FreeAfter);
    // Optional buffer, if set will be deallocated with tte_free if not Free

    U8 *_Buffer;
    U64 _Off;
    U64 _Size;
    Bool _Owns;

    friend class DataStreamManager;
};

// Sub Stream. Reads and writes to a subsection of the parent stream.
class DataStreamSubStream : public DataStream
{
  public:
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override;

    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override;

    // Provide more functionality to move around the seek position. NOTE: a value larger than the current size will expand the memory.
    virtual U64 GetPosition() override { return _Off; }

    virtual void SetPosition(U64 newPosition) override;

    inline virtual U64 GetSize() override { return _Size; }

    virtual ~DataStreamSubStream();

  protected:
    // Parent stream must be seekable. Must be
    DataStreamSubStream(const DataStreamRef &parent, U64 pos, U64 size); // Specify section of parent stream

    U64 _BaseOff;
    U64 _Off;
    U64 _Size;
    DataStreamRef _Prnt;

    friend class DataStreamManager;
};

// Older game meta streams. Write mode always fails! Re-encryption not needed. Reads will appear unencrypted (returning MBIN/etc, not MBES)
class DataStreamLegacyEncrypted : public DataStreamDeferred
{
public:
    
    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override; // write is disabled
    
    inline virtual U64 GetSize() override { return _Prnt->GetSize() - _BaseOff; }
    
    inline virtual ~DataStreamLegacyEncrypted() {}
    
protected:
    
    void _GetBlock(U32); // puts block into read back buffer, for given block index.
    
    virtual Bool _SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 pageOffset, Bool IsWrite) override;

    // Parent stream must be seekable. Pass in block size and encryption frequencies
    // Also pass in the base offset where this stream starts in the parent stream (like a sub stream).
    DataStreamLegacyEncrypted(const DataStreamRef &parent, U64 parentOff, U16 block_size, U8 raw_freq, U8 bf_freq);
    
    DataStreamRef _Prnt; // parent stream we are reading from
    
    U64 _BaseOff; // offset start in parent stream
    
    U8 _RawFreq; // every how many blocks of data do we have an unencrypted block
    U8 _EncFreq; // every how many blocks of data do we have a blowfished block
    U8 _Rb[0x100]; // read back buffer (temp) (max block size is 256)
    
    friend class DataStreamManager;
};

/// Data stream containers are where you see the 'ZCTT/zCTT/NCTT/etc' file magic headers. These are not intrinsically related to TTARCH2 files or anything. They wrap another data stream.
/// They allow for it to be compressed, with either zlib or oodle, as well as encrypted. If they are encrypted, it must be compressed. They compress in blocks of a set uncompressed size,
/// pretty much always 65536 bytes. These bytes are compressed into a smaller block, and the offsets of each of these blocks as well as the number of them are stored at the beginning of the
/// stream in memory. Its a wrapper, so of course there is a mode where it does not compress or encrypt, just writes the bytes as is (NCTT - ideally meaning Telltale Container None or equivalent).
/// The magic is written in this mode, followed by the U64 total remaining size. If its one of the others, the magic determines only the compression, and encryption. The little or big E means its encrypted
/// and compressed. If its a little z or c there is a U32 after stating the compression type used, while if a big E or Z is used the compression is assumed zlib - and should be. If compressed, after the
/// magic and compression type there are a list of block offsets (not sizes). The last offset is the end of the file and the first offset is the start of the first compressed block in the file, relative to the offset
/// of the magic number (eg ECTT). The sizes of compressed blocks are calculated as just the difference of the block at the offset with the offset of the  block after, which is why the offset of the end of the
/// file is also stored in the array of offsets at the top of the stream. The uncompressed size is the same for each and also stored in the first few bytes of the stream.  Seeking is therefore quite easy, given
/// an offset in the uncompressed version to read bytes at that offset you just divide the offset by the block size to find the start block index. You then get that block by reading the compressed size and
/// decompressing it into a local buffer. You can then seek in that buffer to the modulo of the offset given with the block size. You can see here that reading is therefore a paged read, which is why we
/// used the deferred data stream and just use an overriden get page function to decompress pages. Note that if encrypted, obviously, we encrypt the compressed block not the unencrypted data - such that
/// we still obtain good compression ratios - although I see the logic here, Blowfish encryption is pretty repetitive on the same data - but for zeros - whicih in a lot of uncompressed data there are - we would
/// defintely want to compress that instead of the encrypted version. Therefore you first decrypt the compressed block, then decompress it. The compression used in all games is Zlib 1.2.8 and oodle
/// LZNA (rare, in some Batman archives). Note that all zlib compressed blocks have a negative block size (namely -15), meaning we raw compress it. The first byte of all compressed zlib streams
/// in telltale games is the byte 0xEB.
/// To write containers, use the flush container in the manager class below. Use this one to read from the stream with paged reads and a cached internally buffer.
class DataStreamContainer : public DataStreamDeferred
{
public:
    
    inline virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override { return false; }; // write is disabled
    
    // return 0 if any error
    inline virtual U64 GetSize() override { return _Valid ? _Size : 0; }
    
    inline virtual ~DataStreamContainer()
    {
        if(_CachedPage)
            TTE_FREE(_CachedPage);
        _CachedPage = nullptr;
        if(_IntPage)
            TTE_FREE(_IntPage);
        _IntPage = nullptr;
    }
    
    // returns if valid
    inline Bool IsValid()
    {
        return _Valid;
    }
    
protected:
    
    virtual Bool _SerialisePage(U64 index, U8 *Buffer, U64 Nbytes, U64 pageOffset, Bool IsWrite) override;
    
    // Parent stream must be seekable.
    DataStreamContainer(const DataStreamRef &parent);
    
    DataStreamRef _Prnt; // parent stream we are reading from
    
    Compression::Type _Compression; // compression type
    Bool _Encrypted; // is encrypted
    Bool _Compressed; // is compressed
    Bool _Valid; // is valid
    U64 _CachedPageIndex; // cached page buffer below
    U64 _DataOffsetStart; // compressed pages start or uncompressed data start
    U8* _CachedPage = nullptr; // cached current page
    U8* _IntPage = nullptr; // intermediate buffer for compression

    std::vector<U64> _PageOffsets; // for compressed. last element is end of file offset
    
    friend class DataStreamManager;
};

// Null stream. Writes and reads succeed but do nothing (albeit they will warn). Used in actual engine too
class DataStreamNull : public DataStream
{
public:
    
    inline virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override {
        TTE_LOG("WARN: Trying to read fro NULL data stream. Ignoring.");
        return true;
    }
    
    inline virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override {
        TTE_LOG("WARN: Trying to write to NULL data stream. Ignoring.");
        return true;
    }

    virtual U64 GetPosition() override { return 0; }
    
    virtual void SetPosition(U64 newPosition) override {}
    
    inline virtual U64 GetSize() override { return 0; }
    
    virtual ~DataStreamNull();
    
protected:
    
    DataStreamNull(const ResourceURL& url);
    
    friend class DataStreamManager;
};

///
/// You can only call Write() on this stream. Writes any buffers to all the children streams it contains. Children steams should not
/// wrap around and back reference this stream, as it would cause circular writing which would cause a stack overflow as it would go on forever.
///
class DataStreamAppendStream : public DataStream
{
public:
    
    // Pass in begin iterator if copying from vector.
    DataStreamAppendStream(const ResourceURL& url, DataStreamRef* pStreams, U32 N);
    
    inline virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override
    {
        TTE_ASSERT(false, "Cannot read from appending data stream");
        return false;
    }
    
    virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override;
    
    inline virtual U64 GetPosition() override
    {
        TTE_ASSERT(false, "DataStreamAppendStream::GetPosition() is unavailable");
        return 0;
    }
    
    inline virtual void SetPosition(U64) override
    {
        TTE_ASSERT(false, "DataStreamAppendStream::SetPosition(U64) is unavailable");
    }
    
    inline virtual U64 GetSize() override
    { // stream only meant for appending to multiple others.
        TTE_ASSERT(false, "DataStreamAppendStream::GetSize() is unavailable");
        return 0;
    }
    
    virtual ~DataStreamAppendStream() = default;

private:
    
    std::vector<DataStreamRef> _AppendStreams;
    
};

/// This stream reads from multiple streams. Calls to set and get position can be slow as well as get size, so try to only call once and then read.
class DataStreamSequentialStream : public DataStream
{
public:
    
    inline virtual Bool Write(const U8 *InputBuffer, U64 Nbytes) override
    {
        TTE_ASSERT(false, "DataStreamSequentialStream::Write() is unavailable"); // use append stream. only readable.
        return false;
    }
    
    /// Pushes a stream. Does not change current position. Don't change its size while reading from this stream after this call!
    inline virtual void PushStream(DataStreamRef& stream)
    {
        TTE_ASSERT(stream.get() != this, "Cannot push this stream as a child stream");
        if(stream)
            _OrderedStreams.push_back(stream);
    }
    
    DataStreamSequentialStream(const ResourceURL& url);
    
    virtual Bool Read(U8 *OutputBuffer, U64 Nbytes) override;
    
    virtual U64 GetPosition() override;
    
    virtual void SetPosition(U64) override;
    
    virtual U64 GetSize() override;
    
    virtual ~DataStreamSequentialStream() = default;
    
private:
    
    U32 _StreamIndex = 0; // current stream in ordered streams
    U64 _BytesRead = 0; // total bytes read from 0 from first 0, ie total position.
    U64 _StreamPos = 0; // position in streamIndex stream
    std::vector<DataStreamRef> _OrderedStreams{};
    
};

// Parameters for creating containers
struct ContainerParams
{
    Bool Encrypt = false; // if true, compression should be set (else default zlib is used)
    Compression::Type Compression = Compression::Type::END_LIBRARY;
};

// This manages lifetimes of data streams, finding URLs, opening files, and everything related to sources and destinations of byte streams.
class DataStreamManager
{
  public:
    // Creates a file stream from the given resource URL.
    DataStreamRef CreateFileStream(const ResourceURL &path);

    // Creates a file stream to a temporary file on disk.
    DataStreamRef CreateTempStream();
    
    // Creates a null stream which does nothing on reads and writes (does warn)
    DataStreamRef CreateNullStream();

    // Creates a memory stream that is a fixed size (DataStreamBuffer). Optionally pass a pre-allocated buffer, which will be TTE_FREE'd after.
    // If you pass in your own buffer, set the last argument to true such that it will get deleted with free after.
    DataStreamRef CreateBufferStream(const ResourceURL &path, U64 Size, U8 *pPreAllocated, Bool bTransferOwnership);

    // Creates a sub stream which reads from the specific part of the parent stream. The parent stream must be seekable (ie not network etc).
    DataStreamRef CreateSubStream(const DataStreamRef &parent, U64 offset, U64 size);
    
    // Creates a legacy encrypted reading stream for old meta streams. Only should be used by meta stream. Starts reading from base offset.
    // Base offset should be such that its after the magic (eg MBES) and you should pass in the correct block size and frequencies.
    DataStreamRef CreateLegacyEncryptedStream(const DataStreamRef& src, U64 baseOffset, U16 blockSize, U8 rawf, U8 blowf);
    
    // Creates a wrapper container data stream. This is used for archives (.ttarch2), shaders and meta stream sections sometimes.
    DataStreamRef CreateContainerStream(const DataStreamRef& src);

    // Creates a cached stream, such that all of src lies within memory, speeding up reads for smaller files. If src is already
    // a cached stream (DataStreamBuffer/Memory) then it return src
    DataStreamRef CreateCachedStream(const DataStreamRef& src);
    
    // Resolves a symbol. Will return the resource URL with the full path. If not, returns an invalid stream because it was not found.
    ResourceURL Resolve(const Symbol &sym);

    // Searches cached (memory) files for the given path, in the cache scheme.
    std::shared_ptr<DataStreamMemory> FindCache(const String &path);
    
    // Creates a sequential stream used for reading from multiple sources. Pass in the temporary URL.
    std::shared_ptr<DataStreamSequentialStream> CreateSequentialStream(const String& path);

    // Creates a new private cache memory stream. Once you destroy the return value, the stream is deleted. Ie, CreateMemoryStream.
    std::shared_ptr<DataStreamMemory> CreatePrivateCache(const String &path, U32 pageSize = MEMORY_STREAM_DEFAULT_PAGESIZE);

    // Creates a new public cache*d* memory stream. FindCache will return this pointer. Call ReleaseCache to stop that, so it gets deleted.
    // It is publicly accessible after this call not just by the returned value. If a cached file at the path exists, it is replaced -
    // any subsequent calls to FindCache returns this new returned stream.
    std::shared_ptr<DataStreamMemory> CreatePublicCache(const String &path, U32 pageSize = MEMORY_STREAM_DEFAULT_PAGESIZE);
    
    // Writes and fully flushes a data stream from the source stream, reading Nbytes, into the output destination stream, with parameters.
    // Passes out the JOB HANDLE. This runs asynchronously this can take a lot of time for large game data. Returns false if invalid inputs.
    // Wait on the output job. NOTE: If this returns true but outJob is 0, then it was just done on the main thread as no need for async.
    // If an async job is pushed, YOU MUST NOT access source or dest UNTIL the job finishes! If you want to finish asap, just wait on it
    // immidiately after this call if needed.
    Bool FlushContainer(DataStreamRef& src, U64 Nbytes, DataStreamRef& dst, ContainerParams params, JobHandle& outJob);

    // Releases the internal reference such that FindCache after this call will not return the stream anymore.
    // Only call after CreatePublicCache.
    void ReleaseCache(const String &path);

    // Makes a private cache memory stream publicly accessible by FindCache, adding it to the internal map if it doesn't exist already.
    void Publicise(std::shared_ptr<DataStreamMemory> &stream);

    // Transfers bytes from the source stream to the destination stream in chunks. Ensure source and destination are correctly
    // positioned before this call such that you copy the data which you want. Set Nbytes to the number of bytes you want to copy.
    // Returns false if a Read or Write to source or dest failed.
    Bool Transfer(DataStreamRef &src, DataStreamRef &dst, U64 Nbytes);
    
    // Reads a string from the data stream argument. Reads the size, then ASCII string. Checks if size is valid.
    String ReadString(DataStreamRef& stream);
    
    // Writes a string to the data stream argument. Writes the size, then the ASCII string.
    void WriteString(DataStreamRef& stream, const String& str);
    
    // Write N bytes to the given stream
    void WriteZeros(DataStreamRef& stream, U64 N);

    static void Initialise();

    static void Shutdown();

    inline static DataStreamManager *GetInstance() { return Instance; }

  private:
    // Map of cache URL paths to cached streams. Not thread safe, operations on each memory stream must not be done simulateously.
    std::map<String, std::shared_ptr<DataStreamMemory>> _Cache;

    static DataStreamManager *Instance;
};
