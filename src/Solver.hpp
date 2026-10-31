#pragma once
#include "Game.hpp"

#define GRID_SIZE 12

class Solver {
private:
  // Cell[][] calculateBoard();

public:
  Game &game;

  Solver(Game &currentGame);

  void playTurn();
  void solve();
};
