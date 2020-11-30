#pragma once

#include <string>
#include <SFML/Graphics.hpp>

#include "xrect.h"

/*------------------------------------------------------------------------------------------------*/

using ID = std::string;

using Seconds  = float;

using Px       = float;
using PxPerSec = float;
using PxRect   = sf::XRect<float>;
using PxVec2   = sf::Vector2<float>;

using Radian = float;
using Degree = float;

/*------------------------------------------------------------------------------------------------*/

// No drawable object should be larger than this limit (prevents unexpected end-user-text).
constexpr Px PX_LIMIT = 10'000.f;

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds SECONDS_IN_NANOSECOND = 0.000000001f;

/*------------------------------------------------------------------------------------------------*/