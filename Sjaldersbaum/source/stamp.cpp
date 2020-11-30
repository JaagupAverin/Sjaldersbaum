#include "stamp.h"

#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds PROGRESSION_DURATION = 0.1f;

Stamp::Stamp()  :
    type{ Type::Neutral },
    idle{ false },
    old_stamp_color{ Colors::WHITE_TRANSPARENT },
    stamp_color    { Colors::WHITE_TRANSPARENT },
    stamp_size{ { 0.f, 0.f } },
    opacity_multiplier{ 1.f }
{
    old_stamp_color.set_progression_duration(0.5f);
    stamp_color.set_progression_duration(PROGRESSION_DURATION);
    stamp_size.set_progression_duration(PROGRESSION_DURATION);
}

void Stamp::update(const Seconds elapsed_time)
{
    if (idle)
        return;

    particles.update(elapsed_time);

    old_stamp_color.update(elapsed_time);
    if (old_stamp_color.has_changed_since_last_check())
        old_stamp.setColor(blend(Colors::TRANSPARENT, old_stamp_color.get_current(), opacity_multiplier));

    stamp_size.update(elapsed_time);
    if (stamp_size.has_changed_since_last_check())
        set_size(stamp, stamp_size.get_current());

    stamp_color.update(elapsed_time);
    if (stamp_color.has_changed_since_last_check())
        stamp.setColor(blend(Colors::TRANSPARENT, stamp_color.get_current(), opacity_multiplier));

    if (!old_stamp_color.is_progressing() &&
        !stamp_color.is_progressing() &&
        !stamp_size.is_progressing() &&
        particles.is_idle())
        idle = true;
}

void Stamp::set_texture(const std::string& path)
{
    texture.load(path);
    old_stamp.setTexture(texture.get());
    stamp.setTexture(texture.get());

    const sf::Vector2i texture_size{ texture.get().getSize() };
    texture_rect_size = { texture_size.x / static_cast<int>(Type::Count), texture_size.y };

    const PxVec2 origin{ PxVec2{ texture_rect_size } / 2.f };
    old_stamp.setOrigin(origin);
    stamp.setOrigin(origin);

    set_type(type);
    set_size(old_stamp, default_size);
}

void Stamp::set_explosions(ParticleExplosion positive,
                           ParticleExplosion negative,
                           ParticleExplosion neutral)
{
    this->positive_explosion = std::move(positive);
    this->negative_explosion = std::move(negative);
    this->neutral_explosion  = std::move(neutral);
}

void Stamp::set_type(const Type type, const bool explosion_effect, const bool lock_effect)
{
    idle = false;

    if (explosion_effect)
    {
        if (type == Type::Positive)
            particles.create_explosion(center, positive_explosion);
        else if (type == Type::Negative)
            particles.create_explosion(center, negative_explosion);
        else if (type == Type::Neutral)
            particles.create_explosion(center, neutral_explosion);
    }

    old_stamp_color.set_current(Colors::WHITE_SEMI_TRANSPARENT);
    old_stamp_color.set_target(Colors::WHITE_TRANSPARENT);

    if (!lock_effect)
    {
        stamp_size.set_current(default_size * 4.f);
        stamp_size.set_target(default_size);
    }
    stamp_color.set_current(Colors::WHITE_TRANSPARENT);
    stamp_color.set_target(lock_effect ? sf::Color{ 80, 80, 80 } : Colors::WHITE);

    auto set_rect = [=](sf::Sprite& stamp, const Type type)
    {
        stamp.setTextureRect(sf::IntRect{ { static_cast<int>(type) * texture_rect_size.x, 0 },
                                            texture_rect_size });
    };

    set_rect(old_stamp, this->type);
    this->type = type;
    set_rect(stamp,     this->type);
}

void Stamp::set_base_size(const PxVec2 size)
{
    base_size = default_size = size;
    stamp_size.set_current(default_size);
    set_size(stamp,     default_size);
    set_size(old_stamp, default_size);
}

void Stamp::set_center(const PxVec2 center)
{
    this->center = center;
    old_stamp.setPosition(round_hu(center));
    stamp.setPosition(round_hu(center));
}

void Stamp::set_opacity(const float opacity)
{
    opacity_multiplier = opacity;
    old_stamp.setColor(blend(Colors::TRANSPARENT, old_stamp_color.get_current(), opacity_multiplier));
    stamp.setColor(blend(Colors::TRANSPARENT, stamp_color.get_current(), opacity_multiplier));
}

bool Stamp::is_idle() const
{
    return idle;
}

void Stamp::set_state(const State state)
{
    idle = false;

    if (state == State::Default)
        default_size = base_size;
    else if (state == State::Hovered)
        default_size = base_size * 1.15f;
    else if (state == State::Active)
        default_size = base_size * 1.25f;

    stamp_size.set_target(default_size);
    set_size(old_stamp, default_size);
}

void Stamp::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(particles, states);
    target.draw(old_stamp, states);
    target.draw(stamp, states);
}