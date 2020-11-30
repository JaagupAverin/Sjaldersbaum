#include "highlight.h"

#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds PROGRESSION_DURATION = 0.08f;

Highlight::Highlight() :
    size{ { 0.f, 0.f } },
    base_color{ Colors::WHITE },
    color     { Colors::WHITE },
    opacity{ 1.f },
    always_visible{ true },
    active        { false },
    hovered       { false },
    idle          { false }
{
    size.set_progression_duration(PROGRESSION_DURATION);
    color.set_progression_duration(3.f * PROGRESSION_DURATION);
}

void Highlight::update(const Seconds elapsed_time)
{
    if (idle)
        return;
    
    size.update(elapsed_time);
    if (size.has_changed_since_last_check())
        set_size(highlight, size.get_current());

    color.update(elapsed_time);
    if (color.has_changed_since_last_check())
        highlight.setColor(blend(Colors::TRANSPARENT, color.get_current(), opacity));

    if (!color.is_progressing() &&
        !size.is_progressing())
        idle = true;
}

void Highlight::set_texture(const std::string& texture_path)
{
    texture.load(texture_path);
    set_texture(texture.get());
}

void Highlight::set_texture(const sf::Texture& texture)
{
    highlight.setTexture(texture, true);

    const sf::Vector2f texture_size{ texture.getSize() };
    highlight.setOrigin(texture_size / 2.f);
    set_size(highlight, base_size);
}

void Highlight::set_always_visible(const bool always_visible)
{
    this->always_visible = always_visible;
    if (always_visible)
        color.set_current(base_color);
    else
        color.set_current(Colors::WHITE_TRANSPARENT);
}

void Highlight::set_color(const sf::Color color)
{
    idle = false;

    this->base_color = color;
    this->color.set_target(color);
}

void Highlight::set_opacity(const float opacity)
{
    this->opacity = opacity;
    highlight.setColor(blend(Colors::TRANSPARENT, color.get_current(), opacity));
}

void Highlight::set_base_size(const PxVec2 base_size)
{
    this->base_size = base_size;
    size.set_current(base_size);
    if (texture.is_loaded())
        set_size(highlight, size.get_current());
}

void Highlight::set_size_margins(const PxVec2 hovered_state_margins,
                                 const PxVec2 active_state_margins)
{
    this->hovered_state_margins = hovered_state_margins;
    this->active_state_margins  = active_state_margins;
}

void Highlight::set_center(const PxVec2 center)
{
    highlight.setPosition(round_hu(center));
}

bool Highlight::is_idle() const
{
    return idle;
}

void Highlight::set_state(const State state)
{
    idle = false;

    if (state == State::Default)
    {
        size.set_target(base_size);

        if (always_visible)
            color.set_target(base_color);
        else
            color.set_target(Colors::WHITE_TRANSPARENT);
    }
    else if (state == State::Hovered)
    {
        size.set_target({ base_size.x + 2.f * hovered_state_margins.x,
                          base_size.y + 2.f * hovered_state_margins.y });
        color.set_target(base_color);
    }
    else if (state == State::Active)
    {
        size.set_target({ base_size.x + 2.f * active_state_margins.x,
                          base_size.y + 2.f * active_state_margins.y});
        color.set_target(base_color);
    }
}

void Highlight::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (color.get_current().a != 0)
        target.draw(highlight, states);
}