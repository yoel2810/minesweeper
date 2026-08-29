#include "Cell.hpp"

Cell::Cell(int minesCount, bool isRevealed, bool isMine, bool isFlag)
    : minesCount(minesCount), isRevealed(isRevealed), isMine(isMine), isFlag(isFlag) {
}
