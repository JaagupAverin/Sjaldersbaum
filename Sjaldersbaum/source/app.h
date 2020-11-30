#pragma once

#include <SFML/Graphics.hpp>

#include "app_settings.h"
#include "fps_display.h"
#include "debug_window.h"
#include "mouse.h"
#include "keyboard.h"
#include "game.h"
#include "events-requests.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

// Environment for the game to be run in.
class App : Observer
{
public:
    App();

    void run_loop();

private:
    void update(Seconds elapsed_time_sec);

    void handle_SFML_events();

    void render();

    void on_resize();

    void initialize_debug_components();

    // Cannot be set while vSync is enabled. 0 for unlimited.
    void set_fps_cap(int fps_cap);

    // Cannot be enabled if fpscap is set.
    void set_vsync(bool enable);

    void set_fullscreen(bool enable);

    void set_audio_volume(int volume);

    void create_window();

    void save_and_terminate();

    void on_event(Event event, const Data& data) override;
    void on_request(Request request, Data& data) override;

private:
    Game game;

    Keyboard keyboard;
    Mouse mouse;
    bool mouse_hovering_window_area;

    AppSettings settings;
    sf::RenderWindow window;
    bool ignore_next_resize;

    DebugWindow debug_window;
    FPS_Display fps_display;
    bool debug_components_initialized;

    float timeflow_multiplier;
    bool app_running;
};