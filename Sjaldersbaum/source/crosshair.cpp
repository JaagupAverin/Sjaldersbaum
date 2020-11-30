#include "crosshair.h"

#include <random>
#include <algorithm>

#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds STATE_TRANSITION_DURATION = 0.16f;
constexpr Seconds FADEOUT_INTERVAL          = 0.60f;

extern const sf::Color CROSSHAIR_ON_OBJECT_COLOR     = sf::Color{ 200, 20, 60,  60 };
extern const sf::Color CROSSHAIR_ON_TABLE_COLOR      = sf::Color{ 200, 60, 180, 60 };
extern const sf::Color CROSSHAIR_MOVING_CAMERA_COLOR = CROSSHAIR_ON_TABLE_COLOR;

/*------------------------------------------------------------------------------------------------*/

Crosshair::Crosshair() :
    size{ { 0.f, 0.f } },
    center{ { 0.f, 0.f } },
    opacity{ 0.f },
    color{ Colors::BLACK },
    clasped{ false },
    type{ Indicator::Type::Unassigned },
    inactivity_lag{ 0.f },
    interaction_timer{ 0.f },
    visible{ false }
{
    size.set_progression_duration(STATE_TRANSITION_DURATION);
    center.set_progression_duration(STATE_TRANSITION_DURATION);
    opacity.set_progression_duration(STATE_TRANSITION_DURATION);
    color.set_progression_duration(STATE_TRANSITION_DURATION);
}

void Crosshair::update(const Seconds elapsed_time)
{
    interaction_timer -= elapsed_time;
    if (!clasped && interaction_timer <= 0.f)
        size.set_target(default_size);

    if (clasped)
        center.set_current(clasped_object->get_center());
    else
        inactivity_lag += elapsed_time;

    size.update(elapsed_time);
    center.update(elapsed_time);
    if (size.has_changed_since_last_check() || center.has_changed_since_last_check())
        position_crosshair_lines();

    color.update(elapsed_time);
    opacity.update(elapsed_time);
    if (color.has_changed_since_last_check() || opacity.has_changed_since_last_check())
    {
        for (auto& line : crosshair)
            line.set_color(color.get_current(), opacity.get_current());
    }

    if ((!visible || inactivity_lag >= FADEOUT_INTERVAL) && !clasped)
        opacity.set_target(0.f);
    else
        opacity.set_target(1.f);
}

void Crosshair::set_type(const Indicator::Type type)
{
    if (this->type == type)
        return;

    this->type = type;

    if (type == Indicator::Type::HoveringMovable)
    {
        default_size = { 16.f, 16.f };
        color.set_target(CROSSHAIR_ON_OBJECT_COLOR);
    }
    else if (type == Indicator::Type::HoveringButton)
    {
        default_size = { 34.f, 34.f };
        color.set_target(CROSSHAIR_ON_OBJECT_COLOR);
    }
    else if (type == Indicator::Type::HoveringTextField)
    {
        default_size = { 0.5f, 30.f };
        color.set_target(CROSSHAIR_ON_OBJECT_COLOR);
    }
    else if (type == Indicator::Type::MovingCamera)
    {
        default_size = { 4.f, 4.f };
        color.set_target(CROSSHAIR_MOVING_CAMERA_COLOR);
    }
    else
    {
        default_size = { 14.f, 14.f };
        color.set_target(CROSSHAIR_ON_TABLE_COLOR);
    }

    if (!clasped && interaction_timer <= 0.f)
        size.set_target(default_size);
}

void Crosshair::set_center(const PxVec2 center, const bool show)
{
    if (default_center != center)
    {
        default_center = center;
        if (!clasped)
            this->center.set_current(center);

        if (show)
            inactivity_lag = 0.f;
    }
}

void Crosshair::set_visible(const bool visible)
{
    this->visible = visible;
}

void Crosshair::clasp(std::shared_ptr<Object> object)
{
    if (!object)
    {
        LOG_ALERT("unexpected nullptr.");
        return;
    }

    clasped_object = std::move(object);
    clasped = true;

    size.set_target({ clasped_object->get_size().x, 
                      clasped_object->get_size().y });
    center.set_target(clasped_object->get_center());

    if (size.is_progressing() || color.is_progressing())
        inactivity_lag = 0.f;
}

void Crosshair::unclasp()
{
    clasped_object.reset();
    clasped = false;

    size.set_target(default_size);
    center.set_target(default_center);

    if (size.is_progressing() || color.is_progressing())
        inactivity_lag = 0.f;
}

void Crosshair::on_interaction()
{
    if (interaction_timer <= 0.f)
    {
        interaction_timer = STATE_TRANSITION_DURATION;
        size.set_target({ 2.f, 2.f });
        inactivity_lag = 0.f;
    }
}

void Crosshair::position_crosshair_lines()
{
    const PxVec2 size   = this->size.get_current();
    const PxVec2 center = round_hu(this->center.get_current());

    const PxVec2 tlc{ center.x - size.x / 2.f, center.y - size.y / 2.f };   // Top Left.
    const PxVec2 trc{ center.x + size.x / 2.f, center.y - size.y / 2.f };   // Top Right.
    const PxVec2 blc{ center.x - size.x / 2.f, center.y + size.y / 2.f };   // Bottom Left.
    const PxVec2 brc{ center.x + size.x / 2.f, center.y + size.y / 2.f };   // Bottom Right.

    crosshair[0].set_points(tlc, trc);
    crosshair[1].set_points(trc, brc);
    crosshair[2].set_points(brc, blc);
    crosshair[3].set_points(blc, tlc);
}

void Crosshair::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() == 0.f)
        return;

    for (auto& line : crosshair)
        target.draw(line, states);
}