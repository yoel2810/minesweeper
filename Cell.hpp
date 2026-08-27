#pragma once

class Cell {
    public:
        int minesCount;
        bool isRevealed;
        bool isMine;
        bool isFlag;

        Cell(int minesCount, bool, bool isRevealed, bool isMine, bool isFlag);
        Cell() = default;
};
