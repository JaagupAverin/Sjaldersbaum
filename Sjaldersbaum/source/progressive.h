#pragma once

#include <SFML/System.hpp>

#include "units.h"

/*------------------------------------------------------------------------------------------------*/

template<typename T>
class ProgressiveValue;

using ProgressiveFloat  = ProgressiveValue<float>;
using ProgressivePx     = ProgressiveValue<Px>;
using ProgressivePxVec2 = ProgressiveValue<PxVec2>;
using ProgressiveColor  = ProgressiveValue<sf::Color>;
using ProgressiveBool   = ProgressiveValue<bool>;

/*------------------------------------------------------------------------------------------------*/

// Returns a blend of two values. By default the blend is 50/50.
// The factor argument can be specified to tilt the result towards one of the two:
// Towards 0 the first value is dominant  (0 returns value1);
// Towards 1 the second value is dominant (1 returns value2).
template<typename T>
inline T blend(const T value1, const T value2, const float factor = 0.5f)
{
    return static_cast<T>((1.f - factor) * value1 + factor * value2);
}

template<>
inline PxVec2 blend(const PxVec2 pv1, const PxVec2 pv2, const float factor)
{
    return { blend(pv1.x, pv2.x, factor),
             blend(pv1.y, pv2.y, factor) };
}

template<>
inline sf::Color blend(const sf::Color color1, const sf::Color color2, const float factor)
{
    sf::Color res;

    res.r = blend(color1.r, color2.r, factor);
    res.g = blend(color1.g, color2.g, factor);
    res.b = blend(color1.b, color2.b, factor);
    res.a = blend(color1.a, color2.a, factor);

    return res;
}

template<>
inline bool blend(const bool b1, const bool b2, const float factor)
{
    return factor == 1.f ? b2 : b1;
}

/*------------------------------------------------------------------------------------------------*/

// Interface to an object of type T, whose value can be set progressively, using blend<T>.
template<typename T>
class ProgressiveValue
{
public:
    explicit ProgressiveValue(T initial, Seconds progression_duration = 0.f);

    // Progresses the current value towards the target value.
    void update(Seconds elapsed_time);

    // The current value will take this long to turn (blend) into the target value.
    void set_progression_duration(Seconds duration);

    void set_target(T target, bool restart_progress = true);
    void set_current(T current);

    T get_current() const;
    T get_target() const;

    bool has_changed_since_last_check() const;
    bool is_progressing() const;


private:
    mutable bool changed_since_last_check;
    Seconds progression_duration;
    float progress;
    T before;
    T current;
    T target;
};

/*------------------------------------------------------------------------------------------------*/
// Implementation:

template<typename T>
ProgressiveValue<T>::ProgressiveValue(T initial, Seconds progression_duration) :
    before { initial },
    current{ initial },
    target { initial },
    changed_since_last_check{ true },
    progression_duration{ progression_duration },
    progress{ 1.f }
{

}

template<typename T>
void ProgressiveValue<T>::update(const Seconds elapsed_time)
{
    if (!is_progressing())
        return;

    if (progression_duration > 0.f)
        progress += (1.f / progression_duration) * elapsed_time;
    else
        progress = 1.f;

    if (progress >= 1.f)
        current = target;
    else
        current = blend<T>(before, target, progress);

    changed_since_last_check = true;
}

template<typename T>
void ProgressiveValue<T>::set_progression_duration(const Seconds duration)
{
    progression_duration = duration;
}

template<typename T>
void ProgressiveValue<T>::set_target(const T target, const bool restart_progress)
{
    if (this->target == target)
        return;

    this->target = target;

    if (restart_progress)
    {
        before = current;
        progress = 0.f;
    }
    else if (progress >= 1.f)
        set_current(target);
}

template<typename T>
void ProgressiveValue<T>::set_current(const T current)
{
    this->current = before = target = current;
    changed_since_last_check = true;
    progress = 1.f;
}

template<typename T>
T ProgressiveValue<T>::get_current() const
{
    return current;
}

template<typename T>
T ProgressiveValue<T>::get_target() const
{
    return target;
}

template<typename T>
bool ProgressiveValue<T>::has_changed_since_last_check() const
{
    const bool res = changed_since_last_check;
    changed_since_last_check = false;
    return res;
}

template<typename T>
bool ProgressiveValue<T>::is_progressing() const
{
    return progress < 1.f;
}