#include "Cell.hpp"
#include "Game.hpp"
#include "UI.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <iostream>
#include <optional>
#include <unordered_map>

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

void renderBoard(sf::RenderWindow &window, Game &game) {
  sf::Sprite sprite = sf::Sprite(cellTexturesDictionary["default"]);
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      if (game.grid[i][j].isRevealed) {
        if (game.grid[i][j].isMine)
          sprite.setTexture(cellTexturesDictionary["mine"]);
        else if (game.grid[i][j].isFlag)
          sprite.setTexture(cellTexturesDictionary["flag"]);
        else
          sprite.setTexture(cellTexturesDictionary[std::to_string(
              game.grid[i][j].minesCount)]);
      } else {
        if (game.grid[i][j].isFlag)
          sprite.setTexture(cellTexturesDictionary["flag"]);
        else
          sprite.setTexture(cellTexturesDictionary["default"]);
      }
      sprite.setPosition(
          sf::Vector2f(static_cast<float>(i * CELL_SIZE_PX + BOARD_OFFSET_X),
                       static_cast<float>(j * CELL_SIZE_PX + BOARD_OFFSET_Y)));
      window.draw(sprite);
    }
  }
}

int main() {
  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(640, 480)), "Minesweeper");
  window.setFramerateLimit(60);

  sf::Font font;
  if (!font.openFromFile("assets/fonts/DejaVuSans.ttf")) {
    std::cerr << "Failed to load font assets/fonts/DejaVuSans.ttf\n";
    return 1;
  }

  Game game;
  game.toMenu();

  // Create buttons
  Button menuStartButton(font, "Start Game", sf::Vector2f(220.f, 185.f),
                         sf::Vector2f(200.f, 50.f), 20);
  Button hudRestartButton(font, "R", sf::Vector2f(304.f, 8.f),
                          sf::Vector2f(32.f, 32.f), 18);
  Button gameOverRestartButton(font, "Restart", sf::Vector2f(220.f, 240.f),
                               sf::Vector2f(200.f, 42.f), 18);
  Button gameOverMenuButton(font, "Menu", sf::Vector2f(220.f, 292.f),
                            sf::Vector2f(200.f, 42.f), 18);
  gameOverMenuButton.normal = sf::Color(90, 90, 90);
  gameOverMenuButton.hover = sf::Color(70, 70, 70);
  gameOverMenuButton.shape.setFillColor(gameOverMenuButton.normal);

  Button gameWonAgainButton(font, "Play Again", sf::Vector2f(220.f, 240.f),
                            sf::Vector2f(200.f, 42.f), 18);
  Button gameWonMenuButton(font, "Menu", sf::Vector2f(220.f, 292.f),
                           sf::Vector2f(200.f, 42.f), 18);
  gameWonMenuButton.normal = sf::Color(90, 90, 90);
  gameWonMenuButton.hover = sf::Color(70, 70, 70);
  gameWonMenuButton.shape.setFillColor(gameWonMenuButton.normal);

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      // Global key handling
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::R) {
          game.restart();
        } else if (key->code == sf::Keyboard::Key::Escape) {
          game.toMenu();
        }
      }

      // Mouse moved -> hover updates
      if (const auto *moved = event->getIf<sf::Event::MouseMoved>()) {
        sf::Vector2i pos(moved->position.x, moved->position.y);
        // Update hover per state
        if (game.state == MENU) {
          menuStartButton.setHovered(menuStartButton.contains(pos));
        } else if (game.state == PLAYING) {
          hudRestartButton.setHovered(hudRestartButton.contains(pos));
        } else if (game.state == GAME_OVER) {
          gameOverRestartButton.setHovered(gameOverRestartButton.contains(pos));
          gameOverMenuButton.setHovered(gameOverMenuButton.contains(pos));
        } else if (game.state == GAME_WON) {
          gameWonAgainButton.setHovered(gameWonAgainButton.contains(pos));
          gameWonMenuButton.setHovered(gameWonMenuButton.contains(pos));
        }
      }

      if (const auto *mouseClick =
              event->getIf<sf::Event::MouseButtonPressed>()) {
        sf::Vector2i mousePos(mouseClick->position.x, mouseClick->position.y);

        // MENU handling
        if (game.state == MENU) {
          if (mouseClick->button == sf::Mouse::Button::Left) {
            if (menuStartButton.contains(mousePos)) {
              // Start playing: empty board, clock started, wait for first click
              // to place mines Reset any leftover state and go to PLAYING
              for (int i = 0; i < GRID_SIZE; i++)
                for (int j = 0; j < GRID_SIZE; j++)
                  game.grid[i][j] = Cell();
              game.state = PLAYING;
              game.isFirstClick = true;
              game.gameClock.restart();
              game.elapsedSeconds = 0.f;
            }
          }
          continue;
        }

        // HUD restart button is global (visible in PLAYING, but also handle in
        // terminal states) For PLAYING, check header area first
        if (game.state == PLAYING) {
          if (hudRestartButton.contains(mousePos) &&
              mouseClick->button == sf::Mouse::Button::Left) {
            game.restart();
            continue;
          }
        }

        if (game.state == GAME_OVER) {
          if (mouseClick->button == sf::Mouse::Button::Left) {
            if (gameOverRestartButton.contains(mousePos)) {
              game.restart();
            } else if (gameOverMenuButton.contains(mousePos)) {
              game.toMenu();
            }
          }
          continue;
        }

        if (game.state == GAME_WON) {
          if (mouseClick->button == sf::Mouse::Button::Left) {
            if (gameWonAgainButton.contains(mousePos)) {
              game.restart();
              // After restart we are in MENU; optionally auto-start
              // Keep in MENU so player clicks Start again
            } else if (gameWonMenuButton.contains(mousePos)) {
              game.toMenu();
            }
          }
          continue;
        }

        if (game.state == RESTARTING) {
          continue;
        }

        // PLAYING board clicks
        if (game.state == PLAYING) {
          int col = (mouseClick->position.x - BOARD_OFFSET_X) / CELL_SIZE_PX;
          int row = (mouseClick->position.y - BOARD_OFFSET_Y) / CELL_SIZE_PX;

          if (col < 0 || col >= GRID_SIZE || row < 0 || row >= GRID_SIZE)
            continue;

          if (mouseClick->button == sf::Mouse::Button::Left) {
            std::cout << "Left click detected at: " << col << ", " << row
                      << std::endl;
            if (game.isFirstClick) {
              game.resetGame(col, row);
              game.isFirstClick = false;
            }
            game.openCells(col, row);
            game.checkAndHandleWin();
            if (game.state == GAME_OVER) {
              game.elapsedSeconds = game.gameClock.getElapsedTime().asSeconds();
            } else if (game.state == GAME_WON) {
              // elapsed set in checkAndHandleWin
            }
          } else if (mouseClick->button == sf::Mouse::Button::Right) {
            std::cout << "Right click detected at: " << col << ", " << row
                      << std::endl;
            game.toggleFlag(col, row);
          }
        }
      }
    }

    // RESTARTING is instant, but handle transition if it was set directly
    if (game.state == RESTARTING) {
      game.toMenu();
    }

    window.clear(sf::Color(180, 180, 180));
    renderBoard(window, game);

    // Always draw HUD (header) on top of board background
    UI::drawHUD(window, font, game, hudRestartButton);

    if (game.state == MENU) {
      drawOverlay(window, sf::Color(180, 180, 180, 180));
      // Re-draw board dimmed? overlay already dims, just draw menu
      UI::drawMenu(window, font, menuStartButton);
    } else if (game.state == GAME_OVER) {
      UI::drawGameOver(window, font, game, gameOverRestartButton,
                       gameOverMenuButton);
    } else if (game.state == GAME_WON) {
      UI::drawGameWon(window, font, game, gameWonAgainButton,
                      gameWonMenuButton);
    } else if (game.state == RESTARTING) {
      UI::drawRestarting(window, font);
    }

    window.display();
  }
  return 0;
}
