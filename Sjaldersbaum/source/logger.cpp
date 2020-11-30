#include "logger.h"

#include <fstream>
#include <iostream>

/*------------------------------------------------------------------------------------------------*/

// For ResourceManager debugging:

bool RESOURCE_LOGGING = false;

const std::string LOG_PATH = "latest_log.txt";

/*------------------------------------------------------------------------------------------------*/

Logger::Logger()
{
    // Redirect std::cout/cerr to the local buffer:
    std::cout.rdbuf(new_input.rdbuf());
    std::cerr.rdbuf(new_input.rdbuf());
}

Logger::~Logger()
{
    std::ofstream file;
    file.open(LOG_PATH, std::ios_base::out | std::ios_base::trunc);
    if (file)
        file << input.str();
}

void Logger::write(std::string&& str)
{
    new_input << str;
    input     << str;
}

std::string Logger::extract_new_input()
{
    const std::string str = new_input.str();

    new_input.str(std::string());
    new_input.clear();

    return str;
}