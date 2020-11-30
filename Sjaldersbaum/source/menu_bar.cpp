#include "menu_bar.h"

#include "colors.h"
#include "maths.h"
#include "commands.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Px MENU_BAR_HEIGHT = 40.f;

constexpr Seconds MESSAGE_DURATION      = 3.8f;
constexpr Seconds SLIDE_DURATION        = 0.2f;
constexpr Seconds AUTO_HIDE_INTERVAL    = MESSAGE_DURATION - SLIDE_DURATION;

constexpr Seconds ACTION_DELAY          = 0.3f;
constexpr Seconds ACTION_HOLD_DURATION  = 0.6f;
constexpr Seconds ACTION_DECAY_DURATION = 1.5f;
constexpr Seconds ACTION_COOLDOWN       = 1.0f;

const std::string FONT_PATH = "resources/fonts/leander.ttf";
const std::string BG_PATH   = "resources/textures/system/menu.png";

const std::string ALPHA_SHADER_PATH = "resources/shaders/alpha.vert";

const std::string EXIT_COMMAND = "exit";

/*------------------------------------------------------------------------------------------------*/

// Returns time_played as a (HH:)MM:SS string.
std::string get_formatted_time(const Seconds time_played)
{
    const int total = static_cast<int>(std::round(time_played));

    const int h = static_cast<int>(std::floor(total / 3600.f));
    const int m = static_cast<int>(std::floor(total % 3600 / 60.f));
    const int s = static_cast<int>(std::floor(total % 3600 % 60));

    std::stringstream buffer;
    buffer << std::setfill('0');

    if (h != 0)
        buffer << h << ':';

    buffer
        << std::setw(2) << m
        << std::setw(1) << ":"
        << std::setw(2) << s;

    return buffer.str();
}

/*------------------------------------------------------------------------------------------------*/

MenuBar::MenuBar() : 
    action_sound          { UNINITIALIZED_SOUND },
    action_cooldown       { 0.f },
    time_played           { 0.f },
    second_counter        { 0.f },
    action_progress       { 0.f },
    action_key_held       { false },
    message_time_remaining{ 0.f },
    message_mode          { false },
    inactivity_lag        { 0.f },
    position              { { 0.f, -MENU_BAR_HEIGHT } },
    opacity               { 0.f }
{
    texture.load(BG_PATH);

    bg.setTexture(texture.get());
    bg.setColor(Colors::BLACK);

    action_bar.setTexture(texture.get());
    action_bar.setColor(Colors::CRIMSON);

    text_props.font.load(FONT_PATH);
    text_props.height  = MENU_BAR_HEIGHT * 0.5f;
    text_props.fill    = Colors::WHITE;
    text_props.outline = Colors::BLACK;
    text_props.outline_thickness = 3.f;
    max_text_height = text_props.get_max_height();

    std::vector<sf::Text*> texts{ &user_id, &action_description, &time_display, &message };
    for (auto text : texts)
        text_props.apply(*text);

    // Time display is the only text with a static origin point:
    time_display.setOrigin(0, max_text_height / 2.f);

    if (!alpha_shader.loadFromFile(ALPHA_SHADER_PATH, sf::Shader::Type::Vertex))
        LOG_ALERT("alpha shader could not be loaded from:\n" + ALPHA_SHADER_PATH);

    position.set_progression_duration(SLIDE_DURATION);
}

void MenuBar::update_keyboard_input(const Keyboard& keyboard)
{
    if (keyboard.is_keybind_held(DefaultKeybinds::ESCAPE))
        action_key_held = true;
}

void MenuBar::update(const Seconds elapsed_time)
{
    position.update(elapsed_time);
    if (position.has_changed_since_last_check())
        position_objects();

    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
        alpha_shader.setUniform("alpha", opacity.get_current());

    // Time display:

    second_counter += elapsed_time;
    time_played    += elapsed_time;
    if (second_counter >= 0.f)
    {
        time_display.setString(get_formatted_time(time_played));
        second_counter = 0.f;
    }

    // Messages:

    if (!message_queue.empty())
        message_mode = true;

    if (message_mode)
    {
        message_time_remaining -= (message_queue.size() + 1) * elapsed_time;

        if (message_time_remaining <= 0.f)
        {
            if (message_queue.empty())
                message_mode = false;
            else
            {
                message.setString(message_queue.front());
                message.setOrigin(message.getLocalBounds().width / 2.f, max_text_height / 2.f);

                message_queue.pop();

                message_time_remaining = MESSAGE_DURATION;
                inactivity_lag = 0.f;
            }
        }
    }

    // Action:

    action_cooldown -= elapsed_time;

    const float action_progress_prev = action_progress;

    if (action_key_held)
    {
        if (action_progress < 1.f)
            action_progress += (action_command_sequence == EXIT_COMMAND ? 0.6f : 1.f) *
                               (1.f / ACTION_HOLD_DURATION) * elapsed_time;

        if (action_progress >= 1.f && action_cooldown <= 0.f)
        {
            Executor::instance().queue_execution(action_command_sequence);
            AudioPlayer::instance().play(action_sound);
            action_cooldown = ACTION_COOLDOWN;
        }

        inactivity_lag = 0.f;
    }
    else
    {
        if (action_progress > -ACTION_DELAY && action_cooldown <= 0.f)
            action_progress -= (1.f / ACTION_DECAY_DURATION) * elapsed_time;

        inactivity_lag += elapsed_time;
    }
    action_key_held = false;

    if (action_progress_prev != action_progress)
    {
        float action_bar_x_size_multiplier = std::round(action_progress * 100.f) / 100.f;
        assure_bounds(action_bar_x_size_multiplier, 0.f, 1.f);

        action_bar.setTextureRect(sf::IntRect{ 0, 0,
            static_cast<int>(std::round(texture.get().getSize().x * action_bar_x_size_multiplier)),
            static_cast<int>(texture.get().getSize().y) });
        set_size(action_bar, { size.x * action_bar_x_size_multiplier, size.y });
    }

    // Auto-hide after inactivity:

    if (inactivity_lag >= AUTO_HIDE_INTERVAL)
        position.set_target({ 0.f, -MENU_BAR_HEIGHT });
    else
        position.set_target({ 0.f, 0.f });
}

void MenuBar::set_opacity(const float opacity, const Seconds progression_duration)
{
    this->opacity.set_progression_duration(progression_duration);
    this->opacity.set_target(opacity);
}

void MenuBar::set_current_user_data(const ID& id, const Seconds time_played)
{
    user_id.setString(id);
    user_id.setOrigin(user_id.getLocalBounds().width, max_text_height / 2.f);

    this->time_played = time_played;
}

void MenuBar::set_action(const std::string& command_sequence, const std::string& description,
                         const std::string& sound_path)
{
    action_progress = 0.f;
    action_bar.setTextureRect(sf::IntRect{ 0, 0, 0, 0});
    action_command_sequence = command_sequence;

    action_description.setString(description);
    action_description.setOrigin(action_description.getLocalBounds().width / 2.f,
                                 max_text_height / 2.f);

    action_sound = AudioPlayer::instance().load(sound_path, false);
}

void MenuBar::queue_message(const std::string& message)
{
    message_queue.emplace(message);
}

void MenuBar::clear_messages()
{
    std::queue<std::string> empty;
    std::swap(message_queue, empty);
    message_time_remaining = 0.f;
    message_mode = false;
}

void MenuBar::set_width(const Px width)
{
    size = { width, MENU_BAR_HEIGHT };

    set_size(bg, size);
    set_size(action_bar, { 0.f, 0.f });

    position_objects();
}

void MenuBar::position_objects()
{
    static constexpr Px m = 10.f;

    const PxVec2 p = position.get_current();

    bg.setPosition(                round_hu({ p.x,                p.y }));
    action_bar.setPosition(        round_hu({ p.x,                p.y }));
    user_id.setPosition(           round_hu({ p.x + size.x - m,   p.y + size.y / 2.f }));
    action_description.setPosition(round_hu({ p.x + size.x / 2.f, p.y + size.y / 2.f }));
    time_display.setPosition(      round_hu({ p.x + m,            p.y + size.y / 2.f }));
    message.setPosition(           round_hu({ p.x + size.x / 2.f, p.y + size.y / 2.f }));
}

void MenuBar::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
    {
        if (opacity.get_current() != 1.f)
            states.shader = &alpha_shader;

        target.draw(bg, states);
        target.draw(action_bar, states);

        if (message_mode)
            target.draw(message, states);
        else
        {
            target.draw(user_id, states);
            target.draw(time_display, states);
            target.draw(action_description, states);
        }
    }
}