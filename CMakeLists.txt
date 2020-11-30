cmake_minimum_required(VERSION 3.12.0)
set(TARGET_NAME "Sjaldersbaum")
project(${TARGET_NAME})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS Sjaldersbaum/source/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS Sjaldersbaum/source/*.hpp)

if(WIN32)
    add_executable(${TARGET_NAME} WIN32 ${sources} ${headers} Sjaldersbaum/Sjaldersbaum.rc)
else()
    add_executable(${TARGET_NAME} ${sources} ${headers})
endif()

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

target_include_directories(${TARGET_NAME} PRIVATE Sjaldersbaum/source)
target_include_directories(${TARGET_NAME} PRIVATE Sjaldersbaum/source/tsl)

find_package(SFML COMPONENTS system window graphics audio CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

target_link_libraries(${TARGET_NAME} PRIVATE
    sfml-graphics sfml-audio FLAC OpenAL OpenGL Vorbis
    yaml-cpp
)