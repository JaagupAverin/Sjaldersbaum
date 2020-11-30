#pragma once

#include <string>

/*------------------------------------------------------------------------------------------------*/

// Structure for loading, storing and saving application settings.
struct AppSettings
{
    AppSettings();

    void load_from_file(const std::string& path);

    void save_to_file(const std::string& path) const;

public:
    unsigned int window_width;
    unsigned int window_height;
    int fps_cap;
    bool vsync;
    bool fullscreen;
    int volume;
    bool debug;
};