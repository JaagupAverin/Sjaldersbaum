#pragma once

#include <SFML/Graphics.hpp>

#include "audio.h"
#include "yaml.h"
#include "progressive.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// For Light's orbits.
struct Ellipse
{
    Ellipse();

    PxVec2 get_point(const Degree angle) const;

    // Debug only.
    void render(sf::RenderTarget& target) const;

public:
    PxVec2 center;

    Px semi_major_axis;
    Px semi_minor_axis;

    Degree major_axis_angle;
};

/*------------------------------------------------------------------------------------------------*/

// Manages the light shader.
class Light : public YAML::Serializable
{
public:
    Light();

    void update(Seconds elapsed_time);

    // Draws the content of source_canvas to target_canvas, applying the shader.
    // Since the content of the canvas is no longer in table-coordinates, but the Light always is,
    // the ("table") view is required to also translate the Light.
    void apply(const sf::RenderTexture& source_canvas,
               sf::RenderTexture& target_canvas,
               const sf::View& view) const;

    void set_shader(const std::string& path);

    void set_radius(Px radius, Seconds progression_duration = 0.f);

    void set_source(PxVec2 source, Seconds progression_duration = 0.f);

    void set_brightness(float brightness, Seconds progression_duration = 0.f);

    // Sets the maximum radius (semi major axis) of the orbit around which the light swings.
    // Set to 0 for a static light.
    void set_swing(Px radius, Seconds progression_duration = 0.f);

    // Note that the Light is "off" by default.
    void set_on(bool on, Seconds transition_duration, bool sound = true);

    // Draws the light's source (a point representing it) and its orbit.
    void render_debug_lines(sf::RenderTarget& target) const;

    // Expects a map that consists of:
    // ==ADVANCED==============================
    // * shader:     <std::string> = <LANTERN>
    // * radius:     <Px>          = 1600
    // * source:     <PxVec2>      = (0, 100)
    // * brightness: <float>       = 1
    // * swing:      <Px>          = 100
    // * on_sound:   <std::string> = <LANTERN>
    // * off_sound:  <std::string> = <LANTERN>
    // ========================================
    bool initialize(const YAML::Node& node) override;

    // Returns a map that consists of:
    // =======================
    // * radius:     <Px>
    // * source:     <PxVec2>
    // * brightness: <float>
    // * swing:      <Px>
    // =======================
    YAML::Node serialize_dynamic_data() const override;

private:
    void update_inner_orbit_size_and_angle(Seconds elapsed_time);

    void update_source_angle(Seconds elapsed_time);

private:
    mutable sf::Shader shader;
    std::string shader_path;

    SoundID on_sound;
    SoundID off_sound;

    ProgressivePxVec2 source;
    ProgressivePx radius;
    ProgressiveFloat brightness;

    bool on;
    // When the light is turned off, base values for radius and brightness are lost, so save them:
    Px    base_radius;
    float base_brightness;

    // The inner orbit's major-axis-vertex orbits this ellipse:
    Ellipse       outer_orbit;
    ProgressivePx outer_orbit_radius;

    // The source orbits this ellipse:
    Ellipse inner_orbit;
    Degree  inner_orbit_angle;
    Degree source_angle;
};