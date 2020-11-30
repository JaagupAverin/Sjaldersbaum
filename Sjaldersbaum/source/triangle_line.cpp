#include "triangle_line.h"

#include "maths.h"
#include "progressive.h"
#include "colors.h"

constexpr unsigned int LINE_TRIANGLE_COUNT = 20u;

TriangleLine::TriangleLine(const bool overstep) : 
    overstep{ overstep }
{
    multipliers.resize(LINE_TRIANGLE_COUNT * 3u);
    for (size_t i = 0u; i != multipliers.size() - 3u; i += 3)
    {
        // x: how far the vertex is along the line
        // y: how far the vertex is from the line
        // z: color blend factor

        multipliers[i].x = randf(0.8f, 1.f);
        multipliers[i].y = 0.f;
        multipliers[i].z = randf(0.f, 0.8f);

        multipliers[i + 1u].x = multipliers[i].x - randf(0.02f, 0.1f);
        multipliers[i + 1u].y = (rand(0, 2) == 0 ? 1.f : -1.f) * randf(0.1f, 0.5f);
        multipliers[i + 1u].z = randf(0.f, 1.f);

        multipliers[i + 2] = { randf(-0.1f, 0.1f), randf(-0.1f, 0.1f), randf(0.2f, 1.f) };
    }

    vertices.setPrimitiveType(sf::Triangles);
    vertices.resize(multipliers.size());
}

void TriangleLine::set_points(PxVec2 p1, PxVec2 p2)
{
    enum Axis { Horizontal, Vertical };
    const Axis line_axis =
        std::abs(p1.x - p2.x) > std::abs(p1.y - p2.y) ? Horizontal : Vertical;

    Px distance = get_distance(p1, p2);

    if (overstep)
    {
        const Px overstep = pow(distance, 0.7f) + 14.f;
        move_away_from(p1, p2, overstep * 1.0f);
        move_away_from(p2, p1, overstep * 0.4f);

        distance = get_distance(p1, p2);
    }

    const Px line_width = pow(distance, 0.4f) + 6.f;

    for (size_t i = 0u; i != vertices.getVertexCount(); ++i)
    {
        auto& pos = vertices[i].position = p1;
        move_towards(pos, p2, multipliers[i].x * distance);
        (line_axis == Horizontal ? pos.y : pos.x) += multipliers[i].y * line_width;
    }
}

void TriangleLine::set_color(const sf::Color color, const float opacity)
{
    for (size_t i = 0; i != vertices.getVertexCount(); ++i)
    {
        vertices[i].color = blend(Colors::BLACK_SEMI_TRANSPARENT, color, multipliers[i].z);
        vertices[i].color.a = static_cast<sf::Uint8>(vertices[i].color.a * opacity);
    }
}

void TriangleLine::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(vertices, states);
}