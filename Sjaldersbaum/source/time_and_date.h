#pragma once

#include <string>

/*------------------------------------------------------------------------------------------------*/

// The Time namespace provides easier access to common time related functions.
namespace Time
{
// Returns the absolute time in nanoseconds.
long long get_absolute_ns();

// Gets current system date in the following format:
// Year.Month.Day
// Example: 2000.12.30
std::string get_date();

// Gets current system time in the following format:
// Hour.Minute.Second
// Example: 06.30.00
std::string get_time();

// Returns current system date and time in the following format:
// Year.Month.Day.Hour.Minute.Second
std::string get_date_and_time();
}