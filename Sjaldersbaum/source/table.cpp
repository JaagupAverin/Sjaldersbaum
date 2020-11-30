#include "table.h"

#include "maths.h"

const std::string REGULAR_WOOD_PATH = "resources/textures/tables/regular_wood.png";

/*------------------------------------------------------------------------------------------------*/

bool Table::assure_contains(Object& object) const
{
    PxRect central_bounds;
    central_bounds.setSizeKeepCenter({
        bounds.width  - object.get_size().x - 2.f,
        bounds.height - object.get_size().y - 2.f });

    PxVec2 object_center = object.get_center();

    if (assure_is_contained_by(object_center, central_bounds))
        return true;
    else
    {
        object.set_position(object_center, Origin::Center);
        return false;
    }
}

PxVec2 Table::get_size() const
{
    return size;
}

PxRect Table::get_bounds() const
{
    return bounds;
}

bool Table::initialize(const YAML::Node& node)
{
    std::string texture_path = REGULAR_WOOD_PATH;
                size         = { 2700.f, 1500.f };
    PxVec2      bounds_size  = { size.x - 100.f, size.y - 100.f };

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node texture_node     = node["texture"];
            const YAML::Node size_node        = node["size"];
            const YAML::Node bounds_size_node = node["bounds"];

            if (texture_node.IsDefined())
                texture_path = texture_node.as<std::string>();

            if (size_node.IsDefined())
                size = size_node.as<PxVec2>();

            if (bounds_size_node.IsDefined())
                bounds_size = bounds_size_node.as<PxVec2>();
            else
                bounds_size = { size.x - 100.f, size.y - 100.f };
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "========================================================\n"
                      "* texture: <std::string> = <REGULAR_WOOD>\n"
                      "* size:    <PxVec2>      = (2700, 1500)\n"
                      "==ADVANCED==============================================\n"
                      "* bounds:  <PxVec2>      = (size.x - 100, size.y - 100)\n"
                      "========================================================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    texture.load(texture_path);
    background.setTexture(texture.get(), true);
    background.setOrigin(PxVec2{ texture.get().getSize() } / 2.f);

    if (!(assure_bounds(size.x, 1.f, PX_LIMIT) &
          assure_bounds(size.y, 1.f, PX_LIMIT)))
        LOG_ALERT("invalid size had to be adjusted.");
    set_size(background, size);
    bounds.setSizeKeepCenter(bounds_size);

    return true;
}

void Table::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(background, states);
}