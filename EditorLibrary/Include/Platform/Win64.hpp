#pragma once

#ifndef _WIN64
#error "Must be building in Windows64 to be able to include this header"
#endif

#include <Windows.h> // std. windows header

// Platform name for future use
#define PLATFORM_NAME "Windows"
