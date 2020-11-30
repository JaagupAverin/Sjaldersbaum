#pragma once

#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Stores the metadata for traversing a framesheet.
struct Animation
{
    Animation(std::string framesheet_path,
              int         frame_columns,
              int         frame_rows,
              Seconds     frame_interval,
              bool    auto_reverse,
              bool    auto_restart,
              Seconds auto_reverse_delay = 0.f,
              Seconds auto_restart_delay = 0.f);

public:
    std::string framesheet_path;
    int         frame_columns;
    int         frame_rows;
    Seconds     frame_interval;

    bool    auto_reverse;
    bool    auto_restart;
    Seconds auto_reverse_delay;
    Seconds auto_restart_delay;
};

namespace Animations
{
const Animation PLACEHOLDER{ "resources/textures/system/placeholder_framesheet.png",
                             5, 1, 0.25f,
                             true, true, 0.5f, 0.5f };

const Animation CARET{ "resources/textures/system/caret_framesheet.png",
                       2, 1, 0.4f,
                       false, true, 0.0f, 0.2f };
}

/*------------------------------------------------------------------------------------------------*/

// Plays Animations.
class AnimationPlayer : public sf::Drawable
{
public:
    enum class State
    {
        Unassigned,
        Idle,

        ForwardTraverse,
        ReverseTraverse,

        WaitingBeforeReverse,
        WaitingBeforeRestart
    };

public:
    AnimationPlayer();

    void update_frame(Seconds elapsed_time);

    // Starts the animation from the beginning. (Irrelevant for auto-restarting animations).
    void start();

    void set_animation(const Animation& animation);

    // Scales the frame to match this size.
    // Using 0 as width/height means no scaling (default frame width/height).
    void set_size(PxVec2 size);

    // Sets the origin within the frame using factors between 0 and 1;
    void set_origin(sf::Vector2f origin_factors);

    void set_position(PxVec2 position);

    State get_state();

private:
    void traverse_frame();

    void scale_frame_to_size();

    void apply_origin_factors();

    void reset_frame();

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    Animation animation;

    TextureReference framesheet_texture;
    sf::Sprite framesheet;
    std::vector<sf::IntRect> frames;
    size_t frame_index;

    PxVec2 size;
    sf::Vector2f origin_factors;

    Seconds lag;

    State state;
};