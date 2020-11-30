#pragma once

#include <optional>

#include "units.h"

/*------------------------------------------------------------------------------------------------*/

constexpr int USER_LIST_SIZE = 3;

class User
{
public:
    User();

    // Creates an empty user.
    // Note that this does nothing if an user with specified ID already exists.
    static void create(const ID& id);

    static void erase(const ID& id);

    // Limited-size vector that contains all known user IDs, with active user at [0].
    static std::vector<ID>& get_user_list();

    static bool exists(const ID& id);

    static void load_user_list();

    static void save_user_list();

    // Becomes a temporary user that can only play the TUTORIAL level.
    void become_guest();

    // Loads data from specified user's file (automatically saves any current data).
    // If the load is successful, returns true;
    // otherwise returns false (without altering current data).
    bool load(const ID& id);

    // Loads the last active user, cross instance (ID stored on drive).
    // If an user is found and can be loaded, returns true; otherwise returns false.
    bool load_active_from_drive();

    // Saves data to the user's file (in the user_folder), creating them if necessary.
    // Note that this does nothing if the user is a Guest.
    void save() const;

    // If the level (its path) exists in the user_folder, returns true;
    // otherwise returns false.
    bool has_save_for_level(const std::string& level_path) const;

    // Prepends the user_folder to the level path and assures the directory exists.
    // Returns empty string if path is invalid or directory could not be created.
    std::string get_save_path_for_level(const std::string& level_path) const;

    const ID& get_id() const;

    bool is_guest() const;

public:
    std::string last_level_path;
    Seconds time_played;
    std::vector<std::pair<std::string, std::string>> stored_command_sequences;

private:
    ID id;
    bool guest;
    std::string folder;
};