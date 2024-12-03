#include "Config.hpp"

#ifdef _WIN64 // Only compile this file in windows!

// Windows workaround for set thread name. See below link =========================================================
// https://learn.microsoft.com/en-gb/previous-versions/visualstudio/visual-studio-2015/debugger/how-to-set-a-thread-name-in-native-code?view=vs-2015&redirectedfrom=MSDN

#pragma pack(push,8)  
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.  
	LPCSTR szName; // Pointer to name (in user addr space).  
	DWORD dwThreadID; // Thread ID (-1=caller thread).  
	DWORD dwFlags; // Reserved for future use, must be zero.  
} THREADNAME_INFO;
#pragma pack(pop)  

void _SetWindowsThreadName(const String& tName)
{
	THREADNAME_INFO info{};
	info.dwType = 0x1000;
	info.szName = tName.c_str();
	info.dwThreadID = (DWORD)-1;
	info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {
		RaiseException(0x406D1388U, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)  
}

// =============================================================================================================

#endif
