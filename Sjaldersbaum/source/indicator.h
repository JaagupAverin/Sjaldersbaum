#pragma once

#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Since some input from Keyboard and Mouse can be used to update Entities almost identically,
// this structure is used to combine that input and forward it to a single update method.
class Indicator
{
public:
    enum class Type
    {
        Unassigned = -1,
        Regular,
        HoveringMovable,
        HoveringButton,
        HoveringTextField,
        MovingCamera,

        Count
    };

    enum class InputSource
    {
        None,
        Keyboard,
        Mouse,
        Auto // Preserves current source.
    };

    Indicator();

    void reset_input();

    void set_interaction_key_pressed(bool interaction_key_pressed,
                                     InputSource source = InputSource::Auto);
    void set_interaction_key_double_pressed(bool interaction_key_double_pressed,
                                            InputSource source = InputSource::Auto);

    void set_position(PxVec2 position, InputSource source = InputSource::Auto);

    void set_type(Type type) const;

    bool is_interaction_key_pressed() const;
    bool is_interaction_key_double_pressed() const;

    PxVec2 get_position() const;
    Type get_type() const;
    InputSource get_latest_input_source() const;

private:
    bool interaction_key_pressed;
    bool interaction_key_double_pressed;
    PxVec2 position;
    InputSource latest_input_source;

    mutable Type type;
};