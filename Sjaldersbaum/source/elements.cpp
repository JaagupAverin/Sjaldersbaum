#include "elements.h"

#include "events-requests.h"
#include "string_assist.h"
#include "colors.h"
#include "maths.h"

/*------------------------------------------------------------------------------------------------*/

// For all kinds of paddings.
constexpr Px MARGIN = 7.f;

constexpr Seconds INTERACTION_COOLDOWN = 0.1f;

constexpr Seconds OPACITY_PROGRESSION_DURATION = 0.5f;

const std::string SFML_LOGO_PATH = "resources/textures/images/sfml.png";

/*------------------------------------------------------------------------------------------------*/

const std::unordered_map<std::string, Element::Type> KNOWN_ELEMENT_TYPES
{
    { "image",          Element::Type::Image },
    { "text",           Element::Type::Text },
    { "button",         Element::Type::Button },
    { "inputline",      Element::Type::InputLine }
};

Element::Element(const bool activatable, const Type type) :
    Entity(activatable ? EntityConfigs::ACTIVATABLE_ELEMENT : EntityConfigs::INACTIVATABLE_ELEMENT),
    type{ type },
    opacity{ 1.f }
{
    opacity.set_progression_duration(OPACITY_PROGRESSION_DURATION);
}

void Element::on_setting_visible()
{
    this->set_idle(false);

    if (this->is_visible())
        this->is_initialized() ? opacity.set_target(1.f) : opacity.set_current(1.f);
    else
        this->is_initialized() ? opacity.set_target(0.f) : opacity.set_current(0.f);
}

std::shared_ptr<Element> create_element(const YAML::Node& node)
{
    if (!node.IsDefined())
    {
        LOG_ALERT("undefined node.");
        return nullptr;
    }

    std::shared_ptr<Element> element;
    Element::Type type = Element::Type::Unimplemented;

    try
    {
        type = Convert::str_to_enum(node["type"].as<std::string>(), KNOWN_ELEMENT_TYPES);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("Type not resolved; exception: " + e.what() + "\nDUMP: " + YAML::Dump(node));
        return nullptr;
    }

    if (type == Element::Type::Image)
        element = std::make_shared<Image>();
    else if (type == Element::Type::Text)
        element = std::make_shared<Text>();
    else if (type == Element::Type::InputLine)
        element = std::make_shared<InputLine>();
    else if (type == Element::Type::Button)
        element = std::make_shared<Button>();
    else
    {
        LOG_ALERT("unimplemented Element Type.");
        return nullptr;
    }

    element->type = type;
    if (!element->initialize(node))
        return nullptr;

    // Note that elements are set as finalized by the Objects that own them.
    return element;
}

/*------------------------------------------------------------------------------------------------*/

// Returns the index of the character that is closest to the specified point.
// Note that the returned index is end-inclusive:
// if the point is beyond the final character, the text's length is returned instead
// (this specialized behaviour for InputString's end-inclusive indexing).
size_t find_character_index(const sf::Text& text, const PxVec2 point)
{
    size_t closest_char_index = 0;
    Px closest_char_distance = get_distance(point, text.findCharacterPos(0));

    for (size_t i = 0; i <= text.getString().getSize(); ++i)
    {
        const Px distance = get_distance(point, text.findCharacterPos(i));
        if (distance < closest_char_distance)
        {
            closest_char_index = i;
            closest_char_distance = distance;
        }
    }

    return closest_char_index;
}

/*------------------------------------------------------------------------------------------------*/
// Image:

Image::Image() : Element(false, Element::Type::Image)
{

}

void Image::update(Seconds elapsed_time)
{
    if (this->is_idle())
        return;

    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
        image.setColor(blend(Colors::WHITE_TRANSPARENT, color, opacity.get_current()));
    if (!opacity.is_progressing())
        this->set_idle(true);
}

void Image::on_reposition()
{
    if (!this->is_initialized())
        return; // Elements are positioned properly only after their initialization.

    image.setPosition(round_hu(this->get_tlc()));
}

bool Image::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node texture_node = node["texture"];
        const YAML::Node size_node    = node["size"];
        const YAML::Node color_node   = node["color"];

        texture.load(texture_node.IsDefined() ? texture_node.as<std::string>() : SFML_LOGO_PATH);
        image.setTexture(texture.get(), true);

        PxVec2 size;
        if (size_node.IsDefined())
        {
            size = size_node.as<PxVec2>();
            if (!(assure_bounds(size.x, 1.f, PX_LIMIT) &
                  assure_bounds(size.y, 1.f, PX_LIMIT)))
                LOG_ALERT("invalid size had to be adjusted.");
        }
        else
            size = PxVec2{ texture.get().getSize() };
        this->disclose_size(size);
        ::set_size(image, this->get_size());

        color = color_node.IsDefined() ? color_node.as<sf::Color>() : Colors::WHITE;
        image.setColor(color);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "==========================================\n"
                  "* texture: <std::string> = <SFML_LOGO>\n"
                  "* size:    <PxVec2>      = <TEXTURE_SIZE>\n"
                  "==ADVANCED================================\n"
                  "* color: <sf::Color> = <WHITE>\n"
                  "==========================================\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }

    return true;
}

void Image::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
        target.draw(image, states);
}

/*------------------------------------------------------------------------------------------------*/
// Text:

Text::Text() : Element(false, Element::Type::Text)
{

}

void Text::update(const Seconds elapsed_time)
{
    if (this->is_idle())
        return;

    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
    {
        text.setFillColor(blend(Colors::TRANSPARENT, text_props.fill, opacity.get_current()));
        text.setOutlineColor(blend(Colors::TRANSPARENT, text_props.outline, opacity.get_current()));
    }
    if (!opacity.is_progressing())
        this->set_idle(true);
}

void Text::set_string(const std::string& str)
{
    text.setString(str);
    disclose_size({ text.getLocalBounds().width, text.getLocalBounds().height });
}

void Text::on_reposition()
{
    if (!this->is_initialized())
        return; // Elements are positioned properly only after their initialization.

    text.setPosition(round_hu(this->get_tlc()));
}

bool Text::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node text_node       = node["text"];
        const YAML::Node text_props_node = node["text_props"];

        text.setString(text_node.IsDefined() ? text_node.as<std::string>() : "Placeholder");

        if (!text_props.initialize(text_props_node))
            throw YAML::Exception{ YAML::Mark::null_mark(), "invalid text_props node." };
        text_props.apply(text);

        this->disclose_size({ text.getLocalBounds().width, text.getLocalBounds().height });
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "============================================\n"
                  "* text:       <std::string> = \"Placeholder\"\n"
                  "* text_props: <TextProps>   = <SYSTEM>\n"
                  "============================================\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }

    return true;
}

void Text::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
        target.draw(text, states);
}

/*------------------------------------------------------------------------------------------------*/
// Action:

Action::Action() :
    repeatable{ true },
    executed{ false },
    delay{ 0.f },
    delay_remaining{ 0.f }
{

}

void Action::update(const Seconds elapsed_time)
{
    if (delay_remaining > 0.f)
    {
        delay_remaining -= elapsed_time;
        if (delay_remaining <= 0.f)
        {
            Executor::instance().queue_execution(formatted_command_sequence);
            executed = true;
        }
    }
}

void Action::initiate_execution(const std::string& arg) 
{
    if (is_executable())
    {
        formatted_command_sequence = command_sequence;
        find_and_replace(formatted_command_sequence, "{}", arg);

        if (delay == 0.f)
        {
            Executor::instance().queue_execution(formatted_command_sequence);
            executed = true;
        }
        else
            delay_remaining = delay;
    }
}

void Action::set_delay(const Seconds delay)
{
    this->delay = delay;
}

bool Action::is_executable() const
{
    return repeatable || (!executed && delay_remaining <= 0.f);
}

bool Action::is_idle() const
{
    // While the delay is ticking, execution has been initiated and must be waited out:
    return delay_remaining <= 0.f;
}

bool Action::initialize(const YAML::Node& node)
{
    command_sequence = "message(\"Placeholder. Command not set for action!\")";
    repeatable       = true;
    executed         = false;

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node command_sequence_node = node["command"];
            const YAML::Node repeatable_node       = node["repeatable"];
            const YAML::Node executed_node         = node["executed"];

            if (command_sequence_node.IsDefined())
                command_sequence = command_sequence_node.as<std::string>();

            if (repeatable_node.IsDefined())
                repeatable = repeatable_node.as<bool>();

            if (executed_node.IsDefined())
                executed = executed_node.as<bool>();
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "==================================================\n"
                      "* command: <std::string> = message(\"Placeholder\")\n"
                      "==ADVANCED========================================\n"
                      "* repeatable: <bool> = true\n"
                      "* executed:   <bool> = false\n"
                      "==================================================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    return true;
}

YAML::Node Action::serialize_dynamic_data() const
{
    YAML::Node node{ YAML::NodeType::Map };
    node["executed"] = executed;
    return node;
}

/*------------------------------------------------------------------------------------------------*/
// Button:

const std::string CASLAME_PATH          = "resources/fonts/caslame.ttf";
const std::string LOCK_STAMPS_PATH      = "resources/textures/system/lock_stamps.png";
const std::string BUTTON_HIGHLIGHT_PATH = "resources/textures/system/button_highlight.png";

constexpr Seconds BUTTON_ACTION_DELAY = 0.5f;

Button::Button() : Element(true, Element::Type::Button),
    action_sound{ UNINITIALIZED_SOUND },
    text_and_highlight_enabled{ false },
    action_cooldown{ 0.f },
    locked{ false }
{

}

void Button::update_indicator_input(const Indicator& indicator)
{
    if (locked)
        return;

    indicator.set_type(Indicator::Type::HoveringButton);

    if (indicator.is_interaction_key_pressed() && action_cooldown <= 0.f)
    {
        if (action.is_executable())
            action.initiate_execution();
        else
            EARManager::instance().queue_event(Event::DisplayMessage,
                                               "You have already done that! Move on, man.");

        this->set_idle(false);
        stamp.set_type(Stamp::Type::Positive, true);
        action_cooldown = INTERACTION_COOLDOWN;

        AudioPlayer::instance().play(action_sound);
    }
}

void Button::update(const Seconds elapsed_time)
{
    if (this->is_idle())
        return;

    if (text_and_highlight_enabled)
        highlight.update(elapsed_time);
    stamp.update(elapsed_time);

    action.update(elapsed_time);
    action_cooldown -= elapsed_time;

    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
    {
        highlight.set_opacity(opacity.get_current());
        stamp.set_opacity(opacity.get_current());
        text.setFillColor(blend(Colors::TRANSPARENT, text_props.fill, opacity.get_current()));
        text.setOutlineColor(blend(Colors::TRANSPARENT, text_props.outline, opacity.get_current()));
    }

    if (!opacity.is_progressing() &&
        (!text_and_highlight_enabled || highlight.is_idle())
        && stamp.is_idle() && action.is_idle() && action_cooldown <= 0.f)
        this->set_idle(true);
}

void Button::set_locked(const bool locked)
{
    this->locked = locked;
    this->set_idle(false);
    highlight.set_color(locked ? sf::Color{ 0, 0, 0, 220 } : highlight_color);
    stamp.set_type(locked ? Stamp::Type::Negative : Stamp::Type::Neutral, this->is_initialized());

    if (!locked && this->is_initialized())
        AudioPlayer::instance().play(reveal_sound);
}

bool Button::is_activatable() const
{
    return !locked;
}

void Button::on_reposition()
{
    if (!this->is_initialized())
        return; // Elements are positioned properly only after their initialization.

    highlight.set_center(this->get_center());

    text.setPosition(round_hu({ this->get_tlc().x + stamp_side.x,
                                this->get_tlc().y + MARGIN }));
    stamp.set_center({ this->get_tlc().x + stamp_side.x / 2.f,
                       this->get_center().y });
}

void Button::on_setting_hovered()
{
    this->set_idle(false);

    highlight.set_hovered(this->is_hovered());
    stamp.set_hovered(this->is_hovered());

    if (this->is_hovered())
        AudioPlayer::instance().play(GlobalSounds::GENERIC_HOVER);
}

bool Button::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node text_node = node["text"];
        if (text_node.IsDefined())
        {
            text_and_highlight_enabled = true;

            text.setString(text_node.as<std::string>());

            text_props.font.load(CASLAME_PATH);
            text_props.style = sf::Text::Style::Bold;
            text_props.height = 26;
            text_props.fill = Colors::BLACK;
            text_props.outline = sf::Color{ 0, 0, 0, 40 };
            text_props.outline_thickness = 3.f;
            text_props.letter_spacing_multiplier = 0.8f;

            const YAML::Node text_props_node = node["text_props"];
            if (text_props_node.IsDefined())
                if (!text_props.initialize(text_props_node))
                    throw YAML::Exception{ YAML::Mark::null_mark(), "invalid text_props node." };
            text_props.apply(text);
            
            const Px height = text_props.get_max_height() + MARGIN;
            stamp_side = { height, height };
            stamp.set_base_size(stamp_side);
            const Px width = text.getLocalBounds().width + stamp_side.x + 2.f * MARGIN;
            this->disclose_size({ width, height });

            highlight.set_base_size(this->get_size());
            highlight.set_texture(BUTTON_HIGHLIGHT_PATH);
            highlight.set_size_margins({ 5.f, 2.5f }, { 5.f, 2.5f });
        }
        else
        {
            text_and_highlight_enabled = false;

            const YAML::Node size_node = node["size"];
            stamp_side = size_node.IsDefined() ? size_node.as<PxVec2>() : PxVec2{ 50.f, 50.f };
            this->disclose_size(stamp_side);
            stamp.set_base_size(stamp_side);
        }

        std::string stamp_texture_path;
        const YAML::Node stamp_texture_node = node["texture"];
        if (stamp_texture_node.IsDefined())
            stamp_texture_path = stamp_texture_node.as<std::string>();
        else
            stamp_texture_path = LOCK_STAMPS_PATH;
        stamp.set_texture(stamp_texture_path);

        const YAML::Node color_node = node["color"];
        highlight_color = color_node.IsDefined() ? color_node.as<sf::Color>() : Colors::GOLD;

        const ParticleExplosion interaction = highlight_color.a == 0U ? EMPTY_EXPLOSION :
                                              ParticleExplosion{ highlight_color,
                                                                 Colors::BLACK,
                                                                 360.f, 0.5f };

        const ParticleExplosion unlock = highlight_color.a == 0U ? EMPTY_EXPLOSION :
                                         ParticleExplosion{ highlight_color,
                                                            sf::Color{ 1, 1, 1, 0 },
                                                            400.f, 0.7f };

        stamp.set_explosions(interaction, EMPTY_EXPLOSION, unlock);

        const YAML::Node action_node = node["action"];
        if (!action.initialize(action_node))
            throw YAML::Exception{ YAML::Mark::null_mark(), "invalid action node." };
        action.set_delay(BUTTON_ACTION_DELAY);

        const YAML::Node locked_node = node["locked"];
        set_locked(locked_node.IsDefined() ? locked_node.as<bool>() : false);

        const YAML::Node sound_path_node = node["sound"];
        if (sound_path_node.IsDefined())
            action_sound = AudioPlayer::instance().load(sound_path_node.as<std::string>(), false);
        else
            action_sound = GlobalSounds::LOCKS_HIT;
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "========================================\n"
                  "* text:    <std::string> = \"\"\n"
                  "* action:  <Action>      = <DEFAULT>\n"
                  "==ADVANCED==============================\n"
                  "* text_props: <TextProps>   = <CASLAME>\n"
                  "* texture:    <std::string> = <LOCKS>\n"
                  "* color:      <sf::Color>   = <GOLD>\n"
                  "* size:       <PxVec2>      = (50, 50)\n"
                  "* locked:     <bool>        = false\n"
                  "* sound:      <std::string> = <LOCKS>\n"
                  "========================================\n"
                  "The 'texture' node must consist of 3 adjacent icons representing:\n"
                  "the following states: INTERACTION | UNLOCKED | LOCKED.\n"
                  "The 'size' node is only relevant if no 'text' is specified.\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }

    return true;
}

YAML::Node Button::on_dynamic_data_serialization() const
{
    YAML::Node node{ YAML::NodeType::Map };
    node["action"] = action.serialize_dynamic_data();
    node["locked"] = locked;
    return node;
}

void Button::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
    {
        if (text_and_highlight_enabled)
        {
            target.draw(highlight, states);
            target.draw(text, states);
        }
        target.draw(stamp, states);
    }
}

/*------------------------------------------------------------------------------------------------*/
// InputLine:

// Compares two strings after performing the following procedure on both:
// 1) decapitalize;
// 2) remove whitespace;
// 3) remove apostrophes;
// 4) replace commas with dots;
bool matches_semantically(std::string str1, std::string str2)
{
    auto apply_procedure = [](std::string& str)
    {
        decapitalize(str);
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
        str.erase(std::remove(str.begin(), str.end(), '\''), str.end());
        std::replace(str.begin(), str.end(), ',', '.');
    };

    apply_procedure(str1);
    apply_procedure(str2);

    return str1 == str2;
}

/*------------------------------------------------------------------------------------------------*/

const std::unordered_map<std::string, CharChecker> KNOWN_CHAR_CHECKERS
{
    { "graphic",    CharChecker::Graphic },
    { "numeric",    CharChecker::Numeric },
    { "systemic",   CharChecker::Systemic },
    { "usernamic",  CharChecker::Usernamic }
};

const std::string DEFAULT_SOLUTION_KEY = "DEFAULT";

const std::string MYUNDERWOOD_PATH      = "resources/fonts/myunderwood.ttf";
const std::string INPUTLINE_STAMPS_PATH = "resources/textures/system/inputline_stamps.png";
const std::string INPUTLINE_LONG_HIGHLIGHT_PATH =
    "resources/textures/system/inputline_long_highlight.png";  // expected AR: 9:1
const std::string INPUTLINE_SHORT_HIGHLIGHT_PATH =
    "resources/textures/system/inputline_short_highlight.png"; // expected AR: 4:1

namespace InputLineColors
{
const sf::Color LOCKED   = sf::Color{ 10,  10,  10, 220 };
const sf::Color POSITIVE = sf::Color{ 20,  250, 20      };
const sf::Color NEGATIVE = sf::Color{ 250, 0,   40      };
const sf::Color NEUTRAL  = sf::Color{ 40,  120, 250     };
}

namespace InputLineExplosions
{
const ParticleExplosion POSITIVE{ InputLineColors::POSITIVE, Colors::BLACK_SEMI_TRANSPARENT,
                                  280, 1.2f, 1200 };

const ParticleExplosion NEGATIVE{ InputLineColors::NEGATIVE, Colors::BLACK_SEMI_TRANSPARENT,
                                  200.f, 1.0f };

const ParticleExplosion NEUTRAL{ InputLineColors::NEUTRAL, Colors::BLACK_SEMI_TRANSPARENT,
                                 220.f, 0.8f };
}

constexpr Seconds INPUTLINE_ACTION_DELAY         = 2.0f;
constexpr Seconds INPUTLINE_DEFAULT_ACTION_DELAY = 0.5f;

const sf::Color UNDERLINE_COLOR = { 1u, 1u, 1u, 1u };
constexpr float DEFAULT_UNDERLINE_OPACITY = 0.3f;
constexpr float ACTIVE_UNDERLINE_OPACITY  = 0.5f;

/*------------------------------------------------------------------------------------------------*/

InputLine::InputLine() : Element(true, Element::Type::InputLine),
    theme{ Theme::Default },
    input_sound{ UNINITIALIZED_SOUND },
    stamp_side{ 0.f },
    underline_opacity{ DEFAULT_UNDERLINE_OPACITY },
    underline{ false },
    default_action{ false },
    commit_cooldown{ 0.f },
    input_save{ true },
    auto_clear{ false },
    locked{ false }
{
    highlight.set_size_margins({ 6.f, 3.f }, { 10.f, 5.f });

    stamp.set_texture(INPUTLINE_STAMPS_PATH);
    stamp.set_explosions(InputLineExplosions::POSITIVE,
                         InputLineExplosions::NEGATIVE,
                         InputLineExplosions::NEUTRAL);

    underline_opacity.set_progression_duration(0.15f);

    caret.set_origin({ 0.5f, 1.f });
    caret.set_animation(Animations::CARET);
}

void InputLine::update_keyboard_input(const Keyboard& keyboard)
{
    if (locked)
        return;

    input.update_keyboard_input(keyboard);
    if (keyboard.is_keybind_pressed(DefaultKeybinds::ENTER))
        commit_input();
}

void InputLine::update_indicator_input(const Indicator& indicator)
{
    if (locked)
        return;

    const bool indicator_on_stamp =
        indicator.get_position().x < this->get_tlc().x + stamp_side + MARGIN;

    if (indicator_on_stamp)
        indicator.set_type(Indicator::Type::HoveringButton);
    else
        indicator.set_type(Indicator::Type::HoveringTextField);

    if (indicator.is_interaction_key_pressed())
    {
        if (indicator_on_stamp &&
            indicator.get_latest_input_source() == Indicator::InputSource::Mouse)
            commit_input();
        else
            input.set_index(find_character_index(text, indicator.get_position()));
    }
}

void InputLine::update(const Seconds elapsed_time)
{
    if (this->is_idle())
        return;

    highlight.update(elapsed_time);
    stamp.update(elapsed_time);
    if (this->is_active())
        caret.update_frame(elapsed_time);

    bool all_actions_idle = true;
    for (auto& [key, action] : solutions)
    {
        action.update(elapsed_time);
        if (!action.is_idle())
            all_actions_idle = false;
    }
    commit_cooldown -= elapsed_time;

    if (input.has_string_been_altered())
    {
        text.setString(this->input.get_string());
        AudioPlayer::instance().play(input_sound);
    }
    if (input.has_index_been_altered())
    {
        position_caret();
        caret.start();
        AudioPlayer::instance().play(input_sound);
    }

    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
    {
        highlight.set_opacity(opacity.get_current());
        stamp.set_opacity(opacity.get_current());
        text.setFillColor(blend(Colors::TRANSPARENT, text_props.fill, opacity.get_current()));
        text.setOutlineColor(blend(Colors::TRANSPARENT, text_props.outline, opacity.get_current()));
    }
    underline_opacity.update(elapsed_time);
    if (underline_opacity.has_changed_since_last_check() || opacity.has_changed_since_last_check())
        underline.set_color(UNDERLINE_COLOR, underline_opacity.get_current() * opacity.get_current());

    if (!opacity.is_progressing() && !underline_opacity.is_progressing()
        && commit_cooldown <= 0.f && !this->is_active() &&
        highlight.is_idle() && stamp.is_idle() && all_actions_idle)
        this->set_idle(true);
}

void InputLine::set_locked(const bool locked)
{
    this->locked = locked;
    set_theme(theme);
    if (!locked && this->is_initialized())
        AudioPlayer::instance().play(reveal_sound);
}

bool InputLine::is_activatable() const
{
    return !locked;
}

void InputLine::set_input(const std::string& input)
{
    // Note that the string might be truncated, so 'input' != 'this->input.get_string()'
    this->input.set_string(input);

    last_committed_input = this->input.get_string();

    if (default_action)
    {
        set_theme(Theme::Default);
        return;
    }

    const std::string input_string = this->input.get_string();
    for (const auto& [key, command_sequence] : solutions)
        if (matches_semantically(input_string, key))
        {
            set_theme(Theme::CorrectInput);
            return;
        }
    set_theme(Theme::IncorrectInput);
}

void InputLine::set_theme(const Theme theme)
{
    this->set_idle(false);

    this->theme = theme;
    if (theme == Theme::CorrectInput)
    {
        stamp.set_type(Stamp::Type::Positive, this->is_initialized() && !locked, locked);
        highlight.set_color(locked ? InputLineColors::LOCKED : InputLineColors::POSITIVE);
    }
    else if (theme == Theme::IncorrectInput)
    {
        stamp.set_type(Stamp::Type::Negative, this->is_initialized() && !locked, locked);
        highlight.set_color(locked ? InputLineColors::LOCKED : InputLineColors::NEGATIVE);
    }
    else
    {
        stamp.set_type(Stamp::Type::Neutral, this->is_initialized() && !locked, locked);
        highlight.set_color(locked ? InputLineColors::LOCKED : InputLineColors::NEUTRAL);
    }
}

void InputLine::commit_input()
{
    if (commit_cooldown > 0.f)
        return;
    commit_cooldown = INTERACTION_COOLDOWN;

    AudioPlayer::instance().play(input_sound);

    // Since input is used to determine the InputLine's theme upon initialization,
    // we must have the last committed input stored for serialization.
    last_committed_input = input.get_string();

    const std::string input_string = input.get_string();
    if (auto_clear)
        input.clear();

    for (auto& [solution, action] : solutions)
    {
        if (matches_semantically(input_string, solution) && action.is_executable())
        {
            action.initiate_execution();
            set_theme(Theme::CorrectInput);
            AudioPlayer::instance().play(GlobalSounds::POSITIVE);
            return;
        }
    }

    if (default_action)
    {
        solutions.at(DEFAULT_SOLUTION_KEY).initiate_execution(input_string);
        set_theme(Theme::Default);
        AudioPlayer::instance().play(GlobalSounds::NEUTRAL);
    }
    else
    {
        set_theme(Theme::IncorrectInput);
        AudioPlayer::instance().play(GlobalSounds::NEGATIVE);
    }
}

void InputLine::position_underline()
{
    const PxRect bounds = this->get_bounds();

    // Bottom corners:
    const PxVec2 blc{ bounds.left + stamp_side * 0.5f, bounds.getBottom() - MARGIN * 0.1f };
    const PxVec2 brc{ bounds.getRight() - MARGIN,      bounds.getBottom() - MARGIN * 0.5f };
    underline.set_points(brc, blc);
}

void InputLine::position_caret()
{
    caret.set_position({ text.findCharacterPos(input.get_index()).x,
                                               this->get_bounds().getBottom() - MARGIN });
}

void InputLine::on_reposition()
{
    if (!this->is_initialized())
        return;

    highlight.set_center(this->get_center());

    text.setPosition(round_hu({
        this->get_tlc().x + MARGIN + stamp_side,
        this->get_tlc().y }));

    stamp.set_center({ this->get_tlc().x + stamp_side / 2.f,
                       this->get_center().y });

    position_underline();
    position_caret();
}

void InputLine::on_setting_hovered()
{
    this->set_idle(false);

    highlight.set_hovered(this->is_hovered());
    stamp.set_hovered(this->is_hovered());

    if (this->is_hovered() && !this->is_active())
        AudioPlayer::instance().play(GlobalSounds::GENERIC_HOVER);
}

void InputLine::on_setting_active()
{
    this->set_idle(false);

    highlight.set_active(this->is_active());
    stamp.set_active(this->is_active());

    if (this->is_active())
        underline_opacity.set_target(ACTIVE_UNDERLINE_OPACITY);
    else
        underline_opacity.set_target(DEFAULT_UNDERLINE_OPACITY);
}

bool InputLine::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node solutions_node        = node["solutions"];
        const YAML::Node text_props_node       = node["text_props"];
        const YAML::Node char_checker_node     = node["char_checker"];
        const YAML::Node length_node           = node["length"];
        const YAML::Node input_node            = node["input"];
        const YAML::Node input_save_node       = node["input_save"];
        const YAML::Node auto_clear_node       = node["auto_clear"];
        const YAML::Node locked_node           = node["locked"];
        const YAML::Node input_sound_path_node = node["input_sound"];

        default_action = false;
        if (solutions_node.IsDefined())
        {
            for (auto solution_node : solutions_node)
            {
                std::string key = solution_node.first.as<std::string>();
                if (contains(solutions, key))
                {
                    LOG_ALERT("solution key is not unique: " + key);
                    return false;
                }

                Action action;
                if (!action.initialize(solution_node.second))
                    throw YAML::Exception{ YAML::Mark::null_mark(), "invalid action node." };

                if (key == DEFAULT_SOLUTION_KEY)
                {
                    default_action = true;
                    action.set_delay(INPUTLINE_DEFAULT_ACTION_DELAY);
                }
                else
                    action.set_delay(INPUTLINE_ACTION_DELAY);

                solutions.emplace(std::move(key), std::move(action));
            }
        }
        else
        {
            default_action = true;
            solutions.emplace(DEFAULT_SOLUTION_KEY, Action());
        }

        text_props.font.load(MYUNDERWOOD_PATH);
        text_props.style = sf::Text::Style::Regular;
        text_props.height = 24.f;
        text_props.fill = { 0, 0, 0, 200 };
        text_props.outline = { 0, 0, 0, 40 };
        text_props.outline_thickness = 1;
        text_props.letter_spacing_multiplier = 0.6f;
        text_props.offsets = { 0.f, 11.f };

        if (text_props_node.IsDefined())
            if (!text_props.initialize(text_props_node))
                throw YAML::Exception{ YAML::Mark::null_mark(), "invalid text_props node." };
        text_props.apply(text);

        if (length_node.IsDefined())
        {
            int length = length_node.as<int>();
            if (!assure_greater_than_or_equal_to(length, 1))
                LOG_ALERT("non-positive length had to be adjusted.");
            input.set_max_length(length);
        }
        else
            input.set_max_length(20);

        if (char_checker_node.IsDefined())
        {
            const CharChecker char_checker = 
                Convert::str_to_enum(char_checker_node.as<std::string>(), KNOWN_CHAR_CHECKERS);
            input.set_char_checker(char_checker);
        }

        const Px height = text_props.get_max_height() + 2.f * MARGIN;
        stamp_side = height - MARGIN / 2.f;
        const Px width = text_props.get_max_width(input.get_max_length()) + stamp_side + MARGIN;
        this->disclose_size({ width, height });
        highlight.set_base_size(this->get_size());
        stamp.set_base_size({ stamp_side, stamp_side });
        caret.set_size({ 0.f, text_props.get_max_height() });

        const float aspect_ratio = this->get_size().x / this->get_size().y;
        if (aspect_ratio >= 6.f)
            highlight.set_texture(INPUTLINE_LONG_HIGHLIGHT_PATH);
        else
            highlight.set_texture(INPUTLINE_SHORT_HIGHLIGHT_PATH);

        set_input(input_node.IsDefined() ? input_node.as<std::string>() : "");
        input_save = input_save_node.IsDefined() ? input_save_node.as<bool>() : true;
        auto_clear = auto_clear_node.IsDefined() ? auto_clear_node.as<bool>() : false;

        if (input.has_string_been_altered())
            text.setString(input.get_string());
        if (input.has_index_been_altered())
            position_caret();

        set_locked(locked_node.IsDefined() ? locked_node.as<bool>() : false);

        if (input_sound_path_node.IsDefined())
            input_sound = AudioPlayer::instance().load(input_sound_path_node.as<std::string>(), false);
        else
            input_sound = GlobalSounds::TYPEWRITER;
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "=======================================================\n"
                  "* solutions:  map<std::string, Action> = {<DEFAULT>}\n"
                  "* text_props: <TextProps>              = <TYPEWRITER>\n"
                  "* length:     <int>                    = 20\n"
                  "==ADVANCED=============================================\n"
                  "* char_checker: <CharChecker> = <GRAPHIC>\n"
                  "* input:        <std::string> = \"\"\n"
                  "* input_save:   <bool>        = true\n"
                  "* auto_clear:   <bool>        = false\n"
                  "* locked:       <bool>        = false\n"
                  "* input_sound:  <std::string> = <TYPEWRITER>\n"
                  "=======================================================\n"
                  "Character checkers: [graphic, numeric, systemic]\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }

    return true;
}

YAML::Node InputLine::on_dynamic_data_serialization() const
{
    YAML::Node solutions_node{ YAML::NodeType::Map };
    for (const auto& [key, action] : solutions)
        solutions_node[key] = action.serialize_dynamic_data();

    YAML::Node node;
    node["solutions"] = solutions_node;
    if (input_save && !auto_clear && !last_committed_input.empty())
        node["input"] = last_committed_input;
    node["locked"] = locked;
    return node;
}

void InputLine::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
    {
        target.draw(underline, states);
        target.draw(highlight, states);
        target.draw(text, states);
        if (this->is_active())
            target.draw(caret, states);
        target.draw(stamp, states);
    }
}