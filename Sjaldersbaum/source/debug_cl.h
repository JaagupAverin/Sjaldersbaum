#pragma once

#include <SFML/Graphics.hpp>

#include "input_string.h"
#include "keyboard.h"
#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Basic text-box for the DebugWindow.
class DebugCL : public sf::Drawable
{
public:
    DebugCL();

    void update_keyboard_input(const Keyboard& keyboard);

    void update(Seconds elapsed_time);

    void set_size(PxVec2 size);

    void set_position(PxVec2 position);

    void set_properties(const std::string& font_path, Px text_height,
                        sf::Color text_fill, sf::Color text_ol, Px text_ol_thickness,
                        sf::Color bg_fill,   sf::Color bg_ol,   Px bg_ol_thickness);

    bool has_committed_input() const;

    std::string extract_input();

private:
    void set_input(const std::string& str);

    void update_caret_blink(Seconds elapsed_time);

    void position_caret();

    void render_text_to_canvas();

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    FontReference font;

    sf::RectangleShape bg;
    sf::Text text;
    sf::Text caret;

    sf::View          text_view;
    sf::RenderTexture text_canvas;
    sf::Sprite        text_sprite;

    InputString input;
    bool input_committed;

    size_t  caret_index;
    bool    caret_visible;
    Seconds caret_blink_lag;

    std::vector<std::string> history;
    int history_index;
};