#include "audio.h"

#include <filesystem>

namespace fs = std::filesystem;

/*------------------------------------------------------------------------------------------------*/

namespace GlobalSounds
{
extern SoundID GENERIC_HOVER        = UNINITIALIZED_SOUND;
extern SoundID GENERIC_REVEAL       = UNINITIALIZED_SOUND;
extern SoundID INTERACTION          = UNINITIALIZED_SOUND;
extern SoundID LOCKS_HIT            = UNINITIALIZED_SOUND;
extern SoundID LIGHT_ON             = UNINITIALIZED_SOUND;
extern SoundID LIGHT_OFF            = UNINITIALIZED_SOUND;
extern SoundID PAPER_PICKUPS        = UNINITIALIZED_SOUND;
extern SoundID PAPER_RELEASE        = UNINITIALIZED_SOUND;
extern SoundID TYPEWRITER           = UNINITIALIZED_SOUND;
extern SoundID POSITIVE             = UNINITIALIZED_SOUND;
extern SoundID NEGATIVE             = UNINITIALIZED_SOUND;
extern SoundID NEUTRAL              = UNINITIALIZED_SOUND;
}

const std::string GENERIC_HOVER_PATH            = "resources/audio/sounds/hover.ogg";
const std::string GENERIC_UNLOCK_PATH           = "resources/audio/sounds/reveal.ogg";
const std::string INTERACTION_PATH              = "resources/audio/sounds/tap.ogg"; 
const std::string LOCKS_HIT_PATH                = "resources/audio/sounds/locks.ogg";
const std::string LIGHT_ON_PATH                 = "resources/audio/sounds/light_on_0.ogg";
const std::string LIGHT_OFF_PATH                = "resources/audio/sounds/light_off.ogg";
const std::string PAPER_PICKUPS_PATH            = "resources/audio/sounds/paper_pickup_0.ogg";
const std::string PAPER_RELEASE_PATH            = "resources/audio/sounds/paper_release.ogg";
const std::string TYPEWRITER_PATH               = "resources/audio/sounds/typewriter_0.ogg";
const std::string POSITIVE_PATH                 = "resources/audio/sounds/positive.ogg";
const std::string NEGATIVE_PATH                 = "resources/audio/sounds/negative_0.ogg";
const std::string NEUTRAL_PATH                  = "resources/audio/sounds/neutral.ogg";

void load_global_sounds()
{
    GlobalSounds::GENERIC_HOVER         = AudioPlayer::instance().load(GENERIC_HOVER_PATH,          true);
    GlobalSounds::GENERIC_REVEAL        = AudioPlayer::instance().load(GENERIC_UNLOCK_PATH,         true);
    GlobalSounds::INTERACTION           = AudioPlayer::instance().load(INTERACTION_PATH,            true);
    GlobalSounds::LOCKS_HIT             = AudioPlayer::instance().load(LOCKS_HIT_PATH,              true);
    GlobalSounds::LIGHT_ON              = AudioPlayer::instance().load(LIGHT_ON_PATH,               true);
    GlobalSounds::LIGHT_OFF             = AudioPlayer::instance().load(LIGHT_OFF_PATH,              true);
    GlobalSounds::PAPER_PICKUPS         = AudioPlayer::instance().load(PAPER_PICKUPS_PATH,          true);
    GlobalSounds::PAPER_RELEASE         = AudioPlayer::instance().load(PAPER_RELEASE_PATH,          true);
    GlobalSounds::TYPEWRITER            = AudioPlayer::instance().load(TYPEWRITER_PATH,             true);
    GlobalSounds::POSITIVE              = AudioPlayer::instance().load(POSITIVE_PATH,               true);
    GlobalSounds::NEGATIVE              = AudioPlayer::instance().load(NEGATIVE_PATH,               true);
    GlobalSounds::NEUTRAL               = AudioPlayer::instance().load(NEUTRAL_PATH,                true);
}

/*------------------------------------------------------------------------------------------------*/

extern bool RESOURCE_LOGGING;
constexpr Seconds VOLUME_PROGRESSION_DURATION = 0.4f;

constexpr Seconds SOUND_COOLDOWN = 0.07f;

bool exists(std::string path)
{
    if (!consists_of_systemic_characters(path))
        return false;
    return fs::exists(path);
}

/*------------------------------------------------------------------------------------------------*/

SoundBufferWrapper::SoundBufferWrapper(const std::string& path) :
    global{ false },
    playing{ false },
    previous_rand_index{ 0u }
{
    std::string alt_path = path.substr(0u, path.rfind('.'));
    std::string extension = path.substr(path.rfind('.'), std::string::npos);

    if (!ends_with(alt_path, "_0")) // SoundBuffer has no alternatives.
    {
        buffers.emplace_back(SoundBufferReference());
        buffers.back().load(path);
        return;
    }

    size_t i = 0u;
    while (true)
    {
        buffers.emplace_back(SoundBufferReference());
        buffers.back().load(alt_path + extension);

        alt_path.replace(alt_path.rfind('_') + 1u, std::string::npos, Convert::to_str(++i));
        if (!exists(alt_path + extension))
        {
            if (RESOURCE_LOGGING)
                LOG_INTEL("sound file not found; assuming end at: " + alt_path);
            break;
        }
    }
}

const sf::SoundBuffer& SoundBufferWrapper::get() const
{
    if (buffers.size() == 1u)
        return buffers.front().get();
    else
    {
        size_t rand_index = rand11(size_t(0), buffers.size());
        while (rand_index == previous_rand_index)
            rand_index = rand11(size_t(0), buffers.size());
        previous_rand_index = rand_index;

        auto it = buffers.begin();
        std::advance(it, rand_index);
        return it->get();
    }
}

/*------------------------------------------------------------------------------------------------*/

AudioPlayer::AudioPlayer() : 
    playlist_shuffle{ false },
    playlist_loudness{ 0.f },
    playlist_interval{ 0.f },
    playlist_interval_timer{ 0.f },
    playlist_index{ 0u },
    volume{ 0.f },
    fade_multiplier{ 0.f },
    force_sounds_fade{ false }
{
    volume.set_progression_duration(VOLUME_PROGRESSION_DURATION);
}

void AudioPlayer::update(const Seconds elapsed_time)
{
    volume.update(elapsed_time);
    fade_multiplier.update(elapsed_time);

    if (volume.has_changed_since_last_check() || fade_multiplier.has_changed_since_last_check())
    {
        for (auto& [sound, loudness] : sounds)
        {
            if (force_sounds_fade)
                sound.setVolume(volume.get_current() * fade_multiplier.get_current() * loudness);
            else
                sound.setVolume(volume.get_current() * loudness);
        }

        current_track.setVolume(volume.get_current() * fade_multiplier.get_current() *
                                playlist_loudness);
    }

    for (auto it = sounds.begin(); it != sounds.end();)
    {
        if (it->first.getStatus() != sf::Sound::Status::Playing)
            it = sounds.erase(it);
        else
            ++it;
    }

    for (auto it = streams.begin(); it != streams.end();)
    {
        auto& local_volume_multiplier = it->second.second;
        auto& stream = *(it->second.first);

        local_volume_multiplier.update(elapsed_time);

        if (local_volume_multiplier.get_current() == 0.f ||
            stream.getStatus() != sf::Sound::Status::Playing)
            it = streams.erase(it);
        else
            ++it;

        stream.setVolume(volume.get_current() *
                         fade_multiplier.get_current() *
                         local_volume_multiplier.get_current());
    }

    for (auto it = cooldowns.begin(); it != cooldowns.end();)
    {
        if ((it->second -= elapsed_time) <= 0.f)
            it = cooldowns.erase(it);
        else
            ++it;
    }

    if (!playlist.empty() && current_track.getStatus() == sf::Sound::Stopped)
    {
        // Music just stopped; start the interval:
        if (playlist_interval_timer <= 0.f)
        {
            if (playlist_shuffle)
                playlist_interval_timer = rand(playlist_interval / 2.f, playlist_interval);
            else
                playlist_interval_timer = playlist_interval;
        }

        // Interval has ended:
        if ((playlist_interval_timer -= elapsed_time) <= 0.f)
        {
            if (playlist.size() != 1u)
            {
                if (playlist_shuffle)
                {
                    const size_t old_playlist_index = playlist_index;
                    while (old_playlist_index == playlist_index)
                        playlist_index = rand(size_t(0), playlist.size());
                }
                else
                    if (++playlist_index == playlist.size())
                        playlist_index = 0u;
            }

            if (!current_track.openFromFile(playlist[playlist_index]))
                LOG_ALERT("could not play track from:\n" + playlist[playlist_index]);
            current_track.play();
        }
    }
}

SoundID get_id(const std::string& path)
{
    return std::hash<std::string>{}(path);
}

SoundID AudioPlayer::load(const std::string& path, const bool global)
{
    if (path.empty())
        return 0u;

    const SoundID id = get_id(path);
    if (!contains(buffers, id))
        buffers.emplace(id, std::make_pair(SoundBufferWrapper(path), global));
    return id;
}

void AudioPlayer::play(const SoundID id, float loudness)
{
    if (!contains(buffers, id))
    {
        if  (id != 0u)
        {
            if (id == UNINITIALIZED_SOUND)
                LOG_ALERT("uninitialized sound; error in code.");
            else
                LOG_ALERT("unknown id: " + Convert::to_str(id));
        }
        return;
    }
    if (!assure_bounds(loudness, 0.f, 1.f))
        LOG_ALERT("invalid loudness had to be adjusted; [0-1]");

    if (contains(cooldowns, id))
        return;
    cooldowns.emplace(id, SOUND_COOLDOWN);

    sounds.emplace_back(std::make_pair(sf::Sound{ buffers.at(id).first.get() }, loudness));
    sounds.back().first.setVolume(volume.get_current() * fade_multiplier.get_current() * loudness);
    sounds.back().first.play();
}

void AudioPlayer::stream(const std::string& path, float loudness)
{
    auto stream = std::make_unique<sf::Music>();
    if (!stream->openFromFile(path))
    {
        LOG_ALERT("could not stream from:\n" + path);
        return;
    }
    if (!assure_bounds(loudness, 0.f, 1.f))
        LOG_ALERT("invalid loudness had to be adjusted; [0-1]");
    stream->setVolume(volume.get_current() * fade_multiplier.get_current() * loudness);
    stream->play();
    streams.emplace(path, std::make_pair(std::move(stream),
                                         ProgressiveFloat(loudness, VOLUME_PROGRESSION_DURATION)));
}

void AudioPlayer::stop(const std::string& path)
{
    for (auto& [m_path, stream_volume_pair] : streams)
        if (m_path == path)
            stream_volume_pair.second.set_target(0.f);
}

void AudioPlayer::stop_and_unload_all()
{
    streams.clear();

    // Sounds are not stoppable. They go on until the end because they're expected to be short anyways.
    // Even though their wrappers are erased here, the actual destruction of buffers occurs after a
    // lenghty delay, so this should be fine as long as sounds are short (less than a minute).

    for (auto it = buffers.begin(); it != buffers.end();)
    {
        if (it->second.second) // global
            ++it;
        else
            it = buffers.erase(it);
    }
}

void AudioPlayer::set_playlist(const std::vector<std::string>& playlist,
                               const bool shuffle, const Seconds interval, float loudness)
{
    current_track.stop();

    this->playlist = playlist;
    playlist_index = playlist.size() - 1u;
    playlist_shuffle = shuffle;
    playlist_interval = interval;

    if (!assure_bounds(loudness, 0.f, 1.f))
        LOG_ALERT("invalid loudness had to be adjusted; [0-1]");
    playlist_loudness = loudness;
    current_track.setVolume(volume.get_current() *
                            this->fade_multiplier.get_current() * playlist_loudness);
}

void AudioPlayer::set_volume(const int volume)
{
    this->volume.set_target(static_cast<float>(volume));
}

void AudioPlayer::fade_out(const Seconds progression_duration, const bool force_sounds_fade)
{
    fade_multiplier.set_progression_duration(progression_duration);
    fade_multiplier.set_target(0.f);
    this->force_sounds_fade = force_sounds_fade;
}

void AudioPlayer::fade_in(const Seconds progression_duration)
{
    fade_multiplier.set_progression_duration(progression_duration);
    fade_multiplier.set_target(1.f);
}
