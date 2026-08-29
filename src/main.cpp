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
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")},
    {"0", sf::Texture("assets/images/empty0.png")}};

Cell grid[GRID_SIZE][GRID_SIZE];

void resetGame() {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = Cell();
      if (rand() % 5 == 0)
        grid[i][j].isMine = true;
    }
  }

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

  if (!grid[i][j].isRevealed && !grid[i][j].isFlag) {
    grid[i][j].isRevealed = true;

    if (grid[i][j].minesCount == 0 && !grid[i][j].isMine) {
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

void renderBoard(sf::RenderWindow &window) {
  sf::Sprite sprite = sf::Sprite(cellTexturesDictionary["default"]);
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (grid[i][j].isMine)
        sprite.setTexture(cellTexturesDictionary["mine"]);
      else if (grid[i][j].isFlag)
        sprite.setTexture(cellTexturesDictionary["flag"]);
      else {
        sprite.setTexture(sf::Texture("assets/"))
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

  sf::Texture defaultTexture("assets/images/default.png");

  resetGame();

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (const auto *mouseClick =
              event->getIf<sf::Event::MouseButtonPressed>()) {

        int col = mouseClick->position.x / CELL_SIZE_PX;
        int row = mouseClick->position.y / CELL_SIZE_PX;

        std::cout << "Left click detected at: " << col << ", " << row
                  << std::endl;
        openCells(col, row);
      }
    }

    window.clear(sf::Color(180, 180, 180));
    renderBoard(window, defaultTexture);
    window.display();
  }
  return 0;
}
