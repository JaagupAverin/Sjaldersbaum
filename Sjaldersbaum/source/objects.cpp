#include "objects.h"

#include "logger.h"
#include "convert.h"
#include "maths.h"
#include "string_assist.h"
#include "colors.h"

/*------------------------------------------------------------------------------------------------*/

constexpr int OPACITY_CHUNK_SIZE = 10;

constexpr Px      HIGHLIGHT_TILE_SIZE     = 40.f;
constexpr int     HIGHLIGHT_TILE_VERSIONS = 10;
const std::string HIGHLIGHT_TILES_PATH    = "resources/textures/system/object_highlight_tiles.png";

constexpr Seconds OPACITY_PROGRESSION_DURATION = 0.25f;

const std::string ALPHA_SHADER_PATH = "resources/shaders/alpha.vert";

extern const std::string ID_TREE_DELIM;

const std::string SQUARE_GRID_TEXTURE_PATH = "resources/textures/objects/square_grid.png";
const std::string PAPER_SOUNDS_PATH = "resources/audio/sounds/paper_0.ogg";

/*------------------------------------------------------------------------------------------------*/

const std::unordered_map<std::string, Object::Type> KNOWN_OBJECT_TYPES
{
    { "sheet",      Object::Type::Sheet },
    { "binder",     Object::Type::Binder }
};

Object::Object(const EntityConfig& config, const Type type) :
    Entity(config),
    type{ type }
{

}

std::shared_ptr<Object> create_object(const YAML::Node& node)
{
    if (!node.IsDefined())
    {
        LOG_ALERT("undefined node.");
        return nullptr;
    }

    std::shared_ptr<Object> object;
    Object::Type type = Object::Type::Unimplemented;

    YAML::Node type_node = node["type"];

    try
    {
        if (type_node.IsDefined())
            type = Convert::str_to_enum(node["type"].as<std::string>(), KNOWN_OBJECT_TYPES);
        else
            type = Object::Type::Sheet;
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("Type not resolved; exception: " + e.what() + "\nDUMP: " + YAML::Dump(node));
        return nullptr;
    }

    if (type == Object::Type::Sheet)
        object = std::make_shared<Sheet>(true);
    else if (type == Object::Type::Binder)
        object = std::make_shared<Binder>();
    else
    {
        LOG_ALERT("unimplemented Object Type.");
        return nullptr;
    }

    if (!object->initialize(node))
        return nullptr;
    return object;
}

/*------------------------------------------------------------------------------------------------*/
// Sheet:

Sheet::Sheet(const bool independent) :
    Object(independent ? EntityConfigs::INDEPENDENT_SHEET : EntityConfigs::BOUND_SHEET,
           Object::Type::Sheet),
    independent{ independent },
    pickup_sound { UNINITIALIZED_SOUND },
    release_sound{ UNINITIALIZED_SOUND },
    horizontal_flip{ false },
    opacity{ 0.f }
{
    if (!alpha_shader.loadFromFile(ALPHA_SHADER_PATH, sf::Shader::Type::Vertex))
        LOG_ALERT("alpha shader could not be loaded from:\n" + ALPHA_SHADER_PATH);

    highlight.set_size_margins({ 3.f, 3.f }, { 0.f, 0.f });
}

void Sheet::update_keyboard_input(const Keyboard& keyboard)
{
    if (active_element)
        active_element->update_keyboard_input(keyboard);
}

void Sheet::update_indicator_input(const Indicator& indicator)
{
    indicator.set_type(Indicator::Type::HoveringMovable);
    if (indicator.is_interaction_key_pressed())
        set_active_element(get_elements(indicator.get_position(), true)[0u]);

    set_hovered_element(get_elements(indicator.get_position(), true)[0u]);
    if (hovered_element)
        hovered_element->update_indicator_input(indicator);

    all_hovered_elements = get_elements(indicator.get_position(), false);
}

void Sheet::update(const Seconds elapsed_time)
{
    opacity.update(elapsed_time);
    if (opacity.has_changed_since_last_check())
        alpha_shader.setUniform("alpha", opacity.get_current());

    highlight.update(elapsed_time);

    bool all_elements_idle = true;
    for (auto& [id, element] : elements)
    {
        element->update(elapsed_time);
        if (!element->is_idle())
            all_elements_idle = false;
    }

    if (!opacity.is_progressing() && highlight.is_idle() && all_elements_idle &&
        !this->is_active() && !this->is_hovered())
        this->set_idle(true);
}

std::shared_ptr<Element> Sheet::get_element(const ID& id)
{
    // In case the returned element loses its idleness, so must this:
    this->set_idle(false);

    if (::contains(elements, id))
        return elements.at(id);
    else
    {
        LOG_ALERT("element not found: " + id);
        return nullptr;
    }
}

void Sheet::reveal(const ID& id)
{
    auto element = get_element(id);
    if (!element)
    {
        LOG_ALERT("element not found: " + id);
        return;
    }
    if (!element->is_visible())
        element->set_visible(true);
}

void Sheet::hide(const ID& id)
{
    auto element = get_element(id);
    if (!element)
    {
        LOG_ALERT("element not found: " + id);
        return;
    }
    if (element->is_visible())
    {
        element->set_visible(false);
        if (element == active_element)
            set_active_element(nullptr);
    }
}

void Sheet::set_locked(const ID& id, const bool locked)
{
    auto element = get_element(id);
    if (!element)
    {
        LOG_ALERT("element not found: " + id);
        return;
    }
    if (element->type == Element::Type::Button)
        dynamic_cast<Button&>(*element).set_locked(locked);
    else if (element->type == Element::Type::InputLine)
        dynamic_cast<InputLine&>(*element).set_locked(locked);
    else
    {
        LOG_ALERT("only buttons/inputlines can be (un)locked; invalid element type for: " + id);
        return;
    }

    if (locked && element == active_element)
        set_active_element(nullptr);
}

bool Sheet::contains(const PxVec2 point)
{
    if (!this->get_bounds().contains(point))
        return false;
    else
    {
        if (opacity_chunkmap.empty())
            return false;

        sf::Vector2i local_point{
            static_cast<int>(std::floor((point.x - this->get_tlc().x) / OPACITY_CHUNK_SIZE)),
            static_cast<int>(std::floor((point.y - this->get_tlc().y) / OPACITY_CHUNK_SIZE)) };

        assure_less_than_or_equal_to(local_point.y, static_cast<int>(opacity_chunkmap.size()    - 1));
        assure_less_than_or_equal_to(local_point.x, static_cast<int>(opacity_chunkmap[0].size() - 1));

        return opacity_chunkmap[local_point.y][local_point.x];
    }
}

void Sheet::play_pickup_sound()
{
    AudioPlayer::instance().play(pickup_sound);
}

void Sheet::play_release_sound()
{
    AudioPlayer::instance().play(release_sound);
}

void Sheet::render_debug_bounds(sf::RenderTarget& target) const
{
    this->Entity::render_debug_bounds(target, Colors::RED_SEMI_TRANSPARENT);
    for (const auto& [id, element] : elements)
        element->render_debug_bounds(target, Colors::BLUE_SEMI_TRANSPARENT);

    for (size_t i = 0u; i != all_hovered_elements.size(); ++i)
    {
        auto hovered_element = all_hovered_elements[i];
        if (hovered_element)
        {
            FontReference font{ SYSTEM_FONT_PATH };
            sf::Text text;
            text.setString(get_element_id(hovered_element));
            text.setFont(font.get());
            // Ad-hoc way of assuring (in most cases) that the ID-labels won't overlap:
            text.setPosition(PxVec2(hovered_element->get_center().x + (i + 1) * 20.f,
                                    hovered_element->get_center().y + (i + 1) * 20.f));
            text.setFillColor(Colors::CYAN);
            text.setOutlineColor(Colors::BLACK);
            text.setOutlineThickness(2.f);
            text.setCharacterSize(20u);
            target.draw(text);
        }
    }
}

std::vector<std::shared_ptr<Element>> Sheet::get_elements(const PxVec2 position,
                                                          const bool activatable_and_visible_only)
{
    // In case the returned element loses its idleness, so must this:
    this->set_idle(false);

    std::vector<std::shared_ptr<Element>> res;

    for (auto& [id, element] : elements)
        if (element->get_bounds().contains(position))
        {
            if (activatable_and_visible_only)
            {
                if (element->is_activatable() && element->is_visible())
                    res.emplace_back(element);
            }
            else
                res.emplace_back(element);
        }

    if (res.empty())
        res.emplace_back(nullptr);
    return res;
}

ID Sheet::get_element_id(const std::shared_ptr<Element> element) const
{
    for (const auto& [id, mapped_element] : elements)
    {
        if (element == mapped_element)
            return id;
    }
    return "";
}

void Sheet::set_active_element(std::shared_ptr<Element> element)
{
    if (element == active_element)
        return;

    if (active_element)
        active_element->set_active(false);

    active_element = std::move(element);

    if (active_element)
        active_element->set_active(true);
}

void Sheet::set_hovered_element(std::shared_ptr<Element> element)
{
    if (element == hovered_element)
        return;

    if (hovered_element)
        hovered_element->set_hovered(false);

    hovered_element = std::move(element);

    if (hovered_element)
        hovered_element->set_hovered(true);
}

void Sheet::position_elements()
{
    for (auto& [id, element] : elements)
    {
        const PxVec2 local_position = local_element_positions.at(id);
        const PxVec2 global_position = round_hu({ this->get_tlc().x + local_position.x, 
                                                  this->get_tlc().y + local_position.y });
        element->set_position(global_position);
    }
}

void Sheet::create_opacity_chunkmap_and_highlight()
{
    constexpr static int ocs = OPACITY_CHUNK_SIZE;  // Opacity Chunk size.
    constexpr static Px  hts = HIGHLIGHT_TILE_SIZE; // Highlight Tile Size.
    constexpr static Px  m   = (hts - ocs) / 2.f;   // Margin (Padding on sides).

    const PxVec2 size = this->get_size();

    const int width  = static_cast<int>(size.x);
    const int height = static_cast<int>(size.y);

    const int opacity_chunks_x = static_cast<int>(std::ceil(size.x / ocs));
    const int opacity_chunks_y = static_cast<int>(std::ceil(size.y / ocs));

    opacity_chunkmap.resize(opacity_chunks_y, std::vector<bool>(opacity_chunks_x, false));

    sf::VertexArray highlight_tilemap{
        sf::Quads, static_cast<size_t>(opacity_chunks_x * opacity_chunks_y * 4) };

    const sf::Image image = texture.get().copyToImage();
    if (image.getSize().x == 0u || image.getSize().y == 0u)
    {
        LOG_ALERT("object texture empty; texture_path: " + texture.get_path());
        return;
    }

    const float x_scale = size.x / image.getSize().x;
    const float y_scale = size.y / image.getSize().y;

    sf::Vector2u pixel_position;
    for (int y = 0; y != opacity_chunks_y; ++y)
    {
        // Use the center-most pixel for the opacity-chunk value:
        if (y >= static_cast<int>(std::round(opacity_chunks_y / 2.f)))
            pixel_position.y = static_cast<unsigned int>(std::floor(y * ocs / y_scale));
        else
            pixel_position.y = static_cast<unsigned int>(std::ceil((y + 1.f) * ocs / y_scale));

        for (int x = 0; x != opacity_chunks_x; ++x)
        {
            // Use the center-most pixel for the opacity-chunk value:
            if (x >= static_cast<int>(std::round(opacity_chunks_x / 2.f)))
                pixel_position.x = static_cast<unsigned int>(std::floor(x * ocs / x_scale));
            else
                pixel_position.x = static_cast<unsigned int>(std::ceil((x + 1) * ocs / x_scale));

            if (image.getPixel(pixel_position.x, pixel_position.y).a != 0u)
            {
                // "local x", used for flipping the opacity matrix if needed:
                const int lx = horizontal_flip ? (opacity_chunks_x - 1 - x) : x;
                opacity_chunkmap[y][lx] = true;

                sf::Vertex* tile = &highlight_tilemap[(lx + y * opacity_chunks_x) * 4];
                const PxVec2 tc{ (lx + 0.5f) * ocs + m, (y + 0.5f) * ocs + m }; // Tile Center.
                const Px tr = hts / 2.f; // Tile "Radius".

                tile[0].position = sf::Vector2f{ tc.x - tr, tc.y - tr };
                tile[1].position = sf::Vector2f{ tc.x + tr, tc.y - tr };
                tile[2].position = sf::Vector2f{ tc.x + tr, tc.y + tr };
                tile[3].position = sf::Vector2f{ tc.x - tr, tc.y + tr };

                const int tile_version = rand(0, HIGHLIGHT_TILE_VERSIONS);
                tile[0].texCoords = sf::Vector2f{ tile_version * hts,       0.f };
                tile[1].texCoords = sf::Vector2f{ (tile_version + 1) * hts, 0.f };
                tile[2].texCoords = sf::Vector2f{ (tile_version + 1) * hts, hts };
                tile[3].texCoords = sf::Vector2f{ tile_version * hts,       hts };
            }
        }
    }

    const TextureReference tile_texture{ HIGHLIGHT_TILES_PATH };

    sf::RenderTexture highlight_canvas;
    PxVec2 highlight_size{ opacity_chunks_x * ocs + 2.f * m,
                           opacity_chunks_y * ocs + 2.f * m };
    highlight_canvas.create(static_cast<unsigned int>(highlight_size.x),
                            static_cast<unsigned int>(highlight_size.y));
    highlight_canvas.setSmooth(true);
    highlight_canvas.clear(Colors::TRANSPARENT);
    highlight_canvas.draw(highlight_tilemap, { &tile_texture.get() });
    highlight_canvas.display();
    highlight_texture = highlight_canvas.getTexture();

    highlight.set_texture(highlight_texture);
    highlight.set_base_size({ highlight_size.x - 4.f, highlight_size.y - 4.f });
}

void Sheet::on_reposition()
{
    background.setPosition(round_hu(this->get_tlc()));
    if (this->is_initialized())
    {
        highlight.set_center(this->get_center());
        position_elements();
    }
}

void Sheet::on_setting_visible()
{
    this->set_idle(false);

    opacity.set_progression_duration((independent ? 1.f : 0.75f) *
                                     (this->is_visible() ? 1.f : 2.f) *
                                     OPACITY_PROGRESSION_DURATION);

    if (this->is_visible())
        this->is_initialized() ? opacity.set_target(1.f) : opacity.set_current(1.f);
    else
        this->is_initialized() ? opacity.set_target(0.f) : opacity.set_current(0.f);
}

void Sheet::on_setting_hovered()
{
    this->set_idle(false);

    highlight.set_hovered(this->is_hovered());

    if (!this->is_hovered())
    {
        if (hovered_element)
        {
            hovered_element->set_hovered(false);
            hovered_element.reset();
        }
    }
}

void Sheet::on_setting_active()
{
    this->set_idle(false);

    highlight.set_active(this->is_active());

    if (!this->is_active())
    {
        if (active_element)
        {
            active_element->set_active(false);
            active_element.reset();
        }
    }
}

bool Sheet::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node texture_node            = node["texture"];
        const YAML::Node elements_node           = node["elements"];
        const YAML::Node texture_flip_node       = node["texture_flip"];
        const YAML::Node pickup_sound_path_node  = node["pickup_sound"];
        const YAML::Node release_sound_path_node = node["release_sound"];

        if (independent)
        {
            const YAML::Node size_node = node["size"];
            PxVec2 size;
            if (size_node.IsDefined())
            {
                size = size_node.as<PxVec2>();
                if (!(assure_bounds(size.x, 1.f, PX_LIMIT) &
                      assure_bounds(size.y, 1.f, PX_LIMIT)))
                    LOG_ALERT("invalid size had to be adjusted.");
            }
            else
                size = { 500.f, 500.f };
            this->disclose_size(size);
        } // else: size was determined by the Binder containing this Sheet.

        texture.load(texture_node.IsDefined() ? texture_node.as<std::string>() : SQUARE_GRID_TEXTURE_PATH);
        background.setTexture(texture.get());

        horizontal_flip = texture_flip_node.IsDefined() ? texture_flip_node.as<bool>() : false;
        if (horizontal_flip)
            set_horizontally_flipped(background, true);

        if (elements_node.IsDefined())
        {
            for (auto element_node : elements_node)
            {
                ID id;
                try
                {
                    id = element_node.first.as<ID>();
                }
                catch (const YAML::Exception& e)
                {
                    LOG_ALERT("invalid element node; key exception: " + e.msg +
                              "\nDUMP:\n" + YAML::Dump(element_node));
                    return false;
                }

                if (::contains(elements, id))
                {
                    LOG_ALERT("element ID is not unique: " + id);
                    return false;
                }

                std::shared_ptr<Element> element = create_element(element_node.second);
                if (!element)
                {
                    LOG_ALERT("invalid element will be skipped: " + id);
                    continue;
                }

                local_element_positions.emplace(id, round_hu(element->get_position()));
                elements.emplace(std::move(id), std::move(element));
            }
        }

        if (pickup_sound_path_node.IsDefined())
            pickup_sound = AudioPlayer::instance().load(pickup_sound_path_node.as<std::string>(), false);
        else
            pickup_sound = GlobalSounds::PAPER_PICKUPS;
        if (this->reveal_sound == GlobalSounds::GENERIC_REVEAL)
            this->reveal_sound = pickup_sound;

        if (release_sound_path_node.IsDefined())
            release_sound = AudioPlayer::instance().load(release_sound_path_node.as<std::string>(), false);
        else
            release_sound = GlobalSounds::PAPER_RELEASE;

    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "======================================================\n"
                  "* size:         <PxVec2>         = (500, 500)\n"
                  "* texture:      <std::string>    = <SQUARE_GRID>\n"
                  "* elements:     map<ID, Element> = {}\n"
                  "==ADVANCED============================================\n"
                  "* texture_flip:  <bool>        = false\n"
                  "* pickup_sound:  <std::string> = <PAPER_PICKUPS>\n"
                  "* release_sound: <std::string> = <PAPER_RELEASE>\n"
                  "======================================================\n"
                  "Note that Sheets can be independent (defined in 'objects'),\n"
                  "or they can belong to a Binder (defined within the said Binder),\n"
                  "defined within said Binder. See Binder's initialization method.\n"
                  "======================================================\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }

    ::set_size(background, this->get_size());
    highlight.set_center(this->get_center());
    create_opacity_chunkmap_and_highlight();
    position_elements();

    return true;
}

YAML::Node Sheet::on_dynamic_data_serialization() const
{
    YAML::Node node{ YAML::NodeType::Map };
    if (!elements.empty())
    {
        YAML::Node elements_node{ YAML::NodeType::Map };
        for (const auto& [id, element] : elements)
            elements_node[id] = element->serialize_dynamic_data();

        node["elements"] = elements_node;
    }
    return node;
}

void Sheet::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    if (opacity.get_current() != 0.f)
    {
        if (opacity.get_current() != 1.f)
            states.shader = &alpha_shader;

        target.draw(highlight, states);
        target.draw(background, states);

        for (const auto& [id, element] : elements)
            target.draw(*element, states);
    }
}

/*------------------------------------------------------------------------------------------------*/
// Binder

constexpr Seconds SHEET_TURN_COOLDOWN = 0.4f;

Binder::Binder() : Object(EntityConfigs::BINDER, Object::Type::Binder),
    sheet_turn_cooldown{ 0.f }
{

}

void Binder::update_keyboard_input(const Keyboard& keyboard)
{
    active_sheet->update_keyboard_input(keyboard);
}

void Binder::update_indicator_input(const Indicator& indicator)
{
    if (indicator.is_interaction_key_double_pressed() && sheet_turn_cooldown <= 0.f)
        set_next_sheet();
    else
        active_sheet->update_indicator_input(indicator);
}

void Binder::update(const Seconds elapsed_time)
{
    sheet_turn_cooldown -= elapsed_time;

    for (auto& [id, sheet] : sheets)
        if (sheet == active_sheet || !sheet->is_idle())
            sheet->update(elapsed_time);

    if (active_sheet->is_idle())
        this->set_idle(true);
}

void Binder::set_active_sheet(const ID& sheet_id)
{
    if (active_sheet && (active_sheet_id == sheet_id))
        return;

    this->set_idle(false);

    if (::contains(sheets, sheet_id))
        active_sheet_id = sheet_id;
    else
    {
        LOG_ALERT("sheet not found: " + sheet_id + "; keeping current sheet.");
        return;
    }

    if (active_sheet)
    {
        active_sheet->set_active(false);
        active_sheet->set_hovered(false);
        active_sheet->set_visible(false);
    }
    active_sheet = get_sheet(active_sheet_id);
    active_sheet->set_active(this->is_active());
    active_sheet->set_hovered(this->is_hovered());
    active_sheet->set_visible(this->is_visible());

    if (this->is_initialized())
        active_sheet->play_pickup_sound();
}

std::shared_ptr<Element> Binder::get_element(const ID& id_tree)
{
    // In case the returned element loses its idleness, so must this:
    this->set_idle(false);

    const auto id_pair = str_split(id_tree, "::");
    if (!id_pair.second.has_value())
    {
        LOG_ALERT("invalid format: " + id_tree + "\nexpected both sheet and element ids.");
        return nullptr;
    }

    auto sheet = get_sheet(id_pair.first);
    if (!sheet)
    {
        LOG_ALERT("sheet not found: " + id_pair.first);
        return nullptr;
    }
    return sheet->get_element(id_pair.second.value());
}

void Binder::reveal(const ID& id_tree)
{
    this->set_idle(false);

    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto sheet = get_sheet(id_pair.first);
    if (!sheet)
    {
        LOG_ALERT("sheet not found: " + id_pair.first);
        return;
    }

    set_active_sheet(id_pair.first);
    if (id_pair.second.has_value())
        sheet->reveal(id_pair.second.value());
}

void Binder::hide(const ID& id_tree)
{
    this->set_idle(false);

    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto sheet = get_sheet(id_pair.first);
    if (!sheet)
    {
        LOG_ALERT("sheet not found: " + id_pair.first);
        return;
    }

    if (id_pair.second.has_value())
        sheet->hide(id_pair.second.value());
}

void Binder::set_locked(const ID& id_tree, const bool locked)
{
    this->set_idle(false);

    const auto id_pair = str_split(id_tree, ID_TREE_DELIM);

    auto sheet = get_sheet(id_pair.first);
    if (!sheet)
    {
        LOG_ALERT("sheet not found: " + id_pair.first);
        return;
    }
    if (id_pair.second.has_value())
        sheet->set_locked(id_pair.second.value(), locked);
    else
        LOG_ALERT("invalid id-tree: " + id_tree);
}

void Binder::set_next_sheet()
{
    auto iter = sheets.find(active_sheet_id);
    iter = (iter == sheets.end() - 1) ? sheets.begin() : iter + 1;
    set_active_sheet(iter->first);
    sheet_turn_cooldown = SHEET_TURN_COOLDOWN;
}

std::shared_ptr<Sheet> Binder::get_sheet(const ID& sheet_id)
{
    return ::contains(sheets, sheet_id) ? sheets.at(sheet_id) : nullptr;
}

const std::shared_ptr<Sheet> Binder::get_sheet(const ID& sheet_id) const
{
    return ::contains(sheets, sheet_id) ? sheets.at(sheet_id) : nullptr;
}

bool Binder::contains(const PxVec2 point)
{
    return active_sheet->contains(point);
}

void Binder::play_pickup_sound()
{
    active_sheet->play_pickup_sound();
}

void Binder::play_release_sound()
{
    active_sheet->play_release_sound();
}

void Binder::render_debug_bounds(sf::RenderTarget& target) const
{
    this->Entity::render_debug_bounds(target, Colors::WHITE_SEMI_TRANSPARENT);
    for (auto& [id, sheet] : sheets)
        if (sheet == active_sheet || !sheet->is_idle())
            active_sheet->render_debug_bounds(target);
}

void Binder::on_reposition()
{
    for (auto& [id, sheet] : sheets)
        sheet->set_position(this->get_tlc(), Origin::TopLeftCorner);
}

void Binder::on_setting_visible()
{
    this->set_idle(false);
    if (this->is_initialized()) // active_sheet can be nullptr only during initialization
        active_sheet->set_visible(this->is_visible());
}

void Binder::on_setting_hovered()
{
    this->set_idle(false);
    if (this->is_initialized()) // active_sheet can be nullptr only during initialization
        active_sheet->set_hovered(this->is_hovered());
}

void Binder::on_setting_active()
{
    this->set_idle(false);
    if (this->is_initialized()) // active_sheet can be nullptr only during initialization
        active_sheet->set_active(this->is_active());
}

bool Binder::on_initialization(const YAML::Node& node)
{
    try
    {
        const YAML::Node size_node         = node["size"];
        const YAML::Node sheets_node       = node["sheets"];
        const YAML::Node active_sheet_node = node["active_sheet"];

        PxVec2 size = size_node.as<PxVec2>();
        if (!(assure_bounds(size.x, 1.f, PX_LIMIT) &
              assure_bounds(size.y, 1.f, PX_LIMIT)))
            LOG_ALERT("invalid size had to be adjusted.");
        this->disclose_size(size);

        active_sheet_id = active_sheet_node.IsDefined() ? active_sheet_node.as<ID>() : "";

        std::string first_sheet_texture_path = SQUARE_GRID_TEXTURE_PATH;
        size_t i = 0u;
        for (auto sheet_node : sheets_node)
        {
            // Automatic texture assignment for Sheets that don't specify their texture:
            YAML::Node local_node = sheet_node.second;
            if (local_node["texture"].IsDefined() && i == 0u)
                first_sheet_texture_path = local_node["texture"].as<std::string>();
            else
            {
                if (i == sheets_node.size() - 1u)
                {
                    local_node["texture"] = first_sheet_texture_path;
                    local_node["texture_flip"] = true;
                }
                else
                {
                    local_node["texture"] = first_sheet_texture_path;
                    local_node["texture_flip"] = false;
                }
            }

            ID id;
            try
            {
                id = sheet_node.first.as<ID>();
            }
            catch (const YAML::Exception& e)
            {
                LOG_ALERT("invalid sheet node; key exception: " + e.msg +
                          "\nDUMP:\n" + YAML::Dump(sheet_node));
                return false;
            }
            if (::contains(sheets, id))
            {
                LOG_ALERT("sheet ID is not unique: " + id);
                return false;
            }
            if (i == 0u && active_sheet_id.empty())
                active_sheet_id = id;

            auto sheet = sheets.emplace(id, std::make_shared<Sheet>(false)).first->second;
            sheet->disclose_size(this->get_size());
            sheet->set_position(this->get_tlc(), Origin::TopLeftCorner);
            sheet->set_visible(this->is_visible() && active_sheet_id == id ? true : false);
            if (!sheet->initialize(sheet_node.second))
            {
                LOG_ALERT("invalid sheet: " + id);
                return false;
            }

            ++i;
        }
        if (sheets.empty())
        {
            LOG_ALERT("no sheets specified; an empty binder is invalid.");
            return false;
        }

        set_active_sheet(active_sheet_id);
        if (!active_sheet)
        {
            LOG_ALERT("could not resolve initial active_sheet.");
            return false;
        }
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("exception: " + e.what() + '\n' +
                  "invalid node; expected a map that includes:\n"
                  "===================================================\n"
                  "* size:         <PxVec2>\n"
                  "* sheets:       map<ID, Sheet>\n"
                  "* active_sheet: <ID>           = <FIRST_SHEET>\n"
                  "===================================================\n"
                  "For Sheets defined within the Binder, 'size', 'position' and\n"
                  "'visible' nodes are redundant, as they're determined by the Binder.\n"
                  "Also, the default texture will instead be picked as follows:\n"
                  "1) <PLACEHOLDER>                   for the first sheet of a Binder;\n"
                  "2) <FIRST SHEET'S FLIPPED TEXTURE> for the last sheet of a Binder;\n"
                  "3) <FIRST SHEET'S TEXTURE>         for all other sheets of a Binder.\n"
                  "DUMP:\n" + Dump(node));
        return false;
    }
    this->reveal_sound = 0u;

    return true;
}

YAML::Node Binder::on_dynamic_data_serialization() const
{
    YAML::Node sheets_node{ YAML::NodeType::Map };
    for (const auto& [id, sheet] : sheets)
        sheets_node[id] = sheet->serialize_dynamic_data();

    YAML::Node node{ YAML::NodeType::Map };
    node["sheets"] = sheets_node;
    node["active_sheet"] = active_sheet_id;
    return node;
}

void Binder::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    for (auto& [id, sheet] : sheets)
        if (!sheet->is_idle() && active_sheet != sheet)
            target.draw(*sheet, states);
    target.draw(*active_sheet);
}