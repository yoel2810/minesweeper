#pragma once
#include "BoardSettings.hpp"
#include "Cell.hpp"
#include <chrono>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

enum class GameState { MENU, PLAYING, GAME_OVER, GAME_WON };

// Minesweeper rules and board state. Every change goes through the action
// methods below, so the win/loss bookkeeping (state, timer, exploded cell)
// lives in one place for both the player and the solver.
class Game {
public:
  using Grid = std::vector<std::vector<Cell>>; // indexed grid[column][row]

  // ---- read-only view
  GameState state() const { return state_; }
  int columns() const { return static_cast<int>(grid_.size()); }
  int rows() const { return static_cast<int>(grid_.front().size()); }
  int minePercent() const { return minePercent_; }
  bool inBounds(int column, int row) const {
    return column >= 0 && column < columns() && row >= 0 && row < rows();
  }
  const Cell &cell(int column, int row) const { return grid_[column][row]; }
  const Grid &grid() const { return grid_; }
  // Mines are placed lazily, on the first reveal.
  bool minesPlaced() const { return minesPlaced_; }
  int explodedColumn() const { return explodedColumn_; } // -1 when none
  int explodedRow() const { return explodedRow_; }
  // Changes on every start(), so observers can tell that a new game began.
  std::uint64_t gameNumber() const { return gameNumber_; }

  // Mines on the board; before the first click, the number that will be placed.
  int mineCount() const;
  int flagCount() const;
  int revealedCount() const;
  bool checkWinCondition() const; // every non-mine cell revealed
  // 0 until the first click, then running until the game ends, then frozen.
  float elapsedSeconds() const;

  // ---- actions
  void seed(std::uint32_t value) { rng_.seed(value); }
  // Validates the settings and begins a fresh game; false if out of range.
  bool start(int columns, int rows, int minePercent = DEFAULT_MINE_PERCENT);
  void restart(); // a fresh game with the current size and density
  void toMenu();
  // A left click. The first one places the mines (the click and its
  // neighbours stay mine-free). On a revealed number whose adjacent flags
  // match it, opens the unflagged neighbours (chord). Flagged cells are left
  // alone. Returns the number of cells newly revealed.
  int reveal(int column, int row);
  // Flags or unflags an unrevealed cell. Not allowed before the first click,
  // when nothing is known yet. Returns true if the cell changed.
  bool toggleFlag(int column, int row);

  // Places the mines, keeping (safeColumn, safeRow) and its neighbours free.
  // reveal() calls this on the first click; public for tests.
  void placeMines(int safeColumn, int safeRow);
  // Test hook: puts mines exactly at `mines` instead of at random.
  void loadMines(const std::vector<std::pair<int, int>> &mines);

private:
  using Clock = std::chrono::steady_clock;

  Grid grid_ = Grid(DEFAULT_BOARD_SIZE, std::vector<Cell>(DEFAULT_BOARD_SIZE));
  GameState state_ = GameState::MENU;
  int minePercent_ = DEFAULT_MINE_PERCENT;
  bool minesPlaced_ = false;
  int explodedColumn_ = -1;
  int explodedRow_ = -1;
  std::uint64_t gameNumber_ = 0;
  Clock::time_point startedAt_{};
  float finalSeconds_ = 0.f;
  std::mt19937 rng_; // default seed: deterministic unless seed() is called

  void clearBoard();
  void finishPlacing(); // neighbour counts, then the timer starts
  int openCells(int column, int row);
  int chord(int column, int row);
  void explode(int column, int row);
  void checkForWin();
  float secondsSinceStart() const;

  template <class F> void forEachNeighbour(int column, int row, F f) const {
    for (int dx = -1; dx <= 1; dx++)
      for (int dy = -1; dy <= 1; dy++)
        if ((dx != 0 || dy != 0) && inBounds(column + dx, row + dy))
          f(column + dx, row + dy);
  }
};
