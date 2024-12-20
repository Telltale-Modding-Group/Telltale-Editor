#include <Core/Config.hpp>
#include <Windows.h>
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <string>
#include <vector>

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

void ThreadSleep(U64 milliseconds) { ::Sleep(milliseconds); }

void DebugBreakpoint() { __debugbreak(); }

// FILE STREAMS

U64 FileOpen(CString path)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    std::vector<wchar_t> wpath(wlen);
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath.data(), wlen);

    HANDLE file = CreateFileW(wpath.data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    TTE_ASSERT(file != INVALID_HANDLE_VALUE, "Could not open %s => Windows err %d", path, GetLastError());

    return (U64)file;
}

void FileClose(U64 Handle)
{
    HANDLE file = (HANDLE)Handle;
    CloseHandle(file);
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

    DWORD bytes_read = 0;
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
    /*HANDLE file = (HANDLE)Handle;

    LARGE_INTEGER size;
    BOOL success = GetFileSizeEx(file, &size);

    TTE_ASSERT(success, "Could not get file size. Windows error: %d", GetLastError());

    return (U64)size.QuadPart;*/

    HANDLE hFile = (HANDLE)Handle;
    LARGE_INTEGER cur = {0};
    LARGE_INTEGER size = {0};
    
    TTE_ASSERT(SetFilePointerEx(hFile, (LARGE_INTEGER){.QuadPart = 0}, &cur, FILE_CURRENT),
               "Could not get current file pointer offset");

    TTE_ASSERT(SetFilePointerEx(hFile, (LARGE_INTEGER){.QuadPart = 0}, &size, FILE_END),
               "Could not seek to file end offset");

    TTE_ASSERT(SetFilePointerEx(hFile, cur, NULL, FILE_BEGIN),
               "Could not restore file pointer to original offset");

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