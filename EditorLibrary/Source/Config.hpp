#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>

// =================================================================== LIBRARY CONFIGURATION ===================================================================

#define TTE_VERSION "1.0.0"

// DEBUG or RELEASE must be defined (not to 0, just either defined or not defined)

#if defined(DEBUG) == defined(RELEASE)
#error "Neither or both debug or release configuration macros defined!"
#endif

// ===================================================================        LOGGING        ===================================================================

#ifdef DEBUG

// In DEBUG, simply log any messages simply to the console.
#define TTE_LOG(MESSAGE, ...) ::printf(MESSAGE, __VA_ARGS__)

#else

// In RELEASE, no logging.
#define TTE_LOG(_, ...) ;

#endif

// ===================================================================   PLATFORM SPECIFICS   ===================================================================

#if defined(_WIN64)

// Windows Platform Specifics

#elif defined(MACOS)

// MacOS Platform Specifics

#elif defined(LINUX)

// Linux Platform Specifics | TODO PROTON

#else

#error "Unknown platform!"

#endif

// ===================================================================         TYPES         ===================================================================

using U8 = uint8_t;
using I8 = int8_t;
using U16 = uint16_t;
using I16 = int16_t;
using U32 = uint32_t;
using I32 = int32_t;
using U64 = uint64_t;
using I64 = int64_t;

using String = std::string;

using Bool = bool;
