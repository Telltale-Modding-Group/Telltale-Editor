#pragma once

#include <Core/Config.hpp>

/**
 * @brief Sleeps the current thread for @p milliseconds time
 *
 * @param milliseconds duration in milliseconds
 */
void ThreadSleep(U64 milliseconds);

/**
 * @brief Sets the name of the current thread to @p tName
 *
 * @param tName name
 */
void SetThreadName(const String &tName);
