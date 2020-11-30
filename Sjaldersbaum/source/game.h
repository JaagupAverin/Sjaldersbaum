#pragma once

#include <optional>
#include <SFML/Graphics.hpp>

#include "level_player.h"
#include "mouse.h"
#include "keyboard.h"
#include "menu_bar.h"
#include "user.h"
#include "events-requests.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

class Game : Observer
{
public:
    Game();

    void initialize();

    void update_keyboard_input(const Keyboard& keyboard);
    void update_mouse_input(const Mouse& mouse);
    void update(Seconds elapsed_time);

    void render(sf::RenderWindow& window);

    void set_resolution(PxVec2 resolution);

    void save();

    void initialize_debug_components();

    void toggle_debug_mode();

private:
    void fade_out(bool termination = false);

    void fade_in();

    void fade_out_and_terminate();

    void fade_out_and_load_level(const std::string& level_path);

    void fade_out_and_load_user(const ID& user_id);

    // Executes any stored command_sequences that are destined for the loaded level.
    void try_execute_stored_command_sequences();

    // Data consists of a "level_path" and a "command_sequence"; see implementation.
    void try_store_command_sequence(const std::string& data);
    void store_all_queued_commands();

    void load_level(std::string level_path);

    // Saves the current level based on the user who loaded it.
    void save_current_level() const;

    void erase_save_for_current_level() const;

    void on_event(Event event, const Data& data) override;

    void on_request(Request request, Data& data) override;

private:
    MenuBar menu_bar;

    LevelPlayer level_player;
    std::string queued_level_path;
    std::string current_level_save_path;

    enum class BackgroundState
    {
        None,
        WaitingToTerminate,
        WaitingToLoadLevel,
        WaitingToLoadUser
    };
    BackgroundState background_state;
    Seconds         background_state_timer;

    User user;
    ID queued_user_id;

    bool debug_components_initialized;
};