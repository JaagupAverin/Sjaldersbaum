#pragma once

#include <SFML/System.hpp>

/*------------------------------------------------------------------------------------------------*/

namespace sf
{
// ExtendedRect. Extends sf::Rect with several common ease-of-use functions.
template<typename Num>
class XRect : public Rect<Num>
{
public:
    using Rect<Num>::Rect;

    void setCenter(const Vector2<Num> center)
    {
        this->left = center.x - this->width  / static_cast<Num>(2);
        this->top  = center.y - this->height / static_cast<Num>(2);
    }

    Vector2<Num> getCenter() const
    {
        return { this->left + this->width  / static_cast<Num>(2),
                 this->top  + this->height / static_cast<Num>(2) };
    }

    void setSizeKeepCenter(const Vector2<Num> size)
    {
        const sf::Vector2<Num> center{
            this->left + this->width  / static_cast<Num>(2),
            this->top  + this->height / static_cast<Num>(2) };

        this->width  = size.x;
        this->height = size.y;

        setCenter(center);
    }

    Vector2<Num> getSize() const
    {
        return { this->width, this->height };
    }

    Num getRight() const
    {
        return this->left + this->width;
    }

    Num getBottom() const
    {
        return this->top + this->height;
    }
};
}