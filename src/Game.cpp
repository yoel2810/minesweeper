#include "Game.hpp"
#include <cstdlib>
#include <iostream>

void Game::resetGame(int firstClickI, int firstClickJ) {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = Cell();
      if (i == firstClickI && j == firstClickJ)
        continue;
      if (rand() % 5 == 0)
        grid[i][j].isMine = true;
    }
  }
  state = PLAYING;
  gameClock.restart();
  elapsedSeconds = 0.f;

  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (!grid[i][j].isMine) {
        int numMines = 0;
        for (int dx = -1; dx <= 1; dx++) {
          for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0)
              continue;
            int nx = i + dx;
            int ny = j + dy;
            if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
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
  if (i < 0 || i >= GRID_SIZE || j < 0 || j >= GRID_SIZE)
    return;

  if (grid[i][j].isFlag)
    return;

  if (grid[i][j].isMine) {
    std::cout << "GAME OVER" << std::endl;
    state = GAME_OVER;
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
          if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE) {
            openCells(nx, ny);
          }
        }
      }
    }
  }
}

bool Game::checkWinCondition() {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
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
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = Cell();
    }
  }
  state = MENU;
  isFirstClick = true;
  elapsedSeconds = 0.f;
}

int Game::flagCount() const {
  int c = 0;
  for (int i = 0; i < GRID_SIZE; i++)
    for (int j = 0; j < GRID_SIZE; j++)
      if (grid[i][j].isFlag)
        c++;
  return c;
}

int Game::mineCount() const {
  int c = 0;
  for (int i = 0; i < GRID_SIZE; i++)
    for (int j = 0; j < GRID_SIZE; j++)
      if (grid[i][j].isMine)
        c++;
  return c;
}

int Game::revealedCount() const {
  int c = 0;
  for (int i = 0; i < GRID_SIZE; i++)
    for (int j = 0; j < GRID_SIZE; j++)
      if (grid[i][j].isRevealed)
        c++;
  return c;
}

void Game::toggleFlag(int i, int j) {
  if (i < 0 || i >= GRID_SIZE || j < 0 || j >= GRID_SIZE)
    return;
  if (!grid[i][j].isRevealed)
    grid[i][j].isFlag = !grid[i][j].isFlag;
}

void Game::revealAllMines() {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (grid[i][j].isMine)
        grid[i][j].isRevealed = true;
    }
  }
}
