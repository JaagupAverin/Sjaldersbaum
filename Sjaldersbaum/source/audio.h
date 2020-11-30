#pragma once

#include <list>
#include <string>
#include <SFML/Audio.hpp>

#include "progressive.h"
#include "resources.h"
#include "units.h"

/*------------------------------------------------------------------------------------------------*/

using SoundID = size_t;

constexpr SoundID UNINITIALIZED_SOUND = 360u; // 360 is a randomly chosen number and bears no importance.

namespace GlobalSounds
{
extern SoundID GENERIC_HOVER;
extern SoundID GENERIC_REVEAL;
extern SoundID INTERACTION;
extern SoundID LOCKS_HIT;
extern SoundID LIGHT_ON;
extern SoundID LIGHT_OFF;
extern SoundID PAPER_PICKUPS;
extern SoundID PAPER_RELEASE;
extern SoundID TYPEWRITER;
extern SoundID POSITIVE;
extern SoundID NEGATIVE;
extern SoundID NEUTRAL;
}

void load_global_sounds();

/*------------------------------------------------------------------------------------------------*/

// Implementation detail for AudioPlayer.
struct SoundBufferWrapper
{
    // Wraps one or several SoundBufferReferences.
    // The presence of several SoundBuffers can be indicated by a trailing "_0" in path.
    // In that case, it is assumed that similar paths, ending with "_1", "_2", etc, may exist.
    SoundBufferWrapper(const std::string& path);

    // Note that if several SoundBuffers are wrapped, then only one will be returned at random.
    const sf::SoundBuffer& get() const;
    
    bool global;
    bool playing;

private:
    std::list<SoundBufferReference> buffers;
    mutable size_t previous_rand_index;
};

/*------------------------------------------------------------------------------------------------*/

// Loads Sounds from specified paths, maps them to unique identifiers, and allows them to be
// played using said identifiers. Allows continuous Music to be played in the background.
class AudioPlayer
{
public:
    static AudioPlayer& instance()
    {
        static AudioPlayer singleton;
        return singleton;
    }

    void update(Seconds elapsed_time);

    // Returns an ID that corresponds to a previously or newly loaded SoundBuffer(s).
    // There can be several SoundBuffers linked to a single ID. See SoundBufferWrapper.
    // If the sound is specified as global, it will remain loaded until the application closes.
    // Note that an empty path returns 0. Playing 0 plays nothing and logs no errors.
    SoundID load(const std::string& path, bool global);

    // Plays a short sound through a SoundBuffer(Wrapper).
    // Note that sounds are not affected by the fade effect and continue playing after leaving a level.
    void play(SoundID id, float loudness = 1.f);

    // Streams from a file. Streaming is cancelled upon leaving a level.
    void stream(const std::string& path, float loudness = 1.f);
    void stop(const std::string& path);

    // Note that this will not unload global sounds.
    // Also, SoundBuffers are stored by the ResourceManager, so their actual destruction will be delayed.
    void stop_and_unload_all();

    // If shuffle is enabled, the music is played with random order and intervals in range: [interval / 2, interval].
    // Otherwise, the music is played with default order and fixed intervals.
    void set_playlist(const std::vector<std::string>& playlist,
                      bool shuffle, Seconds interval, float loudness);

    void set_volume(int volume);

    // By default we only fade out the music, since sounds effects may carry over into the next level.
    // However, when we close, the game the sounds would be abruptly ended, so we must then fade the sounds also.
    void fade_out(const Seconds progression_duration, bool force_sounds_fade = false);
    void fade_in(const Seconds progression_duration);

private:
    std::unordered_map<SoundID, std::pair<SoundBufferWrapper, bool>> buffers; // bool -> globality
    std::list<std::pair<sf::Sound, float>> sounds;
    std::unordered_map<SoundID, Seconds> cooldowns; // Same sound can't be played too rapidly.

    // Streams are loaded from file and can be stopped manually (via fading them out, represented by the ProgressiveFloat).
    std::unordered_multimap<std::string, std::pair<std::unique_ptr<sf::Music>, ProgressiveFloat>> streams;

    std::vector<std::string> playlist;
    bool    playlist_shuffle;
    float   playlist_loudness;
    Seconds playlist_interval;
    Seconds playlist_interval_timer;
    size_t  playlist_index;
    sf::Music current_track;

    ProgressiveFloat volume;
    ProgressiveFloat fade_multiplier;
    bool force_sounds_fade;

private:
    AudioPlayer();
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer& operator=(AudioPlayer&&) = delete;
};