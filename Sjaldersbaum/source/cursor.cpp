#include "cursor.h"

#include "resources.h"
#include "maths.h"

/*------------------------------------------------------------------------------------------------*/

const sf::Vector2i CURSOR_SIZE{ 24, 24 };

const std::string CURSORS_PATH = "resources/textures/system/cursors.png";

/*------------------------------------------------------------------------------------------------*/

Cursor::Cursor() :
    type{ Indicator::Type::Unassigned },
    visible{ true }
{
    texture.load(CURSORS_PATH);
    cursor.setTexture(texture.get());
    set_type(Indicator::Type::Regular);
}

Cursor& Cursor::instance()
{
    static Cursor singleton;
    return singleton;
}

void Cursor::update(const Seconds elapsed_time)
{
    visible.update(elapsed_time);
}

void Cursor::set_position(const PxVec2 position)
{
    cursor.setPosition(round_hu(position));
}

void Cursor::set_type(const Indicator::Type type)
{
    if (this->type != type)
    {
        this->type = type;
        cursor.setTextureRect(sf::IntRect{ { static_cast<int>(type) * CURSOR_SIZE.x, 0 },
                                             CURSOR_SIZE });

        if (type == Indicator::Type::Regular)
            cursor.setOrigin(6.f, 5.f);
        else if (type == Indicator::Type::HoveringMovable)
            cursor.setOrigin(12.f, 6.f);
        else if (type == Indicator::Type::HoveringButton)
            cursor.setOrigin(12.f, 4.f);
        else if (type == Indicator::Type::HoveringTextField)
            cursor.setOrigin(12.f, 12.f);
        else if (type == Indicator::Type::MovingCamera)
            cursor.setOrigin(12.f, 12.f);
        else
            LOG_ALERT("unimplemented cursor type");
    }
}

void Cursor::set_visible(const bool visible, const Seconds delay)
{
    if (this->visible.get_target() != visible)
    {
        this->visible.set_progression_duration(delay);
        this->visible.set_target(visible);
    }
}

void Cursor::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (visible.get_current())
        target.draw(cursor);
}