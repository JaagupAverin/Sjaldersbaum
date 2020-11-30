#pragma once

#include <unordered_map>
#include <sstream>

#include <SFML/System.hpp>

#include "string_assist.h"
#include "contains.h"
#include "logger.h"

/*------------------------------------------------------------------------------------------------*/
// SFML operators:

template<typename Numeral>
inline std::ostream& operator<<(std::ostream& os, const sf::Vector2<Numeral> vec2)
{
    os << vec2.x << ", " << vec2.y;
    return os;
}

template<typename Numeral>
inline std::istream& operator>>(std::istream& is, sf::Vector2<Numeral>& vec2)
{
    char comma;
    is >> vec2.x >> comma >> vec2.y;
    return is;
}

/*------------------------------------------------------------------------------------------------*/

namespace Convert
{
// Note that type T must support std::stringstream << T.
template<typename T>
inline std::string to_str(const T value)
{
    std::stringstream buffer;
    buffer << value;
    return buffer.str();
} 

template<>
inline std::string to_str(const bool value)
{
    if (value)
        return "true";
    else
        return "false";
}

// Note that type T must support std::stringstream >> T.
template<typename T>
inline T str_to(const std::string string)
{
    std::stringstream buffer(string);

    T result;
    buffer >> result;
    return result;
}

// Returns true if string equals "true", "yes" or "1".
// Returns false with any other string.
template<>
inline bool str_to(const std::string string)
{
    if (get_decapitalized(string) == "true" || get_decapitalized(string) == "yes" || string == "1")
        return true;
    else
        return false;
}

template<typename Enum>
inline Enum str_to_enum(const std::string str, const std::unordered_map<std::string, Enum>& enum_mapper)
{
    if (contains(enum_mapper, str))
        return enum_mapper.at(str);
    else
    {
        LOG_ALERT("string could not be mapped to an enum; returning default enum;\nstring: " + str);
        return Enum{};
    }
}

template<typename Enum>
inline std::string enum_to_str(const Enum& e, const std::unordered_map<std::string, Enum>& enum_mapper)
{
    for (const auto& [str, e2] : enum_mapper)
        if (e == e2)
            return str;

    LOG_ALERT("enum could not be mapped to a string; returning empty string;\n"
              "enum: " + Convert::to_str(static_cast<int>(e)));
    return std::string{};
}
}