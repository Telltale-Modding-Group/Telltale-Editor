#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <queue>

// =================================================================== LIBRARY CONFIGURATION ===================================================================

#define TTE_VERSION "1.0.0"

// DEBUG or RELEASE must be defined (not to 0, just either defined or not defined)

#if defined(DEBUG) == defined(RELEASE)
#error "Neither or both debug or release configuration macros defined!"
#endif

// ===================================================================        LOGGING        ===================================================================

#ifdef DEBUG

// In DEBUG, simply log any messages simply to the console. Adds a newline character. This is a workaround so empty VA_ARGS works ok. If changing printf, change assert to.
#define _TTE_LOG(fmt, ...) printf(fmt "%s", __VA_ARGS__);
#define TTE_LOG(...) _TTE_LOG(__VA_ARGS__, "\n")

#else

// In RELEASE, no logging.
#define TTE_LOG(_, ...) ;

#endif

// ===================================================================        ASSERTS        ===================================================================

#ifdef DEBUG

// First argument is expression to test, second is format string (optional) then optional format string arguments
#define TTE_ASSERT(EXPR, ...)        \
	if (!(EXPR))                     \
	{                                \
		_TTE_LOG(__VA_ARGS__, "\n"); \
		DebugBreak();                \
	}

#else

// In RELEASE, ignore assertions.
#define TTE_ASSERT(EXPR, MESSAGE, ...) ;

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

using Float = float;

using String = std::string;

using Bool = bool;

// ===================================================================   PLATFORM SPECIFICS   ===================================================================

#if defined(_WIN64)

// Windows Platform Specifics
#include "Platform/Win64.hpp"

#elif defined(MACOS)

// MacOS Platform Specifics
#include "Platform/MacOS.hpp"

#elif defined(LINUX)

// Linux Platform Specifics | TODO PROTON
#include "Platform/Linux.hpp"

#else

#error "Unknown platform!"

#endif

// ===================================================================         UTILS         ===================================================================

#define MAX(A, B) (((A) > (B)) ? (A) : (B))

#define MIN(A, B) (((A) < (B)) ? (A) : (B))

// Helper class. std::priority_queue normally does not let us access by finding elements. Little hack to bypass and get internal vector container.
template <typename T>
class hacked_priority_queue : public std::priority_queue<T>
{ // Note applying library convention, see this as an 'extension' to std::
public:
	std::vector<T> &get_container()
	{
		return this->c;
	}

	const std::vector<T> &get_container() const
	{
		return this->c;
	}

	auto get_cmp()
	{
		return this->comp;
	}
};
