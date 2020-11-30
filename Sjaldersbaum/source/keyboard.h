#pragma once

#include <unordered_map>

#include <SFML/Window.hpp>

/*------------------------------------------------------------------------------------------------*/

struct Keybind
{
    enum class Modifier
    {
        None,
        Control,
        Alt,
        ControlOrAlt,
        ControlAndAlt,
        Any
    };

    Keybind(sf::Keyboard::Key key, Modifier modifier = Modifier::None);

    sf::Keyboard::Key key;
    Modifier modifier;
};

struct DualKeybind
{
    DualKeybind(Keybind primary, Keybind secondary);

    Keybind primary;
    Keybind secondary;
};

/*------------------------------------------------------------------------------------------------*/

namespace DefaultKeybinds
{
const Keybind MOVE_UP       { sf::Keyboard::W, Keybind::Modifier::ControlOrAlt };
const Keybind MOVE_DOWN     { sf::Keyboard::S, Keybind::Modifier::ControlOrAlt };
const Keybind MOVE_LEFT     { sf::Keyboard::A, Keybind::Modifier::ControlOrAlt };
const Keybind MOVE_RIGHT    { sf::Keyboard::D, Keybind::Modifier::ControlOrAlt };

const DualKeybind ZOOM_IN   { { sf::Keyboard::I, Keybind::Modifier::ControlOrAlt },
                              { sf::Keyboard::Q, Keybind::Modifier::ControlOrAlt } };
const DualKeybind ZOOM_OUT  { { sf::Keyboard::O, Keybind::Modifier::ControlOrAlt },
                              { sf::Keyboard::E, Keybind::Modifier::ControlOrAlt } };

const Keybind INTERACT            { sf::Keyboard::F, Keybind::Modifier::ControlOrAlt };
const DualKeybind TOGGLE_CLASP  { { sf::Keyboard::P, Keybind::Modifier::ControlOrAlt },
                                  { sf::Keyboard::C, Keybind::Modifier::ControlOrAlt } };

const Keybind TOGGLE_PREVIOUS_OBJECT{ sf::Keyboard::Tab };

const Keybind MOVE_INDEX_LEFT       { sf::Keyboard::Left  };
const Keybind MOVE_INDEX_RIGHT      { sf::Keyboard::Right };
const Keybind MOVE_INDEX_UP         { sf::Keyboard::Up    };
const Keybind MOVE_INDEX_DOWN       { sf::Keyboard::Down  };
const Keybind MOVE_INDEX_TO_START   { sf::Keyboard::Home  };
const Keybind MOVE_INDEX_TO_END     { sf::Keyboard::End   };
const Keybind MOVE_INDEX_LEFT_BY_WORD  { sf::Keyboard::Left,  Keybind::Modifier::Control };
const Keybind MOVE_INDEX_RIGHT_BY_WORD { sf::Keyboard::Right, Keybind::Modifier::Control };

const Keybind ERASE_ALL         { sf::Keyboard::K, Keybind::Modifier::Control };
const Keybind ERASE_PRECEDING   { sf::Keyboard::Backspace };
const Keybind ERASE_PROCEEDING  { sf::Keyboard::Delete    };
const Keybind ERASE_PRECEDING_WORD  { sf::Keyboard::Backspace, Keybind::Modifier::Control };
const Keybind ERASE_PROCEEDING_WORD { sf::Keyboard::Delete,    Keybind::Modifier::Control };

const Keybind ENTER     { sf::Keyboard::Enter,  Keybind::Modifier::Any };
const Keybind ESCAPE    { sf::Keyboard::Escape, Keybind::Modifier::Any };

const Keybind TOGGLE_FULLSCREEN { sf::Keyboard::Enter, Keybind::Modifier::Alt };
}

namespace DebugKeybinds
{
const Keybind TOGGLE_DEBUG_WINDOW   { sf::Keyboard::F1 };
const Keybind TOGGLE_DEBUG_MODE     { sf::Keyboard::F2 };
const Keybind TOGGLE_FPS_DISPLAY    { sf::Keyboard::F3 };
const Keybind RELOAD_ACTIVE_LEVEL   { sf::Keyboard::F4 };
const Keybind RELOAD_TEXTURES       { sf::Keyboard::F5 };
const Keybind RELOAD_SOUNDBUFFERS   { sf::Keyboard::F6 };
const Keybind RESET_ACTIVE_LEVEL    { sf::Keyboard::F8 };

const Keybind GRANT_DEBUG_RIGHTS    { sf::Keyboard::F12, Keybind::Modifier::ControlAndAlt };
}

/*------------------------------------------------------------------------------------------------*/

// Stores input from sf::Event::KeyEvent and sf::Event::TextEvent.
// Must be updated manually by reading said events.
class Keyboard
{
public:
    Keyboard();

    // Must be called before each loop to avoid old input (event data from previous loop).
    void reset_input();

    void set_key_pressed(sf::Event::KeyEvent key_event);

    // Updated with data from sf::Event::TextEvent.
    void set_text_input(sf::Uint32 utf32_char);
    
    // Returns true if the binding is pressed according to sf::Event::KeyPressed;
    // (keys may have cooldowns, simultaneous bindings may cancel each other, etc).
    bool is_keybind_pressed(Keybind keybind) const;
    bool is_keybind_pressed(DualKeybind dual_keybind) const;

    // Returns true if the binding's underlying keys are currently held down;
    // (unlike is_keybind_pressed(), this performs a check for the real state of the keys).
    bool is_keybind_held(Keybind keybind) const;
    bool is_keybind_held(DualKeybind dual_keybind) const;

    // Returns an UTF-32 character. Returns 0 for out-of-date input.
    sf::Uint32 get_text_input() const;

private:
    std::unordered_map<sf::Keyboard::Key, sf::Event::KeyEvent> pressed_key_events;
    sf::Uint32 text_input;
};