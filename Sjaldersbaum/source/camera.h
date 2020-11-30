#pragma once

#include <SFML/Graphics.hpp>

#include "yaml.h"
#include "keyboard.h"
#include "mouse.h"
#include "progressive.h"
#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Manages the primary ("table") view.
class Camera : public YAML::Serializable
{
public:
    Camera();
    
    void update_keyboard_input(const Keyboard& keyboard);
    void update_mouse_input(const Mouse& mouse);
    void update(Seconds elapsed_time);

    // View's center cannot leave these bounds.
    void set_central_bounds(PxRect central_bounds);

    void set_resolution(PxVec2 resolution);

    enum class Zoom { In, Out };
    void set_zoom_progressively(Zoom option, Seconds progression_duration);
    void set_center_progressively(PxVec2 target_center, Seconds progression_duration);

    const sf::View& get_view() const;

    PxVec2 get_center() const;

    bool is_moved_by_keyboard() const;
    bool is_moved_by_mouse() const;

    bool is_zoomed_by_keyboard() const;
    bool is_zoomed_by_mouse() const;

    void render_debug_stats(sf::RenderTarget& target) const;

    void initialize_debug_components();

    // In debug mode, stats such as resolution, the view's center and zoom are displayed on-screen.
    // Also, zooming limits and central bounds are removed.
    void toggle_debug_mode();

    // Expects a map that consists of:
    // ==ADVANCED====================
    // * center:   <PxVec2> = (0, 100)
    // * zoom_out: <float>  = false
    // ==============================
    bool initialize(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ===================
    // * center:   <PxVec2>
    // * zoom_out: <bool>  
    // ===================
    YAML::Node serialize_dynamic_data() const override;

private:
    void set_center(PxVec2 center);

    void move_center(PxVec2 offsets);

    void update_velocities_and_movement(Seconds elapsed_time);

    void apply_zoom();

private:
    sf::View view;

    PxVec2 resolution;
    PxRect central_bounds;
    ProgressivePxVec2 center;
    ProgressiveFloat  zoom;

    PxVec2 velocities;
    bool moving_up;
    bool moving_down;
    bool moving_left;
    bool moving_right;
    bool moved_by_keyboard;
    bool moved_by_mouse;
    bool zoomed_by_keyboard;
    bool zoomed_by_mouse;

    // Debug:
    FontReference font;
    sf::Text mouse_position_display;

    bool debug_components_initialized;
    bool debug_mode;
};