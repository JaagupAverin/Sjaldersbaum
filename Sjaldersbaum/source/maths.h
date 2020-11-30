#pragma once

#include <random>
#include <SFML/System.hpp>

#include "units.h"
#include "logger.h"

/*------------------------------------------------------------------------------------------------*/

constexpr float PI = 3.14159265359f;

constexpr Radian to_rad(const Degree angle)
{
    return angle * (PI / 180.f);
}

constexpr Degree to_degree(const Radian rad)
{
    return rad * (180.f / PI);
}

/*------------------------------------------------------------------------------------------------*/

// Returns the angle between two vectors.
// vector 1: c to p1
// vector 2: c to p2
template<typename Num>
constexpr Degree get_angle(const sf::Vector2<Num> c,
                           const sf::Vector2<Num> p1,
                           const sf::Vector2<Num> p2)
{
    const float rad = atan2(p1.y - c.y, p1.x - c.x) -
                      atan2(p2.y - c.y, p2.x - c.x);

    return to_degree(rad);
}

// Returns the distance between two points.
template<typename Num>
constexpr Num get_distance(const sf::Vector2<Num> p1,
                           const sf::Vector2<Num> p2)
{
    const sf::Vector2<Num> vec{ p1 - p2 };
    return static_cast<Num>(sqrt((vec.x * vec.x) + (vec.y * vec.y)));
}

/*------------------------------------------------------------------------------------------------*/

// Moves point away from source by specified amount.
void move_away_from(PxVec2& point, const PxVec2 source, const Px move);

// Moves point towards target by specified amount.
// If the target is reached (or passed, in which case it is set to target), returns true;
// otherwise returns false.
bool move_towards(PxVec2& point, const PxVec2 target, const Px move);

// Moves value towards target by specified amount.
// If the target is reached (or exceeded, in which case it is set to target), returns true;
// otherwise returns false.
template<typename Num>
inline bool move_towards(Num& value, const Num target, const Num move)
{
    const Num difference = target - value;
    if (difference == 0.f)
        return true;

    if (move <= 0.f)
        return false;

    if (abs(difference) > abs(move))
    {
        if (target > value)
            value += move;
        else
            value -= move;
        return false;
    }
    else
    {
        value = target;
        return true;
    }
}

/*------------------------------------------------------------------------------------------------*/

// Round half up (towards positive infinity).
inline Px round_hu(const Px px)
{
    return std::floor(px + 0.5f);
}

// Round half up (towards positive infinity).
inline PxVec2 round_hu(const PxVec2 v)
{
    return { round_hu(v.x), round_hu(v.y) };
}

/*------------------------------------------------------------------------------------------------*/

// If x is within [min:max] bounds, returns true;
// otherwise sets x to min/max accordingly and returns false.
template<typename Num>
inline bool assure_bounds(Num& x,
                          const Num min,
                          const Num max)
{
    if (x < min)
    {
        x = min;
        return false;
    }
    else if (x > max)
    {
        x = max;
        return false;
    }
    else
        return true;
}

// If x == num, returns true;
// otherwise sets x to num and returns false.
template<typename Num>
inline bool assure_equals(Num& x, const Num num)
{
    if (x == num)
        return true;
    else
    {
        x = num;
        return false;
    }
}

// If x <= max, returns true;
// otherwise sets x to max and returns false.
template<typename Num>
inline bool assure_less_than_or_equal_to(Num& x, const Num max)
{
    if (x <= max)
        return true;
    else
    {
        x = max;
        return false;
    }
}

// If x >= min, returns true;
// otherwise sets x to min and returns false.
template<typename Num>
inline bool assure_greater_than_or_equal_to(Num& x, const Num min)
{
    if (x >= min)
        return true;
    else
    {
        x = min;
        return false;
    }
}

/*------------------------------------------------------------------------------------------------*/

// If area contains p, returns true;
// otherwise repositions p and returns false.
inline bool assure_is_contained_by(PxVec2& p, const PxRect area)
{
    if (area.contains(p))
        return true;
    else
    {
        if (p.x < area.left)
            p.x = area.left;
        else if (p.x > area.getRight())
            p.x = area.getRight();

        if (p.y < area.top)
            p.y = area.top;
        else if (p.y > area.getBottom())
            p.y = area.getBottom();

        return false;
    }
}

// Returns a float in [min, max] range.
inline float randf(float min, float max)
{
    const float random = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX));
    return min + random * (max - min);
}

// Returns an integer in [min, max) range.
template <typename Int>
inline Int rand(const Int min, const Int max)
{
    if (max - min == static_cast<Int>(0))
        return min;
    else
        return static_cast<Int>(rand() % static_cast<int>(max - min)) + min;
}

extern std::mt19937 GLOBAL_MT;
// Returns an integer in [min, max) range. Uses C++11 features for better randomness. Slower.
template <typename Int>
inline Int rand11(const Int min, const Int max)
{
    std::uniform_int_distribution<Int> distr(min, max - 1);
    return distr(GLOBAL_MT);
}

/*------------------------------------------------------------------------------------------------*/

// Scales the sprite using the quotient between specified size and the sprite's textureRect.
// Note that if the textureRect's width/height are uninitialized (has no size),
// then the sprite won't have a size either.
void set_size(sf::Sprite& sprite, PxVec2 size);

// Flips a sprite by modifying its textureRect.
// Note that this function assumes the sprite uses its entire texture (not a spritesheet).
void set_horizontally_flipped(sf::Sprite& sprite, bool flipped);