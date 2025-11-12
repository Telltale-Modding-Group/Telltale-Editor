#include <Core/Config.hpp>
#include <Windows.h>
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <string>
#include <vector>

#include <dxcapi.h>

// Windows workaround for set thread name. See below link =========================================================
// https://learn.microsoft.com/en-gb/previous-versions/visualstudio/visual-studio-2015/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2015&redirectedfrom=MSDN

#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;     // Must be 0x1000.
    LPCSTR szName;    // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(const String &tName)
{
    THREADNAME_INFO info{};
    info.dwType = 0x1000;
    info.szName = tName.c_str();
    info.dwThreadID = (DWORD)-1;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
    __try
    {
        RaiseException(0x406D1388U, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}

void PlatformMessageBoxAndWait(const String& title, const String& message)
{
    MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void ThreadSleep(U64 milliseconds) { ::Sleep(milliseconds); }

void DebugBreakpoint() { __debugbreak(); }

// FILE STREAMS

U64 FileOpen(CString path)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    std::vector<wchar_t> wpath(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath.data(), wlen);

    HANDLE file = CreateFileW(wpath.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(file == INVALID_HANDLE_VALUE)
    {
        TTE_LOG("Could not open %s => Windows err %d", path, GetLastError());
        return FileNull();
    }

    return (U64)file;
}

void FileClose(U64 Handle, U64 truncateOffset) 
{
    HANDLE hFile = (HANDLE)(uintptr_t)Handle;

    if (truncateOffset > 0) {
        LARGE_INTEGER li{};
        li.QuadPart = truncateOffset;
        if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN))
	{
            SetEndOfFile(hFile);
        }
    }

    CloseHandle(hFile);
}

Bool FileWrite(U64 Handle, const U8 *Buffer, U64 Nbytes)
{
    HANDLE file = (HANDLE)Handle;

    DWORD bytes_written = 0;
    BOOL success = WriteFile(file, Buffer, (DWORD)Nbytes, &bytes_written, NULL);

    if (!success)
    {
        TTE_ASSERT(false, "Could not write to file. Windows error: %d", GetLastError());
        return false;
    }

    return bytes_written == Nbytes;
}

Bool FileRead(U64 Handle, U8 *Buffer, U64 Nbytes)
{
    HANDLE file = (HANDLE)Handle;

    DWORD bytes_read{0};
    BOOL success = ReadFile(file, Buffer, (DWORD)Nbytes, &bytes_read, NULL);

    if (!success)
    {
        TTE_ASSERT(false, "Could not read from file. Windows error: %d", GetLastError());
        return false;
    }

    return bytes_read == Nbytes;
}

U64 FileSize(U64 Handle)
{
    HANDLE hFile = (HANDLE)Handle;
    LARGE_INTEGER cur{0};
    LARGE_INTEGER size{0};

    TTE_ASSERT(SetFilePointerEx(hFile, LARGE_INTEGER{0}, &cur, FILE_CURRENT), "Could not get current file pointer offset");

    TTE_ASSERT(SetFilePointerEx(hFile, LARGE_INTEGER{0}, &size, FILE_END), "Could not seek to file end offset");

    TTE_ASSERT(SetFilePointerEx(hFile, cur, nullptr, FILE_BEGIN), "Could not restore file pointer to original offset");

    return (U64)size.QuadPart;
}

U64 FileNull() { return (U64)INVALID_HANDLE_VALUE; }

String FileNewTemp()
{
    wchar_t temp_path[MAX_PATH];
    DWORD path_len = GetTempPathW(MAX_PATH, temp_path);
    TTE_ASSERT(path_len > 0 && path_len < MAX_PATH, "Could not get temp path. Windows error: %d", GetLastError());

    wchar_t temp_file[MAX_PATH];
    UINT result = GetTempFileNameW(temp_path, L"tte", 0, temp_file);
    TTE_ASSERT(result != 0, "Could not create temp file. Windows error: %d", GetLastError());

    int utf8len = WideCharToMultiByte(CP_UTF8, 0, temp_file, -1, NULL, 0, NULL, NULL);
    std::vector<char> utf8path(utf8len);
    WideCharToMultiByte(CP_UTF8, 0, temp_file, -1, utf8path.data(), utf8len, NULL, NULL);

    return String(utf8path.data());
}

U64 FilePos(U64 Handle)
{
    HANDLE hFile = (HANDLE)Handle;
    LARGE_INTEGER zero{0};
    LARGE_INTEGER pos{0};

    BOOL success = SetFilePointerEx(hFile, zero, &pos, FILE_CURRENT);
    TTE_ASSERT(success, "Could not get current file position. Windows error: %d", GetLastError());

    return (U64)pos.QuadPart;
}

void FileSeek(U64 Handle, U64 Offset)
{
    HANDLE hFile = (HANDLE)Handle;
    LARGE_INTEGER move;
    move.QuadPart = Offset;

    BOOL success = SetFilePointerEx(hFile, move, NULL, FILE_BEGIN);
    TTE_ASSERT(success, "Could not set file position. Windows error: %d", GetLastError());
}

static HMODULE OodleLibrary = nullptr; // can be static. only loaded at init.

void OodleOpen(void*& c, void*& d)
{
    if(OodleLibrary == nullptr)
    {
        OodleLibrary = LoadLibraryA("oodle_win_x86.dll");
        if (OodleLibrary == nullptr)
        {
            TTE_LOG("Cannot load oodle library: does not exist");
            return;
        }
    }
    
    c = (void*)GetProcAddress(OodleLibrary, "OodleLZ_Compress");
    if (!c)
    {
        TTE_LOG("Cannot load oodle compress function from DLL");
        return;
    }
    
    d = GetProcAddress(OodleLibrary, "OodleLZ_Decompress");
    if (!d)
    {
        TTE_LOG("Cannot load oodle compress function from DLL");
        return;
    }
}

void OodleClose()
{
    if(OodleLibrary)
    {
        FreeLibrary(OodleLibrary);
    }
    OodleLibrary = nullptr;
}

U8* AllocateAnonymousMemory(U64 size)
{
    void* region = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READONLY);
    if (region == nullptr) {
        TTE_LOG("Failed to allocate %d anonymous zero memory bytes in windows!", (U32)size);
        return nullptr;
    }

    return (U8*)region;
}

void FreeAnonymousMemory(U8* ptr, U64 size)
{
    if (ptr)
        VirtualFree(ptr, 0, MEM_RELEASE);
}

