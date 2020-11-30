#pragma once

#include <SFML/Graphics.hpp>

#include "yaml.h"
#include "highlight.h"
#include "keyboard.h"
#include "mouse.h"
#include "units.h"
#include "indicator.h"
#include "audio.h"

/*------------------------------------------------------------------------------------------------*/

enum class Origin
{
    Center,
    TopLeftCorner,
    TopRightCorner
};

struct EntityConfig
{
    const bool activatable;
    const bool position_load;
    const bool position_save;
    const bool visibility_serialization;
    const Origin default_origin;
};

namespace EntityConfigs
{
constexpr EntityConfig INDEPENDENT_SHEET    { true,
                                              true, true, true,
                                              Origin::Center };
constexpr EntityConfig BOUND_SHEET          { true,
                                              false, false, false,
                                              Origin::TopLeftCorner };
constexpr EntityConfig BINDER               { true,
                                              true, true, true,
                                              Origin::Center };
constexpr EntityConfig ACTIVATABLE_ELEMENT  { true,
                                              true, false, true,
                                              Origin::TopLeftCorner };
constexpr EntityConfig INACTIVATABLE_ELEMENT{ false,
                                              true, false, true,
                                              Origin::TopLeftCorner };
}
/*------------------------------------------------------------------------------------------------*/

// Base for anything that exists on the Table.
class Entity : public sf::Drawable, public YAML::Serializable
{
public:
    Entity(const EntityConfig& config);

    // Note that only active (+visible) Entities should be updated with keyboard input.
    virtual void update_keyboard_input(const Keyboard& keyboard) {}
    // Note that only hovered (+visible) Entities should be updated with indicator input.
    virtual void update_indicator_input(const Indicator& indicator) {}
    // Note that idle entities are static and do not have to be updated with time input.
    virtual void update(Seconds elapsed_time) {}

    // Note that the origin is temporary and does NOT override the default origin.
    // (This assures our serialization origin will match with the initialization origin).
    void set_position(PxVec2 position, Origin origin);
    // Uses the default origin.
    void set_position(PxVec2 position);

    void set_visible(bool visible);
    void set_hovered(bool hovered);
    void set_active(bool active);
    void set_idle(bool idle);

    // Entity sizes are static and determined BY the derived object upon initialization.
    // i.e: This method is used to inform the Entity of its own size, not set it.
    void disclose_size(PxVec2 size);

    // Note that this returns Center/TopLeftCorner/TopRightCorner, based on default origin.
    PxVec2 get_position() const;
    PxVec2 get_center() const;
    PxVec2 get_size() const;
    PxRect get_bounds() const;

    bool is_visible() const;
    virtual bool is_activatable() const;
    bool is_hovered() const;
    bool is_active() const;
    bool is_idle() const;
    bool is_initialized() const;

    void render_debug_bounds(sf::RenderTarget& target, sf::Color color) const;

    // Expects a map that consists of:
    // ==========================================
    // * center:   <PxVec2> = (0, 0)
    // * visible:  <bool>   = true
    // * <nodes expected by derived classes>
    // ==ADVANCED================================
    // * tlc:          <PxVec2>      = (?, ?)
    // * trc:          <PxVec2>      = (?, ?)
    // * reveal_sound: <std::string> = <GENERIC>
    // ==========================================
    bool initialize(const YAML::Node& node) override final;

    // Returns a map that consists of:
    // ======================================
    // * position: <PxVec2>     ?
    // * visible:  <bool>
    // * <nodes returned by derived classes>
    // ======================================
    // Note that position is not saved for certain Types (e.g: Elements).
    YAML::Node serialize_dynamic_data() const override final;

protected:
    PxVec2 get_tlc() const; // Top Left Corner
    PxVec2 get_trc() const; // Top Right Corner

    virtual void on_reposition() = 0;
    virtual void on_setting_visible() {}
    virtual void on_setting_hovered() {}
    virtual void on_setting_active() {}

    virtual bool on_initialization(const YAML::Node& node);
    virtual YAML::Node on_dynamic_data_serialization() const;

    SoundID reveal_sound;

private:
    PxRect bounds;
    Origin initial_origin;

    bool visible;
    bool hovered;
    bool active;
    bool idle;
    bool initialized;

    const EntityConfig& config;
};