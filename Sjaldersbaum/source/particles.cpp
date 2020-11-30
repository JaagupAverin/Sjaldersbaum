#include "particles.h"

#include <set>
#include <functional>

#include "maths.h"
#include "progressive.h"

constexpr int MAX_EXPLOSIONS = 4;

/*------------------------------------------------------------------------------------------------*/

ParticleSystem::ParticleSystem() :
    idle{ true }
{

}

void ParticleSystem::update(const Seconds elapsed_time)
{
    if (idle)
        return;

    for (auto& explosion : explosions)
    {
        // We handle vertices/particles in groups of 3 (triangles):
        for (size_t i = 2u; i < explosion.particles.size(); i += 3u)
        {
            auto& ps = explosion.particles;
            auto& vs = explosion.vertices;

            const Seconds lifetime = ps[i].lifetime -= elapsed_time;
            if (lifetime > 0.f)
            {
                if (lifetime < 1.f)
                    for (size_t j = 0; j != 3; ++j)
                        vs[i - j].color.a = static_cast<sf::Uint8>(ps[i - j].base_alpha * lifetime);

                for (size_t j = 0; j != 3; ++j)
                    vs[i - j].position += ps[i - j].velocity * elapsed_time;
            }
        }
    }
    
    auto it = explosions.begin();
    while (it != explosions.end())
    {
        const Seconds remaining = it->lifetime -= elapsed_time;
        if (remaining <= 0)
            it = explosions.erase(it);
        else
            it++;
    }

    if (explosions.empty())
        idle = true;
}

void ParticleSystem::create_explosion(const PxVec2 source, const ParticleExplosion& e)
{
    idle = false;

    // Logically, we should be popping the oldest/front element.
    // But aesthetically, popping the newest/back one looks best:
    if (explosions.size() == MAX_EXPLOSIONS)
        explosions.pop_back();

    int triangles = e.triangles;
    if (triangles == 0)
        return;

    Explosion explosion;
    explosion.particles.reserve(triangles * 3);
    explosion.lifetime = e.lifetime;

    for (int i = 0; i != triangles; ++i)
    {
        Particle p1, p2, p3;

        p3.lifetime = e.lifetime * randf(0.3f, 1.f);

        const Radian   common_angle = to_rad(randf(e.min_angle, e.max_angle));
        const PxPerSec common_speed = e.speed * randf(0.25f, 1.f);

        p1.velocity = { std::cos(common_angle) * common_speed,
                        std::sin(common_angle) * common_speed };
        p2.velocity = { std::cos(common_angle + to_rad(randf(20.f, 30.f))) * common_speed,
                        std::sin(common_angle + to_rad(randf(20.f, 30.f))) * common_speed };
        p3.velocity = { std::cos(common_angle - to_rad(randf(20.f, 30.f))) * common_speed,
                        std::sin(common_angle - to_rad(randf(20.f, 30.f))) * common_speed };

        const sf::Color p1_color = blend(e.color1, e.color2, randf(0.f, 1.f));
        const sf::Color p2_color = blend(e.color1, e.color2, randf(0.f, 1.f));
        const sf::Color p3_color = blend(e.color1, e.color2, randf(0.f, 1.f));

        p1.base_alpha = p1_color.a;
        p2.base_alpha = p2_color.a;
        p3.base_alpha = p3_color.a;

        explosion.particles.emplace_back(std::move(p1));
        explosion.particles.emplace_back(std::move(p2));
        explosion.particles.emplace_back(std::move(p3));

        const PxVec2 p1_source = { source.x + randf(-4.f, 4.f), source.y + randf(-4.f, 4.f)};
        const PxVec2 p2_source = { source.x + randf(-4.f, 4.f), source.y + randf(-4.f, 4.f)};
        const PxVec2 p3_source = { source.x + randf(-4.f, 4.f), source.y + randf(-4.f, 4.f)};

        explosion.vertices.append({ p1_source, p1_color });
        explosion.vertices.append({ p2_source, p2_color });
        explosion.vertices.append({ p3_source, p3_color });
    }

    explosions.emplace_back(std::move(explosion));
}

void ParticleSystem::clear()
{
    explosions.clear();
}

bool ParticleSystem::is_idle() const
{
    return idle;
}

void ParticleSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    for (const auto& explosion : explosions)
        target.draw(explosion.vertices, states);
}