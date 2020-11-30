#pragma once

#include <SFML/Graphics.hpp>

#include "resources.h"
#include "progressive.h"
#include "particles.h"
#include "hoverable_detail.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Specialized sprite for visualizing a state (Positive, Negative, Neutral).
class Stamp : public sf::Drawable, public HoverableDetail
{
public:
    enum class Type
    {
        Positive,
        Negative,
        Neutral,

        Count
    };

    Stamp();

    void update(Seconds elapsed_time);

    // Texture must contain every stamp-type, placed in a row in matching order with the enums.
    void set_texture(const std::string& path);
    
    void set_explosions(ParticleExplosion positive,
                        ParticleExplosion negative,
                        ParticleExplosion neutral);

    // Lock effect reduces the color of the stamp. Also reduces visual effect of the transition.
    void set_type(Type type, bool explosion_effect = false, bool lock_effect = false);
    void set_base_size(PxVec2 size);
    void set_center(PxVec2 center);
    void set_opacity(float opacity);

    bool is_idle() const;

private:
    void set_state(State state) override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite old_stamp;
    sf::Sprite stamp;
    Type type;

    ProgressiveColor  old_stamp_color;
    ProgressivePxVec2 stamp_size;
    ProgressiveColor  stamp_color;
    float opacity_multiplier;

    ParticleExplosion positive_explosion;
    ParticleExplosion negative_explosion;
    ParticleExplosion neutral_explosion;

    sf::Vector2i texture_rect_size;
    PxVec2 base_size;
    PxVec2 default_size;
    PxVec2 center;

    ParticleSystem particles;

    bool idle;
};