#include "commands.h"

#include <sstream>
#include <regex>

#include "logger.h"
#include "events-requests.h"
#include "string_assist.h"
#include "convert.h"
#include "rm.h"

/*------------------------------------------------------------------------------------------------*/

const std::unordered_map<std::string, Event> COMMAND_EVENTS
{
    { "exit",           Event::FadeAndTerminate },
    { "res",            Event::SetResolution },
    { "fpscap",         Event::SetFPSCap },
    { "vsync",          Event::SetVSync },
    { "fullscreen",     Event::SetFullscreen },
    { "volume",         Event::SetAudioVolume },
    { "tfmul",          Event::SetTFMul },
    { "menu",           Event::LoadMenu },
    { "load_level",     Event::LoadLevel },
    { "load_user",      Event::LoadUser },
    { "create_user",    Event::CreateUser },
    { "erase_user",     Event::EraseUser },

    { "message",        Event::DisplayMessage },
    { "center",         Event::SetCameraCenter },
    { "zoom_in",        Event::ZoomIn },
    { "zoom_out",       Event::ZoomOut },
    { "lshader",        Event::SetLightShader },
    { "lsource",        Event::SetLightSource },
    { "lradius",        Event::SetLightRadius },
    { "lbrightness",    Event::SetLightBrightness },
    { "lswing",         Event::SetLightSwing },
    { "lon",            Event::SetLightOn },
    { "advance",        Event::AdvanceObjective },
    { "hide",           Event::Hide },
    { "hide_ic",        Event::HideMoveCamera },
    { "reveal",         Event::Reveal },
    { "reveal_ic",      Event::RevealDoNotMoveCamera },
    { "unlock",         Event::Unlock },
    { "lock",           Event::Lock },
    { "play",           Event::PlayAudio },
    { "stream",         Event::StreamAudio },
    { "stop",           Event::StopStream },
    { "all",            Event::RevealAllObjects },
    { "store",          Event::StoreCommandSequence }
};

#define EMPLACE_POSTPONE_COMMAND(sec) commands.emplace("postpone(" + Convert::to_str(sec) + ")")

/*------------------------------------------------------------------------------------------------*/

Executor& Executor::instance()
{
    static Executor singleton;
    return singleton;
}

void Executor::update(const Seconds elapsed_time)
{
    if (postpone_timer > 0.f)
        postpone_timer -= elapsed_time;

    while (!commands.empty() && postpone_timer <= 0.f)
    {
        const std::string command = commands.front();
        commands.pop();
        execute(command);
    }
}

void Executor::queue_execution(const std::string& command_sequence, const Seconds postpone)
{
    if (command_sequence.empty())
        return;

    if (postpone != 0.f)
        EMPLACE_POSTPONE_COMMAND(postpone);

    // Commands are separated only by unparenthesized semicolons;
    // * parentheses indicate the semicolon belongs to some command's argument.
    // Parentheses are therefore tracked, but only outside of quotes;
    // * quotes indicate string-arguments such as arbitrary messages.

    std::vector<int> command_separator_indices;

    int quotes_level = 0;
    int parentheses_level = 0;

    for (size_t i = 0; i != command_sequence.size(); ++i)
    {
        const char ch = command_sequence[i];

        auto prev_ch = i == 0 ? 0 : command_sequence[i - 1];
        auto next_ch = i == command_sequence.size() - 1 ? 0 : command_sequence[i + 1];

        if (ch == '"' && prev_ch == '(')
            ++quotes_level;
        else if (ch == '"' && next_ch == ')')
            --quotes_level;
        else if (quotes_level == 0)
        {
            if (ch == '(')
                ++parentheses_level;
            else if (ch == ')')
                --parentheses_level;
            else if (ch == ';' && parentheses_level == 0)
                command_separator_indices.emplace_back(i);
        }
    }

    // If the final character isn't a separator, insert an end-of-string index:
    if (command_separator_indices.empty() ||
        (command_separator_indices.back() != command_sequence.length() - 1))
        command_separator_indices.emplace_back(command_sequence.length());

    int begin = 0;
    int end;
    for (const auto i : command_separator_indices)
    {
        end = i;
        commands.emplace(command_sequence.substr(begin, end - begin));
        begin = end + 1;
    }
}

void Executor::queue_execution(const std::vector<std::string>& command_sequence_list,
                               const Seconds postpone)
{
    if (postpone != 0.f)
        EMPLACE_POSTPONE_COMMAND(postpone);

    for (const auto& command_sequence : command_sequence_list)
        queue_execution(command_sequence, 0.f);
}

bool Executor::is_busy() const
{
    return !commands.empty();
}

std::queue<std::string> Executor::extract_queue()
{
    auto res = std::queue<std::string>();
    std::swap(res, commands);
    return res;
}

void Executor::execute(const std::string command)
{
    // Pattern:
    // <command_name> ['(' <optional_args> ')']
    static const std::regex command_pattern{
        R"(\s*(\w+)\s*(\(\s*([\S\s]*?)\s*\))?\s*)" };

    std::smatch matches;
    // matches[0] -> full matchs
    // matches[1] -> command_name before parentheses
    // matches[2] -> args with parentheses
    // matches[3] -> args without parentheses

    std::string command_name;
    std::string args;

    if (std::regex_match(command, matches, command_pattern))
    {
        command_name = matches[1];
        args = matches[3];
    }
    else
    {
        if (std::all_of(command.begin(), command.end(), isspace))
            return; // Don't log error for empty command.

        LOG_ALERT("invalid syntax (within quotes): \"" + command + '"');
        return;
    }

    dequote(args);

    if (contains(COMMAND_EVENTS, command_name))
    {
        EARManager::instance().queue_event(COMMAND_EVENTS.at(command_name), args);
    }
    else if (command_name == "help")
    {
        LOG("--------- HELP0 ------------------------------------------------------\n"
            "F1 - toggle the console\n"
            "F2 - toggle debug mode\n"
            "F3 - toggle fps cap\n"
            "F4 - reload level\n"
            "F5 - reload textures\n"
            "F6 - reload soundbuffers\n"
            "F8 - reset level (erase and reload)\n"
            "help1 .... technical commands\n"
            "help2 .... level-design commands");
    }
    else if (command_name == "help1")
    {
        LOG("--------- HELP1 ------------------------------------------------------\n"
            "exit ............. terminate app\n"
            "res(x,y) ......... set window resolution\n"
            "fpscap(i) ........ set FPS cap\n"
            "vsync(bool) ...... set vSync\n"
            "fullscreen(bool) . set fullscreen\n"
            "volume(int) ...... set audio volume\n"
            "tfmul(mul) ....... set timeflow multiplier\n"
            "list_rsrcs ....... log all loaded resources\n"
            "rsrc_log(bool) ... set resource logging\n"
            "menu.............. load the menu level\n"
            "load_level(path) . load level\n"
            "load_user(ID) .... load user (automatically loads its last level)\n"
            "create_user(ID) .. create new user\n"
            "erase_user(ID) ... erase a non-active user\n");
    }
    else if (command_name == "help2")
    {
        LOG("--------- HELP2 ------------------------------------------------------\n"
            "message(\"str\") ...... display a string on menu_bar\n"
            "center(x,y,sec) ..... set camera center over period\n"
            "zoom_in(sec) ........ zoom in over a period\n"
            "zoom_out(sec) ....... zoom out over a period\n"
            "lshader(path) ....... set active level's light shader\n"
            "lsource(x,y,sec) .... set light's source over period\n"
            "lradius(x,sec) ...... set light's radius over period\n"
            "lbrightness(x,sec) .. set light's brightness over period\n"
            "lswing(x,sec) ....... set light's swing over period\n"
            "lon(bool, sec)....... set light on or off over period\n"
            "list_users .......... log all users\n"
            "advance(ID) ......... increment objective's progress\n"
            "hide(ID[::ID]) ...... hide an entity\n"
            "hide_ic(ID[::ID]) ... --||-- but also move camera to object\n"
            "reveal(ID[::ID]) .... reveal an entity (and any entity containing it)\n"
            "reveal_ic(ID[::ID]) . --||-- but do not move camera to object\n"
            "unlock(ID::ID)....... unlock an element (button/inputline)\n"
            "lock(ID::ID)......... lock an element (button/inputline)\n"
            "play(path, vol) ..... plays an audio loaded from path (with volume)\n"
            "stream(path, vol) ... streams an audio from path (with volume)\n"
            "stop(path) .......... stops all streams from path\n"
            "all ................. reveal all objects\n"
            "postpone(sec) ....... postpone proceeding commands for a period\n"
            "store(lvl?cmnd) ..... if (loaded_level == lvl) execute cmnd");
    }
    else if (command_name == "list_rsrcs")
    {
        log_all_loaded_resources();
    }
    else if (command_name == "rsrc_log")
    {
        RESOURCE_LOGGING = Convert::str_to<bool>(args);
        LOG_INTEL("resource logging set to: " + Convert::to_str(RESOURCE_LOGGING));
    }
    else if (command_name == "list_users")
    {
        LOG_INTEL("users:\n" + EARManager::instance().request(Request::UserList).as<std::string>());
    }
    else if (command_name == "postpone")
    {
        Seconds postpone_duration = Convert::str_to<Seconds>(args);
        if (!assure_bounds(postpone_duration, 0.f, 10.f))
            LOG_ALERT("invalid postpone duration had to be adjusted; [0-10]");
        postpone_timer = postpone_duration;
    }
    else
    {
        LOG_ALERT("command not recognized: " + command_name);
    }
}