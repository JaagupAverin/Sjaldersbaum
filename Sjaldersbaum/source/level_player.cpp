#include "level_player.h"

#include <optional>
#include <chrono>
#include <fstream>
#include <regex>

#include "audio.h"
#include "user.h"
#include "yaml.h"
#include "logger.h"
#include "cursor.h"
#include "convert.h"
#include "colors.h"
#include "maths.h"
#include "string_assist.h"
#include "units.h"
#include "level_paths.h"

/*------------------------------------------------------------------------------------------------*/
// MenuBarData:

bool MenuBarData::initialize(const YAML::Node& node)
{
    title            = "Untitled";
    command_sequence = "menu";
    description      = "Hold Escape to return to Menu.";
    sound_path       = "";

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node title_node       = node["title"];
            const YAML::Node command_node     = node["command"];
            const YAML::Node description_node = node["description"];
            const YAML::Node sound_path_node  = node["sound"];

            if (title_node.IsDefined())
                title = title_node.as<std::string>();

            if (command_node.IsDefined())
                command_sequence = command_node.as<std::string>();
            
            if (description_node.IsDefined())
                description = description_node.as<std::string>();

            if (sound_path_node.IsDefined())
                sound_path = sound_path_node.as<std::string>();
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "======================================================================\n"
                      "* title:       <std::string> = \"Untitled\"\n"
                      "* command:     <std::string> = \"menu\"\n"
                      "* description: <std::string> = \"Hold Escape to return to menu-level.\"\n"
                      "* sound:       <std::string> = <NONE>\n"
                      "======================================================================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    return true;
}

/*------------------------------------------------------------------------------------------------*/
// AudioData:

bool AudioData::initialize(const YAML::Node& node)
{
    known_sound_paths.clear();
    playlist.clear();
    playlist_shuffle = false;
    playlist_interval = 0.f;
    playlist_loudness = 1.f;

    if (node.IsDefined())
    {
        try
        {
            const YAML::Node sounds_node      = node["sounds"];
            const YAML::Node playlist_node    = node["playlist"];
            const YAML::Node pl_shuffle_node  = node["pl_shuffle"];
            const YAML::Node pl_interval_node = node["pl_interval"];
            const YAML::Node pl_loudness_node = node["pl_loudness"];

            if (sounds_node.IsDefined())
                for (auto subnode : sounds_node)
                {
                    const std::string path = subnode.as<std::string>();
                    known_sound_paths[path] = AudioPlayer::instance().load(path, false);
                }

            if (playlist_node.IsDefined())
                for (auto subnode : playlist_node)
                    playlist.emplace_back(subnode.as<std::string>());

            if (pl_shuffle_node.IsDefined())
                playlist_shuffle = pl_shuffle_node.as<bool>();

            if (pl_interval_node.IsDefined())
                playlist_interval = pl_interval_node.as<Seconds>();

            if (pl_loudness_node.IsDefined())
                playlist_loudness = pl_loudness_node.as<float>();
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "// =========================================\n"
                      "// * sounds:    seq<std::string> = []\n"
                      "// * playlist:  seq<std::string> = []\n"
                      "// ==ADVANCED===============================\n"
                      "// * pl_shuffle:  <bool>    = false\n"
                      "// * pl_interval: <Seconds> = 1\n"
                      "// * pl_loudness: <float>   = 1\n"
                      "// =========================================\n"
                      "DUMP:\n" + YAML::Dump(node));
            return false;
        }
    }

    return true;
}

/*------------------------------------------------------------------------------------------------*/
// Objective:

Objective::Objective() :
    progress{ 0 },
    target  { 0 }
{

}

void Objective::advance()
{
    if (progress >= target)
        LOG_INTEL("objective already complete;");
    else if (++progress == target)
        Executor::instance().queue_execution(command_sequence);
}

bool Objective::initialize(const YAML::Node& node)
{
    if (!node.IsDefined())
    {
        LOG_ALERT("undefined node.");
        return false;
    }

    try
    {
        const YAML::Node command_node  = node["command"];
        const YAML::Node target_node   = node["target"];
        const YAML::Node progress_node = node["progress"];

        command_sequence = command_node.as<std::string>();

        int target = target_node.as<int>();
        if (!assure_greater_than_or_equal_to(target, 1))
            LOG_ALERT("non-positive target had to be adjusted.");
        this->target = target;

        if (progress_node.IsDefined())
        {
            int progress = progress_node.as<int>();
            if (!assure_greater_than_or_equal_to(progress, 0))
                LOG_ALERT("negative progress had to be adjusted.");
            this->progress = progress;
        }
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that consists of:\n"
                  "==========================\n"
                  "* command:  <std::string>\n"
                  "* target:   <int>\n"
                  "* progress: <int> = 0\n"
                  "==========================\n"
                  "DUMP:\n" + YAML::Dump(node));
        return false;
    }

    return true;
}

YAML::Node Objective::serialize_dynamic_data() const
{
    YAML::Node node;
    node["progress"] = progress;
    return node;
}

/*------------------------------------------------------------------------------------------------*/
// Macros:

// Macros are shorthands for other strings, defined at the start of level files with #define.
// They consist of an "ID" and a "constant":
// ID       -> a string ([\w] chars only) that identifies the macro;
// constant -> a string that will replace the ID.
void find_and_apply_macros(std::string& content)
{
    std::stringstream buffer{ content };

    // Macro definition pattern:
    // (\n #define (spaces) <ID> (spaces) <constant> \n)
    static const std::regex macro_pattern{ R"(#define +(\w+) +(.+))" };

    std::smatch matches;
    // matches[0] -> full match
    // matches[1] -> ID
    // matches[2] -> constant
    std::vector<std::pair<ID, std::string>> macros;

    std::string line;
    while (std::getline(buffer, line))
    {
        if (std::regex_match(line, matches, macro_pattern))
        {
            macros.emplace_back(matches[1], matches[2]);
        }
        else if (!line.empty() && line[0] != '#')
        {
            // Macros can only be defined at the start of the file;
            // If a non-comment/non-empty line is found, stop looking for macros.
            break;
        }
        content.erase(0, content.find_first_of('\n') + 1);
    }

    // Macros may include others defined before them, and must therefore be applied in reverse order:
    for (int i = static_cast<int>(macros.size()) - 1; i >= 0; --i)
        find_and_replace(content, macros[i].first, macros[i].second);
}

#define AS_YAML_STR(str) "\"" + str + '"'

// Returns the pre-defined system macros.
std::unordered_map<ID, std::string> get_system_macros()
{
    return {
    { "__FPS_CAP",
      AS_YAML_STR(EARManager::instance().request(Request::FPSCap).as<std::string>()) },
    { "__VSYNC",
      AS_YAML_STR(EARManager::instance().request(Request::VSync).as<std::string>()) },
    { "__FULLSCREEN",
      AS_YAML_STR(EARManager::instance().request(Request::Fullscreen).as<std::string>()) },
    { "__VOLUME",
    AS_YAML_STR(EARManager::instance().request(Request::AudioVolume).as<std::string>()) },
    { "__ACTIVE_USER",
      AS_YAML_STR(EARManager::instance().request(Request::ActiveUser).as<std::string>()) },
    { "__MENU_PATH",
      LevelPaths::MAIN_MENU },
    { "__FOCUS", // The default center of focus is at (0, 100) for aesthetic reasons. See Light/Camera.
      "{ x: 0, y: 100 }" } };
}

/*------------------------------------------------------------------------------------------------*/

constexpr Seconds CAMERA_SLIDE_DURATION = 0.2f;
constexpr Seconds GENERAL_INPUT_COOLDOWN = 0.1f;
constexpr Seconds CURSOR_HIDE_DELAY = 0.4f;
constexpr Seconds OBJECT_MINIMUM_PICKUP_DURATION = 0.3f;

extern const sf::Color CROSSHAIR_ON_OBJECT_COLOR;
extern const sf::Color CROSSHAIR_ON_TABLE_COLOR;

const std::string DEFAULT_TLC_OVERLAY_TEXTURE_PATH = "resources/textures/overlays/smudgy_tlc.png";
const std::string DEFAULT_BRC_OVERLAY_TEXTURE_PATH = "resources/textures/overlays/smudgy_brc.png";

const ParticleExplosion CROSSHAIR_EXPLOSION_ON_TABLE{ Colors::BLACK, CROSSHAIR_ON_TABLE_COLOR,
                                                      160.f, 0.4f };

const ParticleExplosion CROSSHAIR_EXPLOSION_ON_OBJECT{ Colors::BLACK, CROSSHAIR_ON_OBJECT_COLOR,
                                                       160.f, 0.4f };

const ParticleExplosion MOUSE_EXPLOSION{ Colors::BLACK, Colors::BLACK_SEMI_TRANSPARENT,
                                         170.f, 0.3f };

const ParticleExplosion MOUSE_BIG_EXPLOSION{ Colors::BLACK, Colors::BLACK_SEMI_TRANSPARENT,
                                             100.f, 0.6f };

LevelPlayer::LevelPlayer() :
    clasp_cooldown              { 0.f },
    clasp_duration              { 0.f },
    mouse_grab_duration         { 0.f },
    tab_cooldown                { 0.f },
    interaction_key_lag         { DOUBLE_CLICK_INTERVAL },
    level_loaded                { false },
    debug_components_initialized{ false },
    debug_mode                  { false }
{

}

void LevelPlayer::update_keyboard_input(const Keyboard& keyboard)
{
    camera.update_keyboard_input(keyboard);

    if (camera.is_moved_by_keyboard())
        indicator.set_position(camera.get_center(), Indicator::InputSource::Keyboard);

    if (keyboard.is_keybind_pressed(DefaultKeybinds::INTERACT) && interaction_key_lag >= 0.1f)
    {
        indicator.set_interaction_key_pressed(true, Indicator::InputSource::Keyboard);
        indicator.set_position(camera.get_center());
        unclasp();

        if (interaction_key_lag <= DOUBLE_CLICK_INTERVAL)
            indicator.set_interaction_key_double_pressed(true, Indicator::InputSource::Keyboard);
        interaction_key_lag = 0.f;
    }
    else if (keyboard.is_keybind_pressed(DefaultKeybinds::TOGGLE_CLASP) && clasp_cooldown <= 0.f)
    {
        if (!clasped_object)
            clasp(get_topmost_visible_object(camera.get_view().getCenter()));
        else
            unclasp();
        clasp_cooldown = GENERAL_INPUT_COOLDOWN;
    }

    if (keyboard.is_keybind_pressed(DefaultKeybinds::TOGGLE_PREVIOUS_OBJECT))
    {
        if (previous_object && tab_cooldown <= 0.f)
        {
            if (!previous_object->is_visible())
                previous_object.reset();
            else
            {
                camera.set_center_progressively(previous_object->get_center(), CAMERA_SLIDE_DURATION);
                set_active_object(previous_object);
                tab_cooldown = GENERAL_INPUT_COOLDOWN;
            }
        }
    }

    if (active_object)
        active_object->update_keyboard_input(keyboard);
}

void LevelPlayer::update_mouse_input(const Mouse& mouse)
{    
    camera.update_mouse_input(mouse);

    // Free clasped object upon mouse activity:
    if (mouse.is_left_held() || mouse.is_right_held() || mouse.get_wheel_ticks_delta() != 0)
        unclasp();

    const PxVec2 mouse_position = mouse.get_position_in_view(camera.get_view());

    if (indicator.get_latest_input_source() == Indicator::InputSource::Mouse ||
        mouse.has_moved() || camera.is_zoomed_by_mouse())
        indicator.set_position(mouse_position, Indicator::InputSource::Mouse);
    
    if (mouse.is_left_clicked())
    {
        indicator.set_interaction_key_pressed(true, Indicator::InputSource::Mouse);
        indicator.set_position(mouse_position);
    }
    if (mouse.is_left_double_clicked())
        indicator.set_interaction_key_double_pressed(true, Indicator::InputSource::Mouse);

    // Mouse grabbing:
    if (mouse.has_left_dragging_just_started())
    {
        mouse_pos_after_grab = mouse.get_left_position_initial_in_view(camera.get_view());
        mouse_grab(get_topmost_visible_object(mouse_pos_after_grab));
    }
    if (!mouse.is_left_held())
        mouse_ungrab();

    if (mouse_grabbed_object)
    {
        const PxVec2 mouse_initial = mouse_pos_after_grab;
        const PxVec2 mouse_current = mouse_position;

        const PxVec2 object_center{
            mouse_grabbed_object_initial_center.x + mouse_current.x - mouse_initial.x,
            mouse_grabbed_object_initial_center.y + mouse_current.y - mouse_initial.y };

        if (object_center != mouse_grabbed_object->get_center())
            mouse_grabbed_object->set_position(object_center, Origin::Center);
        
        table.assure_contains(*mouse_grabbed_object);
    }
}

void LevelPlayer::update_indicator_input()
{
    indicator.set_type(Indicator::Type::Regular);

    const std::shared_ptr<Object> indicated_topmost_visible_object =
        get_topmost_visible_object(indicator.get_position());

    if (indicator.is_interaction_key_pressed())
    {
        set_active_object(indicated_topmost_visible_object);

        if (indicator.get_latest_input_source() == Indicator::InputSource::Keyboard)
        {
            if (indicated_topmost_visible_object)
                indicator_particles.create_explosion(indicator.get_position(), CROSSHAIR_EXPLOSION_ON_OBJECT);
            else
                indicator_particles.create_explosion(indicator.get_position(), CROSSHAIR_EXPLOSION_ON_TABLE);

            crosshair.on_interaction();
            crosshair.set_visible(true);
        }
        else if (indicator.get_latest_input_source() == Indicator::InputSource::Mouse)
        {
            indicator_particles.create_explosion(indicator.get_position(),
                                                 indicator.is_interaction_key_double_pressed() ? 
                                                 MOUSE_BIG_EXPLOSION : MOUSE_EXPLOSION);
        }
        AudioPlayer::instance().play(GlobalSounds::INTERACTION);
    }

    set_hovered_object(indicated_topmost_visible_object);
    if (hovered_object)
        hovered_object->update_indicator_input(indicator);

    // Camera movement overwrites all other Indicator Types:
    if (camera.is_moved_by_mouse())
        indicator.set_type(Indicator::Type::MovingCamera);
    else if (camera.is_moved_by_keyboard() && indicator.get_type() != Indicator::Type::HoveringTextField)
        indicator.set_type(Indicator::Type::MovingCamera);

    if (indicator.get_latest_input_source() == Indicator::InputSource::Keyboard)
    {
        crosshair.set_type(indicator.get_type());
        crosshair.set_visible(true);
        Cursor::instance().set_visible(false, CURSOR_HIDE_DELAY);
    }
    else if (indicator.get_latest_input_source() == Indicator::InputSource::Mouse)
    {
        crosshair.set_visible(false);
        Cursor::instance().set_type(indicator.get_type());
        Cursor::instance().set_visible(true);
    }

    indicator.reset_input();
}

void LevelPlayer::update(const Seconds elapsed_time)
{
    update_indicator_input();

    clasp_cooldown      -= elapsed_time;
    clasp_duration      += elapsed_time;
    mouse_grab_duration += elapsed_time;
    tab_cooldown        -= elapsed_time;
    interaction_key_lag += elapsed_time;

    /*--------------------------------------------------------------------------------------------*/
    // Camera:

    camera.update(elapsed_time);

    if (clasped_object)
    {
        const PxVec2 camera_offsets = camera.get_center() - camera_center_after_clasp;
        const PxVec2 object_center = clasped_object_initial_center + camera_offsets;

        if (object_center != clasped_object->get_center())
            clasped_object->set_position(object_center, Origin::Center);

        table.assure_contains(*clasped_object);
    }

    /*--------------------------------------------------------------------------------------------*/
    // Crosshair:

    crosshair.set_center(camera.get_center(), camera.is_moved_by_keyboard());
    crosshair.update(elapsed_time);

    /*--------------------------------------------------------------------------------------------*/
    // Light and other effects:

    light.update(elapsed_time);
    indicator_particles.update(elapsed_time);

    /*--------------------------------------------------------------------------------------------*/
    // Objects:

    for (const auto& [id, object] : objects)
        if (!object->is_idle())
            object->update(elapsed_time);
}

void LevelPlayer::set_light_on(const bool on, const Seconds transition_duration, const bool sound)
{
    light.set_on(on, transition_duration, sound);
}

void LevelPlayer::set_resolution(const PxVec2 resolution)
{
    camera.set_resolution(resolution);
    GUI_view = sf::View{ sf::FloatRect{ 0.f, 0.f, resolution.x, resolution.y } };

    if (level_loaded)
        scale_and_position_overlays();

    static const sf::ContextSettings base_settings{ 0, 0, 4 };

    base_canvas.create(static_cast<unsigned int>(resolution.x),
                       static_cast<unsigned int>(resolution.y),
                       base_settings);
    final_canvas.create(static_cast<unsigned int>(resolution.x),
                        static_cast<unsigned int>(resolution.y));

    final_sprite.setTexture(final_canvas.getTexture(), true);
}

bool LevelPlayer::load(const std::string& level_path, const std::string& save_path)
{
    // Clean up:

    level_loaded = false;
    loaded_level_path = "";
    clear_objects();
    objectives.clear();
    AudioPlayer::instance().stop_and_unload_all();

    const auto start = std::chrono::steady_clock::now();

    // Load level:

    LOG_INTEL("loading level: " + level_path);

    if (!consists_of_systemic_characters(level_path))
    {
        LOG_ALERT("level path contains unsupported characters.");
        return false;
    }

    std::ifstream level_file{ level_path };
    if (!level_file)
    {
        LOG_ALERT("level file could not be opened.");
        return false;
    }

    std::stringstream buffer;
    buffer << level_file.rdbuf();
    std::string level_data = buffer.str();

    // Macros:

    for (const auto& [id, constant] : get_system_macros())
        find_and_replace(level_data, id, constant);
    find_and_apply_macros(level_data);

    // Parse level:

    YAML::Node level_node;
    try
    {
        level_node = YAML::Load(level_data);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during level_data deserialization;\nexception: " + e.msg +
                  "\nline: " + Convert::to_str(e.mark.line) + "\ncontent:\n" + level_data);
        return false;
    }

    // Load and apply save:

    std::vector<ID> objects_save_order;

    if (!save_path.empty())
    {
        LOG_INTEL("applying save data from: " + save_path);

        if (!consists_of_systemic_characters(save_path))
        {
            LOG_ALERT("save path contains unsupported characters.");
            return false;
        }

        std::ifstream save_file{ save_path };
        if (!save_file)
        {
            LOG_ALERT("save file could not be opened.");
            return false;
        }

        std::stringstream buffer;
        buffer << save_file.rdbuf();
        const std::string save_data = buffer.str();

        YAML::Node save_node;
        try
        {
            save_node = YAML::Load(save_data);
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("unknown YAML during save_data deserialization;\nexception: " + e.msg +
                      "\nline: " + Convert::to_str(e.mark.line) + "\nsave data:\n" + save_data);
            return false;
        }

        try
        {
            YAML::insert_all_values(level_node, save_node);

            YAML::Node saved_objects_node = save_node["objects"];
            if (saved_objects_node.IsDefined() && saved_objects_node.IsMap())
                for (const auto& node : saved_objects_node)
                    objects_save_order.emplace_back(node.first.Scalar());
        }
        catch (const YAML::Exception& e)
        {
            LOG_ALERT("unknown YAML exception during save-data insertion;\n"
                      "\nexception: " + e.msg + "\nDUMP:\n" + YAML::Dump(save_node));
            return false;
        }
    }

    // Initialize:

    if (!initialize(level_node))
        return false;
    if (!objects_save_order.empty())
        order_objects(objects_save_order);
    if (level_path == LevelPaths::MAIN_MENU)
        insert_user_list_into_menu_level();

    // Finish:

    loaded_level_path = level_path;
    level_loaded = true;

    const auto end = std::chrono::steady_clock::now();
    LOG_INTEL("level successfully loaded in " + Convert::to_str(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) + " ms.");

    return true;
}

bool LevelPlayer::save(const std::string& save_path) const
{
    if (!level_loaded)
    {
        LOG_ALERT("no level is loaded.");
        return false;
    }

    LOG_INTEL("saving level to: " + save_path);

    if (!consists_of_systemic_characters(save_path))
    {
        LOG_ALERT("save path contains unsupported characters.");
        return false;
    }

    const auto start = std::chrono::steady_clock::now();

    // Serialize:

    std::stringstream buffer;
    try
    {
        YAML::Node node = serialize_dynamic_data();
        buffer << YAML::Dump(node);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during serialization:\n" + e.msg);
        return false;
    }

    // Save:

    std::ofstream file{ save_path, std::ios_base::trunc };
    if (!file)
    {
        LOG_ALERT("level file could not be opened for writing;");
        return false;
    }
    file << buffer.rdbuf();

    const auto end = std::chrono::steady_clock::now();
    LOG_INTEL("level successfully saved in " + Convert::to_str(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) + " ms.");

    return true;
}

void LevelPlayer::render(sf::RenderTarget& target)
{
    /*--------------------------------------------------------------------------------------------*/
    // Draw the base canvas (a RenderTexture portraying the currently viewed region of the table):

    base_canvas.clear(Colors::BLACK);

    base_canvas.setView(camera.get_view());
    base_canvas.draw(table);
    for (const auto& [id, object] : objects)
        base_canvas.draw(*object);
    base_canvas.draw(indicator_particles);
    base_canvas.draw(crosshair);

    base_canvas.setView(GUI_view);
    base_canvas.draw(tlc_overlay);
    base_canvas.draw(brc_overlay);

    base_canvas.display();

    /*--------------------------------------------------------------------------------------------*/
    // Shaders + final draw:

    light.apply(base_canvas, final_canvas, camera.get_view());
    target.draw(final_sprite);

    /*--------------------------------------------------------------------------------------------*/
    // Debug:

    if (debug_mode)
    {
        target.setView(camera.get_view());

        light.render_debug_lines(target);
        if (hovered_object)
        {
            hovered_object->render_debug_bounds(target);

            sf::Text text;
            text.setString(get_object_id(hovered_object));
            text.setFont(debug_font.get());
            text.setPosition(hovered_object->get_center());
            text.setFillColor(Colors::RED);
            text.setOutlineColor(Colors::BLACK);
            text.setOutlineThickness(2.f);
            text.setCharacterSize(20u);
            target.draw(text);
        }
        target.setView(GUI_view);
        camera.render_debug_stats(target);
    }
}

const MenuBarData& LevelPlayer::get_menu_bar_data() const
{
    return menu_bar_data;
}

std::string LevelPlayer::get_loaded_level_path() const
{
    return loaded_level_path;
}

bool LevelPlayer::has_level_loaded() const
{
    return level_loaded;
}

void LevelPlayer::initialize_debug_components()
{
    debug_components_initialized = true;
    debug_font.load(SYSTEM_FONT_PATH);
    camera.initialize_debug_components();
}

void LevelPlayer::toggle_debug_mode()
{
    debug_mode = !debug_mode;
    camera.toggle_debug_mode();
}

ID LevelPlayer::get_object_id(const std::shared_ptr<Object> object) const
{
    for (const auto& [id, mapped_object] : objects)
    {
        if (object == mapped_object)
            return id;
    }
    return "";
}

std::shared_ptr<Object> LevelPlayer::get_object(const ID& object_id)
{
    if (contains(objects, object_id))
        return objects.at(object_id);
    return nullptr;
}

std::shared_ptr<Object> LevelPlayer::get_topmost_visible_object(const PxVec2 position)
{
    for (size_t i = 0; i != objects.size(); ++i)
    {
        const auto& pair = objects.cend() - i - 1;
        std::shared_ptr<Object> object = pair->second;
        if (object->is_visible() && object->contains(position))
            return object;
    }
    return nullptr;
}

void LevelPlayer::set_topmost_object(std::shared_ptr<Object> object)
{
    if (object && (!objects.empty() && objects.back().second != object))
    {
        ID object_id = get_object_id(object);
        if (object_id.empty())
        {
            LOG_ALERT("unexpected empty ID.");
            return;
        }

        object->play_pickup_sound();

        objects.erase(object_id);
        objects.emplace(std::move(object_id), std::move(object));
    }
}

extern const std::string ID_TREE_DELIM = "::";

void LevelPlayer::reveal(const ID& id_tree, const bool move_camera_to_object)
{
    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto object = get_object(id_pair.first);
    if (!object)
    {
        LOG_ALERT("object not found: " + id_pair.first);
        return;
    }
    set_topmost_object(object);
    if (move_camera_to_object)
    {
        camera.set_center_progressively(object->get_center(), CAMERA_SLIDE_DURATION);
        set_active_object(object);
        unclasp();
    }
    if (!object->is_visible())
        object->set_visible(true);

    if (id_pair.second.has_value())
        object->reveal(id_pair.second.value());
}

void LevelPlayer::hide(const ID& id_tree, const bool move_camera_to_object)
{
    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto object = get_object(id_pair.first);
    if (!object)
    {
        LOG_ALERT("object not found: " + id_pair.first);
        return;
    }

    if (move_camera_to_object)
    {
        set_topmost_object(object);
        camera.set_center_progressively(object->get_center(), CAMERA_SLIDE_DURATION);
        unclasp();
    }
    if (id_pair.second.has_value())
        object->hide(id_pair.second.value());
    else
    {
        if (object == hovered_object)
        {
            hovered_object->set_hovered(false);
            hovered_object.reset();
        }
        if (object == active_object)
        {
            active_object->set_active(false);
            active_object.reset();
        }

        if (object->is_visible())
            object->set_visible(false);
    }
}

void LevelPlayer::set_locked(const ID& id_tree, const bool locked)
{
    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto object = get_object(id_pair.first);
    if (!object)
    {
        LOG_ALERT("object not found: " + id_pair.first);
        return;
    }

    if (id_pair.second.has_value())
        object->set_locked(id_pair.second.value(), locked);
    else
        LOG_ALERT("invalid id-tree: " + id_tree);
}

void LevelPlayer::set_active_object(std::shared_ptr<Object> object)
{
    set_topmost_object(object);

    if (object == active_object)
        return;

    if (active_object)
    {
        active_object->set_active(false);
        previous_object = active_object;
    }

    active_object = std::move(object);

    if (active_object)
        active_object->set_active(true);
}

void LevelPlayer::set_hovered_object(std::shared_ptr<Object> object)
{
    if (object == hovered_object)
        return;

    if (hovered_object)
        hovered_object->set_hovered(false);

    hovered_object = std::move(object);

    if (hovered_object)
        hovered_object->set_hovered(true);
}

void LevelPlayer::clasp(std::shared_ptr<Object> object)
{
    if (object)
    {
        set_topmost_object(object);

        clasped_object = std::move(object);
        clasped_object_initial_center = clasped_object->get_center();
        camera_center_after_clasp = camera.get_center();
        clasp_duration = 0.f;

        crosshair.clasp(clasped_object);

        clasped_object->play_pickup_sound();
    }
}

void LevelPlayer::unclasp()
{
    if (clasped_object && clasp_duration >= OBJECT_MINIMUM_PICKUP_DURATION)
        clasped_object->play_release_sound();
    clasped_object.reset();
    crosshair.unclasp();
}

void LevelPlayer::mouse_grab(std::shared_ptr<Object> object)
{
    if (object)
    {
        set_topmost_object(object);

        mouse_grabbed_object = std::move(object);
        mouse_grabbed_object_initial_center = mouse_grabbed_object->get_center();
        mouse_grab_duration = 0.f;

        mouse_grabbed_object->play_pickup_sound();
    }
}

void LevelPlayer::mouse_ungrab()
{
    if (mouse_grabbed_object && mouse_grab_duration >= OBJECT_MINIMUM_PICKUP_DURATION)
        mouse_grabbed_object->play_release_sound();
    mouse_grabbed_object.reset();
}

void LevelPlayer::clear_objects()
{
    objects.clear();
    active_object.reset();
    hovered_object.reset();
    previous_object.reset();

    unclasp();
    mouse_ungrab();
}

void LevelPlayer::order_objects(const std::vector<ID>& order)
{
    for (const auto& id : order)
    {
        std::shared_ptr<Object> object = get_object(id);
        if (object)
        {
            objects.erase(id);
            objects.emplace(id, std::move(object));
        }
    }
}

void LevelPlayer::scale_and_position_overlays()
{
    const float tlc_overlay_scale = GUI_view.getSize().y /
                                    static_cast<Px>(tlc_overlay_texture.get().getSize().y);
    const float brc_overlay_scale = GUI_view.getSize().y /
                                    static_cast<Px>(brc_overlay_texture.get().getSize().y);
    tlc_overlay.setScale(tlc_overlay_scale, tlc_overlay_scale);
    brc_overlay.setScale(brc_overlay_scale, brc_overlay_scale);

    brc_overlay.setPosition(GUI_view.getSize());
}

void LevelPlayer::insert_user_list_into_menu_level()
{
    // Note that this is the only hard-coded relationship the engine has with a level:
    // The engine isn't meant to intervene in routine level interpretation,
    // and as such lacks elegant means to do so.
    // This is an ad-hoc hack in order to update the user_list within the menu.

    const std::shared_ptr<Object> object = get_object("USER_MANAGER");
    if (!object)
    {
        LOG_ALERT("USER_MANAGER object not found.");
        return;
    }

    const std::shared_ptr<Element> user_list = object->get_element("USER_LIST");
    if (!user_list || user_list->type != Element::Type::Text)
    {
        LOG_ALERT("invalid USER_MANAGER object; USER_LIST element not found or invalid."); 
        return;
    }
    dynamic_cast<Text&>(*user_list).set_string(
        EARManager::instance().request(Request::UserList).as<ID>());
}

void LevelPlayer::on_event(const Event event, const Data& data)
{
    if (event == Event::RevealAllObjects)
    {
        for (auto& [id, object] : objects)
            if (!object->is_visible())
                object->set_visible(true);
    }

    else if (event == Event::SetCrosshair)
        crosshair.set_type(static_cast<Indicator::Type>(data.as<int>()));

    else if (event == Event::AdvanceObjective)
    {
        const ID objective_id = data.as<ID>();
        if (contains(objectives, objective_id))
            objectives.at(objective_id).advance();
        else
            LOG_ALERT("objective ID not found: " + objective_id);
    }

    else if (event == Event::Hide)
        hide(data.as<std::string>(), false);
    else if (event == Event::HideMoveCamera)
        hide(data.as<std::string>(), true);

    else if (event == Event::Reveal)
        reveal(data.as<std::string>(), true);
    else if (event == Event::RevealDoNotMoveCamera)
        reveal(data.as<std::string>(), false);

    else if (event == Event::Unlock)
        set_locked(data.as<std::string>(), false);
    else if (event == Event::Lock)
        set_locked(data.as<std::string>(), true);

    else if (event == Event::PlayAudio)
    {
        auto pair = str_split(data.as<std::string>(), ",");

        const std::string path = pair.first;
        Seconds loudness = 1.f;
        if (pair.second)
            loudness = Convert::str_to<float>(pair.second.value());

        auto it = audio_data.known_sound_paths.find(path);
        if (it != audio_data.known_sound_paths.end())
            AudioPlayer::instance().play(it->second, loudness);
        else
            LOG_ALERT("unknown sound: " + path + "\nsound paths must be specified in the audio node.");
    }

    else if (event == Event::StreamAudio)
    {
        auto pair = str_split(data.as<std::string>(), ",");

        const std::string path = pair.first;
        Seconds loudness = 1.f;
        if (pair.second)
            loudness = Convert::str_to<float>(pair.second.value());
        AudioPlayer::instance().stream(path, loudness);
    }
    
    else if (event == Event::StopStream)
        AudioPlayer::instance().stop(data.as<std::string>());

    else if (event == Event::SetLightShader)
        light.set_shader(data.as<std::string>());

    // All of the following commands share the 'progression_duration' parameter.
    else if (event == Event::SetCameraCenter ||
             event == Event::ZoomIn ||
             event == Event::ZoomOut ||
             event == Event::SetLightSource ||
             event == Event::SetLightRadius ||
             event == Event::SetLightBrightness ||
             event == Event::SetLightSwing ||
             event == Event::SetLightOn)
    {
        std::stringstream buffer{ data.as<std::string>() };

        Seconds progression_duration = 0.f;
        char comma;
        if (event == Event::SetCameraCenter)
        {
            PxVec2 center;
            buffer >> center >> comma >> progression_duration;
            camera.set_center_progressively(center, progression_duration);
        }
        else if (event == Event::ZoomIn || event == Event::ZoomOut)
        {
            buffer >> progression_duration;
            camera.set_zoom_progressively(
                event == Event::ZoomIn ? Camera::Zoom::In : Camera::Zoom::Out,
                progression_duration);
        }
        else if (event == Event::SetLightSource)
        {
            PxVec2 source;
            buffer >> source >> comma >> progression_duration;
            light.set_source(source, progression_duration);
        }
        else if (event == Event::SetLightRadius)
        {
            Px radius;
            buffer >> radius >> comma >> progression_duration;
            light.set_radius(radius, progression_duration);
        }
        else if (event == Event::SetLightBrightness)
        {
            float brightness;
            buffer >> brightness >> comma >> progression_duration;
            light.set_brightness(brightness, progression_duration);
        }
        else if (event == Event::SetLightSwing)
        {
            Px swing;
            buffer >> swing >> comma >> progression_duration;
            light.set_swing(swing, progression_duration);
        }
        else if (event == Event::SetLightOn)
        {
            auto pair = str_split(data.as<std::string>(), ",");
            light.set_on(Convert::str_to<bool>(pair.first),
                         pair.second ? Convert::str_to<Seconds>(pair.second.value()) : 0.f);
        }
    }

    else if (event == Event::UserListUpdated)
    {
        if (loaded_level_path == LevelPaths::MAIN_MENU)
            insert_user_list_into_menu_level();
    }
}

bool LevelPlayer::initialize(const YAML::Node& node)
{
    // Menu-bar data:

    if (!menu_bar_data.initialize(node["bar"]))
        return false;

    // Audio data:

    if (!audio_data.initialize(node["audio"]))
        return false;
    AudioPlayer::instance().set_playlist(audio_data.playlist,
                                         audio_data.playlist_shuffle,
                                         audio_data.playlist_interval,
                                         audio_data.playlist_loudness);

    // Table:

    if (!table.initialize(node["table"]))
        return false;

    // Light:

    if (!light.initialize(node["light"]))
        return false;

    // Camera:

    if (!camera.initialize(node["camera"]))
        return false;
    camera.set_central_bounds(table.get_bounds());

    // Objectives:

    YAML::Node objectives_node = node["objectives"];
    if (objectives_node.IsDefined() && objectives_node.IsMap())
    {
        for (auto objective_node : objectives_node)
        {
            ID id;
            try
            {
                id = objective_node.first.as<ID>();
            }
            catch (const YAML::Exception& e)
            {
                LOG_ALERT("invalid objective node; key exception: " + e.msg +
                          "\nDUMP:\n" + YAML::Dump(objective_node));
                return false;
            }

            if (contains(objectives, id))
            {
                LOG_ALERT("objective ID is not unique: " + id);
                return false;
            }

            Objective objective;
            if (!objective.initialize(objective_node.second))
                return false;
            objectives.emplace(std::move(id), std::move(objective));
        }
    }

    // Objects:

    YAML::Node objects_node = node["objects"];
    if (objects_node.IsDefined() && objects_node.IsMap())
    {
        for (auto object_node : objects_node)
        {
            ID id;
            try
            {
                id = object_node.first.as<ID>();
            }
            catch (const YAML::Exception& e)
            {
                LOG_ALERT("invalid object node; key exception: " + e.msg +
                          "\nDUMP:\n" + YAML::Dump(object_node));
                return false;
            }

            if (contains(objects, id))
            {
                LOG_ALERT("object ID is not unique: " + id);
                return false;
            }

            std::shared_ptr<Object> object = create_object(object_node.second);
            if (!object)
            {
                LOG_ALERT("invalid object will be skipped: " + id);
                continue;
            }

            table.assure_contains(*object);
            objects.emplace(std::move(id), std::move(object));
        }
    }

    // Overlays:

    YAML::Node tlc_overlay_node = node["tlc_overlay"];
    tlc_overlay_texture.load(tlc_overlay_node.IsDefined() ?
                             tlc_overlay_node.as<std::string>() : DEFAULT_TLC_OVERLAY_TEXTURE_PATH);

    YAML::Node brc_overlay_node = node["brc_overlay"];
    brc_overlay_texture.load(brc_overlay_node.IsDefined() ?
                             brc_overlay_node.as<std::string>() : DEFAULT_BRC_OVERLAY_TEXTURE_PATH);

    tlc_overlay.setTexture(tlc_overlay_texture.get(), true);

    brc_overlay.setTexture(brc_overlay_texture.get(), true);
    brc_overlay.setOrigin(static_cast<float>(brc_overlay_texture.get().getSize().x),
                          static_cast<float>(brc_overlay_texture.get().getSize().y));
    scale_and_position_overlays();

    return true;
}

YAML::Node LevelPlayer::serialize_dynamic_data() const
{
    YAML::Node objectives_node{ YAML::NodeType::Map };
    for (const auto& [id, objective] : objectives)
        objectives_node[id] = objective.serialize_dynamic_data();

    YAML::Node objects_node{ YAML::NodeType::Map };
    for (const auto& [id, object] : objects)
        objects_node[id] = object->serialize_dynamic_data();

    YAML::Node node;
    node["light"]      = light.serialize_dynamic_data();
    node["camera"]     = camera.serialize_dynamic_data();
    node["objectives"] = objectives_node;
    node["objects"]    = objects_node;
    return node;
}