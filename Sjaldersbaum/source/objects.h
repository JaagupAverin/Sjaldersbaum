#pragma once

#include <memory>
#include <tsl/ordered_map.h>
#include <optional>

#include "elements.h"
#include "keyboard.h"
#include "mouse.h"
#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

class Object : public Entity
{
public:
    enum class Type
    {
        Unimplemented,

        Sheet,
        Binder
    };
    Type type;

    Object(const EntityConfig& config, Type type);
    virtual std::shared_ptr<Element> get_element(const ID& id) = 0;
    virtual void reveal(const ID& id) = 0;
    virtual void hide(const ID& id) = 0;
    virtual void set_locked(const ID& id, bool locked) = 0;
    virtual bool contains(const PxVec2 point) = 0;
    virtual void play_pickup_sound() = 0;
    virtual void play_release_sound() = 0;

    virtual void render_debug_bounds(sf::RenderTarget& target) const = 0;
};

// Expects a map that includes:
// =========================================
// * type: <Object::Type> = Sheet
// =========================================
// Types: [sheet, binder]
std::shared_ptr<Object> create_object(const YAML::Node& node);

/*------------------------------------------------------------------------------------------------*/

// Container of Elements.
class Sheet : public Object
{
public:
    Sheet(bool independent);

    void update_keyboard_input(const Keyboard& keyboard) override;
    void update_indicator_input(const Indicator& indicator) override;
    void update(Seconds elapsed_time) override;

    // Returns nullptr if ID does not correspond to an Element.
    std::shared_ptr<Element> get_element(const ID& id) override;
    void reveal(const ID& id) override;
    void hide(const ID& id) override;
    void set_locked(const ID& id_tree, bool locked) override;

    // Returns true if the background (opaque pixels only) contains point;
    // otherwise returns false.
    bool contains(const PxVec2 point) override;

    void play_pickup_sound() override;
    void play_release_sound() override;

    void render_debug_bounds(sf::RenderTarget& target) const override;

private:
    // Returns Elements at specified position.
    // If no Elements are found, returns a vector of one element, which is a nullptr.
    std::vector<std::shared_ptr<Element>> get_elements(PxVec2 position, bool activatable_and_visible_only);

    ID get_element_id(const std::shared_ptr<Element> element) const;

    void set_active_element(std::shared_ptr<Element> element);
    void set_hovered_element(std::shared_ptr<Element> element);

    void position_elements();

    // Opacity chunkmap is a matrix of booleans,
    // roughly representing the alpha values of the background.
    // Also used to generate the highlight texture.
    void create_opacity_chunkmap_and_highlight();

    void on_reposition() override;
    void on_setting_visible() override;
    void on_setting_hovered() override;
    void on_setting_active() override;

    // Expects a map that includes:
    // ======================================================
    // * size:         <PxVec2>         = (500, 500)
    // * texture:      <std::string>    = <SQUARE_GRID>
    // * elements:     map<ID, Element> = {}
    // ==ADVANCED============================================
    // * texture_flip:  <bool>        = false
    // * pickup_sound:  <std::string> = <PAPER_PICKUPS>
    // * release_sound: <std::string> = <PAPER_RELEASE>
    // ======================================================
    // Note that Sheets can be independent (defined in 'objects'),
    // or they can belong to a Binder (defined within the said Binder),
    // defined within said Binder. See Binder's initialization method.
    bool on_initialization(const YAML::Node& node) override;

    // Returns a map that consists of:
    // ================================
    // * elements: map<ID,Element>
    // ================================
    YAML::Node on_dynamic_data_serialization() const override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::Texture highlight_texture;
    Highlight highlight;

    SoundID pickup_sound;
    SoundID release_sound;

    TextureReference texture;
    sf::Sprite background;
    std::vector<std::vector<bool>> opacity_chunkmap;
    bool horizontal_flip;

    tsl::ordered_map<ID, std::shared_ptr<Element>> elements;
    std::unordered_map<ID, PxVec2> local_element_positions;
    std::shared_ptr<Element> active_element;
    std::shared_ptr<Element> hovered_element;
    std::vector<std::shared_ptr<Element>> all_hovered_elements;

    sf::Shader alpha_shader;
    ProgressiveFloat opacity;
    bool independent; // Indicates whether the Sheet belongs to a Binder or not.
};

/*------------------------------------------------------------------------------------------------*/

// Container of Sheets.
class Binder : public Object
{
public:
    Binder();

    void update_keyboard_input(const Keyboard& keyboard) override;
    void update_indicator_input(const Indicator& indicator) override;
    void update(Seconds elapsed_time) override;

    void set_active_sheet(const ID& sheet_id);

    // ID-tree format: "sheet_id::element_id"
    // Returns nullptr if ID does not correspond to a Sheet/Element.
    std::shared_ptr<Element> get_element(const ID& id_tree) override;
    void reveal(const ID& id_tree) override;
    void hide(const ID& id_tree) override;
    void set_locked(const ID& id, bool locked) override;

    // Returns true if the active Sheet's background (opaque pixels only) contains point;
    // otherwise returns false.
    bool contains(const PxVec2 point) override;

    void play_pickup_sound() override;
    void play_release_sound() override;

    void render_debug_bounds(sf::RenderTarget& target) const override;

private:
    void set_next_sheet();

    std::shared_ptr<Sheet> get_sheet(const ID& sheet_id);
    const std::shared_ptr<Sheet> get_sheet(const ID& sheet_id) const;

    void on_reposition() override;
    void on_setting_visible() override;
    void on_setting_hovered() override;
    void on_setting_active() override;

    // Expects a map that includes:
    // ===================================================
    // * size:         <PxVec2>
    // * sheets:       map<ID, Sheet>
    // * active_sheet: <ID>           = <FIRST_SHEET>
    // ===================================================
    // For Sheets defined within the Binder, 'size', 'position' and
    // 'visible' nodes are redundant, as they're determined by the Binder.
    // Also, the default texture will instead be picked as follows:
    // 1) <PLACEHOLDER>                   for the first sheet of a Binder;
    // 2) <FIRST SHEET'S FLIPPED TEXTURE> for the last sheet of a Binder;
    // 3) <FIRST SHEET'S TEXTURE>         for all other sheets of a Binder.
    bool on_initialization(const YAML::Node& node) override;

    // Returns a map that consists of:
    // =========================================
    // * sheets:       map<ID, Sheet>
    // * active_sheet: <ID>
    // =========================================
    YAML::Node on_dynamic_data_serialization() const override;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    tsl::ordered_map<ID, std::shared_ptr<Sheet>> sheets;
    std::shared_ptr<Sheet> active_sheet;
    std::string active_sheet_id;
    Seconds sheet_turn_cooldown;
};