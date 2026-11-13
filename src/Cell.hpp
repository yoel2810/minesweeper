#pragma once

struct Cell {
  int minesCount = 0; // adjacent mines; meaningful for non-mine cells
  bool isRevealed = false;
  bool isMine = false;
  bool isFlag = false;
};
