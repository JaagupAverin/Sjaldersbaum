#pragma once

#include <vector>
#include <SFML/Graphics.hpp>

#include "colors.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

struct ParticleExplosion
{
    sf::Color color1;
    sf::Color color2;
    PxPerSec  speed;
    Seconds   lifetime;

    int triangles = 800;

    Degree min_angle = 0.f;
    Degree max_angle = 360.f;
};

const ParticleExplosion EMPTY_EXPLOSION{ sf::Color(), sf::Color(), 0.f, 0.f, 0 };

/*------------------------------------------------------------------------------------------------*/

class ParticleSystem : public sf::Drawable
{
    struct Particle
    {
        PxVec2 velocity;
        Seconds lifetime;
        sf::Uint8 base_alpha;
    };

    struct Explosion
    {
        Explosion() : lifetime{ 0.f } { vertices.setPrimitiveType(sf::Triangles); }

        std::vector<Particle> particles;
        sf::VertexArray vertices;
        Seconds lifetime;
    };

public:
    ParticleSystem();

    void update(Seconds elapsed_time);

    void create_explosion(PxVec2 source, const ParticleExplosion& explosion);

    void clear();

    bool is_idle() const;

private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    std::vector<Explosion> explosions;
    bool idle;
};