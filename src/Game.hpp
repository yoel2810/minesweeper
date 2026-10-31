#pragma once
#include "Cell.hpp"
#include "BoardSettings.hpp"
#include <vector>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON, RESTARTING };

class Game {
public:
  GameState state = MENU;
  std::vector<std::vector<Cell>> grid = std::vector<std::vector<Cell>>(
      DEFAULT_BOARD_SIZE, std::vector<Cell>(DEFAULT_BOARD_SIZE));
  bool isFirstClick = true;
  sf::Clock gameClock;
  float elapsedSeconds = 0.f;

  int columns() const { return static_cast<int>(grid.size()); }
  int rows() const { return static_cast<int>(grid.front().size()); }
  bool start(int columns, int rows);

  void resetGame(int firstClickI, int firstClickJ);
  void openCells(int i, int j);
  bool checkWinCondition();
  void checkAndHandleWin();
  void restart(); // instant: clear and go to MENU
  void toMenu();
  int flagCount() const;
  int mineCount() const;
  int revealedCount() const;
  void toggleFlag(int i, int j);
  void revealAllMines();
};
