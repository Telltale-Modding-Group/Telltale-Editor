#include <Config.hpp>
#include <unistd.h>

void ThreadSleep(U64 milliseconds)
{
    (void)sleep(milliseconds / 1000);
}

void SetThreadName(const String &tName) // TODO: Implement, might need to seperate linux and mac
{
}

void DebugBreak() // TODO: Implement
{
}