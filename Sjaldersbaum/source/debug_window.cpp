#include "debug_window.h"

#include <regex>
#include <sstream>

#include "cursor.h"
#include "commands.h"
#include "logger.h"
#include "events-requests.h"
#include "convert.h"
#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

const PxVec2 DEFAULT_POSITION{ 0.f, 0.f };
const PxVec2 SIZE{ 600.f, 500.f };

const sf::Color BACKGROUND_FILL = sf::Color{ 10, 10, 10, 200 };
const sf::Color BACKGROUND_OL   = Colors::RED;
const sf::Color TEXT_FILL = Colors::WHITE;
const sf::Color TEXT_OL   = Colors::BLACK;

constexpr Px BACKGROUND_OL_THICKNESS = 2.f;
constexpr Px TEXT_OL_THICKNESS       = 1.f;

constexpr Px ELEMENT_MARGIN = 10.f;
constexpr Px TEXT_HEIGHT    = 14.f;

constexpr Px CL_HEIGHT = 18.f;

/*------------------------------------------------------------------------------------------------*/

DebugWindow::DebugWindow() :
    using_keyboard_input{ false },
    using_mouse_input   { false },
    state{ State::Uninitialized }
{

}

void DebugWindow::initialize()
{
    log.set_properties(SYSTEM_FONT_PATH, TEXT_HEIGHT,
                       TEXT_FILL,        TEXT_OL,       TEXT_OL_THICKNESS,
                       BACKGROUND_FILL,  BACKGROUND_OL, BACKGROUND_OL_THICKNESS);

    cl.set_properties(SYSTEM_FONT_PATH, TEXT_HEIGHT,
                      TEXT_FILL,        TEXT_OL,       TEXT_OL_THICKNESS,
                      BACKGROUND_FILL,  BACKGROUND_OL, BACKGROUND_OL_THICKNESS);

    bg.setFillColor(BACKGROUND_FILL);

    set_size(SIZE);
    set_position(DEFAULT_POSITION);

    state = State::Minimized;
}

void DebugWindow::update_keyboard_input(const Keyboard& keyboard)
{
    if (state != State::Maximized || !using_keyboard_input)
        return;

    cl.update_keyboard_input(keyboard);
    if (cl.has_committed_input())
    {
        const std::string input = cl.extract_input();
        if (input == "clear")
            log.clear();
        else
            Executor::instance().queue_execution(input);
    }
}

void DebugWindow::update_mouse_input(const Mouse& mouse)
{
    if (state != State::Maximized)
        return;

    using_mouse_input = bg.getGlobalBounds().contains(mouse.get_position_in_window());

    if (using_mouse_input)
    {
        log.scroll(mouse);
        if (mouse.is_left_held() || mouse.is_right_held())
            set_position(position + mouse.get_position_delta_in_window());

        Cursor::instance().set_type(Indicator::Type::Regular);
        Cursor::instance().set_visible(true);
    }

    if (mouse.is_left_clicked())
        using_keyboard_input = using_mouse_input;
}

void DebugWindow::update(const Seconds elapsed_time)
{
    if (state != State::Maximized)
        return;

    if (using_keyboard_input)
        cl.update(elapsed_time);

    const std::string logger_input = Logger::instance().extract_new_input();
    if (!logger_input.empty())
        log.write(logger_input);
}

void DebugWindow::toggle_maximized()
{
    if (state == State::Uninitialized)
        return;

    if (state == State::Minimized)
    {
        state = State::Maximized;
        using_keyboard_input = true;
        set_position({ 0.f, 0.f });
    }
    else
    {
        state = State::Minimized;
        using_keyboard_input = using_mouse_input = false;
    }
}

bool DebugWindow::is_using_keyboard_input() const
{
    return using_keyboard_input;
}

bool DebugWindow::is_using_mouse_input() const
{
    return using_mouse_input;
}

void DebugWindow::set_size(const PxVec2 size)
{
    this->size = size;

    const PxVec2 log_size{ size.x - 2 * ELEMENT_MARGIN,
                           size.y - 3 * ELEMENT_MARGIN - CL_HEIGHT };

    const PxVec2 cl_size{ size.x - 2 * ELEMENT_MARGIN, CL_HEIGHT };

    bg.setSize(size);
    log.set_size(log_size);
    cl.set_size(cl_size);
}

void DebugWindow::set_position(const PxVec2 position)
{
    this->position = position;

    const PxVec2 log_pos{ position.x + ELEMENT_MARGIN,
                          position.y + ELEMENT_MARGIN };

    const PxVec2 cl_pos{ position.x + ELEMENT_MARGIN,
                         position.y + size.y - (CL_HEIGHT + ELEMENT_MARGIN) };

    bg.setPosition(round_hu(position));
    log.set_position(log_pos);
    cl.set_position(cl_pos);
}

void DebugWindow::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (state == State::Maximized)
    {
        target.draw(bg);
        target.draw(log);
        target.draw(cl);
    }
}