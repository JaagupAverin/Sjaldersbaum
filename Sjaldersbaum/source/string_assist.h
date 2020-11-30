#pragma once

#include <cctype>
#include <string>
#include <optional>

#include "maths.h"

/*------------------------------------------------------------------------------------------------*/

// Graphic:
// graphic characters and spaces
inline bool is_graphic(const unsigned char ch)
{
    return (std::isgraph(ch) || ch == ' ');
}

// Numeric:
// digits, decimal point separators (.,) and spaces
inline bool is_numeric(const unsigned char ch)
{
    return (std::isdigit(ch) || ch == '.' || ch == ',' || ch == ' ' || ch == '-');
}

// Systemic:
// alphanumeric characters, underscores, dashes, slashes, dots and spaces
inline bool is_systemic(const unsigned char ch)
{
    return (std::isalnum(ch) ||
            ch == '_'   || ch == '-'   ||
            ch == '/'   || ch == '\\'  ||
            ch == '.'   || ch == ' ');
}

// Username characters:
// Systemic characters minus anything that could be used to mess with paths.
inline bool is_usernamic(const unsigned char ch)
{
    return (std::isalnum(ch) || ch == ' ' || ch == '-' || ch == '_');
}

/*------------------------------------------------------------------------------------------------*/

inline bool consists_of_systemic_characters(const std::string& str)
{
    for (auto ch : str)
        if (!is_systemic(ch))
            return false;
    return true;
}

// With "usernamic" characters, the user won't be able to create/erase paths like \..\..\Desktop
inline bool consists_of_usernamic_characters(const std::string& str)
{
    for (auto ch : str)
        if (!is_usernamic(ch))
            return false;
    return true;
}

/*------------------------------------------------------------------------------------------------*/

inline std::string get_as_formatted_string(const std::vector<std::string>& vec)
{
    std::stringstream res;
    for (size_t i = 0; i != vec.size(); ++i)
        res << "- " << vec[i] << '\n';
    return res.str();
}

/*------------------------------------------------------------------------------------------------*/

inline void quote(std::string& str)
{
    str.insert(str.begin(), '"');
    str.insert(str.end(), '"');
}

// Removes (double) quotes from the start and end of the string, but only if both are present.
inline void dequote(std::string& str)
{
    if (str.length() >= 2 && *str.begin() == '"' && *(str.end() - 1) == '"')
    {
        str.erase(str.begin());
        str.erase(str.end() - 1);
    }
}

/*------------------------------------------------------------------------------------------------*/

// Splits a string into two, if possible. Returns a pair of two strings, where second is optional.
inline std::pair<std::string, std::optional<std::string>> str_split(const std::string& str,
                                                                    const std::string& delim)
{
    std::pair<std::string, std::optional<std::string>> res;

    // main_substr: from 0 to first delimiter.
    const auto delim_1 = str.find(delim, 0);
    res.first = str.substr(0, delim_1);
    if (delim_1 == std::string::npos)
        return res;
    else
    {
        // optional_substr: from first delimiter to the end of string.
        res.second = str.substr(delim_1 + delim.length(), std::string::npos);
        return res;
    }
}

/*------------------------------------------------------------------------------------------------*/

inline void find_and_replace(std::string& str, const std::string& what, const std::string& with)
{
    size_t index;
    while ((index = str.find(what)) != std::string::npos)
        str.replace(index, what.length(), with);
}

/*------------------------------------------------------------------------------------------------*/

inline bool ends_with(const std::string& str, const std::string& substr)
{
     return str.substr(str.length() - substr.length()) == substr;
}

/*------------------------------------------------------------------------------------------------*/

inline void decapitalize(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) { return tolower(ch); });
}

inline std::string get_decapitalized(std::string str)
{
    decapitalize(str);
    return str;
}