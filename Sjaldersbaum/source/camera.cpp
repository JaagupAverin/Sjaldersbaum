#include "camera.h"

#include "audio.h"
#include "logger.h"
#include "maths.h"
#include "convert.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr float MIN_ZOOM = 0.5f;
constexpr float MAX_ZOOM = 1.0f;

constexpr Seconds ZOOM_PROGRESSION_DURATION = 0.12f;

constexpr PxPerSec BASE_CAMERA_MOVE_SPEED = 1000.f;
constexpr float ACCELERATION_MULTIPLIER = 2.2f;
constexpr float DECELERATION_MULTIPLIER = 6.f;

/*------------------------------------------------------------------------------------------------*/

Camera::Camera() :
    center{ { 0.f, 0.f }},
    zoom{ 1.f },
    moving_up   { false },
    moving_down { false },
    moving_left { false },
    moving_right{ false },
    moved_by_keyboard { false },
    moved_by_mouse    { false },
    zoomed_by_keyboard{ false },
    zoomed_by_mouse   { false },
    debug_components_initialized{ false },
    debug_mode                  { false }
{
    central_bounds.setSizeKeepCenter({ PX_LIMIT, PX_LIMIT });
}

void Camera::update_keyboard_input(const Keyboard& keyboard)
{
    if (keyboard.is_keybind_held(DefaultKeybinds::MOVE_UP))
        moving_up = true;

    if (keyboard.is_keybind_held(DefaultKeybinds::MOVE_DOWN))
        moving_down = true;

    if (keyboard.is_keybind_held(DefaultKeybinds::MOVE_LEFT))
        moving_left = true;

    if (keyboard.is_keybind_held(DefaultKeybinds::MOVE_RIGHT))
        moving_right = true;

    zoomed_by_keyboard = false;
    if (keyboard.is_keybind_held(DefaultKeybinds::ZOOM_IN) && !zoom.is_progressing())
    {
        set_zoom_progressively(Zoom::In, ZOOM_PROGRESSION_DURATION);
        zoomed_by_keyboard = true;
    }
    if (keyboard.is_keybind_held(DefaultKeybinds::ZOOM_OUT) && !zoom.is_progressing())
    {
        set_zoom_progressively(Zoom::Out, ZOOM_PROGRESSION_DURATION);
        zoomed_by_keyboard = true;
    }
}

void Camera::update_mouse_input(const Mouse& mouse)
{
    moved_by_mouse = false;
    if (mouse.is_right_dragging() && !center.is_progressing())
    {
        move_center(-mouse.get_position_delta_in_view(view));
        moved_by_mouse = true;
    }

    zoomed_by_mouse = false;
    if (mouse.get_wheel_ticks_delta() != 0.f && !zoom.is_progressing())
    {
        set_zoom_progressively(mouse.get_wheel_ticks_delta() > 0.f ? Zoom::In : Zoom::Out,
                               ZOOM_PROGRESSION_DURATION);
        if (zoom.is_progressing())
        {
            sf::View local_view = view;
            const PxVec2 mouse_position_current = mouse.get_position_in_view(local_view);
            local_view.setSize(round_hu(resolution / zoom.get_target()));
            const PxVec2 mouse_position_target = mouse.get_position_in_view(local_view);

            // Maintain view center relatively to the mouse:
            const PxVec2 offsets{ mouse_position_current.x - mouse_position_target.x,
                                  mouse_position_current.y - mouse_position_target.y };
            set_center_progressively(center.get_current() + offsets, ZOOM_PROGRESSION_DURATION);

            zoomed_by_mouse = true;
        }
    }

    if (debug_components_initialized)
        mouse_position_display.setString(
            "x: " + Convert::to_str(mouse.get_position_in_view(view).x) + '\n' +
            "y: " + Convert::to_str(mouse.get_position_in_view(view).y));
}

void Camera::update(const Seconds elapsed_time)
{
    zoom.update(elapsed_time);
    if (zoom.has_changed_since_last_check())
        apply_zoom();

    center.update(elapsed_time);
    if (center.has_changed_since_last_check())
        view.setCenter(round_hu(center.get_current()));

    if (!center.is_progressing())
        update_velocities_and_movement(elapsed_time);
    moving_up = moving_down = moving_left = moving_right = false;
}

void Camera::set_central_bounds(const PxRect central_bounds)
{
    this->central_bounds = central_bounds;
    set_center(center.get_current());
}

void Camera::set_resolution(const PxVec2 resolution)
{
    this->resolution = resolution;

    apply_zoom();
}

void Camera::set_zoom_progressively(const Zoom option, Seconds progression_duration)
{
    if (!assure_bounds(progression_duration, 0.f, 60.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-60]");

    const float new_zoom = option == Zoom::In ? 1.f : 0.5f;
    if (new_zoom == zoom.get_target())
        return;

    zoom.set_progression_duration(progression_duration);
    zoom.set_target(new_zoom);
}

void Camera::set_center_progressively(PxVec2 target_center, Seconds progression_duration)
{
    if (!assure_bounds(progression_duration, 0.f, 60.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-60]");

    velocities = { 0.f, 0.f };

    assure_is_contained_by(target_center, central_bounds);
    center.set_progression_duration(progression_duration);
    center.set_target(target_center);
}

const sf::View& Camera::get_view() const
{
    return view;
}

PxVec2 Camera::get_center() const
{
    return view.getCenter();
}

bool Camera::is_moved_by_keyboard() const
{
    return moved_by_keyboard;
}

bool Camera::is_moved_by_mouse() const
{
    return moved_by_mouse;
}

bool Camera::is_zoomed_by_keyboard() const
{
    return zoomed_by_keyboard;
}

bool Camera::is_zoomed_by_mouse() const
{
    return zoomed_by_mouse;
}

void Camera::render_debug_stats(sf::RenderTarget& target) const
{
    if (debug_mode)
        target.draw(mouse_position_display);
}

void Camera::initialize_debug_components()
{
    debug_components_initialized = true;

    font.load(SYSTEM_FONT_PATH);
    mouse_position_display.setFont(font.get());
    mouse_position_display.setCharacterSize(14u);
    mouse_position_display.setFillColor(Colors::GREEN);
    mouse_position_display.setOutlineColor(Colors::BLACK);
    mouse_position_display.setOutlineThickness(1);
    mouse_position_display.setPosition(1.f, 50.f);
}

void Camera::toggle_debug_mode()
{
    if (debug_components_initialized)
        debug_mode = !debug_mode;
    else
        LOG_ALERT("cannot toggle uninitialized debug stats.");
}

bool Camera::initialize(const YAML::Node& node)
{
    central_bounds.setSizeKeepCenter({ PX_LIMIT, PX_LIMIT });
    velocities = { 0.f, 0.f };

    PxVec2 center = { 0.f, 100.f };
    bool zoom_out = false;

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node center_node   = node["center"];
            const YAML::Node zoom_out_node = node["zoom_out"];

            if (center_node.IsDefined())
                center = center_node.as<PxVec2>();

            if (zoom_out_node.IsDefined())
                zoom_out = zoom_out_node.as<bool>();
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "==ADVANCED====================\n"
                      "* center:   <PxVec2> = (0, 100)\n"
                      "* zoom_out: <float>  = false\n"
                      "==============================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    set_center(center);
    this->zoom.set_current(zoom_out ? 0.5f : 1.f);

    return true;
}

YAML::Node Camera::serialize_dynamic_data() const
{
    YAML::Node node;
    node["center"]   = center.get_target();
    node["zoom_out"] = zoom.get_target() == 1.f ? false : true;
    return node;
}

void Camera::set_center(PxVec2 center)
{
    assure_is_contained_by(center, central_bounds);

    this->center.set_current(center);
    view.setCenter(round_hu(center));
}

void Camera::move_center(const PxVec2 offsets)
{
    if (abs(offsets.x) + abs(offsets.y) > 0.f)
        set_center(center.get_current() + offsets);
}

void Camera::update_velocities_and_movement(const Seconds elapsed_time)
{
    moved_by_keyboard = false;

    static constexpr Px speed = BASE_CAMERA_MOVE_SPEED;

    const PxPerSec acceleration = ACCELERATION_MULTIPLIER * speed * elapsed_time;
    const PxPerSec deceleration = DECELERATION_MULTIPLIER * acceleration;

    // Up:
    if (moving_up)
    {
        velocities.y -= acceleration;
        assure_greater_than_or_equal_to(velocities.y, -speed);
    }
    else if (velocities.y < 0)
    {
        velocities.y += deceleration;
        assure_less_than_or_equal_to(velocities.y, 0.f);
    }

    // Down:
    if (moving_down)
    {
        velocities.y += acceleration;
        assure_less_than_or_equal_to(velocities.y, speed);
    }
    else if (velocities.y > 0.f)
    {
        velocities.y -= deceleration;
        assure_greater_than_or_equal_to(velocities.y, 0.f);
    }

    // Left:
    if (moving_left)
    {
        velocities.x -= acceleration;
        assure_greater_than_or_equal_to(velocities.x, -speed);
    }
    else if (velocities.x < 0)
    {
        velocities.x += deceleration;
        assure_less_than_or_equal_to(velocities.x, 0.f);
    }

    // Right:
    if (moving_right)
    {
        velocities.x += acceleration;
        assure_less_than_or_equal_to(velocities.x, speed);
    }
    else if (velocities.x > 0.f)
    {
        velocities.x -= deceleration;
        assure_greater_than_or_equal_to(velocities.x, 0.f);
    }

    if (abs(velocities.x) + abs(velocities.y) > 0.f)
    {
        move_center((velocities * elapsed_time) / powf(zoom.get_current(), 0.4f));
        moved_by_keyboard = true;
    }
}

void Camera::apply_zoom()
{
    view.setSize(round_hu(resolution / zoom.get_current()));
}