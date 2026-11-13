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

add_executable(minesweeper src/main.cpp src/Cell.cpp src/Game.cpp src/UI.cpp src/Solver.cpp)
target_compile_features(minesweeper PRIVATE cxx_std_17)
target_link_libraries(minesweeper PRIVATE SFML::Graphics)

add_executable(solver_bench src/SolverBench.cpp src/Cell.cpp src/Game.cpp src/Solver.cpp)
target_compile_features(solver_bench PRIVATE cxx_std_17)
target_link_libraries(solver_bench PRIVATE SFML::Graphics)

include(CTest)
if(BUILD_TESTING)
    add_executable(board_tests tests/BoardTests.cpp src/Game.cpp src/Cell.cpp src/UI.cpp)
    target_include_directories(board_tests PRIVATE src)
    target_compile_features(board_tests PRIVATE cxx_std_17)
    target_link_libraries(board_tests PRIVATE SFML::Graphics)
    add_test(NAME board_tests COMMAND board_tests)

    add_executable(solver_tests tests/SolverTests.cpp src/Game.cpp src/Cell.cpp src/Solver.cpp)
    target_include_directories(solver_tests PRIVATE src)
    target_compile_features(solver_tests PRIVATE cxx_std_17)
    target_link_libraries(solver_tests PRIVATE SFML::Graphics)
    add_test(NAME solver_tests COMMAND solver_tests)
    set_tests_properties(board_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT minesweeper)
