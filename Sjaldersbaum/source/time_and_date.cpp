#include "time_and_date.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std::chrono;

#pragma warning(disable:4996) // To use std::localtime.

/*------------------------------------------------------------------------------------------------*/

long long Time::get_absolute_ns()
{
    static high_resolution_clock clock;
    return duration_cast<nanoseconds>(clock.now().time_since_epoch()).count();
}

std::string Time::get_date()
{
    const auto now = system_clock::now();
    const auto time_t = system_clock::to_time_t(now);

    std::stringstream buffer;
    buffer << std::put_time(std::localtime(&time_t), "%Y.%m.%d");

    return buffer.str();
}

std::string Time::get_time()
{
    const auto now  = system_clock::now();
    const auto time = system_clock::to_time_t(now);

    std::stringstream buffer;
    buffer << std::put_time(std::localtime(&time), "%H.%M.%S");

    return buffer.str();
}

std::string Time::get_date_and_time()
{
    return get_date() + '.' + get_time();
}