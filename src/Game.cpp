#include "Game.hpp"
#include <cstdlib>
#include <iostream>

bool Game::start(int newColumns, int newRows) {
  if (newColumns < MIN_BOARD_SIZE || newColumns > MAX_BOARD_SIZE ||
      newRows < MIN_BOARD_SIZE || newRows > MAX_BOARD_SIZE)
    return false;
  grid.assign(newColumns, std::vector<Cell>(newRows));
  state = PLAYING;
  isFirstClick = true;
  gameClock.restart();
  elapsedSeconds = 0.f;
  explodedColumn = explodedRow = -1;
  return true;
}

void Game::resetGame(int firstClickI, int firstClickJ) {
  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      grid[i][j] = Cell();
      // Keep the first click and its 8 neighbours mine-free so the opening
      // cell is always a 0 and flood-fills a region.
      if (std::abs(i - firstClickI) <= 1 && std::abs(j - firstClickJ) <= 1)
        continue;
      if (rand() % 5 == 0)
        grid[i][j].isMine = true;
    }
  }
  state = PLAYING;
  gameClock.restart();
  elapsedSeconds = 0.f;
  explodedColumn = explodedRow = -1;

  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      if (!grid[i][j].isMine) {
        int numMines = 0;
        for (int dx = -1; dx <= 1; dx++) {
          for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0)
              continue;
            int nx = i + dx;
            int ny = j + dy;
            if (nx >= 0 && nx < columns() && ny >= 0 && ny < rows()) {
              if (grid[nx][ny].isMine)
                numMines++;
            }
          }
        }
        grid[i][j].minesCount = numMines;
      }
    }
  }
}

void Game::openCells(int i, int j) {
  if (i < 0 || i >= columns() || j < 0 || j >= rows())
    return;

  if (grid[i][j].isFlag)
    return;

  if (grid[i][j].isMine) {
    std::cout << "GAME OVER" << std::endl;
    state = GAME_OVER;
    explodedColumn = i;
    explodedRow = j;
    revealAllMines();
    return;
  }

  if (!grid[i][j].isRevealed) {
    grid[i][j].isRevealed = true;
    if (grid[i][j].minesCount == 0) {
      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          if (dx == 0 && dy == 0)
            continue;
          int nx = i + dx;
          int ny = j + dy;
          if (nx >= 0 && nx < columns() && ny >= 0 && ny < rows()) {
            openCells(nx, ny);
          }
        }
      }
    }
  }
}

bool Game::checkWinCondition() {
  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      if (!grid[i][j].isMine && !grid[i][j].isRevealed)
        return false;
    }
  }
  return true;
}

void Game::checkAndHandleWin() {
  if (state == PLAYING && checkWinCondition()) {
    std::cout << "GAME WON" << std::endl;
    state = GAME_WON;
    elapsedSeconds = gameClock.getElapsedTime().asSeconds();
  }
}

void Game::restart() { toMenu(); }

void Game::toMenu() {
  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      grid[i][j] = Cell();
    }
  }
  state = MENU;
  isFirstClick = true;
  elapsedSeconds = 0.f;
  explodedColumn = explodedRow = -1;
}

int Game::flagCount() const {
  int c = 0;
  for (int i = 0; i < columns(); i++)
    for (int j = 0; j < rows(); j++)
      if (grid[i][j].isFlag)
        c++;
  return c;
}

int Game::mineCount() const {
  int c = 0;
  for (int i = 0; i < columns(); i++)
    for (int j = 0; j < rows(); j++)
      if (grid[i][j].isMine)
        c++;
  return c;
}

int Game::revealedCount() const {
  int c = 0;
  for (int i = 0; i < columns(); i++)
    for (int j = 0; j < rows(); j++)
      if (grid[i][j].isRevealed)
        c++;
  return c;
}

void Game::toggleFlag(int i, int j) {
  if (i < 0 || i >= columns() || j < 0 || j >= rows())
    return;
  if (!grid[i][j].isRevealed)
    grid[i][j].isFlag = !grid[i][j].isFlag;
}

void Game::revealAllMines() {
  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      if (grid[i][j].isMine)
        grid[i][j].isRevealed = true;
    }
  }
}
