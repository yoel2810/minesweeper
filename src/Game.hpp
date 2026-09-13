#pragma once
#include "Cell.hpp"
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

#define CELL_SIZE_PX 32
#define GRID_SIZE 12

constexpr int HEADER_H = 48;
constexpr int BOARD_OFFSET_X = (640 - GRID_SIZE * CELL_SIZE_PX) / 2; // 128
constexpr int BOARD_OFFSET_Y = HEADER_H;

enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON, RESTARTING };

class Game {
public:
  GameState state = MENU;
  Cell grid[GRID_SIZE][GRID_SIZE];
  bool isFirstClick = true;
  sf::Clock gameClock;
  float elapsedSeconds = 0.f;

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
