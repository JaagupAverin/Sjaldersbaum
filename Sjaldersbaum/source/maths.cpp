#include "maths.h"

void move_away_from(PxVec2& point, const PxVec2 source, const Px move)
{
    const Px distance = get_distance(point, source);
    if (distance == 0.f)
        return;

    const PxVec2 vector{ point.x - source.x, point.y - source.y };
    const PxVec2 normalized_vector = vector / distance;

    point.x += normalized_vector.x * move;
    point.y += normalized_vector.y * move;
}

bool move_towards(PxVec2& point, const PxVec2 target, const Px move)
{
    const Px distance = get_distance(point, target);
    if (distance == 0.f)
        return true;

    if (move <= 0.f)
        return false;

    if (distance > move)
    {
        const PxVec2 vector{ target.x - point.x, target.y - point.y };
        const PxVec2 normalized = vector / distance;

        point.x += normalized.x * move;
        point.y += normalized.y * move;
        return false;
    }
    else
    {
        point = target;
        return true;
    }
}

void set_size(sf::Sprite& sprite, PxVec2 size)
{
    PxVec2 texture_rect_size{
        static_cast<float>(std::abs(sprite.getTextureRect().width)),
        static_cast<float>(std::abs(sprite.getTextureRect().height)) };

    if (texture_rect_size.x == 0.f || texture_rect_size.y == 0.f)
        sprite.setScale(0.f, 0.f);
    else
    {
        sprite.setScale(size.x / texture_rect_size.x,
                        size.y / texture_rect_size.y);
    }
}

void set_horizontally_flipped(sf::Sprite& sprite, const bool flipped)
{
    if (sprite.getTexture() == nullptr)
    {
        LOG_ALERT("unexpected nullptr; can't flip sprite.");
        return;
    }

    if (flipped)
    {
        sprite.setTextureRect(sf::IntRect(static_cast<int>(sprite.getTexture()->getSize().x), 0,
                                          -static_cast<int>(sprite.getTexture()->getSize().x),
                                          static_cast<int>(sprite.getTexture()->getSize().y)));
    }
    else
    {
        sprite.setTextureRect(sf::IntRect(0, 0,
                                          static_cast<int>(sprite.getTexture()->getSize().x),
                                          static_cast<int>(sprite.getTexture()->getSize().y)));
    }
}
