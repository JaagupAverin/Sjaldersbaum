#pragma once

#include <SFML/Graphics.hpp>

#include "audio.h"
#include "entity.h"
#include "commands.h"
#include "keyboard.h"
#include "mouse.h"
#include "resources.h"
#include "animations.h"
#include "input_string.h"
#include "stamp.h"
#include "text_props.h"
#include "units.h"
#include "triangle_line.h"

/*------------------------------------------------------------------------------------------------*/

// Comprises Objects.
class Element : public Entity
{
public:
    enum class Type
    {
        Unimplemented,

        Image,
        Text,
        Button,
        InputLine,
    };
    Type type;

    Element(bool activatable, Type type);

protected:
    ProgressiveFloat opacity;
private:
    void on_setting_visible() override final;
};

// Expects a map that includes:
// =========================================
// * type: <Element::Type>
// =========================================
// Types: [image, text, button, inputline]
std::shared_ptr<Element> create_element(const YAML::Node& node);

/*------------------------------------------------------------------------------------------------*/
// Image:

class Image : public Element
{
public:
    Image();

    void update(Seconds elapsed_time) override;

private:
    void on_reposition() override;

    // Expects a map that includes:
    // ==========================================
    // * texture: <std::string> = <SFML_LOGO>
    // * size:    <PxVec2>      = <TEXTURE_SIZE>
    // ==ADVANCED================================
    // * color: <sf::Color> = <WHITE>
    // ==========================================
    bool on_initialization(const YAML::Node& node) override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextureReference texture;
    sf::Sprite image;
    sf::Color color;
};

/*------------------------------------------------------------------------------------------------*/
// Text:

class Text : public Element
{
public:
    Text();

    void update(Seconds elapsed_time) override;

    void set_string(const std::string& str);

private:
    void on_reposition() override;

    // Expects a map that includes:
    // ============================================
    // * text:       <std::string> = "Placeholder"
    // * text_props: <TextProps>   = <SYSTEM>
    // ============================================
    bool on_initialization(const YAML::Node& node) override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    TextProps text_props;
    sf::Text text;
};

/*------------------------------------------------------------------------------------------------*/
// Action:

// Specialized command_sequence wrapper that keeps track of its execution and optionally delays it.
class Action : public YAML::Serializable
{
public:
    Action();

    void update(Seconds elapsed_time);

    // The optional arg-string will be formatted into the command_sequence at a "{}" sign, if found.
    void initiate_execution(const std::string& arg = "");

    // The command_sequence will be executed after a delay upon initiation. 0 by default.
    void set_delay(Seconds delay);

    // Relevant only for non-repeatable actions.
    bool is_executable() const;

    bool is_idle() const;

    // Expects a map that consists of:
    // ==================================================
    // * command: <std::string> = message(<WARNING>)
    // ==ADVANCED========================================
    // * repeatable: <bool> = true
    // * executed:   <bool> = false
    // ==================================================
    bool initialize(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ===================
    // * executed: <bool>
    // ===================
    YAML::Node serialize_dynamic_data() const override;

private:
    std::string command_sequence;
    bool repeatable;
    mutable bool executed;

    std::string formatted_command_sequence;
    Seconds delay;
    Seconds delay_remaining;
};

/*------------------------------------------------------------------------------------------------*/
// Button:

class Button : public Element
{
public:
    Button();

    void update_indicator_input(const Indicator& indicator) override;
    void update(Seconds elapsed_time) override;

    void set_locked(bool locked);
    bool is_activatable() const override;

private:
    void on_reposition() override;
    void on_setting_hovered() override;

    // Expects a map that includes:
    // ========================================
    // * text:    <std::string> = ""
    // * action:  <Action>      = <DEFAULT>
    // ==ADVANCED==============================
    // * text_props: <TextProps>   = <CASLAME>
    // * texture:    <std::string> = <LOCKS>
    // * color:      <sf::Color>   = <GOLD>
    // * size:       <PxVec2>      = (50, 50)
    // * locked:     <bool>        = false
    // * sound:      <std::string> = <LOCKS>
    // ========================================
    // The 'texture' node must consist of 3 adjacent icons representing: 
    // the following states: INTERACTION | UNLOCKED | LOCKED.
    // The 'size' node is only relevant if no 'text' is specified.
    bool on_initialization(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ===================
    // * action: <Action>
    // * locked: <bool>
    // ===================
    YAML::Node on_dynamic_data_serialization() const override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    Highlight highlight;
    sf::Color highlight_color;

    SoundID action_sound;

    TextProps text_props;
    sf::Text text;
    bool text_and_highlight_enabled;

    Stamp stamp;
    PxVec2 stamp_side;

    Action action;
    Seconds action_cooldown;
    bool locked;
};

/*------------------------------------------------------------------------------------------------*/
// InputLine:

// Gathers text input and compares it against known solutions.
// A solution is a pair:
// first  -> a key string;
// second -> an Action (executed if committed input matches with key);
// Furthermore, a default action may be specified by using "DEFAULT" as key.
// If specified, it will be executed with any input (if no other solution matches with input),
// which in turn will be formatted into the action's command_sequence.
class InputLine : public Element
{
public:
    InputLine();

    void update_keyboard_input(const Keyboard& keyboard) override;
    void update_indicator_input(const Indicator& indicator) override;
    void update(Seconds elapsed_time) override;

    void set_locked(bool locked);
    bool is_activatable() const override;

private:
    void set_input(const std::string& input);

    enum class Theme
    {
        CorrectInput,
        IncorrectInput,
        Default
    };
    void set_theme(Theme theme);

    void commit_input();

    void position_underline();
    void position_caret();

    void on_reposition() override;
    void on_setting_hovered() override;
    void on_setting_active() override;

    // Expects a map that includes:
    // =======================================================
    // * solutions:  map<std::string, Action> = { <DEFAULT> }
    // * text_props: <TextProps>              = <TYPEWRITER>
    // * length:     <int>                    = 20
    // ==ADVANCED=============================================
    // * char_checker: <CharChecker> = <GRAPHIC>
    // * input:        <std::string> = ""
    // * input_save:   <bool>        = true
    // * auto_clear:   <bool>        = false
    // * locked:       <bool>        = false
    // * input_sound:  <std::string> = <TYPEWRITER>
    // =======================================================
    // Character checkers: [graphic, numeric, systemic]
    bool on_initialization(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ======================================
    // * solutions: map<std::string, Action>
    // * input:     <std::string>
    // * locked:    <bool>
    // ======================================
    // Note that input is only saved if input saving remained enabled during initialization.
    YAML::Node on_dynamic_data_serialization() const override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    Highlight highlight;
    Theme theme;
    TextProps text_props;
    sf::Text text;
    
    SoundID input_sound;

    // Note that the stamp also acts as a button that commits current input.
    Stamp stamp;
    Px    stamp_side;

    ProgressiveFloat underline_opacity;
    TriangleLine underline;
    AnimationPlayer caret;

    InputString input;
    std::string last_committed_input;
    std::unordered_map<std::string, Action> solutions;
    bool default_action;
    bool input_save;
    bool auto_clear;

    Seconds commit_cooldown;
    bool locked;
};