#pragma once

#include <SFML/Graphics.hpp>

#include "yaml.h"
#include "resources.h"
#include "objects.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Objects are drawn on top of and bound by this.
// The table is always centered at (0, 0).
class Table : public sf::Drawable, public YAML::Serializable
{
public:
    // If object's bounds are (fully) contained by the table's, returns true;
    // otherwise positions the object accordingly and returns false.
    bool assure_contains(Object& object) const;

    PxVec2 get_size() const;

    PxRect get_bounds() const;

    // Expects a map that consists of:
    // ========================================================
    // * texture: <std::string> = <REGULAR_WOOD>
    // * size:    <PxVec2>      = (2700, 1500)
    // ==ADVANCED==============================================
    // * bounds:  <PxVec2>      = (size.x - 100, size.y - 100)
    // ========================================================
    bool initialize(const YAML::Node& node) override;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite background;

    PxVec2 size;
    PxRect bounds;
};