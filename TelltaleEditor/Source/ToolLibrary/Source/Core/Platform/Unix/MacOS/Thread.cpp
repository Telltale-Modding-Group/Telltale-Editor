#include <Core/Config.hpp>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

void ThreadSleep(U64 milliseconds)
{
    struct timespec time = {.tv_sec = static_cast<time_t>(milliseconds / 1000), .tv_nsec = static_cast<time_t>((milliseconds % 1000) * 1000000)};
    (void)nanosleep(&time, nullptr);
}

void SetThreadName(const String &tName) { (void)pthread_setname_np(tName.c_str()); }

void DebugBreakpoint() { (void)raise(SIGINT); }