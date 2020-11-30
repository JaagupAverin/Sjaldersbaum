#pragma once

#include <SFML/Graphics.hpp>

#include "progressive.h"
#include "resources.h"
#include "hoverable_detail.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Interface to a sprite; specialized to be drawn as a background/highlight/halo for an entity.
class Highlight : public sf::Drawable, public HoverableDetail
{
public:
    Highlight();

    void update(Seconds elapsed_time);

    void set_texture(const std::string& texture_path);
    void set_texture(const sf::Texture& texture);

    // Determines whether or not the highlight will be drawn in its Default state (idle). True by default.
    void set_always_visible(bool always_visible);
    void set_color(sf::Color color);
    void set_opacity(float opacity);
    void set_base_size(PxVec2 base_size);
    void set_center(PxVec2 center);

    // When the highlight is Hovered/Activated, these margins will be added (2x) to the base size.
    void set_size_margins(PxVec2 hovered_state_margins, PxVec2 active_state_margins);

    bool is_idle() const;

private:
    void set_state(State state) override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite highlight;

    PxVec2    base_size;
    sf::Color base_color;
    ProgressivePxVec2 size;
    ProgressiveColor  color;
    float opacity;

    PxVec2 hovered_state_margins;
    PxVec2 active_state_margins;

    bool always_visible;
    bool hovered;
    bool active;
    bool idle;
};