#pragma once

#include <array>

#include "objects.h"
#include "progressive.h"
#include "units.h"
#include "indicator.h"
#include "triangle_line.h"

/*------------------------------------------------------------------------------------------------*/

// Represents the center of screen in most cases.
// Can be clasped to an Object, in which case it will surround and follow it instead.
class Crosshair : public sf::Drawable
{
public:
    Crosshair();

    void update(Seconds elapsed_time);

    void set_type(Indicator::Type type);
    void set_center(PxVec2 center, bool show = true);
    void set_visible(bool visible);

    void clasp(std::shared_ptr<Object> object);
    void unclasp();

    void on_interaction();

    void position_crosshair_lines();

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    std::array<TriangleLine, 4> crosshair;

    ProgressivePxVec2 size;
    ProgressivePxVec2 center;
    ProgressiveFloat  opacity;
    ProgressiveColor  color;

    PxVec2 default_size;
    PxVec2 default_center;

    std::shared_ptr<Object> clasped_object;
    bool clasped;

    Indicator::Type type;
    Seconds interaction_timer;
    Seconds inactivity_lag;
    bool visible;
};