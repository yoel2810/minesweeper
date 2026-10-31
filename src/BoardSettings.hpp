#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <utility>

constexpr int MIN_BOARD_SIZE = 4;
constexpr int MAX_BOARD_SIZE = 30;
constexpr int DEFAULT_BOARD_SIZE = 12;
constexpr int CELL_SIZE_PX = 32;
constexpr int HEADER_H = 48;

inline std::optional<int> parseBoardDimension(const std::string &text) {
  if (text.empty() || text.size() > 2)
    return std::nullopt;
  int value = 0;
  for (char digit : text) {
    if (digit < '0' || digit > '9')
      return std::nullopt;
    value = value * 10 + digit - '0';
  }
  if (value < MIN_BOARD_SIZE || value > MAX_BOARD_SIZE)
    return std::nullopt;
  return value;
}

struct BoardLayout {
  int x, y, cellSize, columns, rows;

  // Dimensions have already been validated by Game::start.
  static BoardLayout fit(int columns, int rows, unsigned int width,
                         unsigned int height) {
    const int viewportWidth = static_cast<int>(width);
    const int viewportHeight = static_cast<int>(height);
    const int size = std::min(
        {CELL_SIZE_PX, viewportWidth / columns,
         (viewportHeight - HEADER_H) / rows});
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
