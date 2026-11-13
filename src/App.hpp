#pragma once
#include "Audio.hpp"
#include "DisplaySettings.hpp"
#include "Game.hpp"
#include "Solver.hpp"
#include "UI.hpp"
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <future>
#include <optional>
#include <string>
#include <vector>

// The application: window, input routing, solver animation and drawing.
// Game owns the rules; App only turns events into Game/Solver calls.
class App {
public:
  App();
  // Runs until the window closes. Returns the process exit code.
  int run();

private:
  // ">>" plays one solver turn every SOLVER_TURN_SECONDS so the solution can
  // be followed. A second press (or any board click) stops it.
  static constexpr float SOLVER_TURN_SECONDS = 0.15f;
  static constexpr float NO_HINT_FLASH_SECONDS = 0.6f;

  sf::RenderWindow window;
  DisplaySettings display;
  sf::Font font;
  std::optional<Controls> controls; // built once the font is loaded
  Audio audio;
  Game game;
  Solver solver{game};

  Difficulty difficulty = Difficulty::Medium;
  std::string menuError;
  bool solverRunning = false;
  sf::Clock solverTimer;
  std::optional<CellPos> solverMark; // cell the solver acted on last
  std::optional<CellPos> hintMark;   // cell the hint key pointed at
  std::optional<sf::Clock> noHintSince; // hint key flashes "nothing provable"
  GameState lastState = GameState::MENU;
  std::uint64_t lastGame = 0; // Game::gameNumber() seen by the last update

  // Solver thinking runs on a worker thread so a slow search on a big board
  // never freezes the window. A result is used only if the board still
  // matches the snapshot it was computed from; otherwise it is dropped.
  std::future<Solver::Plan> pendingPlan;
  Solver::Snapshot planBasis;
  std::future<int> pendingHint;
  Solver::Snapshot hintBasis;
  std::vector<char> hintSkip;

  void createWindow(sf::Vector2u size);
  void toggleFullscreen();

  void handleEvent(const sf::Event &event);
  void handleMenuEvent(const sf::Event &event);
  void handleKey(sf::Keyboard::Key key);
  void handleClick(sf::Mouse::Button button, sf::Vector2i position);
  void handleBoardClick(sf::Mouse::Button button, sf::Vector2i position);

  void selectDifficulty(Difficulty selected);
  void startGame();
  void requestHint();
  void requestSolverTurn();
  void collectSolverWork(); // applies finished worker results
  void clearMarks();

  void update();
  void draw();
};
