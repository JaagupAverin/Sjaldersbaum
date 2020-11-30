#pragma once

// Base class for a basic drawable object that can be hovered and/or activated,
// but doesn't care about these values individually.
class HoverableDetail
{
public:
    void set_hovered(const bool hovered)
    {
        this->hovered = hovered;
        if (!active)
            hovered ? set_state(State::Hovered) : set_state(State::Default);
    }

    void set_active(const bool active)
    {
        this->active = active;
        if (active)
            set_state(State::Active);
        else
            hovered ? set_state(State::Hovered) : set_state(State::Default);
    }

protected:
    // Note that each state overrides the previous.
    enum class State
    {
        Default,
        Hovered,
        Active
    };
    virtual void set_state(State state) = 0;

private:
    bool hovered = false;
    bool active  = false;
};