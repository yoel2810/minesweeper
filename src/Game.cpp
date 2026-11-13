#include "Game.hpp"
#include <algorithm>
#include <cstdlib>

bool Game::start(int newColumns, int newRows, int newMinePercent) {
  if (newColumns < MIN_BOARD_SIZE || newColumns > MAX_BOARD_SIZE ||
      newRows < MIN_BOARD_SIZE || newRows > MAX_BOARD_SIZE ||
      newMinePercent < MIN_MINE_PERCENT || newMinePercent > MAX_MINE_PERCENT)
    return false;
  grid_.assign(newColumns, std::vector<Cell>(newRows));
  minePercent_ = newMinePercent;
  state_ = GameState::PLAYING;
  minesPlaced_ = false;
  explodedColumn_ = explodedRow_ = -1;
  finalSeconds_ = 0.f;
  gameNumber_++;
  return true;
}

void Game::restart() { start(columns(), rows(), minePercent_); }

void Game::toMenu() {
  clearBoard();
  state_ = GameState::MENU;
  minesPlaced_ = false;
  explodedColumn_ = explodedRow_ = -1;
  finalSeconds_ = 0.f;
}

void Game::clearBoard() {
  for (auto &column : grid_)
    std::fill(column.begin(), column.end(), Cell());
}

void Game::placeMines(int safeColumn, int safeRow) {
  clearBoard();
  std::vector<std::pair<int, int>> candidates;
  for (int i = 0; i < columns(); i++)
    for (int j = 0; j < rows(); j++)
      if (std::abs(i - safeColumn) > 1 || std::abs(j - safeRow) > 1)
        candidates.emplace_back(i, j);
  // Exact count: partial Fisher-Yates over the candidates.
  const int available = static_cast<int>(candidates.size());
  const int count =
      std::min(mineCountFor(columns(), rows(), minePercent_), available);
  for (int k = 0; k < count; k++) {
    std::uniform_int_distribution<int> pick(k, available - 1);
    std::swap(candidates[k], candidates[pick(rng_)]);
    grid_[candidates[k].first][candidates[k].second].isMine = true;
  }
  finishPlacing();
}

void Game::loadMines(const std::vector<std::pair<int, int>> &mines) {
  clearBoard();
  for (const auto &[column, row] : mines)
    if (inBounds(column, row))
      grid_[column][row].isMine = true;
  state_ = GameState::PLAYING;
  finishPlacing();
}

void Game::finishPlacing() {
  for (int i = 0; i < columns(); i++)
    for (int j = 0; j < rows(); j++) {
      int count = 0;
      forEachNeighbour(i, j, [&](int c, int r) { count += grid_[c][r].isMine; });
      grid_[i][j].minesCount = count;
    }
  minesPlaced_ = true;
  explodedColumn_ = explodedRow_ = -1;
  startedAt_ = Clock::now();
  finalSeconds_ = 0.f;
}

int Game::reveal(int column, int row) {
  if (state_ != GameState::PLAYING || !inBounds(column, row))
    return 0;
  if (grid_[column][row].isFlag)
    return 0;
  if (!minesPlaced_)
    placeMines(column, row);
  const int opened = grid_[column][row].isRevealed ? chord(column, row)
                                                   : openCells(column, row);
  checkForWin();
  return opened;
}

// Iterative flood fill from (column, row); returns the cells revealed.
int Game::openCells(int column, int row) {
  if (grid_[column][row].isMine) {
    explode(column, row);
    return 0;
  }
  int opened = 0;
  std::vector<std::pair<int, int>> pending{{column, row}};
  while (!pending.empty()) {
    const auto [c, r] = pending.back();
    pending.pop_back();
    Cell &cell = grid_[c][r];
    if (cell.isRevealed || cell.isFlag || cell.isMine)
      continue;
    cell.isRevealed = true;
    opened++;
    if (cell.minesCount == 0)
      forEachNeighbour(c, r, [&](int nc, int nr) { pending.emplace_back(nc, nr); });
  }
  return opened;
}

int Game::chord(int column, int row) {
  const Cell &cell = grid_[column][row];
  if (cell.isMine || cell.minesCount == 0)
    return 0;
  int flags = 0;
  forEachNeighbour(column, row,
                   [&](int c, int r) { flags += grid_[c][r].isFlag; });
  if (flags != cell.minesCount)
    return 0;
  int opened = 0;
  forEachNeighbour(column, row, [&](int c, int r) {
    // A wrong flag can make this open a mine; stop once the game is over.
    if (state_ == GameState::PLAYING && !grid_[c][r].isRevealed &&
        !grid_[c][r].isFlag)
      opened += openCells(c, r);
  });
  return opened;
}

void Game::explode(int column, int row) {
  state_ = GameState::GAME_OVER;
  explodedColumn_ = column;
  explodedRow_ = row;
  finalSeconds_ = secondsSinceStart();
  for (auto &col : grid_)
    for (auto &cell : col)
      if (cell.isMine)
        cell.isRevealed = true;
}

void Game::checkForWin() {
  if (state_ == GameState::PLAYING && checkWinCondition()) {
    state_ = GameState::GAME_WON;
    finalSeconds_ = secondsSinceStart();
  }
}

float Game::secondsSinceStart() const {
  return std::chrono::duration<float>(Clock::now() - startedAt_).count();
}

bool Game::toggleFlag(int column, int row) {
  if (state_ != GameState::PLAYING || !minesPlaced_ || !inBounds(column, row))
    return false;
  Cell &cell = grid_[column][row];
  if (cell.isRevealed)
    return false;
  cell.isFlag = !cell.isFlag;
  return true;
}

bool Game::checkWinCondition() const {
  for (const auto &column : grid_)
    for (const Cell &cell : column)
      if (!cell.isMine && !cell.isRevealed)
        return false;
  return true;
}

float Game::elapsedSeconds() const {
  if (!minesPlaced_ || state_ == GameState::MENU)
    return 0.f;
  if (state_ == GameState::PLAYING)
    return secondsSinceStart();
  return finalSeconds_;
}

int Game::mineCount() const {
  if (!minesPlaced_)
    return mineCountFor(columns(), rows(), minePercent_);
  int count = 0;
  for (const auto &column : grid_)
    for (const Cell &cell : column)
      count += cell.isMine;
  return count;
}

int Game::flagCount() const {
  int count = 0;
  for (const auto &column : grid_)
    for (const Cell &cell : column)
      count += cell.isFlag;
  return count;
}

int Game::revealedCount() const {
  int count = 0;
  for (const auto &column : grid_)
    for (const Cell &cell : column)
      count += cell.isRevealed;
  return count;
}
