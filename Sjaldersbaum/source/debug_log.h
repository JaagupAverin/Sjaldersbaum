#pragma once

#include <deque>
#include <SFML/Graphics.hpp>

#include "mouse.h"
#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Basic text-window for DebugWindow.
class DebugLog : public sf::Drawable
{
public:
    DebugLog();

    void scroll(const Mouse& mouse);

    void set_size(PxVec2 size);

    void set_position(PxVec2 position);

    void set_properties(const std::string& font_path, Px text_height,
                        sf::Color text_fill, sf::Color text_ol, Px text_ol_thickness,
                        sf::Color bg_fill,   sf::Color bg_ol,   Px bg_ol_thickness);

    // Appends the string to the log. Note that the string may be wrapped.
    void write(const std::string& str);

    void clear();

private:
    void position_view_to_newest_line();

    void render_visible_lines_to_canvas();

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::RectangleShape bg;

    std::deque<sf::Text> lines;
    sf::View          lines_view;
    sf::RenderTexture lines_canvas;
    sf::Sprite        lines_sprite;

    FontReference font;
    sf::Color text_fill;
    sf::Color text_ol;
    Px text_height;
    Px text_ol_thickness;

    Px line_height;
};