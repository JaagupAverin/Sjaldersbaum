#pragma once

#include <SFML/Graphics.hpp>

#include "mouse.h"
#include "keyboard.h"
#include "debug_log.h"
#include "debug_cl.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Please note that the debug units are a bit of a hack.

/*------------------------------------------------------------------------------------------------*/

// Consists of a command line, which turns input into macros,
// and a log, which displays new input from the Logger.
class DebugWindow : public sf::Drawable
{
public:
    DebugWindow();

    void initialize();

    void update_keyboard_input(const Keyboard& keyboard);

    void update_mouse_input(const Mouse& mouse);

    void update(Seconds elapsed_time);

    void toggle_maximized();

    bool is_using_keyboard_input() const;

    bool is_using_mouse_input() const;

private:
    void set_size(PxVec2 size);

    void set_position(PxVec2 position);

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::RectangleShape bg;
    DebugLog log;
    DebugCL cl;

    PxVec2 size;
    PxVec2 position;

    bool using_keyboard_input;
    bool using_mouse_input;

    enum class State
    {
        Uninitialized,
        Minimized,
        Maximized
    };
    State state;
};