#include "game.h"

#include <fstream>
#include <regex>
#include <filesystem>
#include <chrono>
#include <thread>

#include "string_assist.h"
#include "convert.h"
#include "level_paths.h"

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds FADEOUT_DURATION = 0.3f;
constexpr Seconds FADEIN_DURATION  = 0.5f;
constexpr Seconds MIN_BLACKOUT_DURATION = 0.7f;

/*------------------------------------------------------------------------------------------------*/

Game::Game() :
    background_state{ BackgroundState::None }, 
    background_state_timer{ 0.f },
    debug_components_initialized{ false }
{

}

void Game::initialize()
{
    EARManager::instance().dispatch_event(Event::SetLoadingScreen, true);

    User::load_user_list();
    if (!user.load_active_from_drive())
        user.become_guest();
    menu_bar.set_current_user_data(user.get_id(), user.time_played);

    load_level(user.last_level_path);
    fade_in();
    EARManager::instance().dispatch_event(Event::SetLoadingScreen, false);
}

void Game::update_keyboard_input(const Keyboard& keyboard)
{
    menu_bar.update_keyboard_input(keyboard);
    level_player.update_keyboard_input(keyboard);

    if (debug_components_initialized)
    {
        if (keyboard.is_keybind_pressed(DebugKeybinds::RELOAD_ACTIVE_LEVEL))
        {
            save_current_level();
            load_level(level_player.get_loaded_level_path());
        }
        else if (keyboard.is_keybind_pressed(DebugKeybinds::RESET_ACTIVE_LEVEL))
        {
            erase_save_for_current_level();
            load_level(level_player.get_loaded_level_path());
            fade_in();
        }
    }
}

void Game::update_mouse_input(const Mouse& mouse)
{
    level_player.update_mouse_input(mouse);
}

void Game::update(const Seconds elapsed_time)
{
    background_state_timer -= elapsed_time;
    // Leaving a level is delayed until all commands have been resolved.
    if (background_state != BackgroundState::None)
    {
        if (background_state_timer <= 0.f)
        {
            if (Executor::instance().is_busy())
                store_all_queued_commands();

            const BackgroundState expired_background_state = background_state;
            background_state = BackgroundState::None;

            if (expired_background_state == BackgroundState::WaitingToTerminate)
            {
                EARManager::instance().dispatch_event(Event::Terminate);
            }
            else if (expired_background_state == BackgroundState::WaitingToLoadLevel ||
                     expired_background_state == BackgroundState::WaitingToLoadUser)
            {
                const auto start = std::chrono::steady_clock::now();

                EARManager::instance().dispatch_event(Event::SetLoadingScreen, true);
                if (expired_background_state == BackgroundState::WaitingToLoadLevel)
                {
                    save_current_level();
                    load_level(queued_level_path);
                }
                else if (user.load(queued_user_id))
                {
                    save_current_level();
                    load_level(user.last_level_path);
                    menu_bar.set_current_user_data(user.get_id(), user.time_played);
                }

                // For aesthetic reasons, assure the blackout (loading) lasts for at least a minimum period of time:
                const auto end = std::chrono::steady_clock::now();
                const auto duration = (end - start).count();
                static constexpr auto min_duration =
                    static_cast<long long>(MIN_BLACKOUT_DURATION / SECONDS_IN_NANOSECOND);
                if (duration < min_duration)
                    std::this_thread::sleep_for(std::chrono::nanoseconds(min_duration - duration));

                EARManager::instance().dispatch_event(Event::SetLoadingScreen, false);
                fade_in();
            }
        }
    }

    user.time_played += elapsed_time;

    menu_bar.update(elapsed_time);
    level_player.update(elapsed_time);
}

void Game::render(sf::RenderWindow& window)
{
    level_player.render(window);
    window.draw(menu_bar);
}

void Game::set_resolution(const PxVec2 resolution)
{
    menu_bar.set_width(resolution.x);
    level_player.set_resolution(resolution);
}

void Game::save()
{
    User::save_user_list();
    user.save();
    save_current_level();
}

void Game::initialize_debug_components()
{
    debug_components_initialized = true;
    level_player.initialize_debug_components();
}

void Game::toggle_debug_mode()
{
    if (debug_components_initialized)
        level_player.toggle_debug_mode();
    else
        LOG_ALERT("cannot toggle uninitialized debug stats.");
}

void Game::fade_out(const bool termination)
{
    menu_bar.set_opacity(0.f, FADEOUT_DURATION);
    level_player.set_light_on(false, FADEOUT_DURATION, !termination);
    AudioPlayer::instance().fade_out(FADEOUT_DURATION, termination);
}

void Game::fade_in()
{
    menu_bar.set_opacity(1.f, FADEIN_DURATION);
    level_player.set_light_on(true, FADEIN_DURATION);
    AudioPlayer::instance().fade_in(FADEOUT_DURATION);
}

void Game::fade_out_and_terminate()
{
    if (background_state != BackgroundState::None)
    {
        LOG_INTEL("background_state already set; potential cause: bad use of postpone() command;\n"
                  "avoid using long postpone durations as it prevents the game from terminating.");
        return;
    }

    fade_out(true);
    background_state = BackgroundState::WaitingToTerminate;
    background_state_timer = FADEOUT_DURATION;
}

void Game::fade_out_and_load_level(const std::string& level_path)
{
    if (background_state != BackgroundState::None)
    {
        LOG_INTEL("background_state already set; potential cause: bad use of load_level() command;\n"
                  "avoid using load_ commands in succession.");
        return;
    }

    fade_out();
    background_state = BackgroundState::WaitingToLoadLevel;
    background_state_timer = FADEOUT_DURATION;
    queued_level_path = level_path;
}

void Game::fade_out_and_load_user(const ID& user_id)
{
    if (background_state != BackgroundState::None)
    {
        LOG_INTEL("background_state already set; potential cause: bad use of load_user() command"
                  "avoid using load_ commands in succession.");
        return;
    }

    if (!User::exists(user_id))
    {
        EARManager::instance().queue_event(Event::DisplayMessage, "User not found: " + user_id);
        return;
    }
    if (user.get_id() == user_id)
    {
        EARManager::instance().queue_event(Event::DisplayMessage, "User already active: " + user_id);
        return;
    }

    fade_out();
    background_state = BackgroundState::WaitingToLoadUser;
    background_state_timer = FADEOUT_DURATION;
    queued_user_id = user_id;
}

void Game::try_execute_stored_command_sequences()
{
    std::vector<std::string> command_list;

    auto& stored_command_sequences = user.stored_command_sequences;
    for (auto it = stored_command_sequences.begin(); it != stored_command_sequences.end();)
    {
        const auto& level_path       = it->first;
        const auto& command_sequence = it->second;

        if (level_path == level_player.get_loaded_level_path())
        {
            command_list.emplace_back(command_sequence);
            it = stored_command_sequences.erase(it);
        }
        else
            ++it;
    }

    Executor::instance().queue_execution(command_list, FADEIN_DURATION);
}

void Game::try_store_command_sequence(const std::string& data)
{
    // Storing a command expects two strings: a level_path and a command_sequence
    // (the later of which is executed whenever former becomes the currently loaded level).

    // Data is interpreted with the following pattern:
    // <level_path> [spaces] '?' [spaces] <command_sequence>
    static const std::regex data_pattern{
        R"(([\w\-\/\\. ]+?) *\? *([\S\s]+))" };

    std::smatch matches;
    // matches[0] -> full match
    // matches[1] -> level_path
    // matches[2] -> command_sequence

    if (std::regex_match(data, matches, data_pattern))
    {
        const std::string& level_path       = matches[1];
        const std::string& command_sequence = matches[2];
        user.stored_command_sequences.emplace_back(level_path, command_sequence);
    }
    else
    {
        LOG_ALERT("invalid command storage pattern:\n" + data);
        return;
    }
}

void Game::store_all_queued_commands()
{
    auto commands = Executor::instance().extract_queue();
    while (!commands.empty())
    {
        user.stored_command_sequences.emplace_back(level_player.get_loaded_level_path(),
                                                   std::move(commands.front()));
        commands.pop();
    }
}

void Game::load_level(std::string level_path)
{
    decapitalize(level_path);

    if (user.is_guest() || user.get_id() == "__NO_SAVES")
        current_level_save_path.clear();
    else
        current_level_save_path = user.get_save_path_for_level(level_path);

    std::string save_path = "";
    if (user.has_save_for_level(level_path))
        save_path = current_level_save_path;

    if (level_player.load(level_path, save_path))
    {
        // No event should be in the queue at this point. Assure that is the case:
        EARManager::instance().clear_queued_events();
    }
    else
    {
        if (level_path != LevelPaths::MAIN_MENU)
        {
            LOG_ALERT("failed to load level from: " + level_path +
                      "\nattempting to load main_menu instead.");
            EARManager::instance().queue_event(Event::DisplayMessage,
                                               "Invalid level! Returning to main menu.");
            load_level(LevelPaths::MAIN_MENU);
        }
        else
        {
            LOG_ALERT("failed to load main_menu level; terminating.");
            EARManager::instance().dispatch_event(Event::Terminate);
        }
        return;
    }

    // Menu-bar:
    menu_bar.clear_messages();
    menu_bar.queue_message("Loaded: " + level_player.get_menu_bar_data().title);
    menu_bar.set_action(level_player.get_menu_bar_data().command_sequence,
                        level_player.get_menu_bar_data().description,
                        level_player.get_menu_bar_data().sound_path);

    user.last_level_path = level_path;
    try_execute_stored_command_sequences();
}

void Game::save_current_level() const
{
    if (level_player.has_level_loaded() && !current_level_save_path.empty())
        level_player.save(current_level_save_path);
}

void Game::erase_save_for_current_level() const
{
    if (!current_level_save_path.empty())
    {
        LOG_INTEL("erasing level: " + current_level_save_path);

        try
        {
            std::filesystem::remove(current_level_save_path);
        }
        catch (const std::exception& e)
        {
            LOG_ALERT("level could not be erased; exception:\n" + e.what());
        }
    }
}

void Game::on_event(const Event event, const Data& data)
{
    if (event == Event::FadeAndTerminate)
        fade_out_and_terminate();

    else if (event == Event::DisplayMessage)
        menu_bar.queue_message(data.as<std::string>());

    else if (event == Event::LoadMenu)
        fade_out_and_load_level(LevelPaths::MAIN_MENU);

    else if (event == Event::LoadLevel)
        fade_out_and_load_level(data.as<std::string>());

    else if (event == Event::LoadUser)
        fade_out_and_load_user(data.as<ID>());

    else if (event == Event::CreateUser)
        User::create(data.as<ID>());

    else if (event == Event::EraseUser)
        User::erase(data.as<ID>());

    else if (event == Event::StoreCommandSequence)
        try_store_command_sequence(data.as<std::string>());
}

void Game::on_request(const Request request, Data& data)
{
    if (request == Request::ActiveUser)
        data.set(user.get_id());

    else if (request == Request::UserList)
        data.set(get_as_formatted_string(User::get_user_list()));
}