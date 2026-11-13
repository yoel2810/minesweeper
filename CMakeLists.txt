cmake_minimum_required(VERSION 3.28)
project(Minesweeper LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Single-config generators (Ninja, Makefiles) build without optimisation when
# no type is given; default to Release so the game and solver run at speed.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
# The game uses SFML Graphics and Audio (sound effects). Skip the unused
# network module when fetching SFML.
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# Shared compile settings for every target in this project.
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)
target_include_directories(project_options INTERFACE src)
if(MSVC)
    target_compile_options(project_options INTERFACE /W4 /permissive-)
else()
    target_compile_options(project_options INTERFACE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# Rules and solver: no window, no audio, no SFML. Compiled once and linked
# into the game, the solver bench and both test programs.
add_library(minesweeper_core STATIC src/Game.cpp src/Solver.cpp)
target_link_libraries(minesweeper_core PUBLIC project_options)

# Drawing helpers and on-screen controls.
add_library(minesweeper_ui STATIC src/UI.cpp)
target_link_libraries(minesweeper_ui PUBLIC minesweeper_core SFML::Graphics)

# WIN32 builds a GUI subsystem app, so no console window opens with the game.
# The solver thinks on a worker thread (std::async).
find_package(Threads REQUIRED)
add_executable(minesweeper WIN32 src/main.cpp src/App.cpp src/Audio.cpp src/app.rc)
target_link_libraries(minesweeper PRIVATE minesweeper_ui SFML::Audio Threads::Threads)

add_executable(solver_bench src/SolverBench.cpp)
target_link_libraries(solver_bench PRIVATE minesweeper_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(board_tests tests/BoardTests.cpp)
    target_link_libraries(board_tests PRIVATE minesweeper_ui)
    add_test(NAME board_tests COMMAND board_tests)
    # board_tests loads assets/fonts/DejaVuSans.ttf relative to the repo root.
    set_tests_properties(board_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_executable(solver_tests tests/SolverTests.cpp)
    target_link_libraries(solver_tests PRIVATE minesweeper_core)
    add_test(NAME solver_tests COMMAND solver_tests)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT minesweeper)
