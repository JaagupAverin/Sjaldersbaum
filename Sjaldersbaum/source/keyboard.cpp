#include "keyboard.h"

#include "contains.h"

/*------------------------------------------------------------------------------------------------*/
// Keybind:

Keybind::Keybind(const sf::Keyboard::Key key, const Modifier modifier) :
    key     { key },
    modifier{ modifier }
{

}

DualKeybind::DualKeybind(const Keybind primary, const Keybind secondary) :
    primary  { primary },
    secondary{ secondary }
{

}

/*------------------------------------------------------------------------------------------------*/
// Keyboard:

Keyboard::Keyboard() :
    text_input{ 0U }
{

}

void Keyboard::set_key_pressed(sf::Event::KeyEvent key_event)
{
    pressed_key_events.emplace(key_event.code, key_event);
}

bool equals(const Keybind keybind, const sf::Event::KeyEvent key_event)
{
    if (keybind.key != key_event.code)
        return false;

    if (keybind.modifier == Keybind::Modifier::Any)
        return true;

    else if (keybind.modifier == Keybind::Modifier::Control)
        return (key_event.control && !key_event.alt);

    else if (keybind.modifier == Keybind::Modifier::Alt)
        return (key_event.alt && !key_event.control);

    else if (keybind.modifier == Keybind::Modifier::ControlOrAlt)
        return (key_event.control || key_event.alt);

    else if (keybind.modifier == Keybind::Modifier::ControlAndAlt)
        return (key_event.control && key_event.alt);

    else if (keybind.modifier == Keybind::Modifier::None)
        return (!key_event.control && !key_event.alt);

    return false;
}

bool Keyboard::is_keybind_pressed(const Keybind keybind) const
{
    if (contains(pressed_key_events, keybind.key))
        return equals(keybind, pressed_key_events.at(keybind.key));
    else
        return false;
}

bool Keyboard::is_keybind_pressed(const DualKeybind dual_keybind) const
{
    return is_keybind_pressed(dual_keybind.primary) ||
           is_keybind_pressed(dual_keybind.secondary);
}

bool Keyboard::is_keybind_held(const Keybind keybind) const
{
    if (sf::Keyboard::isKeyPressed(keybind.key))
    {
        sf::Event::KeyEvent live_key_event;
        live_key_event.code = keybind.key;
        live_key_event.control = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                                 sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        live_key_event.alt = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

        live_key_event.shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                               sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

        return equals(keybind, live_key_event);
    }
    else
        return false;
}

bool Keyboard::is_keybind_held(const DualKeybind dual_keybind) const
{
    return is_keybind_held(dual_keybind.primary) ||
           is_keybind_held(dual_keybind.secondary);
}

void Keyboard::reset_input()
{
    pressed_key_events.clear();
    text_input = sf::Uint32(0);
}

void Keyboard::set_text_input(const sf::Uint32 utf32_char)
{
    text_input = utf32_char;
}

sf::Uint32 Keyboard::get_text_input() const
{
    return text_input;
}