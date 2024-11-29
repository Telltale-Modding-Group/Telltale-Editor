#include <Config.hpp>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

void ThreadSleep(U64 milliseconds)
{
    (void)sleep(milliseconds / 1000);
}

void SetThreadName(const String &tName)
{
    (void)pthread_setname_np(pthread_self(), tName.c_str());
}

void DebugBreak()
{
    (void)raise(SIGINT);
}