#include "light.h"

#include "logger.h"
#include "convert.h"
#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

const sf::Color DEBUG_LINES_COLOR = Colors::MAGENTA;

/*------------------------------------------------------------------------------------------------*/
// Ellipse:

Ellipse::Ellipse() :
    semi_major_axis{ 0.f },
    semi_minor_axis{ 0.f },
    major_axis_angle{ 0.f }
{

}

PxVec2 Ellipse::get_point(const Degree angle) const
{
    sf::Transform transform;
    transform.rotate(major_axis_angle);

    return center + transform.transformPoint({
        semi_major_axis * static_cast<float>(cos(to_rad(angle))),
        semi_minor_axis * static_cast<float>(sin(to_rad(angle))) });
}

void Ellipse::render(sf::RenderTarget& target) const
{
    sf::Transform transform;
    transform.rotate(major_axis_angle);

    static constexpr int    point_count = 30;
    static constexpr Degree point_step = 360.f / point_count;

    sf::VertexArray ellipse{ sf::LineStrip, static_cast<size_t>(point_count) };
    for (int i = 0; i != point_count; ++i)
    {
        ellipse[i].position = get_point(i * point_step);
        ellipse[i].color = DEBUG_LINES_COLOR;
    }
    target.draw(ellipse);
}

/*------------------------------------------------------------------------------------------------*/
// Light:

constexpr Seconds RADIUS_BOOST_DURATION = 0.2f;
constexpr Seconds RADIUS_DECAY_DURATION = 10.f;

Light::Light() :
    on_sound          { UNINITIALIZED_SOUND },
    off_sound         { UNINITIALIZED_SOUND },
    source            { { 0.f, 0.f } },
    radius            { 0.f },
    brightness        { 0.f },
    outer_orbit_radius{ 0.f },
    on                { false },
    base_radius       { 0.f },
    base_brightness   { 0.f },
    inner_orbit_angle { 0.f },
    source_angle      { 0.f }
{

}

void Light::update(const Seconds elapsed_time)
{
    source.update(elapsed_time);
    if (source.has_changed_since_last_check())
    {
        outer_orbit.center = source.get_current();
        inner_orbit.center = source.get_current();
    }

    radius.update(elapsed_time);
    brightness.update(elapsed_time);

    outer_orbit_radius.update(elapsed_time);
    if (outer_orbit_radius.has_changed_since_last_check())
    {
        outer_orbit.semi_major_axis = outer_orbit_radius.get_current();
        outer_orbit.semi_minor_axis = outer_orbit_radius.get_current();
    }

    update_inner_orbit_size_and_angle(elapsed_time);
    update_source_angle(elapsed_time);
}

void Light::apply(const sf::RenderTexture& source_canvas,
                  sf::RenderTexture& target_canvas,
                  const sf::View& view) const
{
    sf::RenderStates local_states;
    local_states.shader = &shader;

    const PxVec2 canvas_size{ static_cast<Px>(source_canvas.getSize().x),
                              static_cast<Px>(source_canvas.getSize().y) };

    const float zoom = canvas_size.x / view.getSize().x;

    // Local uniforms:
    Px     radius     = this->radius.get_current();
    float  brightness = this->brightness.get_current();
    PxVec2 point      = inner_orbit.get_point(source_angle);

    // Blur light (apply "height") based on distance from source:
    Px point_distance = get_distance(inner_orbit.center, point);
    radius += point_distance / 2.2f;
    brightness -= pow(point_distance, 0.3f) / 100.f;

    // Translate light to canvas:
    point = { ( point.x + view.getSize().x / 2.f - view.getCenter().x) * zoom,
              (-point.y + view.getSize().y / 2.f + view.getCenter().y) * zoom };
    radius *= zoom;

    shader.setUniform("canvas_size", canvas_size);
    shader.setUniform("source",      point);
    shader.setUniform("radius",      radius);
    shader.setUniform("brightness",  brightness);

    target_canvas.clear();
    target_canvas.draw(sf::Sprite{ source_canvas.getTexture() }, local_states);
    target_canvas.display();
}

void Light::set_shader(const std::string& path)
{
    shader_path = path;

    if (!shader.loadFromFile(shader_path, sf::Shader::Fragment))
        LOG_ALERT("light shader could not be loaded from: " + shader_path);
}

void Light::set_radius(Px radius, Seconds progression_duration)
{
    if (!assure_bounds(radius, 0.f, PX_LIMIT))
        LOG_ALERT("invalid radius had to be adjusted.");

    if (!assure_bounds(progression_duration, 0.f, 3600.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-3600]");

    base_radius = radius;
    if (on)
    {
        this->radius.set_progression_duration(progression_duration);
        this->radius.set_target(radius);
    }
}

void Light::set_source(PxVec2 source, Seconds progression_duration)
{
    if (!(assure_bounds(source.x, -PX_LIMIT, PX_LIMIT) &
          assure_bounds(source.y, -PX_LIMIT, PX_LIMIT)))
        LOG_ALERT("invalid source had to be adjusted.");

    if (!assure_bounds(progression_duration, 0.f, 3600.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-3600]");

    this->source.set_progression_duration(progression_duration);
    this->source.set_target(source);
}

void Light::set_brightness(float brightness, Seconds progression_duration)
{
    if (!assure_bounds(brightness, 0.f, 100.f))
        LOG_ALERT("invalid brightness had to be adjusted. [0-100]");

    if (!assure_bounds(progression_duration, 0.f, 3600.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-3600]");

    base_brightness = brightness;
    if (on)
    {
        this->brightness.set_progression_duration(progression_duration);
        this->brightness.set_target(brightness);
    }
}

void Light::set_swing(Px radius, Seconds progression_duration)
{
    if (!assure_bounds(radius, 0.f, PX_LIMIT))
        LOG_ALERT("invalid swing radius had to be adjusted.");

    if (!assure_bounds(progression_duration, 0.f, 3600.f))
        LOG_ALERT("invalid progression_duration had to be adjusted; [0-3600]");

    outer_orbit_radius.set_progression_duration(progression_duration);
    outer_orbit_radius.set_target(radius);
}

void Light::set_on(const bool on, const Seconds transition_duration, const bool sound)
{
    if (this->on == on)
        return;

    brightness.set_progression_duration(transition_duration);

    if (on)
    {
        radius.set_progression_duration(transition_duration / 4.f);
        radius.set_target(base_radius);
        brightness.set_target(base_brightness);
        if (sound)
            AudioPlayer::instance().play(on_sound);
    }
    else
    {
        radius.set_progression_duration(transition_duration * 4.f);
        radius.set_target(0.f);
        brightness.set_target(0.f);
        if (sound)
            AudioPlayer::instance().play(off_sound);
    }
    this->on = on;
}

void Light::render_debug_lines(sf::RenderTarget& target) const
{
    inner_orbit.render(target);
    
    static constexpr Px r = 10.f;
    sf::CircleShape source_indicator{ r };
    source_indicator.setOrigin(r, r);
    source_indicator.setPosition(inner_orbit.get_point(source_angle));
    source_indicator.setFillColor(DEBUG_LINES_COLOR);
    target.draw(source_indicator);
}

bool Light::initialize(const YAML::Node& node)
{
    std::string shader_path = "resources/shaders/lantern.frag";
    Px          radius      = 1600.f;
    PxVec2      source      = { 0.f, 100.f };
    float       brightness  = 1.f;
    Px          swing       = 100.f;
                on_sound    = GlobalSounds::LIGHT_ON;
                off_sound   = GlobalSounds::LIGHT_OFF;

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node shader_node         = node["shader"];
            const YAML::Node radius_node         = node["radius"];
            const YAML::Node source_node         = node["source"];
            const YAML::Node brightness_node     = node["brightness"];
            const YAML::Node swing_node          = node["swing"];
            const YAML::Node on_sound_path_node  = node["on_sound"];
            const YAML::Node off_sound_path_node = node["off_sound"];

            if (shader_node.IsDefined())
                shader_path = shader_node.as<std::string>();

            if (radius_node.IsDefined())
                radius = radius_node.as<Px>();

            if (source_node.IsDefined())
                source = source_node.as<PxVec2>();

            if (brightness_node.IsDefined())
                brightness = brightness_node.as<float>();

            if (swing_node.IsDefined())
                swing = swing_node.as<Px>();

            if (on_sound_path_node.IsDefined())
                on_sound = AudioPlayer::instance().load(on_sound_path_node.as<std::string>(), false);
            
            if (off_sound_path_node.IsDefined())
                off_sound = AudioPlayer::instance().load(off_sound_path_node.as<std::string>(), false);
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "==ADVANCED==============================\n"
                      "* shader:     <std::string> = <LANTERN>\n"
                      "* radius:     <Px>          = 1500\n"
                      "* source:     <PxVec2>      = (0, 100)\n"
                      "* brightness: <float>       = 1\n"
                      "* swing:      <Px>          = 100\n"
                      "* on_sound:   <std::string> = <LANTERN>\n"
                      "* off_sound:  <std::string> = <LANTERN>\n"
                      "========================================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    set_shader(shader_path);
    set_radius(radius);
    set_source(source);
    set_brightness(brightness);
    set_swing(swing);

    return true;
}

YAML::Node Light::serialize_dynamic_data() const
{
    YAML::Node node;
    node["radius"]     = base_radius;
    node["source"]     = source.get_target();
    node["brightness"] = base_brightness;
    node["swing"]      = outer_orbit_radius.get_target();
    return node;
}

void Light::update_inner_orbit_size_and_angle(const Seconds elapsed_time)
{
    inner_orbit_angle += 1.f * elapsed_time;
    if (inner_orbit_angle > 360.f)
        inner_orbit_angle -= 360.f;

    inner_orbit.semi_major_axis  = outer_orbit.semi_major_axis;
    inner_orbit.semi_minor_axis  = outer_orbit.semi_major_axis * 0.2f;
    inner_orbit.major_axis_angle = inner_orbit_angle;
}

void Light::update_source_angle(const Seconds elapsed_time)
{
    source_angle += (40.f + pow(outer_orbit_radius.get_current(), 0.65f)) * elapsed_time;
    while (source_angle > 360.f)
        source_angle -= 360.f;
}