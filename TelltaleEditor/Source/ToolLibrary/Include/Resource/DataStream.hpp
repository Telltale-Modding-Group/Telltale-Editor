#pragma once

#include <Core/Config.hpp>
#include <Core/Symbol.hpp>
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
    ResourceURL(const String &path);

    // Constructs with the symbol scheme. The resource URL that points to that symbol. If GetScheme() is still symbol, it was not found.
    // You can still call open and another find attempt is made, so Open() may succeed if it now exists.
    // If it is not this, then it was found and you can open it.
    ResourceURL(const Symbol &symbol);

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
    DataStreamBuffer(const ResourceURL &url, U64 size, U8 *pBuffer = nullptr); // Optional buffer, if set will be deallocated with tte_free

    U8 *_Buffer;
    U64 _Off;
    U64 _Size;

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

    // Parent stream must be seekable. Parent stream offset 0 must be start of the stream. Pass in block size and encryption frequencies
    // Also pass in the base offset where this stream starts in the parent stream (like a sub stream).
    DataStreamLegacyEncrypted(const DataStreamRef &parent, U64 parentOff, U16 block_size, U8 raw_freq, U8 bf_freq);
    
    DataStreamRef _Prnt; // parent stream we are reading from
    
    U64 _BaseOff; // offset start in parent stream
    
    U8 _RawFreq; // every how many blocks of data do we have an unencrypted block
    U8 _EncFreq; // every how many blocks of data do we have a blowfished block
    U8 _Rb[0x100]; // read back buffer (temp) (max block size is 256)
    
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
    DataStreamRef CreateBufferStream(const ResourceURL &path, U64 Size, U8 *pPreAllocated = nullptr);

    // Creates a sub stream which reads from the specific part of the parent stream. The parent stream must be seekable (ie not network etc).
    DataStreamRef CreateSubStream(const DataStreamRef &parent, U64 offset, U64 size);
    
    // Creates a legacy encrypted reading stream for old meta streams. Only should be used by meta stream. Starts reading from base offset.
    // Base offset should be such that its after the magic (eg MBES) and you should pass in the correct block size and frequencies.
    DataStreamRef CreateLegacyEncryptedStream(const DataStreamRef& src, U64 baseOffset, U16 blockSize, U8 rawf, U8 blowf);

    // Resolves a symbol. Will return the resource URL with the full path. If not, returns an invalid stream because it was not found.
    ResourceURL Resolve(const Symbol &sym);

    // Searches cached (memory) files for the given path, in the cache scheme.
    std::shared_ptr<DataStreamMemory> FindCache(const String &path);

    // Creates a new private cache memory stream. Once you destroy the return value, the stream is deleted. Ie, CreateMemoryStream.
    std::shared_ptr<DataStreamMemory> CreatePrivateCache(const String &path, U32 pageSize = MEMORY_STREAM_DEFAULT_PAGESIZE);

    // Creates a new public cache*d* memory stream. FindCache will return this pointer. Call ReleaseCache to stop that, so it gets deleted.
    // It is publicly accessible after this call not just by the returned value. If a cached file at the path exists, it is replaced -
    // any subsequent calls to FindCache returns this new returned stream.
    std::shared_ptr<DataStreamMemory> CreatePublicCache(const String &path, U32 pageSize = MEMORY_STREAM_DEFAULT_PAGESIZE);

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

    static void Initialise();

    static void Shutdown();

    inline static DataStreamManager *GetInstance() { return Instance; }

  private:
    // Map of cache URL paths to cached streams. Not thread safe, operations on each memory stream must not be done simulateously.
    std::map<String, std::shared_ptr<DataStreamMemory>> _Cache;

    static DataStreamManager *Instance;
};
