#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <utility>

constexpr int MIN_BOARD_SIZE = 4;
constexpr int MAX_BOARD_SIZE = 30;
constexpr int DEFAULT_BOARD_SIZE = 12;
constexpr int CELL_SIZE_PX = 32;
constexpr int HEADER_H = 64;
// Space kept between the board and the window edges for the LCD bezel.
constexpr int BOARD_PADDING = 24;

// Mine density, as a percentage of all cells on the board.
constexpr int MIN_MINE_PERCENT = 5;
constexpr int MAX_MINE_PERCENT = 50;

enum class Difficulty { Easy, Medium, Hard, Custom };
constexpr int DIFFICULTY_COUNT = 4;

inline const char *difficultyName(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Easy: return "EASY";
  case Difficulty::Medium: return "MEDIUM";
  case Difficulty::Hard: return "HARD";
  default: return "CUSTOM";
  }
}

// Preset densities. Custom has no preset: the player types the percentage.
inline int presetMinePercent(Difficulty difficulty) {
  switch (difficulty) {
  case Difficulty::Easy: return 10;
  case Difficulty::Hard: return 20;
  default: return 15;
  }
}

constexpr int DEFAULT_MINE_PERCENT = 15; // Medium

// One- or two-digit number within [min, max].
inline std::optional<int> parseTwoDigitNumber(const std::string &text, int min,
                                              int max) {
  if (text.empty() || text.size() > 2)
    return std::nullopt;
  int value = 0;
  for (char digit : text) {
    if (digit < '0' || digit > '9')
      return std::nullopt;
    value = value * 10 + digit - '0';
  }
  if (value < min || value > max)
    return std::nullopt;
  return value;
}

inline std::optional<int> parseBoardDimension(const std::string &text) {
  return parseTwoDigitNumber(text, MIN_BOARD_SIZE, MAX_BOARD_SIZE);
}

inline std::optional<int> parseMinePercent(const std::string &text) {
  return parseTwoDigitNumber(text, MIN_MINE_PERCENT, MAX_MINE_PERCENT);
}

// Cells kept mine-free around the first click: the click and its neighbours.
constexpr int SAFE_OPENING_CELLS = 9;

// Mines placed on a board: the percentage of all cells, rounded, at least one,
// and never more than the cells outside the largest possible safe opening.
// It does not depend on where the first click lands, so the HUD can show the
// count before any mine is placed.
inline int mineCountFor(int columns, int rows, int minePercent) {
  const int cells = columns * rows;
  const int wanted = (cells * minePercent + 50) / 100;
  return std::clamp(wanted, 1, std::max(1, cells - SAFE_OPENING_CELLS));
}

struct BoardLayout {
  int x, y, cellSize, columns, rows;

  // Dimensions have already been validated by Game::start. The cell size
  // never drops below 1 px, so cellAt cannot divide by zero.
  static BoardLayout fit(int columns, int rows, unsigned int width,
                         unsigned int height) {
    const int viewportWidth = static_cast<int>(width);
    const int viewportHeight = static_cast<int>(height);
    const int size = std::max(
        1, std::min({CELL_SIZE_PX, (viewportWidth - 2 * BOARD_PADDING) / columns,
                     (viewportHeight - HEADER_H - 2 * BOARD_PADDING) / rows}));
    return {(viewportWidth - columns * size) / 2,
            HEADER_H + (viewportHeight - HEADER_H - rows * size) / 2,
            size, columns, rows};
  }

  std::optional<std::pair<int, int>> cellAt(int pixelX, int pixelY) const {
    if (pixelX < x || pixelY < y || pixelX >= x + columns * cellSize ||
        pixelY >= y + rows * cellSize)
      return std::nullopt;
    return std::pair{(pixelX - x) / cellSize, (pixelY - y) / cellSize};
  }
};
