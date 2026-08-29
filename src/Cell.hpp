#pragma once

class Cell {
    public:
        int minesCount = 0;
        bool isRevealed = false;
        bool isMine = false;
        bool isFlag = false;

        Cell() = default;
        Cell(int minesCount, bool isRevealed, bool isMine, bool isFlag);
};
