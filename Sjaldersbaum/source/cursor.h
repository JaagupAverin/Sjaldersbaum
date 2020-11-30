#pragma once

#include <SFML/Graphics.hpp>

#include "indicator.h"
#include "progressive.h"
#include "resources.h"

/*------------------------------------------------------------------------------------------------*/

class Cursor : public sf::Drawable
{
public:
    static Cursor& instance();

    void update(Seconds elapsed_time);

    void set_position(PxVec2 position);
    void set_type(Indicator::Type type);
    void set_visible(bool visible, Seconds delay = 0.f);

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite cursor;
    Indicator::Type type;
    // Because we don't want the cursor to disappear immediately:
    ProgressiveBool visible;

private:
    Cursor();
    Cursor(const Cursor&) = delete;
    Cursor(Cursor&&) = delete;
    Cursor& operator=(const Cursor&) = delete;
    Cursor& operator=(Cursor&&) = delete;
};