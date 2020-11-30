#pragma once

#include <deque>
#include <SFML/Graphics.hpp>

#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Text in the upper-left corner that displays FPS based how much time elapses per loop.
class FPS_Display : public sf::Drawable
{
public:
    FPS_Display();

    void initialize();

    void toggle_visible();

    void update(Seconds elapsed_time);

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    FontReference font;
    sf::Text display;

    std::deque<int> fps_history;
    Seconds update_lag;
    bool visible;
};