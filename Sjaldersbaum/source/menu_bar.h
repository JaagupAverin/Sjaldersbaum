#pragma once

#include <queue>
#include <SFML/Graphics.hpp>

#include "audio.h"
#include "particles.h"
#include "keyboard.h"
#include "mouse.h"
#include "text_props.h"
#include "resources.h"
#include "progressive.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// A bar that displays user-information/messages.
// Hidden by default; slides in if action-KeyBind (Escape) is held or a message is queued.
// If action-KeyBind is held down for a ~second, a specified command sequence is executed.
class MenuBar : public sf::Drawable
{
public:
    MenuBar();

    void update_keyboard_input(const Keyboard& keyboard);

    void update(Seconds elapsed_time);

    void set_opacity(float opacity, Seconds progression_duration);

    // Note that time_played will be updated locally.
    void set_current_user_data(const ID& id, Seconds time_played);

    void set_action(const std::string& command_sequence, const std::string& description,
                    const std::string& sound_path);

    // Queues the specified message to be shown instead of the usual user-information.
    void queue_message(const std::string& message);

    void clear_messages();

    void set_width(Px width);

private:
    void position_objects();

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite bg;

    SoundID action_sound;

    TextProps text_props;
    Px max_text_height;

    sf::Text user_id;
    sf::Text time_display;
    Seconds  time_played;
    Seconds  second_counter;
    Seconds  action_cooldown;

    sf::Text    action_description;
    sf::Sprite  action_bar;
    float       action_progress;
    bool        action_key_held;
    std::string action_command_sequence;

    sf::Text                message;
    std::queue<std::string> message_queue;
    Seconds                 message_time_remaining;
    bool                    message_mode;

    PxVec2 size;
    ProgressivePxVec2 position;
    Seconds inactivity_lag;
    sf::Shader alpha_shader;
    ProgressiveFloat opacity;
};