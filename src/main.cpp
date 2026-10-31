#include "Cell.hpp"
#include "DisplaySettings.hpp"
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
  const auto size = window.getSize();
  const auto layout =
      BoardLayout::fit(game.columns(), game.rows(), size.x, size.y);
  for (int i = 0; i < game.columns(); i++) {
    for (int j = 0; j < game.rows(); j++) {
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
      const auto textureSize = sprite.getTexture().getSize();
      sprite.setScale({static_cast<float>(layout.cellSize) / textureSize.x,
                       static_cast<float>(layout.cellSize) / textureSize.y});
      sprite.setPosition(
          sf::Vector2f(static_cast<float>(i * layout.cellSize + layout.x),
                       static_cast<float>(j * layout.cellSize + layout.y)));
      window.draw(sprite);
    }
  }
}

int main() {
  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(640, 480)), "Minesweeper",
                          sf::Style::Default);
  window.setFramerateLimit(60);
  window.setMinimumSize(sf::Vector2u(480, 360));
  DisplaySettings displaySettings;

  sf::Font font;
  if (!font.openFromFile("assets/fonts/DejaVuSans.ttf")) {
    std::cerr << "Failed to load font assets/fonts/DejaVuSans.ttf\n";
    return 1;
  }

  Game game;
  game.toMenu();

  // Create buttons
  Button menuStartButton(font, "Start Game", sf::Vector2f(220.f, 275.f),
                         sf::Vector2f(200.f, 50.f), 20);
  NumberField columnsField(font, "Columns", {180.f, 190.f});
  NumberField rowsField(font, "Rows", {340.f, 190.f});
  std::string sizeError;
  auto startGame = [&]() {
    const auto columns = parseBoardDimension(columnsField.value);
    const auto rows = parseBoardDimension(rowsField.value);
    if (!columns || !rows) {
      sizeError = "Enter columns and rows from 4 to 30.";
      return;
    }
    game.start(*columns, *rows);
    sizeError.clear();
    columnsField.focus(false);
    rowsField.focus(false);
  };
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

  UI::layoutControls(window.getSize(), menuStartButton, columnsField,
                     rowsField, hudRestartButton, gameOverRestartButton,
                     gameOverMenuButton, gameWonAgainButton,
                     gameWonMenuButton);
  auto toggleFullscreen = [&]() {
    displaySettings.toggle(window.getSize());
    const auto targetSize = displaySettings.targetSize(
        sf::VideoMode::getDesktopMode().size);
    window.create(sf::VideoMode(targetSize), "Minesweeper",
                  displaySettings.windowStyle(), displaySettings.windowState());
    window.setFramerateLimit(60);
    if (displaySettings.fullscreen)
      window.setPosition({0, 0});
    else
      window.setMinimumSize(sf::Vector2u(480, 360));
    window.setView(sf::View(sf::FloatRect({}, sf::Vector2f(targetSize))));
    UI::layoutControls(targetSize, menuStartButton, columnsField, rowsField,
                       hudRestartButton, gameOverRestartButton,
                       gameOverMenuButton, gameWonAgainButton,
                       gameWonMenuButton);
  };

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (const auto *resized = event->getIf<sf::Event::Resized>()) {
        window.setView(
            sf::View(sf::FloatRect({}, sf::Vector2f(resized->size))));
        UI::layoutControls(resized->size, menuStartButton, columnsField,
                           rowsField, hudRestartButton,
                           gameOverRestartButton, gameOverMenuButton,
                           gameWonAgainButton, gameWonMenuButton);
      }

      if (const auto *key = event->getIf<sf::Event::KeyPressed>();
          key && key->code == sf::Keyboard::Key::F11) {
        if (displaySettings.pressToggleKey())
          toggleFullscreen();
        continue;
      }

      if (const auto *key = event->getIf<sf::Event::KeyReleased>();
          key && key->code == sf::Keyboard::Key::F11) {
        displaySettings.releaseToggleKey();
        continue;
      }

      if (game.state == MENU) {
        if (const auto *text = event->getIf<sf::Event::TextEntered>()) {
          const auto previousColumns = columnsField.value;
          const auto previousRows = rowsField.value;
          columnsField.enter(text->unicode);
          rowsField.enter(text->unicode);
          if (columnsField.value != previousColumns || rowsField.value != previousRows)
            sizeError.clear();
        }
        if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
          if (key->code == sf::Keyboard::Key::Tab) {
            const bool selectColumns = !columnsField.focused;
            columnsField.focus(selectColumns);
            rowsField.focus(!selectColumns);
          } else if (key->code == sf::Keyboard::Key::Enter) {
            startGame();
          }
          // Menu typing must not trigger gameplay shortcuts.
          continue;
        }
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
            columnsField.focus(columnsField.contains(mousePos));
            rowsField.focus(rowsField.contains(mousePos));
            if (menuStartButton.contains(mousePos)) {
              startGame();
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
          const auto size = window.getSize();
          const auto layout =
              BoardLayout::fit(game.columns(), game.rows(), size.x, size.y);
          const auto cell = layout.cellAt(mousePos.x, mousePos.y);
          if (!cell)
            continue;
          const auto [col, row] = *cell;

          if (mouseClick->button == sf::Mouse::Button::Left) {
            std::cout << "Left click detected at: " << col << ", " << row
                      << std::endl;
            if (game.grid[col][row].isFlag)
              continue;
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
      UI::drawMenu(window, font, menuStartButton, columnsField, rowsField, sizeError);
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
