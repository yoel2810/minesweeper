#include "Cell.hpp"

Cell::Cell(int minesCount, bool, bool isRevealed, bool isMine, bool isFlag) {
    this->minesCount = minesCount;
    this->isRevealed = isRevealed;
    this->isMine = isMine;
    this->isFlag = isFlag;
}
