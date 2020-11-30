#pragma once

#include <functional>

#include <SFML/System.hpp>

#include "keyboard.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// See string_assist.h for implementations.
enum class CharChecker
{
    Graphic,
    Numeric,
    Systemic,
    Usernamic
};

// Specialized std::string that alters itself using text-input and KeyBinds.
class InputString
{
public:
    InputString();

    // Inserts any allowed text-input and modifies existing input.
    void update_keyboard_input(const Keyboard& keyboard);

    void set_string(const std::string& str);
    void set_index(size_t i);

    // Overwrites the default character limit. The default character limit is 100.
    void set_max_length(int max_len);

    // Overwrites the default (Graphic) character checker. Calls clear().
    void set_char_checker(CharChecker char_checker);

    void clear();

    std::string get_string() const;
    size_t get_index() const;
    int get_max_length() const;
    CharChecker get_char_checker() const;

    // ... since the last call to this function. Note that this marks the string as unaltered
    bool has_string_been_altered() const;
    // ... since the last call to this function. Note that this marks the index as unaltered.
    bool has_index_been_altered() const;

private:
    void move_index_left_by_word();
    void move_index_right_by_word();

private:
    std::string string;

    int index;
    int len;
    int max_len;

    CharChecker char_checker;
    std::function<bool(char)> is_char_allowed;

    mutable bool string_altered;
    mutable bool index_altered;
};