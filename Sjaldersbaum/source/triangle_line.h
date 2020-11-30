#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

#include "units.h"

// A fancy line consisting of triangles.
class TriangleLine : public sf::Drawable
{
public:
    // With overstep, both points will be moved away from each other depending on their original distance.
    TriangleLine(bool overstep = true);
    void set_points(PxVec2 p1, PxVec2 p2);
    void set_color(sf::Color color, float opacity);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::VertexArray vertices;
    std::vector<sf::Vector3f> multipliers;
    bool overstep;
};