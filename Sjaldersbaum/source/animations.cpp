#include "animations.h"

#include "logger.h"
#include "maths.h"
#include "convert.h"
#include "contains.h"

/*------------------------------------------------------------------------------------------------*/

Animation::Animation(std::string framesheet_path,
                     int         frame_columns,
                     int         frame_rows,
                     Seconds     frame_interval,
                     bool    auto_reverse,
                     bool    auto_restart,
                     Seconds auto_reverse_delay,
                     Seconds auto_restart_delay) :
    framesheet_path{ framesheet_path },
    frame_columns  { frame_columns },
    frame_rows     { frame_rows },
    frame_interval { frame_interval },
    auto_reverse      { auto_reverse },
    auto_restart      { auto_restart },
    auto_reverse_delay{ auto_reverse_delay },
    auto_restart_delay{ auto_restart_delay }
{

}

/*------------------------------------------------------------------------------------------------*/

// Divides a framesheet into equal-sized columns and rows, resulting in a grid of frames.
// These frames will be returned in a vector, ordered from left to right, top to bottom.
std::vector<sf::IntRect> calculate_frames(const sf::Vector2u framesheet_size,
                                          const int columns, const int rows)
{
    if ((columns * rows <= 0) ||
        (framesheet_size.x % columns != 0) || (framesheet_size.y % rows != 0))
        LOG_ALERT("invalid frames calculation input;\n"
                  "width: "   + Convert::to_str(framesheet_size.x) + "; "
                  "height: "  + Convert::to_str(framesheet_size.y) + "; "
                  "columns: " + Convert::to_str(columns) + "; " 
                  "rows: "    + Convert::to_str(rows));

    const int frame_width  = framesheet_size.x / columns;
    const int frame_height = framesheet_size.y / rows;

    std::vector<sf::IntRect> frames(columns * rows);
    int i = 0;
    for (int row = 0; row != rows; ++row)
    {
        for (int column = 0; column != columns; ++column)
        {
            frames[i] = { column * frame_width,
                          row    * frame_height,
                          frame_width,
                          frame_height };
            ++i;
        }
    }
    return frames;
}

/*------------------------------------------------------------------------------------------------*/

AnimationPlayer::AnimationPlayer() :
    animation  { Animations::PLACEHOLDER },
    frame_index{ 0 },
    lag        { 0.f },
    state      { State::Unassigned }
{

}

void AnimationPlayer::update_frame(const Seconds elapsed_time)
{
    if (state == State::Unassigned || state == State::Idle)
        return;

    lag += elapsed_time;

    if (state == State::WaitingBeforeReverse)
    {
        if (lag >= animation.auto_reverse_delay)
        {
            state = State::ReverseTraverse;
            lag -= animation.auto_reverse_delay;
        }
    }
    else if (state == State::WaitingBeforeRestart)
    {
        if (lag >= animation.auto_restart_delay)
        {
            state = State::ForwardTraverse;
            lag -= animation.auto_restart_delay;
        }
    }

    if (state == State::ForwardTraverse || state == State::ReverseTraverse)
    {
        if (lag >= animation.frame_interval)
        {
            traverse_frame();
            lag -= animation.frame_interval;
        }
    }
}

void AnimationPlayer::start()
{
    if (state == State::Unassigned)
    {
        LOG_ALERT("premature start call; unassigned animation.");
        return;
    }

    reset_frame();
    state = State::ForwardTraverse;
}

void AnimationPlayer::set_animation(const Animation& animation)
{
    this->animation = animation;

    framesheet_texture.load(animation.framesheet_path);
    framesheet.setTexture(framesheet_texture.get());

    frames = calculate_frames(framesheet_texture.get().getSize(),
                              animation.frame_columns,
                              animation.frame_rows);

    state = State::Idle;

    scale_frame_to_size();
    apply_origin_factors();

    if (animation.auto_restart)
        start();
    else
        reset_frame();
}

void AnimationPlayer::set_size(const PxVec2 size)
{
    this->size = size;
    if (state != State::Unassigned)
        scale_frame_to_size();
}

void AnimationPlayer::set_origin(sf::Vector2f origin_factors)
{
    if (!(assure_bounds(origin_factors.x, 0.f, 1.f) &
          assure_bounds(origin_factors.y, 0.f, 1.f)))
        LOG_ALERT("invalid origin factors had to be adjusted.");

    this->origin_factors = origin_factors;
    if (state != State::Unassigned)
        apply_origin_factors();
}

void AnimationPlayer::set_position(const PxVec2 position)
{
    framesheet.setPosition(round_hu(position));
}

AnimationPlayer::State AnimationPlayer::get_state()
{
    return state;
}

void AnimationPlayer::traverse_frame()
{
    if (state == State::ReverseTraverse)
    {
        if (frame_index == 0)
        {
            if (animation.auto_restart)
                state = State::WaitingBeforeRestart;
            else
                state = State::Idle;
        }
        else
            --frame_index;
    }
    else if (state == State::ForwardTraverse)
    {
        if (frame_index == frames.size() - 1)
        {
            if (animation.auto_reverse)
                state = State::WaitingBeforeReverse;
            else
            {
                if (animation.auto_restart)
                {
                    frame_index = 0;
                    state = State::WaitingBeforeRestart;
                }
                else
                    state = State::Idle;
            }
        }
        else
            ++frame_index;
    }
    
    framesheet.setTextureRect(frames.at(frame_index));
}

void AnimationPlayer::scale_frame_to_size()
{
    if (size.x == 0.f)
        size.x = static_cast<Px>(frames.at(0).width);
    if (size.y == 0.f)
        size.y = static_cast<Px>(frames.at(0).height);

    framesheet.setScale(size.x / static_cast<Px>(frames.at(0).width),
                        size.y / static_cast<Px>(frames.at(0).height));
}

void AnimationPlayer::apply_origin_factors()
{
    framesheet.setOrigin(static_cast<Px>(frames.at(0).width)  * origin_factors.x,
                         static_cast<Px>(frames.at(0).height) * origin_factors.y);
}

void AnimationPlayer::reset_frame()
{
    lag = 0.0f;
    frame_index = 0;
    framesheet.setTextureRect(frames.at(0));
}

void AnimationPlayer::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(framesheet, states);
}