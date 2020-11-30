#include "indicator.h"

/*------------------------------------------------------------------------------------------------*/

Indicator::Indicator() :
    interaction_key_pressed       { false },
    interaction_key_double_pressed{ false },
    latest_input_source{ InputSource::None },
    type{ Type::Unassigned }
{

}

void Indicator::reset_input()
{
    interaction_key_pressed        = false;
    interaction_key_double_pressed = false;
}

void Indicator::set_interaction_key_pressed(const bool interaction_key_pressed,
                                            const InputSource source)
{
    this->interaction_key_pressed = interaction_key_pressed;
    if (source != InputSource::Auto)
        latest_input_source = source;
}

void Indicator::set_interaction_key_double_pressed(const bool interaction_key_double_pressed,
                                                   const InputSource source)
{
    this->interaction_key_double_pressed = interaction_key_double_pressed;
    if (source != InputSource::Auto)
        latest_input_source = source;
}

void Indicator::set_position(const PxVec2 position, const InputSource source)
{
    this->position = position;
    if (source != InputSource::Auto)
        latest_input_source = source;
}

void Indicator::set_type(const Type type) const
{
    this->type = type;
}

bool Indicator::is_interaction_key_pressed() const
{
    return interaction_key_pressed;
}

bool Indicator::is_interaction_key_double_pressed() const
{
    return interaction_key_double_pressed;
}

PxVec2 Indicator::get_position() const
{
    return position;
}

Indicator::Type Indicator::get_type() const
{
    return type;
}

Indicator::InputSource Indicator::get_latest_input_source() const
{
    return latest_input_source;
}