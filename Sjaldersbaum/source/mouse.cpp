#include "mouse.h"

#include "maths.h"

/*------------------------------------------------------------------------------------------------*/

// After pressing a button, its position will be saved, around which an area can be imagined.
// If the mouse is released within that area, a click is performed.
// If the mouse is moved out of that area, it will be dragging and cannot click.
constexpr int MIN_DRAG_DISTANCE = 14;

/*------------------------------------------------------------------------------------------------*/

Mouse::Mouse(const sf::RenderWindow& relative_window) :
    relative_window    { relative_window },
    left_held_prev     { false },
    left_held          { false },
    left_dragging_prev { false },
    left_dragging      { false },
    left_clicked       { false },
    left_double_clicked{ false },
    left_click_lag     { DOUBLE_CLICK_INTERVAL },

    right_held_prev     { false },
    right_held          { false },
    right_dragging_prev { false },
    right_dragging      { false },
    right_clicked       { false },
    right_double_clicked{ false },
    right_click_lag     { DOUBLE_CLICK_INTERVAL },

    wheel_ticks_delta{ 0 }
{
    
}

void Mouse::update(const Seconds elapsed_time)
{
    position_prev = position;
    position = sf::Mouse::getPosition(relative_window);
    
    assure_bounds(this->position.x, 0, static_cast<int>(relative_window.getSize().x));
    assure_bounds(this->position.y, 0, static_cast<int>(relative_window.getSize().y));

    update_left_button(elapsed_time);
    update_right_button(elapsed_time);
}

PxVec2 Mouse::get_position_in_window() const
{
    return PxVec2{ position };
}

PxVec2 Mouse::get_position_in_view(const sf::View& view) const
{
    return relative_window.mapPixelToCoords(position, view);
}

PxVec2 Mouse::get_position_delta_in_window() const
{
    return PxVec2{ position - position_prev };
}

PxVec2 Mouse::get_position_delta_in_view(const sf::View& view) const
{
    const PxVec2 position_in_view_prev    = relative_window.mapPixelToCoords(position_prev, view);
    const PxVec2 position_in_view_current = relative_window.mapPixelToCoords(position     , view);

    return { position_in_view_current - position_in_view_prev };
}

PxVec2 Mouse::get_left_position_initial_in_window() const
{
    return PxVec2{ left_position_initial };
}

PxVec2 Mouse::get_left_position_initial_in_view(const sf::View& view) const
{
    return relative_window.mapPixelToCoords(left_position_initial, view);
}

PxVec2 Mouse::get_right_position_initial_in_window() const
{
    return PxVec2{ right_position_initial };
}

PxVec2 Mouse::get_right_position_initial_in_view(const sf::View& view) const
{
    return relative_window.mapPixelToCoords(right_position_initial, view);
}

bool Mouse::is_left_held() const
{
    return left_held;
}

bool Mouse::is_left_pressed() const
{
    return (left_held && !left_held_prev);
}

bool Mouse::is_left_released() const
{
    return (left_held_prev && !left_held);
}

bool Mouse::is_left_clicked() const
{
    return left_clicked;
}

bool Mouse::is_left_double_clicked() const
{
    return left_double_clicked;
}

bool Mouse::is_left_dragging() const
{
    return left_dragging;
}

bool Mouse::has_left_dragging_just_started() const
{
    return (left_dragging && !left_dragging_prev);
}

bool Mouse::is_right_held() const
{
    return right_held;
}

bool Mouse::is_right_pressed() const
{
    return (right_held && !right_held_prev);
}

bool Mouse::is_right_released() const
{
    return (right_held_prev && !right_held);
}

bool Mouse::is_right_clicked() const
{
    return right_clicked;
}

bool Mouse::is_right_double_clicked() const
{
    return right_double_clicked;
}

bool Mouse::is_right_dragging() const
{
    return right_dragging;
}

bool Mouse::has_right_dragging_just_started() const
{
    return (right_dragging && !right_dragging_prev);
}

bool Mouse::has_moved() const
{
    return position != position_prev;
}

void Mouse::reset_wheel_input()
{
    wheel_ticks_delta = 0;
}

void Mouse::set_wheel_ticks_delta(const float wheel_ticks_delta)
{
    this->wheel_ticks_delta = wheel_ticks_delta;
}

float Mouse::get_wheel_ticks_delta() const
{
    return wheel_ticks_delta;
}

void Mouse::update_left_button(const Seconds elapsed_time)
{
    left_held_prev     = left_held;
    left_dragging_prev = left_dragging;

    left_held     = false;
    left_dragging = false;
    left_clicked  = false;
    left_double_clicked = false;

    left_click_lag += elapsed_time;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        left_held = true;

        if (left_held_prev)
        {
            if (left_dragging_prev)
                left_dragging = true;
            else
            {
                const int distance_from_initial = get_distance(left_position_initial, position);
                if (distance_from_initial >= MIN_DRAG_DISTANCE)
                    left_dragging = true;
            }
        }
        else // Initial button press.
            left_position_initial = position;
    }

    if (left_held_prev && !left_held && !left_dragging_prev)
    {
        left_clicked = true;

        if (left_click_lag <= DOUBLE_CLICK_INTERVAL &&
            get_distance(left_position_prev_click, position) < MIN_DRAG_DISTANCE)
            left_double_clicked = true;

        left_position_prev_click = position;

        left_click_lag = 0.f;
    }
}

void Mouse::update_right_button(const Seconds elapsed_time)
{
    right_held_prev     = right_held;
    right_dragging_prev = right_dragging;

    right_held     = false;
    right_dragging = false;
    right_clicked  = false;
    right_double_clicked = false;

    right_click_lag += elapsed_time;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
    {
        right_held = true;

        if (right_held_prev)
        {
            if (right_dragging_prev)
                right_dragging = true;
            else
            {
                const int distance_from_initial = get_distance(right_position_initial, position);
                if (distance_from_initial >= MIN_DRAG_DISTANCE)
                    right_dragging = true;
            }
        }
        else // Initial button press.
            right_position_initial = position;
    }

    if (right_held_prev && !right_held && !right_dragging_prev)
    {
        right_clicked = true;

        if (right_click_lag <= DOUBLE_CLICK_INTERVAL &&
            get_distance(right_position_prev_click, position) < MIN_DRAG_DISTANCE)
            right_double_clicked = true;

        right_position_prev_click = position;

        right_click_lag = 0.f;
    }
}