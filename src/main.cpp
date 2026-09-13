#include "Cell.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <unordered_map>

#define CELL_SIZE_PX 32
#define GRID_SIZE 12

enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON };

GameState currentGameState = MENU;

std::unordered_map<std::string, sf::Texture> cellTexturesDictionary = {
    {"default", sf::Texture("assets/images/default.png")},
    {"flag", sf::Texture("assets/images/flag.png")},
    {"mine", sf::Texture("assets/images/mine.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"1", sf::Texture("assets/images/empty1.png")},
    {"2", sf::Texture("assets/images/empty2.png")},
    {"3", sf::Texture("assets/images/empty3.png")},
    {"4", sf::Texture("assets/images/empty4.png")},
    {"5", sf::Texture("assets/images/empty5.png")},
    {"6", sf::Texture("assets/images/empty6.png")},
    {"7", sf::Texture("assets/images/empty7.png")},
    {"8", sf::Texture("assets/images/empty8.png")}};

Cell grid[GRID_SIZE][GRID_SIZE];

void resetGame(int firstClickI, int firstClickJ) {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = Cell();
      if (i == firstClickI && j == firstClickJ)
        continue;

      if (rand() % 5 == 0)
        grid[i][j].isMine = true;
    }
  }
  currentGameState = PLAYING;

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

void openCells(int i, int j) {
  if (i < 0 || i >= GRID_SIZE || j < 0 || j >= GRID_SIZE)
    return;

  if (grid[i][j].isFlag)
    return;

  if (grid[i][j].isMine) {
    std::cout << "GAME OVER" << std::endl;
    currentGameState = GAME_OVER;
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

bool checkWinCondition() {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (!grid[i][j].isMine && !grid[i][j].isRevealed)
        return false;
    }
  }
  return true;
}

void checkAndHandleWin() {
  if (currentGameState == PLAYING && checkWinCondition()) {
    std::cout << "GAME WON" << std::endl;
    currentGameState = GAME_WON;
  }
}

void renderBoard(sf::RenderWindow &window) {
  sf::Sprite sprite = sf::Sprite(cellTexturesDictionary["default"]);
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {

      if (grid[i][j].isRevealed) {

        if (grid[i][j].isMine)
          sprite.setTexture(cellTexturesDictionary["mine"]);
        else if (grid[i][j].isFlag)
          sprite.setTexture(cellTexturesDictionary["flag"]);
        else
          sprite.setTexture(
              cellTexturesDictionary[std::to_string(grid[i][j].minesCount)]);
      } else {
        if (grid[i][j].isFlag)
          sprite.setTexture(cellTexturesDictionary["flag"]);
        else
          sprite.setTexture(cellTexturesDictionary["default"]);
      }

      sprite.setPosition(sf::Vector2f(static_cast<float>(i * CELL_SIZE_PX),
                                      static_cast<float>(j * CELL_SIZE_PX)));
      window.draw(sprite);
    }
  }
}

int main() {
  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(640, 480)), "Minesweeper");
  window.setFramerateLimit(60);
  bool isFirstClick = true;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (const auto *mouseClick =
              event->getIf<sf::Event::MouseButtonPressed>()) {

        int col = mouseClick->position.x / CELL_SIZE_PX;
        int row = mouseClick->position.y / CELL_SIZE_PX;

        if (currentGameState == GAME_OVER || currentGameState == GAME_WON)
          continue;

        if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE)
          continue;

        if (mouseClick->button == sf::Mouse::Button::Left) {
          std::cout << "Left click detected at: " << col << ", " << row
                    << std::endl;

          if (isFirstClick) {
            resetGame(col, row);
            isFirstClick = false;
          }
          openCells(col, row);
          checkAndHandleWin();
        } else if (mouseClick->button == sf::Mouse::Button::Right) {
          std::cout << "Right click detected at: " << col << ", " << row
                    << std::endl;

          if (!grid[col][row].isRevealed)
            grid[col][row].isFlag = !grid[col][row].isFlag;
          // Optional: also check win after flagging if using flag-based win condition
          // checkAndHandleWin();
        }
      }
    }

    window.clear(sf::Color(180, 180, 180));
    renderBoard(window);
    window.display();
  }
  return 0;
}
