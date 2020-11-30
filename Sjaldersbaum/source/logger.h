#pragma once

#include <string>
#include <sstream>

/*------------------------------------------------------------------------------------------------*/

#define LOG(str) Logger::instance().write(std::string("\n") + str + "\n")

#define LOG_INTEL(info)   \
Logger::instance().write( \
    std::string(          \
        "\n----------------------------------------------------------------------\n") + \
        '<' + __FUNCTION__ + ">\n" + info + '\n')

#define LOG_ALERT(warning) \
Logger::instance().write(  \
    std::string(           \
        "\n######################################################################\n") + \
        '<' + __FUNCTION__ + ">\n" + warning + '\n')

/*------------------------------------------------------------------------------------------------*/

// Interface to a std::stringstream, which's content automatically saves to a file upon its death.
class Logger
{
public:
    static Logger& instance()
    {
        static Logger singleton;
        return singleton;
    }

    void write(std::string&& str);

    // Specifically for DebugWindow.
    std::string extract_new_input();

private:
    std::stringstream input;
    std::stringstream new_input;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;
};