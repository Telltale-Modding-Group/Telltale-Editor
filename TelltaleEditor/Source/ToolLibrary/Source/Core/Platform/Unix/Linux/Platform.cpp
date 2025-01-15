#include <Core/Config.hpp>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/mman.h>

void ThreadSleep(U64 milliseconds)
{
    struct timespec time = {.tv_sec = static_cast<time_t>(milliseconds / 1000), .tv_nsec = static_cast<time_t>((milliseconds % 1000) * 1000000)};
    (void)nanosleep(&time, nullptr);
}

void SetThreadName(const String &tName) { (void)pthread_setname_np(pthread_self(), tName.c_str()); }

void DebugBreakpoint() { (void)raise(SIGINT); }

U64 FileOpen(CString path)
{
    int file = open(path, O_RDWR | O_CREAT, 0777);
    TTE_ASSERT(file != -1, "Could not open %s => posix err %d", path, errno);
    return (U64)file;
}

void FileClose(U64 Handle) { close((int)Handle); }

Bool FileWrite(U64 Handle, const U8 *Buffer, U64 Nbytes)
{
    int fd = (int)Handle;

    ssize_t bytes_written = write(fd, Buffer, (size_t)Nbytes);

    if (bytes_written == -1)
    {
        TTE_ASSERT(false, "Could not write to file");
        return false;
    }

    return bytes_written == (ssize_t)Nbytes;
}

Bool FileRead(U64 Handle, U8 *Buffer, U64 Nbytes)
{
    int fd = (int)Handle;

    ssize_t bytes_read = read(fd, Buffer, (size_t)Nbytes);

    // Check for errors
    if (bytes_read == -1)
    {
        TTE_ASSERT(false, "Could not read from file");
        return false;
    }

    return bytes_read == (ssize_t)Nbytes;
}

U64 FileSize(U64 Handle)
{
    int f = (int)Handle;

    off_t cur = lseek(f, 0, SEEK_CUR);
    TTE_ASSERT(cur != (off_t)-1, "Could not seek in file to cache offset");

    off_t size = lseek(f, 0, SEEK_END);
    TTE_ASSERT(size != (off_t)-1, "Could not seek in file to end offset");

    TTE_ASSERT(lseek(f, cur, SEEK_SET) != (off_t)-1, "Could not re-seek to cache offset");

    return (U64)size;
}

U64 FileNull() { return 0x0000'0000'FFFF'FFFFull; }

String FileNewTemp()
{
    char tmplt[] = "/tmp/tteditorLINUX_XXXXXX";

    if (mktemp(tmplt) == NULL)
    {
        TTE_ASSERT(false, "Call to mktemp failed");
        return "";
    }

    return String(tmplt);
}

U64 FilePos(U64 Handle) {
    return (U64)lseek((int)Handle, 0, SEEK_CUR);
}

void FileSeek(U64 Handle, U64 Offset){
    lseek((int)Handle, Offset, SEEK_SET);
}

static void* OodleLibrary = nullptr; // can be static. only loaded at init.

void OodleOpen(void*& c, void*& d)
{
    if(OodleLibrary == nullptr)
    {
        const char* lib;
#ifdef __x86_64
        lib = "oodle_linux_x86.so";
#elif
        lib = "oodle_linux_arm.so";
#endif
        OodleLibrary = dlopen(lib, RTLD_LAZY);
        const char* error = dlerror();
        if (error)
        {
            TTE_LOG("Cannot load oodle library: does not exist"); // error string is very long, just says all places it has searched
            return;
        }
    }
    
    c = dlsym(OodleLibrary, "OodleLZ_Compress");
    const char* error = dlerror();
    if (error)
    {
        TTE_LOG("Cannot load oodle compress: %s", error);
        return;
    }
    
    d = dlsym(OodleLibrary, "OodleLZ_Decompress");
    error = dlerror();
    if (error)
    {
        TTE_LOG("Cannot load oodle decompress: %s", error);
        return;
    }
}

void OodleClose()
{
    if(OodleLibrary)
    {
        dlclose(OodleLibrary);
    }
    OodleLibrary = nullptr;
}

U8* AllocateAnonymousMemory(U64 size)
{
    void* region = mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        perror("mmap failed");
        return nullptr;
    }
    
    return (U8*)region;
}

void FreeAnonymousMemory(U8* ptr, U64 sz)
{
    munmap(ptr, sz);
}