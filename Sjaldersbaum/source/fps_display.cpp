#include "fps_display.h"

#include <numeric>

#include "convert.h"
#include "colors.h"

constexpr Seconds UPDATE_INTERVAL = 0.2f;

/*------------------------------------------------------------------------------------------------*/

FPS_Display::FPS_Display() : 
    update_lag{ 0.f },
    visible{ false }
{

}

void FPS_Display::initialize()
{
    fps_history.resize(100);
    std::fill(fps_history.begin(), fps_history.end(), 0);

    font.load(SYSTEM_FONT_PATH);

    display.setFont(font.get());
    display.setFillColor(Colors::GREEN);
    display.setOutlineColor(Colors::BLACK);
    display.setOutlineThickness(1.f);
    display.setCharacterSize(14u);
    display.setString("FPS");
    display.setPosition(1.f, 1.f);
}

void FPS_Display::toggle_visible()
{
    visible = !visible;
}

void FPS_Display::update(const Seconds elapsed_time)
{
    fps_history.push_back(static_cast<int>(1 / elapsed_time));
    fps_history.pop_front();

    update_lag += elapsed_time;
    if (update_lag >= UPDATE_INTERVAL)
    {
        const int average_fps =
            std::accumulate(fps_history.begin(), fps_history.end(), 0) / static_cast<int>(fps_history.size());
        display.setString(Convert::to_str(average_fps));

        update_lag -= UPDATE_INTERVAL;
    }
}

void FPS_Display::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (visible)
        target.draw(display);
}