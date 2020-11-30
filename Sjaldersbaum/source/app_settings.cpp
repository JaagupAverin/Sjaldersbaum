#include "app_settings.h"

#include <fstream>

#include "yaml.h"
#include "convert.h"
#include "logger.h"

/*------------------------------------------------------------------------------------------------*/

// Default width and height are handled in app.cpp. 0 is simply an indicator of defaultness.
constexpr unsigned int DEFAULT_WINDOW_WIDTH  = 0u;
constexpr unsigned int DEFAULT_WINDOW_HEIGHT = 0u;
constexpr int  DEFAULT_FPS_CAP    = 0;
constexpr bool DEFAULT_VSYNC      = true;
constexpr bool DEFAULT_FULLSCREEN = false;
constexpr int  DEFAULT_VOLUME     = 50;
constexpr bool DEFAULT_DEBUG_MODE = false;

/*------------------------------------------------------------------------------------------------*/

AppSettings::AppSettings() :
    window_width { DEFAULT_WINDOW_WIDTH },
    window_height{ DEFAULT_WINDOW_HEIGHT },
    fps_cap      { DEFAULT_FPS_CAP },
    vsync        { DEFAULT_VSYNC },
    fullscreen   { DEFAULT_FULLSCREEN },
    volume       { DEFAULT_VOLUME },
    debug        { DEFAULT_DEBUG_MODE }
{

}

void AppSettings::load_from_file(const std::string& path)
{
    try
    {
        std::ifstream settings_file{ path };
        if (!settings_file)
        {
            LOG_ALERT("settings file could not be opened.");
            return;
        }

        std::stringstream buffer;
        buffer << settings_file.rdbuf();
        const std::string settings_data = buffer.str();

        const YAML::Node node = YAML::Load(settings_data);

        LOG_INTEL("DUMP:\n" + settings_data + "\n\nfrom: " + path);

        for (const auto& setting_node : node)
        {
            const std::string key  = setting_node.first.as<std::string>();
            const YAML::Node value = setting_node.second;

            if (key == "window_width")
                window_width = value.as<unsigned int>();

            else if (key == "window_height")
                window_height = value.as<unsigned int>();

            else if (key == "fps_cap")
                fps_cap = value.as<int>();

            else if (key == "vsync")
                vsync = value.as<bool>();

            else if (key == "fullscreen")
                fullscreen = value.as<bool>();

            else if (key == "volume")
                volume = value.as<int>();

            else if (key == "debug")
                debug = value.as<bool>();
        }
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during settings deserialization:\n" + e.msg +
                  "\npath: " + path);
        return;
    }
}

void AppSettings::save_to_file(const std::string& path) const
{
    std::stringstream buffer;
    try
    {
        YAML::Node node;
        node["window_width"]  = window_width;
        node["window_height"] = window_height;
        node["fps_cap"]       = fps_cap;
        node["vsync"]         = vsync;
        node["fullscreen"]    = fullscreen;
        node["volume"]        = volume;
        node["debug"]         = debug;
        buffer << YAML::Dump(node);
    }
    catch (const YAML::Exception& e)
    {
        LOG_ALERT("unknown YAML exception during settings serialization:\n" + e.msg);
        return;
    }

    std::ofstream file;
    file.open(path, std::ios_base::out | std::ios_base::trunc);
    if (!file)
    {
        LOG_ALERT("settings file could not be opened for writing;\npath: " + path);
        return;
    }
    file << buffer.rdbuf();
}