#pragma once

#include <SFML/Graphics.hpp>

/*------------------------------------------------------------------------------------------------*/

namespace Colors
{
// Transparent:

extern const sf::Color TRANSPARENT             { 0,   0,   0,   0   };
extern const sf::Color WHITE_TRANSPARENT       { 255, 255, 255, 0   };

extern const sf::Color WHITE_SEMI_TRANSPARENT  { 255, 255, 255, 60 };
extern const sf::Color GREY_SEMI_TRANSPARENT   { 192, 192, 192, 60 };
extern const sf::Color BLACK_SEMI_TRANSPARENT  { 1,   1,   1,   60 };
extern const sf::Color RED_SEMI_TRANSPARENT    { 255, 0,   0,   60 };
extern const sf::Color GREEN_SEMI_TRANSPARENT  { 0,   255, 0,   60 };
extern const sf::Color BLUE_SEMI_TRANSPARENT   { 0,   0,   255, 60 };

// General:

extern const sf::Color WHITE           { 255, 255, 255 };
extern const sf::Color BLACK           { 0,   0,   0   };
extern const sf::Color GREY            { 100, 100, 100 };
extern const sf::Color RED             { 255, 0,   0   };
extern const sf::Color ORANGE          { 255, 160, 0   };
extern const sf::Color GREEN           { 0,   255, 0   };
extern const sf::Color BLUE            { 0,   0,   255 };
extern const sf::Color MAGENTA         { 255, 0,   255 };
extern const sf::Color CYAN            { 0,   255, 255 };
extern const sf::Color YELLOW          { 255, 255, 0   };
extern const sf::Color PINK            { 255, 20,  147 };
extern const sf::Color GOLD            { 255, 215, 0   };
extern const sf::Color CRIMSON         { 220, 20,  60  };
}