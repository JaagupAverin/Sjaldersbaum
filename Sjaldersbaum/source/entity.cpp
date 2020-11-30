#include "entity.h"

#include "logger.h"
#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

Entity::Entity(const EntityConfig& config) :
    reveal_sound  { UNINITIALIZED_SOUND },
    initial_origin{ Origin::Center },
    visible       { true },
    hovered       { false },
    active        { false },
    idle          { false },
    initialized   { false },
    config        { config }
{

}

void Entity::set_position(const PxVec2 position, const Origin origin)
{
    if (origin == Origin::Center)
    {
        bounds.setCenter(position);
    }
    else if (origin == Origin::TopLeftCorner)
    {
        bounds.left = position.x;
        bounds.top  = position.y;
    }
    else if (origin == Origin::TopRightCorner)
    {
        bounds.left = position.x - bounds.width;
        bounds.top  = position.y;
    }
    on_reposition();
}

void Entity::set_position(const PxVec2 position)
{
    set_position(position, this->initial_origin);
}

void Entity::set_visible(const bool visible)
{
    this->visible = visible;
    on_setting_visible();
    if (initialized && visible)
        AudioPlayer::instance().play(reveal_sound);
}

void Entity::set_hovered(const bool hovered)
{
    if (!config.activatable)
        return;

    this->hovered = hovered;
    on_setting_hovered();
}

void Entity::set_active(const bool active)
{
    if (!config.activatable)
        return;

    this->active = active;
    on_setting_active();
}

void Entity::set_idle(const bool idle)
{
    this->idle = idle;
}

void Entity::disclose_size(const PxVec2 size)
{
    if (initial_origin == Origin::Center)
    {
        bounds.setSizeKeepCenter(size);
        on_reposition();
    }
    else if (initial_origin == Origin::TopLeftCorner)
    {
        bounds.width  = size.x;
        bounds.height = size.y;
    }
    else if (initial_origin == Origin::TopRightCorner)
    {
        bounds.left = bounds.left + (bounds.width - size.x);
        bounds.width  = size.x;
        bounds.height = size.y;
        on_reposition();
    }
}

PxVec2 Entity::get_position() const
{
    if (initial_origin == Origin::TopLeftCorner)
        return get_tlc();
    else if (initial_origin == Origin::TopRightCorner)
        return get_trc();
    else
        return get_center();
}

PxVec2 Entity::get_center() const
{
    return bounds.getCenter();
}

PxVec2 Entity::get_size() const
{
    return { bounds.width, bounds.height };
}

PxRect Entity::get_bounds() const
{
    return bounds;
}

bool Entity::is_visible() const
{
    return visible;
}

bool Entity::is_activatable() const
{
    return config.activatable;
}

bool Entity::is_hovered() const
{
    return hovered;
}

bool Entity::is_active() const
{
    return active;
}

bool Entity::is_idle() const
{
    return idle;
}

bool Entity::is_initialized() const
{
    return initialized;
}

void Entity::render_debug_bounds(sf::RenderTarget& target, const sf::Color color) const
{
    sf::RectangleShape rectangle;
    rectangle.setSize(get_size());
    rectangle.setPosition(round_hu(get_tlc()));
    rectangle.setFillColor(color);
    target.draw(rectangle);
}

bool Entity::initialize(const YAML::Node& node)
{
    // Entity is assumed to be in default-constructed state at this stage.
    // Otherwise old values may remain in effect.

    if (!node.IsDefined())
    {
        LOG_ALERT("undefined node.");
        return false;
    }

    try
    {
        if (config.position_load)
        {
            const YAML::Node center_node = node["center"];
            if (center_node.IsDefined())
                set_position(center_node.as<PxVec2>()); // Origin is Center by default.
            else
            {
                const YAML::Node tlc_node = node["tlc"];
                if (tlc_node.IsDefined())
                {
                    initial_origin = Origin::TopLeftCorner;
                    set_position(tlc_node.as<PxVec2>());
                }
                else
                {
                    const YAML::Node trc_node = node["trc"];
                    if (trc_node.IsDefined())
                    {
                        initial_origin = Origin::TopRightCorner;
                        set_position(trc_node.as<PxVec2>());
                    }
                    else
                    {
                        initial_origin = config.default_origin;
                        set_position({ 0.f, 0.f });
                    }
                }
            }
        }
        else
            initial_origin = config.default_origin;

        if (config.visibility_serialization)
        {
            const YAML::Node visible_node = node["visible"];
            set_visible(visible_node.IsDefined() ? visible_node.as<bool>() : true);
        }

        const YAML::Node reveal_sound_path_node = node["reveal_sound"];
        if (reveal_sound_path_node.IsDefined())
            reveal_sound = AudioPlayer::instance().load(reveal_sound_path_node.as<std::string>(), false);
        else
            reveal_sound = GlobalSounds::GENERIC_REVEAL;
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that consists of:\n"
                  "==========================================\n"
                  "* center:   <PxVec2> = (0, 0)\n"
                  "* visible:  <bool>   = true\n"
                  "* <nodes expected by derived classes>\n"
                  "==ADVANCED================================\n"
                  "* tlc:          <PxVec2>      = (?, ?)\n"
                  "* trc:          <PxVec2>      = (?, ?)\n"
                  "* reveal_sound: <std::string> = <GENERIC>\n"
                  "==========================================\n"
                  "DUMP:\n" + YAML::Dump(node));
        return false;
    }

    return initialized = on_initialization(node);
}

YAML::Node Entity::serialize_dynamic_data() const
{
    YAML::Node node{ YAML::NodeType::Map };
    if (config.position_save)
    {
        if (initial_origin == Origin::Center)
            node["center"] = get_center();
        else if (initial_origin == Origin::TopLeftCorner)
            node["tlc"] = get_tlc();
        else if (initial_origin == Origin::TopRightCorner)
            node["trc"] = get_trc();
    }
    if (config.visibility_serialization)
        node["visible"] = visible;
    YAML::insert_all_values(node, on_dynamic_data_serialization());
    return node;
}

PxVec2 Entity::get_tlc() const
{
    return { bounds.left, bounds.top };
}

PxVec2 Entity::get_trc() const
{
    return { bounds.getRight(), bounds.top };
}

bool Entity::on_initialization(const YAML::Node& node)
{
    return true;
}

YAML::Node Entity::on_dynamic_data_serialization() const
{
    return YAML::Node{ YAML::NodeType::Undefined };
}