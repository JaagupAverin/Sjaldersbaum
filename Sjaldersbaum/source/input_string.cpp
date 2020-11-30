#include "input_string.h"

#include "string_assist.h"
#include "maths.h"

/*------------------------------------------------------------------------------------------------*/

constexpr int DEFAULT_MAX_LEN = 100;

/*------------------------------------------------------------------------------------------------*/

bool is_alnum_or_underscore(const unsigned char ch)
{
    return std::isalnum(ch) || ch == '_';
}

bool is_space(const unsigned char ch)
{
    return std::isspace(ch);
}

/*------------------------------------------------------------------------------------------------*/

InputString::InputString() :
    index          { 0 },
    len            { 0 },
    max_len        { DEFAULT_MAX_LEN },
    char_checker   { CharChecker::Graphic },
    is_char_allowed{ is_graphic },
    string_altered { false },
    index_altered  { false }
{

}

void InputString::update_keyboard_input(const Keyboard& keyboard)
{
    const int old_index = index;

    // Insertion:

    const auto utf32_char = keyboard.get_text_input();
    if (utf32_char != 0u && utf32_char < 127u)
    {
        const char ch = static_cast<char>(utf32_char);
        if (len != max_len && is_char_allowed(ch))
        {
            string.insert(string.begin() + index, ch);
            ++index; ++len;
            string_altered = true;
        }
    }

    // Index alteration:

    if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_LEFT))
        --index;
    else if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_LEFT_BY_WORD))
        move_index_left_by_word();

    if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_RIGHT))
        ++index;
    else if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_RIGHT_BY_WORD))
        move_index_right_by_word();


    if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_TO_START))
        index = 0;
    if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_TO_END))
        index = len;

    assure_bounds(index, 0, len);

    // Erasion:

    if (index > 0)
    {
        if (keyboard.is_keybind_pressed(DefaultKeybinds::ERASE_PRECEDING))
        {
            string.erase(--index, 1);
            --len;
            string_altered = true;
        }
        else if (keyboard.is_keybind_pressed(DefaultKeybinds::ERASE_PRECEDING_WORD))
        {
            const int end = index;
            move_index_left_by_word();
            const int begin = index;

            string.erase(begin, end - begin);
            index = begin;
            len -= end - begin;
            string_altered = true;
        }
    }

    if (index < len)
    {
        if (keyboard.is_keybind_pressed(DefaultKeybinds::ERASE_PROCEEDING))
        {
            string.erase(index, 1);
            --len;
            string_altered = true;
        }
        else if (keyboard.is_keybind_pressed(DefaultKeybinds::ERASE_PROCEEDING_WORD))
        {
            const int begin = index;
            move_index_right_by_word();
            while (index < static_cast<int>(string.length()) && is_space(string[index]))
                ++index;
            const int end = index;

            string.erase(begin, end - begin);
            index = begin;
            len -= end - begin;
            string_altered = true;
        }
    }

    if (keyboard.is_keybind_pressed(DefaultKeybinds::ERASE_ALL))
    {
        clear();
        string_altered = true;
    }

    if (index != old_index)
        index_altered = true;
}

void InputString::set_string(const std::string& str)
{
    std::string filtered_str;
    for (char ch : str)
        if (is_char_allowed(ch))
            filtered_str += ch;

    if (filtered_str.length() > static_cast<size_t>(max_len))
        filtered_str.erase(max_len, len - max_len);

    string = filtered_str;
    len    = static_cast<int>(filtered_str.length());
    index  = len;

    string_altered = true;
    index_altered  = true;
}

void InputString::set_index(size_t i)
{
    index = static_cast<int>(i);
    assure_less_than_or_equal_to(index, len);
    index_altered = true;
}

void InputString::set_max_length(const int max_len)
{
    this->max_len = max_len;
    if (len > max_len)
        string.erase(max_len, len - max_len);
}

void InputString::set_char_checker(const CharChecker char_checker)
{
    if (char_checker == CharChecker::Graphic)
        is_char_allowed = is_graphic;
    else if (char_checker == CharChecker::Numeric)
        is_char_allowed = is_numeric;
    else if (char_checker == CharChecker::Systemic)
        is_char_allowed = is_systemic;
    else if (char_checker == CharChecker::Usernamic)
        is_char_allowed = is_usernamic;
    else
    {
        LOG_ALERT("unimplemented CharChecker type.");
        return;
    }

    this->char_checker = char_checker;

    clear();
}

void InputString::clear()
{
    string.clear();
    len   = 0;
    index = 0;

    string_altered = true;
    index_altered  = true;
}

std::string InputString::get_string() const
{
    return string;
}

size_t InputString::get_index() const
{
    return static_cast<size_t>(index);
}

int InputString::get_max_length() const
{
    return max_len;
}

CharChecker InputString::get_char_checker() const
{
    return char_checker;
}

bool InputString::has_string_been_altered() const
{
    if (string_altered)
    {
        string_altered = false;
        return true;
    }
    else
        return false;
}

bool InputString::has_index_been_altered() const
{
    if (index_altered)
    {
        index_altered = false;
        return true;
    }
    else
        return false;
}

void InputString::move_index_left_by_word()
{
    if (index <= 0)
        return;

    // Move by at least one:
    --index;
    if (index > 0)
    {
        // If the first character was a space...
        if (is_space(string[index]))
        {
            // ...then also move over all following spaces:
            while ((index - 1) >= 0 && is_space(string[index - 1]))
                --index;
            // ...and if no word characters follow, move by one again:
            if (index != 0 && !is_alnum_or_underscore(string[index - 1]))
            {
                --index;
                return;
            }
        }
        else if (!is_alnum_or_underscore(string[index]))
            return;

        // Move over all word characters:
        while ((index - 1) >= 0 && is_alnum_or_underscore(string[index - 1]))
            --index;
    }
}

void InputString::move_index_right_by_word()
{
    if (index >= len)
        return;

    ++index;
    if (index < static_cast<int>(string.length()))
    {
        if (string[index - 1] == ' ')
        {
            while (index < static_cast<int>(string.length()) && is_space(string[index]))
                ++index;
            if (index != len && !is_alnum_or_underscore(string[index]))
            {
                ++index;
                return;
            }
        }
        else if (!is_alnum_or_underscore(string[index]))
            return;

        while (index < static_cast<int>(string.length()) && is_alnum_or_underscore(string[index]))
            ++index;
    }
}