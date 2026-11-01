cmake_minimum_required(VERSION 3.28)
project(Minesweeper LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
# The game only uses SFML Graphics. Avoid configuring unused optional modules
# and their additional third-party dependencies when fetching SFML.
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(minesweeper src/main.cpp src/Cell.cpp src/Game.cpp src/UI.cpp)
target_compile_features(minesweeper PRIVATE cxx_std_17)
target_link_libraries(minesweeper PRIVATE SFML::Graphics)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT minesweeper)
