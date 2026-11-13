#include "Game.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>

bool Game::start(int newColumns, int newRows, int newMinePercent) {
  if (newColumns < MIN_BOARD_SIZE || newColumns > MAX_BOARD_SIZE ||
      newRows < MIN_BOARD_SIZE || newRows > MAX_BOARD_SIZE ||
      newMinePercent < MIN_MINE_PERCENT || newMinePercent > MAX_MINE_PERCENT)
    return false;
  grid.assign(newColumns, std::vector<Cell>(newRows));
  minePercent = newMinePercent;
  state = PLAYING;
  isFirstClick = true;
  gameClock.restart();
  elapsedSeconds = 0.f;
  explodedColumn = explodedRow = -1;
  return true;
}

void Game::resetGame(int firstClickI, int firstClickJ) {
  // Keep the first click and its 8 neighbours mine-free so the opening cell
  // is always a 0 and flood-fills a region.
  std::vector<std::pair<int, int>> candidates;
  for (int i = 0; i < columns(); i++) {
    for (int j = 0; j < rows(); j++) {
      grid[i][j] = Cell();
      if (std::abs(i - firstClickI) > 1 || std::abs(j - firstClickJ) > 1)
        candidates.emplace_back(i, j);
    }
  }
  // Place an exact number of mines: partial Fisher-Yates over the candidates.
  const int available = static_cast<int>(candidates.size());
  const int count = std::min(
      mineCountFor(columns(), rows(), minePercent, available), available);
  for (int k = 0; k < count; k++) {
    const int pick = k + rand() % (available - k);
    std::swap(candidates[k], candidates[pick]);
    grid[candidates[k].first][candidates[k].second].isMine = true;
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

bool Game::chordReveal(int i, int j) {
  if (i < 0 || i >= columns() || j < 0 || j >= rows())
    return false;
  const Cell &cell = grid[i][j];
  if (!cell.isRevealed || cell.isMine || cell.minesCount == 0)
    return false;

  int flags = 0;
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      const int nx = i + dx;
      const int ny = j + dy;
      if ((dx == 0 && dy == 0) || nx < 0 || nx >= columns() || ny < 0 ||
          ny >= rows())
        continue;
      if (grid[nx][ny].isFlag)
        flags++;
    }
  }
  if (flags != cell.minesCount)
    return false;

  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      const int nx = i + dx;
      const int ny = j + dy;
      if ((dx == 0 && dy == 0) || nx < 0 || nx >= columns() || ny < 0 ||
          ny >= rows())
        continue;
      // A wrong flag can make this open a mine; stop once the game is over.
      if (state != PLAYING)
        return true;
      if (!grid[nx][ny].isRevealed && !grid[nx][ny].isFlag)
        openCells(nx, ny);
    }
  }
  return true;
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
