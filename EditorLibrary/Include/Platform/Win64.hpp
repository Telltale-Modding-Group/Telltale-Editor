#pragma once

#ifndef _WIN64
#error "Must be building in Windows64 to be able to include this header"
#endif

#include <Windows.h> // std. windows header

// Platform name for future use
#define PLATFORM_NAME "Windows"

// Break the debugger
#define DEBUG_BREAK() __debugbreak()

// Cause the current thread to sleep MILLIS (U32) milliseconds
#define THREAD_SLEEP(MILLIS) ::Sleep(MILLIS)
 
// Internal windows thread name function, see Win64.cpp. Needs a small workaround.
void _SetWindowsThreadName(const String& tName);

// Set the current thread name for debugging, as as String type
#define SET_THREAD_NAME(STR) _SetWindowsThreadName(STR)
