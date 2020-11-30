#pragma once

#include <string>
#include <unordered_map>
#include <iomanip>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "convert.h"
#include "logger.h"
#include "contains.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

extern bool RESOURCE_LOGGING;

/*------------------------------------------------------------------------------------------------*/
// Known types of shared resources:

template<typename T>
class ResourceManager;

using TextureManager     = ResourceManager<sf::Texture>;
using FontManager        = ResourceManager<sf::Font>;
using SoundBufferManager = ResourceManager<sf::SoundBuffer>;

/*------------------------------------------------------------------------------------------------*/

extern const Seconds RESOURCE_DESTRUCTION_INTERVAL;

// Singleton for loading/storing resources and providing shared access ("reference") to them.
template<typename T>
class ResourceManager
{
public:
    static ResourceManager& instance();

    // Progresses any unreferenced resources towards their destruction.
    void update(Seconds elapsed_time);

    // Returns a reference to a resource loaded from specified path (also loading it if necessary).
    // If the load is unsuccessful, an empty resource will still be returned.
    const T& get(const std::string& path);

    // Returns a reference to a default-constructed resource that is never destructed.
    const T& get_default();

    // Must be called whenever getting() or copying a resource loaded from this path.
    void increment_reference_count(const std::string& path);

    // Must be called whenever a resource loaded from this path is no longer being used.
    // A resource is destructed shortly after its reference count hits 0.
    void decrement_reference_count(const std::string& path);

    // Returns a string containing all loaded resources of type T, and their reference counts.
    // Specifically for the "rsrcs" command.
    std::string get_data_as_formatted_string() const;

    // For development; does not perform any checks.
    void reload_all();

private:
    std::unordered_map<std::string, T> resources;
    std::unordered_map<std::string, int> reference_counts;
    std::unordered_map<std::string, Seconds> destruction_timers;

private:
    ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;
};

/*------------------------------------------------------------------------------------------------*/

inline void update_resource_managers(const Seconds elapsed_time)
{
    TextureManager::instance().update(elapsed_time);
    FontManager::instance().update(elapsed_time);
    SoundBufferManager::instance().update(elapsed_time);
}

inline void log_all_loaded_resources()
{
    LOG("--------- TEXTURES ---------------------------------------------------\n" +
        TextureManager::instance().get_data_as_formatted_string() + '\n' +
        "--------- FONTS ------------------------------------------------------\n" +
        FontManager::instance().get_data_as_formatted_string() + '\n' +
        "--------- SOUND-BUFFERS ----------------------------------------------\n" +
        SoundBufferManager::instance().get_data_as_formatted_string());
}

/*------------------------------------------------------------------------------------------------*/
// Implementation:

template<typename T>
inline ResourceManager<T>& ResourceManager<T>::instance()
{
    static ResourceManager singleton;
    return singleton;
}

template<typename T>
inline void ResourceManager<T>::update(const Seconds elapsed_time)
{
    for (auto it = destruction_timers.begin(); it != destruction_timers.end();)
    {
        auto& timer = it->second;
        timer -= elapsed_time;

        if (timer <= 0.f)
        {
            resources.erase(it->first);
            if (RESOURCE_LOGGING)
                LOG_INTEL("UNLOADED: " + it->first);

            it = destruction_timers.erase(it);
        }
        else
            ++it;
    }
}

template<typename T>
inline const T& ResourceManager<T>::get(const std::string& path)
{
    if (path.empty())
    {
        LOG_INTEL("resource with empty path; assuming default (empty) value.");
        return get_default();
    }

    if (!contains(resources, path))
    {
        T& resource = resources.emplace(path, T()).first->second;
        if (resource.loadFromFile(path))
        {
            if (RESOURCE_LOGGING)
                LOG_INTEL("LOADED: " + path);
        }
        else
            LOG_ALERT("resource could not be loaded:\n" + path);

        if constexpr (std::is_same<T, sf::Texture>::value)
            resource.setSmooth(true);
    }

    return resources.at(path);
}

template<typename T>
inline const T& ResourceManager<T>::get_default()
{
    static T empty_resource;
    return empty_resource;
}

template<typename T>
inline void ResourceManager<T>::increment_reference_count(const std::string& path)
{
    if (path.empty())
        return;

    if (contains(reference_counts, path))
        ++reference_counts[path];
    else
    {
        reference_counts[path] = 1;
        if (contains(destruction_timers, path))
            destruction_timers.erase(path);
    }
}

template<typename T>
inline void ResourceManager<T>::decrement_reference_count(const std::string& path)
{
    if (path.empty())
        return;

    if (contains(reference_counts, path))
    {
        --reference_counts[path];
        if (reference_counts[path] == 0)
        {
            reference_counts.erase(path);
            destruction_timers.insert(std::make_pair(path, RESOURCE_DESTRUCTION_INTERVAL));
        }
    }
    else
        LOG_ALERT("cannot decrement unreferenced resource:\n" + path);
}

template<typename T>
inline std::string ResourceManager<T>::get_data_as_formatted_string() const
{
    // Note that since this method is specifically made for the DebugWindow,
    // it formats the returned string accordingly to 70 characters.

    std::stringstream buffer;

    // Used resources and their reference counts:
    for (const auto& [resource_path, reference_count] : reference_counts)
        buffer << std::setw(66) << std::left  << std::setfill('.')
               << resource_path << '|'
               << std::setw(3)  << std::right << std::setfill(' ')
               << Convert::to_str(reference_count) << '\n';

    // Resources about to be destructed:
    for (const auto& [resource_path, destruction_timer] : destruction_timers)
        buffer << std::setw(66) << std::left  << std::setfill('.')
               << resource_path << '|'
               << std::setw(3)  << std::right << "tbd\n";
               
    return buffer.str();
}
template<typename T>
inline void ResourceManager<T>::reload_all()
{
    for (auto& [path, resource] : resources)
        resource.loadFromFile(path);
}