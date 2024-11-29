#pragma once

#ifndef MACOS
#error "Must be building in MacOS to be able to include this header"
#endif

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

// Platform name for future use
#define PLATFORM_NAME "MacOS"

// Break the debugger
#define DEBUG_BREAK() raise(SIGINT)

// Cause the current thread to sleep MILLIS (U32) milliseconds
#define THREAD_SLEEP(MILLIS) ::usleep(MILLIS * 1000)

// Set the current thread name for debugging, as as String type. TODO
#define SET_THREAD_NAME(STR) ;
