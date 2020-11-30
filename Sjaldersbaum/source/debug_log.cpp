#include "debug_log.h"

#include <sstream>

#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr int MAX_LINES = 10000;
constexpr int MAX_LINE_WIDTH = 70;

constexpr Px NEWLINE_MARGIN   = 5.f;
constexpr Px LEFT_SIDE_MARGIN = 10.f;

/*------------------------------------------------------------------------------------------------*/

DebugLog::DebugLog() :
    text_height{ 0.f },
    text_ol_thickness{ 0.f },
    line_height{ 0.f }
{

}

void DebugLog::scroll(const Mouse& mouse)
{
    if (mouse.get_wheel_ticks_delta() != 0 &&
        bg.getGlobalBounds().contains(mouse.get_position_in_window()))
    {
        const Px y_move = mouse.get_wheel_ticks_delta() * (-4.f) * text_height;
        const Px y_max  = lines.size() * line_height - lines_view.getSize().y / 2.f;

        PxVec2 center{ lines_view.getCenter().x,
                       lines_view.getCenter().y + y_move };
        assure_less_than_or_equal_to(center.y, y_max);

        lines_view.setCenter(center);
        render_visible_lines_to_canvas();
    }
}

void DebugLog::set_size(const PxVec2 size)
{
    bg.setSize(size);

    lines_view.setSize(size);
    lines_canvas.create(static_cast<unsigned int>(size.x),
                        static_cast<unsigned int>(size.y));
    lines_sprite.setTexture(lines_canvas.getTexture(), true);

    position_view_to_newest_line();
    render_visible_lines_to_canvas();
}

void DebugLog::set_position(const PxVec2 position)
{
    bg.setPosition(round_hu(position));
    lines_sprite.setPosition(round_hu(position));
}

void DebugLog::set_properties(const std::string& font_path,
                              const Px        text_height,
                              const sf::Color text_fill,
                              const sf::Color text_ol,
                              const Px        text_ol_thickness,
                              const sf::Color bg_fill,
                              const sf::Color bg_ol,
                              const Px        bg_ol_thickness)
{
    font.load(font_path);

    this->text_height       = text_height;
    this->text_fill         = text_fill;
    this->text_ol           = text_ol;
    this->text_ol_thickness = text_ol_thickness;
    this->line_height       = text_height + NEWLINE_MARGIN;

    bg.setFillColor(bg_fill);
    bg.setOutlineColor(bg_ol);
    bg.setOutlineThickness(bg_ol_thickness);
}

void DebugLog::write(const std::string& str)
{
    if (lines.size() >= MAX_LINES)
        return;

    auto write_line = [&](const std::string& str)
    {
        auto& line = lines.emplace_back(sf::Text());

        line.setString(str);
        line.setFont(font.get());
        line.setCharacterSize(static_cast<unsigned int>(text_height));
        line.setFillColor(text_fill);
        line.setOutlineColor(text_ol);
        line.setOutlineThickness(text_ol_thickness);

        line.setPosition(LEFT_SIDE_MARGIN, (lines.size() - 1) * line_height);
    };

    std::stringstream buffer{ str };
    for (std::string line; std::getline(buffer, line);)
    {
        // Wrap lines:
        while (line.length() > MAX_LINE_WIDTH)
        {
            write_line(line.substr(0, MAX_LINE_WIDTH));
            line.erase(0, MAX_LINE_WIDTH);
        }
        write_line(line);

        if (lines.size() >= MAX_LINES)
        {
            write_line("log full; use 'clear'.");
            break;
        }
    }

    position_view_to_newest_line();
    render_visible_lines_to_canvas();
}

void DebugLog::clear()
{
    lines.clear();
    position_view_to_newest_line();
    render_visible_lines_to_canvas();
}

void DebugLog::position_view_to_newest_line()
{
    lines_view.setCenter(lines_view.getSize().x / 2.f,
                         lines.size() * line_height - lines_view.getSize().y / 2.f);
}

void DebugLog::render_visible_lines_to_canvas()
{
    lines_canvas.setView(lines_view);
    lines_canvas.clear(Colors::TRANSPARENT);

    const float visible_lines = lines_view.getSize().y / line_height + 1.f;
    const int centered_i = static_cast<int>(std::round(lines_view.getCenter().y / line_height));
    
    int begin = centered_i - static_cast<int>(std::ceil(visible_lines / 2.f));
    int end   = centered_i + static_cast<int>(std::ceil(visible_lines / 2.f));

    assure_bounds(end,   0, static_cast<int>(lines.size()));
    assure_bounds(begin, 0, end);

    for (int i = begin; i != end; ++i)
        lines_canvas.draw(lines[i]);

    lines_canvas.display();
}

void DebugLog::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(bg);
    target.draw(lines_sprite);
}