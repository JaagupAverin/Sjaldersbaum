#pragma once

#include <queue>
#include <unordered_set>

#include "logger.h"
#include "convert.h"

/*------------------------------------------------------------------------------------------------*/

enum class Event
{
                            // Expected format of accompanying data:
    // App commands:
    Terminate,              // -
    SetResolution,          // sf::Vector2u
    SetFPSCap,              // int
    SetVSync,               // bool
    SetFullscreen,          // bool
    SetAudioVolume,         // int
    SetTFMul,               // float
    SetLoadingScreen,       // bool
    LoadMenu,               // -
    LoadLevel,              // level path as std::string
    LoadUser,               // user's ID
    CreateUser,             // user's ID
    EraseUser,              // user's ID
    RevealAllObjects,       // -

    // Game commands:
    FadeAndTerminate,       // -
    SetCrosshair,           // Crosshair::Type
    DisplayMessage,         // std::string
    SetLightShader,         // shader path as std::string
    SetCameraCenter,        // sf::Vector2f, progression duration as Seconds
    ZoomIn,                 //               progression duration as Seconds
    ZoomOut,                //               progression duration as Seconds
    SetLightSource,         // sf::Vector2f, progression duration as Seconds
    SetLightRadius,         // Px,           progression duration as Seconds
    SetLightBrightness,     // float,        progression duration as Seconds
    SetLightSwing,          // Px,           progression duration as Seconds
    SetLightOn,             // bool,         progression duration as Seconds
    AdvanceObjective,       // objective's ID
    Hide,                   // entity's ID-tree
    HideMoveCamera,         // entity's ID-tree 
    Reveal,                 // entity's ID-tree
    RevealDoNotMoveCamera,  // entity's ID-tree
    Unlock,                 // entity's ID-tree
    Lock,                   // entity's ID-tree
    PlayAudio,              // SoundID
    StreamAudio,            // audio path as std::string
    StopStream,             // audio path as std::string
    StoreCommandSequence,   // "<level_path>?<command_sequence>" as std::string

    // Game events:
    UserListUpdated,        // -
};

enum class Request
{
    Resolution,             // sf::Vector2u
    FPSCap,                 // int
    VSync,                  // bool
    Fullscreen,             // bool
    AudioVolume,            // int

    ActiveUser,             // user's ID
    UserList                // std::string
};

/*------------------------------------------------------------------------------------------------*/

// Used to transfer arbitrary types of data through the EARManager.
class Data
{
public:
    Data();

    // Note that because data is stored as a string, T must support Convert::to_str(T).
    template<typename T>
    Data(const T& data);

    // Note that because data is stored as a string, T must support Convert::to_str(T).
    // Also note that data can be set only once.
    template<typename T>
    void set(const T& data);

    // Note that because data is stored as a string, T must support Convert::str_to<T>.
    template<typename T>
    T as() const;

    bool has_been_set() const;

private:
    std::string data;
    bool data_set;
};

/*------------------------------------------------------------------------------------------------*/

// Classes derived from this will be notified of any Events/Requests dispatched by the EARManager.
class Observer
{
public:
    Observer();
    ~Observer();

    virtual void on_event(Event event, const Data& data) {};
    virtual void on_request(Request request, Data& data) {};
};

/*------------------------------------------------------------------------------------------------*/

// Event And Request Manager.
// A singleton for dispatching Events and Requests to all existing Observers.
class EARManager
{
    friend Observer;

public:
    static EARManager& instance();
    
    // Dispatches the event at the start of the next loop.
    void queue_event(Event event, Data data = {});

    // Dispatches the event immediately. Unless you need immediate action, use queue_event instead.
    void dispatch_event(Event event, Data data = {});

    // Call once at the start of each loop.
    void dispatch_queued_events();

    void clear_queued_events();

    Data request(Request request) const;

private:
    void add_observer(Observer* observer) { observers.insert(observer); }

    void remove_observer(Observer* observer) { observers.erase(observer); }

private:
    std::unordered_set<Observer*> observers;
    std::queue<std::pair<Event, Data>> queued_events;

private:
    EARManager() = default;
    EARManager(const EARManager&) = delete;
    EARManager(EARManager&&) = delete;
    EARManager& operator=(const EARManager&) = delete;
    EARManager& operator=(EARManager&&) = delete;
};

/*------------------------------------------------------------------------------------------------*/
// Implementations:

inline Data::Data() :
    data_set{ false }
{

}

template<typename T>
inline Data::Data(const T& data) : Data()
{
    set(data);
}

template<typename T>
inline void Data::set(const T& data)
{
    if (data_set == true)
        LOG_ALERT("data already set.\nexisting data: " + this->data);
    else
    {
        this->data = Convert::to_str(data);
        data_set = true;
    }
}

template<typename T>
inline T Data::as() const
{
    return Convert::str_to<T>(data);
}

template<>
inline std::string Data::as() const
{
    return data;
}

inline bool Data::has_been_set() const
{
    return data_set;
}

/*------------------------------------------------------------------------------------------------*/

inline Observer::Observer()
{
    EARManager::instance().add_observer(this);
}

inline Observer::~Observer()
{
    EARManager::instance().remove_observer(this);
}

/*------------------------------------------------------------------------------------------------*/

inline EARManager& EARManager::instance()
{
    static EARManager singleton;
    return singleton;
}

inline void EARManager::queue_event(Event event, Data data)
{
    queued_events.emplace(std::make_pair(std::move(event), std::move(data)));
}

inline void EARManager::dispatch_event(const Event event, Data data)
{
    for (const auto observer : observers)
        observer->on_event(event, data);
}

inline void EARManager::dispatch_queued_events()
{
    while (!queued_events.empty())
    {
        const auto event_data_pair = queued_events.front();
        queued_events.pop();

        for (const auto observer : observers)
            observer->on_event(event_data_pair.first, event_data_pair.second);
    }
}

inline void EARManager::clear_queued_events()
{
    std::queue<std::pair<Event, Data>> empty;
    std::swap(queued_events, empty);
}

inline Data EARManager::request(const Request request) const
{
    Data data;
    for (const auto observer : observers)
    {
        observer->on_request(request, data);
        if (data.has_been_set())
            return data;
    }

    LOG_ALERT("request unanswered; returning empty data;\n"
              "request enum: " + Convert::to_str(static_cast<int>(request)));
    return Data{};
}