#include "debug_cl.h"

#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr int MAX_INPUT_LENGTH = 200;

constexpr Px LEFT_SIDE_MARGIN = 5.f;

/*------------------------------------------------------------------------------------------------*/

DebugCL::DebugCL() :
    input_committed{ false },
    caret_index    { 0 },
    caret_visible  { true },
    caret_blink_lag{ 0.f },
    history_index  { 0 }
{
    caret.setString("|");
    input.set_max_length(MAX_INPUT_LENGTH);
}

void DebugCL::update_keyboard_input(const Keyboard& keyboard)
{
    input.update_keyboard_input(keyboard);

    if (input.has_string_been_altered())
        text.setString(input.get_string());

    if (input.has_index_been_altered())
    {
        caret_index = input.get_index();
        position_caret();

        // Refresh caret blink:
        caret_visible = true;
        caret_blink_lag = 0.f;
    }

    if (keyboard.is_keybind_pressed(DefaultKeybinds::ENTER))
    {
        input_committed = true;
        history.emplace_back(input.get_string());
    }

    else if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_UP) ||
             keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_DOWN))
    {
        if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_UP))
            --history_index;
        if (keyboard.is_keybind_pressed(DefaultKeybinds::MOVE_INDEX_DOWN))
            ++history_index;

        assure_bounds(history_index, 0, static_cast<int>(history.size()));

        if (history_index == static_cast<int>(history.size()))
            set_input("");
        else
            set_input(history[history_index]);
    }
}

void DebugCL::update(const Seconds elapsed_time)
{
    update_caret_blink(elapsed_time);
    render_text_to_canvas();
}

void DebugCL::set_size(const PxVec2 size)
{
    bg.setSize(size);

    text_view.setSize(size);
    text_canvas.create(static_cast<unsigned int>(size.x),
                       static_cast<unsigned int>(size.y));
    text_sprite.setTexture(text_canvas.getTexture(), true);
}

void DebugCL::set_position(const PxVec2 position)
{
    bg.setPosition(round_hu(position));
    text_sprite.setPosition(round_hu(position));
}

void DebugCL::set_properties(const std::string& font_path,
                             Px              text_height,
                             const sf::Color text_fill,
                             const sf::Color text_ol,
                             const Px        text_ol_thickness,
                             const sf::Color bg_fill,
                             const sf::Color bg_ol,
                             const Px        bg_ol_thickness)
{
    font.load(font_path);
    text.setFont(font.get());
    text.setCharacterSize(static_cast<unsigned int>(text_height));
    text.setFillColor(text_fill);
    text.setOutlineColor(text_ol);
    text.setOutlineThickness(text_ol_thickness);

    caret.setFont(font.get());
    caret.setCharacterSize(static_cast<unsigned int>(text_height));
    caret.setFillColor(text_fill);
    caret.setOutlineColor(text_ol);
    caret.setOutlineThickness(text_ol_thickness);

    bg.setFillColor(bg_fill);
    bg.setOutlineColor(bg_ol);
    bg.setOutlineThickness(bg_ol_thickness);

    position_caret();
}

bool DebugCL::has_committed_input() const
{
    return input_committed;
}

std::string DebugCL::extract_input()
{
    const std::string res = input.get_string();

    input.clear();
    text.setString("");
    input_committed = false;

    caret_index = 0;
    position_caret();

    history_index = static_cast<int>(history.size());

    return res;
}

void DebugCL::set_input(const std::string& str)
{
    input.set_string(str);
    text.setString(input.get_string());
    input_committed = false;

    caret_index = input.get_index();
    position_caret();
}

void DebugCL::update_caret_blink(const Seconds elapsed_time)
{
    static constexpr Seconds blink_interval = 0.4f;

    caret_blink_lag += elapsed_time;
    if (caret_blink_lag >= blink_interval)
    {
        caret_visible = !caret_visible;
        caret_blink_lag -= blink_interval;
    }
}

void DebugCL::position_caret()
{
    if (caret_index == 0u)
        caret.setPosition(text.getPosition().x - LEFT_SIDE_MARGIN,
                          text.getPosition().y);
    else
    {
        const PxVec2 pos_of_iterated_char   = text.findCharacterPos(caret_index);
        const PxVec2 pos_of_preceeding_char = text.findCharacterPos(caret_index - 1u);

        // Place caret between the two characters it separates:
        caret.setPosition(round_hu((pos_of_iterated_char + pos_of_preceeding_char) / 2.f));
    }

    text_view.setCenter(caret.getPosition().x,
                        caret.getPosition().y + caret.getLocalBounds().height / 2.f);
}

void DebugCL::render_text_to_canvas()
{
    text_canvas.setView(text_view);
    text_canvas.clear(Colors::TRANSPARENT);

    text_canvas.draw(text);
    if (caret_visible)
        text_canvas.draw(caret);

    text_canvas.display();
}

void DebugCL::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(bg);
    target.draw(text_sprite);
}