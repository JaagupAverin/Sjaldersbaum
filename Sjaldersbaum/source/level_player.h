#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include <tsl/ordered_map.h>

#include "yaml.h"
#include "events-requests.h"
#include "table.h"
#include "light.h"
#include "objects.h"
#include "keyboard.h"
#include "particles.h"
#include "mouse.h"
#include "crosshair.h"
#include "camera.h"
#include "resources.h"

/*------------------------------------------------------------------------------------------------*/

struct MenuBarData : public YAML::Serializable
{
    std::string title;
    std::string command_sequence;
    std::string description;
    std::string sound_path;

    // Expects a map that consists of:
    // ======================================================================
    // * title:       <std::string> = "Untitled"
    // * command:     <std::string> = "menu"
    // * description: <std::string> = "Hold Escape to return to menu-level."
    // * sound:       <std::string> = <NONE>
    // ======================================================================
    bool initialize(const YAML::Node& node) override;
};

/*------------------------------------------------------------------------------------------------*/

struct AudioData : public YAML::Serializable
{
    std::unordered_map<std::string, SoundID> known_sound_paths;
    std::vector<std::string> playlist;
    bool playlist_shuffle;
    Seconds playlist_interval;
    float playlist_loudness;

    // Expects a map that consists of:
    // =========================================
    // * sounds:    seq<std::string> = []
    // * playlist:  seq<std::string> = []
    // ==ADVANCED===============================
    // * pl_shuffle:  <bool>    = false
    // * pl_interval: <Seconds> = 0
    // * pl_loudness: <float>   = 1
    // =========================================
    bool initialize(const YAML::Node& node) override;
};

/*------------------------------------------------------------------------------------------------*/

// Keeps track of incremental progress;
// compares it to a target, and executed a command_sequence if that target is reached.
class Objective : public YAML::Serializable
{
public:
    Objective();

    // Increments the progress counter; if the target is reached, executes action.
    // Note that the action can only be executed once - superfluous advancements are ignored.
    void advance();

    // Expects a map that consists of:
    // ==========================
    // * command:  <std::string>
    // * target:   <int>
    // * progress: <int> = 0
    // ==========================
    bool initialize(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ==================
    // * progress: <int>
    // ==================
    YAML::Node serialize_dynamic_data() const override;

private:
    int progress;
    int target;
    std::string command_sequence;
};

/*------------------------------------------------------------------------------------------------*/

// Loads, plays and saves level files.
class LevelPlayer : Observer, YAML::Serializable
{
public:
    LevelPlayer();

    void update_keyboard_input(const Keyboard& keyboard);
    void update_mouse_input(const Mouse& mouse);
    void update(Seconds elapsed_time);

    void set_light_on(bool on, Seconds transition_duration, bool sound = true);

    void set_resolution(PxVec2 resolution);

    bool load(const std::string& level_path, const std::string& save_path = "");
    bool save(const std::string& save_path) const;

    void render(sf::RenderTarget& target);

    const MenuBarData& get_menu_bar_data() const;

    std::string get_loaded_level_path() const;

    bool has_level_loaded() const;

    void initialize_debug_components();
    void toggle_debug_mode();

private:
    void update_indicator_input();

    // Returns empty string if Object is not found.
    ID get_object_id(std::shared_ptr<Object> object) const;

    // Returns nullptr if ID does not correspond to an Object.
    std::shared_ptr<Object> get_object(const ID& object_id);

    // Returns nullptr if no Object is found at specified position.
    std::shared_ptr<Object> get_topmost_visible_object(PxVec2 position);

    void set_topmost_object(std::shared_ptr<Object> object);

    // ID-tree format: "object_id::entity_id"
    // Revealing an Entity will also reveal the Object containing it.
    void reveal(const ID& id_tree, bool move_camera_to_object = true);
    void hide(const ID& id_tree, bool move_camera_to_object = false);

    // ID-tree format: "object_id::entity_id". Locking is only valid for certain Elements.
    void set_locked(const ID& id_tree, bool locked);

    void set_active_object(std::shared_ptr<Object> object);
    void set_hovered_object(std::shared_ptr<Object> object);

    void clasp(std::shared_ptr<Object> object);
    void unclasp();

    void mouse_grab(std::shared_ptr<Object> object);
    void mouse_ungrab();

    void clear_objects();

    // Like in the objects map, topmost Elements are at the end of the container.
    void order_objects(const std::vector<ID>& order);

    void scale_and_position_overlays();

    void insert_user_list_into_menu_level();

    void on_event(Event event, const Data& data) override;

    // Expects a map that may include:
    // ======================================
    // * bar:         <MenuBarData>
    // * audio:       <AudioData>
    // * table:       <Table>
    // * light:       <Light>
    // * camera:      <Camera>
    // * objectives:  map<ID, Objective>
    // * objects:     map<ID, Object>
    // * tlc_overlay: <std::string>
    // * brc_overlay: <std::string>
    // ======================================
    // All components will have some default value if unspecified.
    bool initialize(const YAML::Node& root_node) override;

    // Returns a map that consists of:
    // =================================
    // * bar:        <MenuBarData>
    // * light:      <Light>
    // * camera:     <Camera>
    // * objectives: map<ID, Objective>
    // * objects:    map<ID, Object>
    // =================================
    YAML::Node serialize_dynamic_data() const override;

private:
    MenuBarData menu_bar_data;
    AudioData audio_data;

    Table table;
    Light light;

    std::unordered_map<ID, Objective> objectives;

    tsl::ordered_map<ID, std::shared_ptr<Object>> objects;

    std::shared_ptr<Object> active_object;
    std::shared_ptr<Object> hovered_object;
    std::shared_ptr<Object> previous_object;

    // Keyboard related:
    std::shared_ptr<Object> clasped_object;
    PxVec2 clasped_object_initial_center;
    PxVec2 camera_center_after_clasp;
    Seconds clasp_cooldown;
    Seconds tab_cooldown;
    Seconds interaction_key_lag;
    Seconds clasp_duration;

    // Mouse related:
    std::shared_ptr<Object> mouse_grabbed_object;
    PxVec2 mouse_grabbed_object_initial_center;
    PxVec2 mouse_pos_after_grab;
    Seconds mouse_grab_duration;

    Indicator indicator;
    ParticleSystem indicator_particles;

    TextureReference tlc_overlay_texture;
    sf::Sprite       tlc_overlay;
    TextureReference brc_overlay_texture;
    sf::Sprite       brc_overlay;

    Camera camera;
    Crosshair crosshair;

    sf::View GUI_view;

    sf::RenderTexture base_canvas;
    sf::RenderTexture final_canvas;
    sf::Sprite        final_sprite;

    std::string loaded_level_path;
    bool level_loaded;

    FontReference debug_font;
    bool debug_components_initialized;
    bool debug_mode;
};