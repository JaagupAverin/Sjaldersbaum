#pragma once

#include <SFML/Graphics.hpp>

#include "units.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds DOUBLE_CLICK_INTERVAL = 1.f / 3.f;

/*
Button terminology:

1) Held     - button is currently held down.

2) Pressed  - button is currently held down, but wasn't held previous loop.

3) Released - button was held down previous loop, but isn't held currently.

4) Clicked  - button was Released and the mouse hasn't been dragging.

5) Dragging - button has been held and moved away from its initial press' position.

Also stores input from sf::Event::MouseWheelScrollEvent, which must be updated manually.
*/
class Mouse
{
public:
    Mouse(const sf::RenderWindow& relative_window);

    void update(Seconds elapsed_time);

    // Returned position will always be within window's resolution bounds.
    PxVec2 get_position_in_window() const;
    PxVec2 get_position_in_view(const sf::View& view) const;

    PxVec2 get_position_delta_in_window() const;
    PxVec2 get_position_delta_in_view(const sf::View& view) const;

    // The position where the left button was first pressed down (applies when dragging).
    PxVec2 get_left_position_initial_in_window() const;
    PxVec2 get_left_position_initial_in_view(const sf::View& view) const;

    // The position where the right button was first pressed down (applies when dragging).
    PxVec2 get_right_position_initial_in_window() const;
    PxVec2 get_right_position_initial_in_view(const sf::View& view) const;

    bool is_left_held() const;
    bool is_left_pressed() const;
    bool is_left_released() const;
    bool is_left_clicked() const;
    bool is_left_double_clicked() const;
    bool is_left_dragging() const;
    bool has_left_dragging_just_started() const;

    bool is_right_held() const;
    bool is_right_pressed() const;
    bool is_right_released() const;
    bool is_right_clicked() const;
    bool is_right_double_clicked() const;
    bool is_right_dragging() const;
    bool has_right_dragging_just_started() const;

    bool has_moved() const;

    // Must be called before each loop to avoid old input (event data from previous loop).
    void reset_wheel_input();

    // Update with data from sf::Event::MouseWheelScrollEvent.
    void set_wheel_ticks_delta(float wheel_ticks_delta);

    // Positive value is scrolling up; negative down. Idle wheel returns 0.
    float get_wheel_ticks_delta() const;

private:
    void update_left_button(Seconds elapsed_time);
    void update_right_button(Seconds elapsed_time);

private:
    const sf::RenderWindow& relative_window;

    sf::Vector2i position_prev;
    sf::Vector2i position;

    sf::Vector2i left_position_initial;
    sf::Vector2i left_position_prev_click;
    bool left_held_prev;
    bool left_held;
    bool left_dragging_prev;
    bool left_dragging;
    bool left_clicked;
    bool left_double_clicked;
    Seconds left_click_lag;

    sf::Vector2i right_position_initial;
    sf::Vector2i right_position_prev_click;
    bool right_held_prev;
    bool right_held;
    bool right_dragging_prev;
    bool right_dragging;
    bool right_clicked;
    bool right_double_clicked;
    Seconds right_click_lag;

    float wheel_ticks_delta;
};