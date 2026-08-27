#include "Cell.hpp"
#include <cstdlib>
#define CELL_SIZE_PX 32
#define GRID_SIZE 12

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    GAME_WON
};

GameState currentGameState = MENU;

Cell grid[GRID_SIZE][GRID_SIZE];

void resetGame () {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (rand() % 5 == 0)
                grid[i][j].isMine = true;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (!grid[i][j].isMine) {
                int numMines = 0;
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx - 1 < 0 || dx + 1 >= GRID_SIZE) continue;
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dy - 1 < 0 || dy + 1 >= GRID_SIZE) continue;
                        if (grid[i + dx][j + dy].isMine)
                            numMines++;
                    }
                }
                grid[i][j].minesCount = numMines;
            }
        }
    }
}

void openCells (int i, int j) {
    if (!grid[i][j].isRevealed) {
        grid[i][j].isRevealed = true;

        if (grid[i][j].minesCount == 0) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx - 1 < 0 || dx + 1 >= GRID_SIZE) continue;
                for (int dy = -1; dy <= 1; dy++) {
                    if (dy - 1 < 0 || dy + 1 >= GRID_SIZE) continue;
                    openCells(i + dx, j + dy);
                }
            }
        }
    }
}
