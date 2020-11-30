#include "text_props.h"

/*------------------------------------------------------------------------------------------------*/

const std::unordered_map<std::string, sf::Text::Style> KNOWN_STYLES
{
    { "regular",        sf::Text::Regular },
    { "bold",           sf::Text::Bold },
    { "italic",         sf::Text::Italic },
    { "underline",      sf::Text::Underlined },
    { "strikethrough",  sf::Text::StrikeThrough }
};

TextProps::TextProps() :
    style                    { sf::Text::Regular },
    height                   { 30.f },
    fill                     { sf::Color::Black },
    outline                  { sf::Color::Black },
    outline_thickness        { 0.f },
    letter_spacing_multiplier{ 1.f },
    line_spacing_multiplier  { 1.f }
{
    font.load(SYSTEM_FONT_PATH);
}

void TextProps::apply(sf::Text& text) const
{
    text.setFont(font.get());
    text.setStyle(style);
    text.setCharacterSize(static_cast<unsigned int>(height));

    text.setFillColor(fill);
    text.setOutlineColor(outline);
    text.setOutlineThickness(outline_thickness);

    text.setLetterSpacing(letter_spacing_multiplier);
    text.setLineSpacing(line_spacing_multiplier);

    text.setOrigin(-offsets);
}

Px TextProps::get_max_height() const
{
    sf::Text temp;
    apply(temp);
    temp.setString(MAIN_CHARACTERS);

    return temp.getLocalBounds().height;
}

Px TextProps::get_max_width(const int ch_count) const
{
    sf::Text temp;
    apply(temp);
    std::string str = std::string(static_cast<size_t>(ch_count), 'W');
    temp.setString(str);

    return temp.getLocalBounds().width;
}

bool TextProps::initialize(const YAML::Node& node)
{
    std::string font_path;
    try
    {
        for (const auto& property_node : node)
        {
            const std::string key  = property_node.first.as<std::string>();
            const YAML::Node value = property_node.second;

            if (key == "font")
                font_path = value.as<std::string>();

            else if (key == "style")
            {
                if (value.IsSequence())
                    for (const auto& style_node : value)
                    {
                        sf::Text::Style style =
                            Convert::str_to_enum(style_node.as<std::string>(), KNOWN_STYLES);
                        this->style |= style;
                    }
                else
                    this->style = Convert::str_to_enum(value.as<std::string>(), KNOWN_STYLES);
            }

            else if (key == "height")
                height = value.as<Px>();

            else if (key == "fill")
                fill = value.as<sf::Color>();

            else if (key == "outline")
                outline = value.as<sf::Color>();

            else if (key == "outline_width")
                outline_thickness = value.as<Px>();

            else if (key == "letter_spacing")
                letter_spacing_multiplier = value.as<float>();

            else if (key == "line_spacing")
                line_spacing_multiplier = value.as<float>();

            else if (key == "offsets")
                offsets = value.as<PxVec2>();
        }
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that consists of:\n"
                  "=============================================================\n"
                  "* font:   <std::string>     = <FIRA_CODE>\n"
                  "* style:  <sf::Text::Style> = <REGULAR>\n"
                  "* height: <Px>              = 30\n"
                  "==ADVANCED===================================================\n"
                  "* fill:           <sf::Color> = <SLIGHTLY_TRANSPARENT_BLACK>\n"
                  "* outline:        <sf::Color> = <BLACK>\n"
                  "* outline_width:  <Px>        = 0\n"
                  "* letter_spacing: <float>     = 1\n"
                  "* line_spacing:   <float>     = 1\n"
                  "* offsets:        <PxVec2>    = (0, 0)\n"
                  "=============================================================\n"
                  "Styles: [regular, bold, italic, underline, strikethrough]\n"
                  "Note that style can also be a sequence of several styles.\n"
                  "DUMP:\n" + YAML::Dump(node));
        return false;
    }

    if (!font_path.empty())
        font.load(font_path);

    return true;
}