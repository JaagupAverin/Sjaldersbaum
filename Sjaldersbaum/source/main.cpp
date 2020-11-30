#include <random>

#include "app.h"

extern const std::string SYSTEM_FONT_PATH = "resources/fonts/fira_medium.ttf";

extern const Seconds RESOURCE_DESTRUCTION_INTERVAL = 120.f;

extern std::mt19937 GLOBAL_MT = std::mt19937{};

#ifdef _WIN32
#define _WIN32_WINNT 0x0502
#include <Windows.h>
INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR lpCmdLine, _In_ INT nCmdShow)

#else
int main()

#endif
{
    // Assure logger gets destructed very last:
    Logger::instance();
    // Assure all the resources, which other singletons may reference, die second-last:
    TextureManager::instance();
    SoundBufferManager::instance();
    FontManager::instance();

    srand(static_cast<unsigned int>(time(nullptr)));
    std::random_device rd;
    GLOBAL_MT.seed(rd());

    App app;
    app.run_loop();
}