#pragma once
#include "Game.hpp"

class Solver {
private:
  // Cell[][] calculateBoard();

public:
  Game &game;

  Solver(Game &currentGame);

  void playTurn();
  void solve();
};
