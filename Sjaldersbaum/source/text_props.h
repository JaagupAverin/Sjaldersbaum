#pragma once

#include <SFML/Graphics.hpp>

#include "yaml.h"
#include "resources.h"
#include "colors.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

const std::string MAIN_CHARACTERS =
    "ABCDEFGHIJKLMNOPQRSŠZŽTUVWÕÄÖÜXYabcdefghijklmnopqrsšzžtuvwõäöüxy/|";

/*------------------------------------------------------------------------------------------------*/

// Contains properties for an sf::Text.
// Note that the font member must outlive any texts initialized from this.
struct TextProps : public YAML::Serializable
{
    TextProps();

    void apply(sf::Text& text) const;

    Px get_max_height() const;

    // Assumes the widest character is 'W'.
    Px get_max_width(int ch_count) const;

    // Expects a map that consists of:
    // =============================================================
    // * font:   <std::string>     = <FIRA_CODE>
    // * style:  <sf::Text::Style> = <REGULAR>
    // * height: <Px>              = 30
    // ==ADVANCED===================================================
    // * fill:           <sf::Color> = <BLACK>
    // * outline:        <sf::Color> = <BLACK>
    // * outline_width:  <Px>        = 0
    // * letter_spacing: <float>     = 1
    // * line_spacing:   <float>     = 1
    // * offsets:        <PxVec2>    = (0, 0)
    // =============================================================
    // Styles: [regular, bold, italic, underline, strikethrough]
    // Note that style can also be a sequence of several styles.
    bool initialize(const YAML::Node& node) override;

public:
    FontReference font;
    sf::Uint32 style;
    Px height;

    sf::Color fill;
    sf::Color outline;
    Px outline_thickness;

    float letter_spacing_multiplier;
    float line_spacing_multiplier;

    PxVec2 offsets;
};