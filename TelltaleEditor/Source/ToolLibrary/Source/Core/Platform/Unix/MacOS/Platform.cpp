#include <Core/Config.hpp>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>

// THREAD

void ThreadSleep(U64 milliseconds)
{
    struct timespec time = {.tv_sec = static_cast<time_t>(milliseconds / 1000), .tv_nsec = static_cast<time_t>((milliseconds % 1000) * 1000000)};
    (void)nanosleep(&time, nullptr);
}

void SetThreadName(const String &tName) { (void)pthread_setname_np(tName.c_str()); }

void DebugBreakpoint() { (void)raise(SIGINT); }

// FILE STREAMS

U64 FileOpen(CString path){
    int file = open(path, O_RDWR | O_CREAT, 0777);
    TTE_ASSERT(file != -1, "Could not open %s => posix err %d", path, errno);
    return (U64)file;
}

void FileClose(U64 Handle){
    close((int)Handle);
}

Bool FileWrite(U64 Handle, const U8* Buffer, U64 Nbytes){
    int fd = (int)Handle;
    
    ssize_t bytes_written = write(fd, Buffer, (size_t)Nbytes);
    
    if (bytes_written == -1) {
        TTE_ASSERT(false, "Could not write to file");
        return false;
    }
    
    return bytes_written == (ssize_t)Nbytes;
}

Bool FileRead(U64 Handle, U8* Buffer, U64 Nbytes){
    int fd = (int)Handle;
    
    ssize_t bytes_read = read(fd, Buffer, (size_t)Nbytes);
    
    // Check for errors
    if (bytes_read == -1) {
        TTE_ASSERT(false, "Could not read from file");
        return false;
    }

    return bytes_read == (ssize_t)Nbytes;
}

U64 FileSize(U64 Handle){
    int f = (int)Handle;
    
    off_t cur = lseek(f, 0, SEEK_CUR);
    TTE_ASSERT(cur != (off_t)-1, "Could not seek in file to cache offset");
    
    off_t size = lseek(f,0,SEEK_END);
    TTE_ASSERT(size != (off_t)-1, "Could not seek in file to end offset");
    
    TTE_ASSERT(lseek(f, cur, SEEK_SET) != (off_t)-1, "Could not re-seek to cache offset");
    
    return (U64)size;
}

U64 FileNull() {
    return 0x0000'0000'FFFF'FFFFull;
}

String FileNewTemp(){
    char tmplt[] = "/tmp/tteditorMACOS_XXXXXX";
    
    if (mktemp(tmplt) == NULL) {
        TTE_ASSERT(false,"Call to mktemp failed");
        return "";
    }
    
    return String(tmplt);
}
