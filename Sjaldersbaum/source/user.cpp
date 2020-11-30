#include "user.h"

#include <fstream>
#include <filesystem>

#include "yaml.h"
#include "events-requests.h"
#include "string_assist.h"
#include "level_paths.h"
#include "logger.h"

namespace fs = std::filesystem;

/*------------------------------------------------------------------------------------------------*/

const std::string USERS_DIRECTORY = "users/";

const std::string USER_FILENAME = "data.yaml";

const std::string USER_LIST_PATH = USERS_DIRECTORY + "user_list.yaml";

/*------------------------------------------------------------------------------------------------*/

// Assures the path's directory-tree exists.
// If directories already exist or are successfully created, returns true; otherwise returns false.
bool assure_directory_exists(const std::string& path)
{
    try
    {
        fs::path directory{ path };
        if (directory.has_extension())
            directory.remove_filename();

        if (fs::is_directory(directory))
            return true;
        else
        {
            fs::create_directories(directory);
            return true;
        }
    }
    catch (const std::exception& e)
    {
        LOG_ALERT("directories could not be created;\nexception: " + e.what() + "\npath: " + path);
        return false;
    }
}

/*------------------------------------------------------------------------------------------------*/

User::User() :
    guest{ true },
    time_played{ 0.f }
{
    become_guest();
}

void User::create(const ID& id)
{
    if (get_user_list().size() >= USER_LIST_SIZE)
    {
        LOG_ALERT("cannot create user; user_list full.");
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "User list full. Erase an user.");
        return;
    }

    if (id.empty())
    {
        LOG_ALERT("cannot create user with empty ID.");
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "Cannot create an user without ID.");
        return;
    }

    if (!consists_of_usernamic_characters(id))
    {
        LOG_ALERT("user ID contains unsupported characters: " + id);
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "User ID contains unsupported characters: " + id);
        return;
    }

    for (const auto& existing_id : get_user_list())
        if (get_decapitalized(existing_id) == get_decapitalized(id))
        {
            LOG_ALERT("cannot create user; ID already in use: " + id);
            EARManager::instance().queue_event(Event::DisplayMessage,
                                               "User already exists: " + id);
            return;
        }

    LOG_INTEL("creating new user: " + id);

    User new_user;
    new_user.id              = id;
    new_user.guest           = false;
    new_user.folder          = USERS_DIRECTORY + get_decapitalized(id) + '/';
    new_user.last_level_path = LevelPaths::MAIN_MENU;

    if (!fs::exists(new_user.folder))
        new_user.save();
    else
        LOG_ALERT("user folder already exists; its content will not be overwritten;\n"
                  "folder path: " + new_user.folder);

    get_user_list().emplace_back(id);
    EARManager::instance().queue_event(Event::UserListUpdated);
    EARManager::instance().queue_event(Event::DisplayMessage, "User created: " + id);
}

void User::erase(const ID& id)
{
    if (id.empty())
    {
        LOG_ALERT("cannot erase user with empty ID.");
        EARManager::instance().queue_event(Event::DisplayMessage, "Cannot erase an user without ID.");
        return;
    }

    if (!consists_of_usernamic_characters(id))
    {
        LOG_ALERT("user ID contains unsupported characters: " + id);
        EARManager::instance().queue_event(Event::DisplayMessage,
                                           "User ID contains unsupported characters: " + id);
        return;
    }

    auto& user_list = get_user_list();
    for (size_t i = 0; i != user_list.size(); ++i)
    {
        if (get_decapitalized(user_list[i]) == get_decapitalized(id))
        {
            if (i == 0)
            {
                LOG_ALERT("cannot erase active user: " + id);
                EARManager::instance().queue_event(Event::DisplayMessage, "Cannot erase active user.");
                return;
            }
            else
            {
                user_list.erase(user_list.begin() + i);
                fs::remove_all(USERS_DIRECTORY + get_decapitalized(id));
                EARManager::instance().queue_event(Event::UserListUpdated);
                EARManager::instance().queue_event(Event::DisplayMessage, "User erased: " + id);
                return;
            }
        }
    }
    LOG_ALERT("user not found: " + id);
    EARManager::instance().queue_event(Event::DisplayMessage, "User not found: " + id);
}

std::vector<ID>& User::get_user_list()
{
    static std::vector<ID> user_list;
    return user_list;
}

bool User::exists(const ID& id)
{
    bool exists = false;
    for (const auto& existing_id : get_user_list())
        if (get_decapitalized(existing_id) == get_decapitalized(id))
            exists = true;
    return exists;
}

void User::load_user_list()
{
    try
    {
        EARManager::instance().queue_event(Event::UserListUpdated);

        assure_directory_exists(USERS_DIRECTORY);

        std::ifstream user_list_file{ USER_LIST_PATH };
        if (!user_list_file)
        {
            get_user_list().clear();
            LOG_INTEL("user_list file not found.");
            return;
        }

        std::stringstream buffer;
        buffer << user_list_file.rdbuf();
        const std::string user_list_data = buffer.str();

        const YAML::Node user_list_node = YAML::Load(user_list_data);

        LOG_INTEL("DUMP:\n" + user_list_data + "\n\nfrom: " + USER_LIST_PATH);

        for (const auto& node : user_list_node)
        {
            if (get_user_list().size() >= USER_LIST_SIZE)
            {
                LOG_ALERT("user_list file includes superfluous IDs.");
                break;
            }
            get_user_list().emplace_back(node.as<ID>());
        }
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during user_list deserialization:\n" + e.msg);
        return;
    }
}

void User::save_user_list()
{
    std::stringstream buffer;
    try
    {
        YAML::Node node{ YAML::NodeType::Sequence };

        for (const auto& id : get_user_list())
            node.push_back(id);

        buffer << YAML::Dump(node);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during user_list serialization:\n" + e.msg);
        return;
    }

    std::ofstream file;
    file.open(USER_LIST_PATH, std::ios_base::out | std::ios_base::trunc);
    if (!file)
    {
        LOG_ALERT("user_list file could not be opened for writing;\npath: " + USER_LIST_PATH);
        return;
    }
    file << buffer.rdbuf();
}

void User::become_guest()
{
    id              = "Guest";
    guest           = true;
    folder          = USERS_DIRECTORY + "__guest/";
    last_level_path = LevelPaths::TUTORIAL;
}

bool User::load(const ID& id)
{
    if (id.empty())
    {
        LOG_ALERT("cannot load user with empty ID.");
        EARManager::instance().queue_event(Event::DisplayMessage, "Cannot load an user without ID.");
        return false;
    }

    if (!consists_of_usernamic_characters(id))
    {
        LOG_ALERT("user ID contains unsupported characters: " + id);
        return false;
    }

    if (get_decapitalized(this->id) == get_decapitalized(id))
    {
        LOG_ALERT("user already active: " + this->id);
        EARManager::instance().queue_event(Event::DisplayMessage, "User already active: " + this->id);
        return false;
    }

    if (!exists(id))
    {
        LOG_ALERT("user ID not found in user_list: " + id);
        EARManager::instance().queue_event(Event::DisplayMessage, "User not found: " + id);
        return false;
    }

    LOG_INTEL("loading user: " + id);

    User user;
    user.guest  = false;
    user.folder = USERS_DIRECTORY + get_decapitalized(id) + '/';

    const std::string file_path = user.folder + USER_FILENAME;

    if (!fs::exists(user.folder))
    {
        LOG_ALERT("user_folder not found;\ndirectory: " + user.folder);
        EARManager::instance().queue_event(Event::DisplayMessage, "User data corrupt for: " + id);
        return false;
    }
    if (!fs::exists(file_path))
    {
        LOG_ALERT("user_file not found;\npath: " + file_path);
        EARManager::instance().queue_event(Event::DisplayMessage, "User data corrupt for: " + id);
        return false;
    }

    try
    {
        std::ifstream user_file{ file_path };
        if (!user_file)
        {
            LOG_ALERT("user_file could not be opened.");
            EARManager::instance().queue_event(Event::DisplayMessage,
                                               "User data inacessible for: " + id);
            return false;
        }

        std::stringstream buffer;
        buffer << user_file.rdbuf();
        const std::string user_data = buffer.str();

        const YAML::Node node = YAML::Load(user_data);

        LOG_INTEL("DUMP:\n" + user_data + "\n\nfrom: " + file_path);

        const YAML::Node id_node                       = node["id"];
        const YAML::Node last_level_path_node          = node["last_level"];
        const YAML::Node time_played_node              = node["time_played"];
        const YAML::Node stored_command_sequences_node = node["stored_command_sequences"];

        // Note that we save the ID in the user_file as to preserve original capitalization,
        // which can get lost in the filesystem.
        user.id              = id_node.as<ID>();
        user.last_level_path = last_level_path_node.as<std::string>();
        user.time_played     = time_played_node.as<Seconds>();

        if (stored_command_sequences_node.IsDefined() && stored_command_sequences_node.IsSequence())
            for (auto node : stored_command_sequences_node)
                for (auto subnode : node)
                    user.stored_command_sequences.emplace_back(subnode.first.as<std::string>(),
                                                               subnode.second.as<std::string>());
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during user deserialization:\n" + e.msg +
                  "\npath: " + file_path);
        EARManager::instance().queue_event(Event::DisplayMessage, "User data corrupt for: " + id);
        return false;
    }

    save();

    *this = std::move(user);

    // Set loaded user as the active user by moving it to the start of the user_list:
    auto& user_list = get_user_list();
    for (size_t i = 0; i != user_list.size(); ++i)
        if (get_decapitalized(user_list[i]) == get_decapitalized(id) && i != 0)
        {
            std::swap(user_list[i], user_list[0]);
            EARManager::instance().queue_event(Event::UserListUpdated);
            break;
        }

    if (time_played == 0.f)
        EARManager::instance().queue_event(Event::DisplayMessage, "Welcome, " + this->id + '!');
    else
        EARManager::instance().queue_event(Event::DisplayMessage, "Welcome back, " + this->id + '!');

    return true;
}

bool User::load_active_from_drive()
{
    const auto& user_list = get_user_list();
    if (user_list.empty())
    {
        LOG_INTEL("no active user found in user_list.");
        return false;
    }

    const ID& id = user_list.at(0);
    LOG_INTEL("active user found in user_list: " + id);
    return load(id);
}

void User::save() const
{
    if (guest)
        return;

    assure_directory_exists(USERS_DIRECTORY);

    LOG_INTEL("saving user: " + id);

    std::stringstream buffer;
    try
    {
        YAML::Node stored_command_sequences_node{ YAML::NodeType::Sequence };
        for (const auto& [level_path, command_sequence] : stored_command_sequences)
        {
            YAML::Node node{ YAML::NodeType::Map };
            node[level_path] = command_sequence;
            stored_command_sequences_node.push_back(node);
        }

        YAML::Node node;
        node["id"]                       = id;
        node["last_level"]               = last_level_path;
        node["time_played"]              = time_played;
        node["stored_command_sequences"] = stored_command_sequences_node;
        buffer << YAML::Dump(node);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during user serialization:\n" + e.msg);
        return;
    }

    if (!(fs::exists(folder) && fs::is_directory(folder)))
    {
        try
        {
            fs::create_directory(folder);
        }
        catch (const std::exception& e)
        {
            LOG_ALERT("user_folder could not be created;\n"
                      "exception: " + e.what() + "\ndirectory: " + folder);
            return;
        }
    }

    std::string path = folder + USER_FILENAME;

    std::ofstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc);
    if (!file)
    {
        LOG_ALERT("user_file could not be opened for writing.\npath: " + path);
        return;
    }
    file << buffer.rdbuf();
}

bool User::has_save_for_level(const std::string& level_path) const
{
    if (!consists_of_systemic_characters(level_path))
    {
        LOG_ALERT("path contains unsupported characters:\n" + level_path);
        return false;
    }

    return fs::exists(folder + level_path);
}

std::string User::get_save_path_for_level(const std::string& level_path) const
{
    if (!consists_of_systemic_characters(level_path))
    {
        LOG_ALERT("path contains unsupported characters:\n" + level_path);
        return std::string{};
    }

    if (folder.empty())
    {
        // Failsafe - folder should never be empty, but the ramifications of an empty folder can be
        // very nasty, so we want to be double sure.
        LOG_ALERT("unexpected behaviour: user folder not initialized; returning empty string.");
        return std::string{};
    }

    const std::string save_path = folder + level_path;
    if (!assure_directory_exists(save_path))
        return std::string{};
    return save_path;
}

const ID& User::get_id() const
{
    return id;
}

bool User::is_guest() const
{
    return guest;
}