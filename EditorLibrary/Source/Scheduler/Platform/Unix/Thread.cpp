#include <Config.hpp>
#include <unistd.h>
#include <pthread.h>

void ThreadSleep(U64 milliseconds)
{
    (void)sleep(milliseconds / 1000);
}

void SetThreadName(const String &tName)
{
    pthread_setname_np(pthread_self(), tName.c_str());
}

void DebugBreak() // TODO: Implement, might need to seperate linux and mac
{
}