#include "app.h"

#include "resources.h"
#include "time_and_date.h"
#include "logger.h"
#include "cursor.h"
#include "convert.h"
#include "maths.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

const std::string TITLE = "Sjaldersbaum";

const std::string SETTINGS_FILE_PATH = "settings.yaml";
const std::string WINDOW_ICON_PATH   = "resources/icon.png";

/*------------------------------------------------------------------------------------------------*/

constexpr unsigned int MIN_WINDOW_WIDTH  = 750u;
constexpr unsigned int MIN_WINDOW_HEIGHT = 500u;

constexpr int MIN_FPS_CAP = 60;

/*------------------------------------------------------------------------------------------------*/

App::App() :
    mouse{ window },
    mouse_hovering_window_area{ false },
    ignore_next_resize{ false },
    debug_components_initialized{ false },
    timeflow_multiplier{ 1.0f },
    app_running{ true }
{
    try
    {
        settings.load_from_file(SETTINGS_FILE_PATH);

        if (settings.debug)
            initialize_debug_components();

        create_window();
        AudioPlayer::instance().set_volume(settings.volume);
        load_global_sounds();

        game.initialize();
    }
    catch (const std::exception& e)
    {
        LOG_ALERT("uncaught exception during initialization;\nwhat: " + e.what());
        save_and_terminate();
    }
}

void App::run_loop()
{
    try
    {
        auto last_time_nanosec = Time::get_absolute_ns();

        while (app_running)
        {
            const auto current_time_nanosec = Time::get_absolute_ns();
            const auto elapsed_time_nanosec = current_time_nanosec - last_time_nanosec;

            last_time_nanosec = current_time_nanosec;

            Seconds elapsed_time = static_cast<Seconds>(elapsed_time_nanosec * SECONDS_IN_NANOSECOND);
            assure_less_than_or_equal_to(elapsed_time, 1.f / MIN_FPS_CAP);
            elapsed_time *= timeflow_multiplier;

            update(elapsed_time);
            render();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ALERT("uncaught exception during runtime;\nwhat: " + e.what());
        save_and_terminate();
    }
}

void App::update(const Seconds elapsed_time)
{
    handle_SFML_events();

    Executor::instance().update(elapsed_time);
    EARManager::instance().dispatch_queued_events();

    AudioPlayer::instance().update(elapsed_time);
    update_resource_managers(elapsed_time);

    mouse.update(elapsed_time);
    Cursor::instance().set_position(mouse.get_position_in_window());
    Cursor::instance().update(elapsed_time);

    if (debug_components_initialized)
        fps_display.update(elapsed_time / timeflow_multiplier);

    // Keyboard input:
    if (window.hasFocus())
    {
        if (keyboard.is_keybind_pressed(DebugKeybinds::GRANT_DEBUG_RIGHTS))
            initialize_debug_components();

        else if (keyboard.is_keybind_pressed(DefaultKeybinds::TOGGLE_FULLSCREEN))
            set_fullscreen(!settings.fullscreen);

        if (debug_components_initialized)
        {
            if (keyboard.is_keybind_pressed(DebugKeybinds::TOGGLE_DEBUG_WINDOW))
                debug_window.toggle_maximized();

            else if (keyboard.is_keybind_pressed(DebugKeybinds::TOGGLE_DEBUG_MODE))
                game.toggle_debug_mode();

            else if (keyboard.is_keybind_pressed(DebugKeybinds::TOGGLE_FPS_DISPLAY))
                fps_display.toggle_visible();

            else if (keyboard.is_keybind_pressed(DebugKeybinds::RELOAD_TEXTURES))
                TextureManager::instance().reload_all();

            else if (keyboard.is_keybind_pressed(DebugKeybinds::RELOAD_SOUNDBUFFERS))
                SoundBufferManager::instance().reload_all();

            debug_window.update_keyboard_input(keyboard);
        }

        if (!debug_window.is_using_keyboard_input())
            game.update_keyboard_input(keyboard);
    }

    // Mouse input:
    if (mouse_hovering_window_area)
    {
        if (debug_components_initialized)
            debug_window.update_mouse_input(mouse);
        if (!debug_window.is_using_mouse_input())
            game.update_mouse_input(mouse);
    }

    // Main update:
    if (debug_components_initialized)
        debug_window.update(elapsed_time / timeflow_multiplier);
    game.update(elapsed_time);

    if (debug_window.is_using_mouse_input() || !mouse_hovering_window_area)
    {
        window.setMouseCursorVisible(true);
        Cursor::instance().set_visible(false);
    }
    else
        window.setMouseCursorVisible(false);
}

void App::handle_SFML_events()
{
    keyboard.reset_input();
    mouse.reset_wheel_input();

    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            EARManager::instance().queue_event(Event::FadeAndTerminate);

        else if (event.type == sf::Event::Resized)
        {
            if (ignore_next_resize)
                ignore_next_resize = false;
            else
                on_resize();
        }

        else if (event.type == sf::Event::TextEntered)
        {
            if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)))
                keyboard.set_text_input(event.text.unicode);
        }

        else if (event.type == sf::Event::KeyPressed)
            keyboard.set_key_pressed(event.key);

        else if (event.type == sf::Event::MouseEntered)
            mouse_hovering_window_area = true;

        else if (event.type == sf::Event::MouseLeft)
            mouse_hovering_window_area = false;

        else if (event.type == sf::Event::MouseWheelScrolled)
        {
            if (event.mouseWheelScroll.wheel == sf::Mouse::Wheel::VerticalWheel)
                mouse.set_wheel_ticks_delta(event.mouseWheelScroll.delta);
        }
    }
}

void App::render()
{
    window.clear(Colors::BLACK);

    game.render(window);

    if (debug_components_initialized)
    {
        window.draw(debug_window);
        window.draw(fps_display);
    }

    if (mouse_hovering_window_area)
        window.draw(Cursor::instance());

    window.display();
}

void App::on_resize()
{
    auto width  = window.getSize().x;
    auto height = window.getSize().y;

    if (!(assure_greater_than_or_equal_to(width,  MIN_WINDOW_WIDTH) &
          assure_greater_than_or_equal_to(height, MIN_WINDOW_HEIGHT)))
    {
        window.setSize({ width, height });
        ignore_next_resize = true;
    }

    const PxVec2 resolution{ static_cast<float>(width), static_cast<float>(height) };

    window.setView(sf::View{ sf::FloatRect{ 0.f, 0.f, resolution.x, resolution.y } });

    // Preserve windowed-mode resolution in settings:
    if (!settings.fullscreen)
    {
        settings.window_width  = window.getSize().x;
        settings.window_height = window.getSize().y;
    }

    game.set_resolution(resolution);

    LOG_INTEL("new resolution applied: " + Convert::to_str(resolution));
}

void App::initialize_debug_components()
{
    if (debug_components_initialized)
        return;

    debug_components_initialized = true;

    debug_window.initialize();
    fps_display.initialize();
    fps_display.toggle_visible();
    game.initialize_debug_components();
}

void App::set_fps_cap(int fps_cap)
{
    if (fps_cap == 0)
    {
        settings.fps_cap = 0;
        window.setFramerateLimit(0);

        if (settings.vsync)
            EARManager::instance().queue_event(Event::DisplayMessage,
                                               "FPS cap removed; disable vSync to see effect.");
        else
            EARManager::instance().queue_event(Event::DisplayMessage, "FPS cap removed.");
        return;
    }

    assure_greater_than_or_equal_to(fps_cap, MIN_FPS_CAP);
    settings.fps_cap = fps_cap;

    if (settings.vsync)
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "FPS cap saved; disable vSync to see effect.");
    else
    {
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "FPS cap set: " + Convert::to_str(fps_cap));
        window.setFramerateLimit(fps_cap);
    }
}

void App::set_vsync(const bool enable)
{
    settings.vsync = enable;
    window.setVerticalSyncEnabled(enable);

    if (enable)
    {
        window.setFramerateLimit(0);
        EARManager::instance().queue_event(Event::DisplayMessage, "vSync enabled.");
    }
    else
    {
        window.setFramerateLimit(settings.fps_cap);
        EARManager::instance().queue_event(Event::DisplayMessage, "vSync disabled.");
    }
}

void App::set_fullscreen(const bool enable)
{
    if (settings.fullscreen != enable)
    {
        settings.fullscreen = enable;
        create_window();
    }
}

void App::set_audio_volume(int volume)
{
    assure_bounds(volume, 0, 100);
    settings.volume = volume;
    AudioPlayer::instance().set_volume(volume);

    if (volume == 0)
        EARManager::instance().queue_event(Event::DisplayMessage, "Audio disabled.");
    else
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "Audio enabled. Volume: " + Convert::to_str(volume));
}

void App::create_window()
{
    if (settings.window_width == 0u || settings.window_height == 0u)
    {
        settings.window_width  = static_cast<unsigned int>(sf::VideoMode::getDesktopMode().width  * 2 / 3);
        settings.window_height = static_cast<unsigned int>(sf::VideoMode::getDesktopMode().height * 3 / 4);
    }

    if (settings.fullscreen)
        window.create(sf::VideoMode::getFullscreenModes()[0],
                      TITLE,
                      sf::Style::Fullscreen);
    else
        window.create({ settings.window_width, settings.window_height },
                      TITLE,
                      sf::Style::Default);

    on_resize();

    window.setVerticalSyncEnabled(settings.vsync);
    if (!settings.vsync)
    {
        if (settings.fps_cap != 0)
            assure_greater_than_or_equal_to(settings.fps_cap, MIN_FPS_CAP);
        window.setFramerateLimit(settings.fps_cap);
    }
    window.setKeyRepeatEnabled(true);

    sf::Image icon;
    if (icon.loadFromFile(WINDOW_ICON_PATH))
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    else
        LOG_ALERT("window icon could not be loaded;\npath: " + WINDOW_ICON_PATH);
}

void App::save_and_terminate()
{
    app_running = false;

    try
    {
        game.save();
        settings.save_to_file(SETTINGS_FILE_PATH);
    }
    catch (const std::exception& e)
    {
        LOG_ALERT("uncaught exception during save_and_terminate;\nwhat: " + e.what());
    }
}

void App::on_event(const Event event, const Data& data)
{
    if (event == Event::Terminate)
        save_and_terminate();

    else if (event == Event::SetResolution)
        window.setSize(data.as<sf::Vector2u>());

    else if (event == Event::SetFPSCap)
        set_fps_cap(data.as<int>());

    else if (event == Event::SetVSync)
        set_vsync(data.as<bool>());

    else if (event == Event::SetFullscreen)
        set_fullscreen(data.as<bool>());

    else if (event == Event::SetAudioVolume)
        set_audio_volume(data.as<int>());

    else if (event == Event::SetTFMul)
    {
        float val = data.as<float>();
        assure_bounds(val, 0.f, 100.f);
        timeflow_multiplier = val;
    }

    else if (event == Event::SetLoadingScreen)
    {
        if (data.as<bool>())
        {
            window.setMouseCursorVisible(true);
            window.clear(Colors::BLACK);
            window.display();
        }
        else if (mouse_hovering_window_area)
            window.setMouseCursorVisible(false);
    }
}

void App::on_request(const Request request, Data& data)
{
    if (request == Request::Resolution)
        data.set(sf::Vector2u{ settings.window_width, settings.window_height });

    else if (request == Request::FPSCap)
        data.set(settings.fps_cap);

    else if (request == Request::VSync)
        data.set(settings.vsync);

    else if (request == Request::Fullscreen)
        data.set(settings.fullscreen);

    else if (request == Request::AudioVolume)
        data.set(settings.volume);
}