#include "Cell.hpp"
#include "DisplaySettings.hpp"
#include "Game.hpp"
#include "Solver.hpp"
#include "UI.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <optional>

int main() {
  // Seed rand() so mine layouts differ between runs.
  std::srand(static_cast<unsigned>(std::time(nullptr)));

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(640, 480)), "Minesweeper",
                          sf::Style::Default);
  window.setFramerateLimit(60);
  window.setMinimumSize(sf::Vector2u(480, 360));
  DisplaySettings displaySettings;

  sf::Font font;
  if (!font.openFromFile("assets/fonts/VT323-Regular.ttf")) {
    std::cerr << "Failed to load font assets/fonts/VT323-Regular.ttf\n";
    return 1;
  }

  Game game;
  game.toMenu();
  Solver solver(game);

  // Create buttons
  // The pixel font should stay crisp rather than being filtered.
  font.setSmooth(false);

  Button menuStartButton(font, "START GAME", sf::Vector2f(220.f, 275.f),
                         sf::Vector2f(200.f, 50.f), 30);
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
  Button hudRestartButton(font, "R", sf::Vector2f(304.f, 20.f),
                          sf::Vector2f(32.f, 32.f), 26);
  hudRestartButton.shadowOffset = 3.f;
  // Solver keys: ">" plays one solver turn, ">>" runs the full solve.
  Button hudStepButton(font, ">", sf::Vector2f(264.f, 20.f),
                       sf::Vector2f(32.f, 32.f), 26);
  Button hudSolveButton(font, ">>", sf::Vector2f(344.f, 20.f),
                        sf::Vector2f(32.f, 32.f), 26);
  hudStepButton.shadowOffset = 3.f;
  hudSolveButton.shadowOffset = 3.f;
  Button gameOverRestartButton(font, "RESTART", sf::Vector2f(220.f, 240.f),
                               sf::Vector2f(200.f, 42.f), 28);
  Button gameOverMenuButton(font, "MENU", sf::Vector2f(220.f, 292.f),
                            sf::Vector2f(200.f, 42.f), 28);
  Button gameWonAgainButton(font, "PLAY AGAIN", sf::Vector2f(220.f, 240.f),
                            sf::Vector2f(200.f, 42.f), 28);
  Button gameWonMenuButton(font, "MENU", sf::Vector2f(220.f, 292.f),
                           sf::Vector2f(200.f, 42.f), 28);
  UI::styleRubber(menuStartButton);
  UI::styleRubber(hudRestartButton);
  UI::styleRubber(hudStepButton);
  UI::styleRubber(hudSolveButton);
  UI::styleScreenPrimary(gameOverRestartButton);
  UI::styleScreenSecondary(gameOverMenuButton);
  UI::styleScreenPrimary(gameWonAgainButton);
  UI::styleScreenSecondary(gameWonMenuButton);

  UI::layoutControls(window.getSize(), menuStartButton, columnsField,
                     rowsField, hudRestartButton, gameOverRestartButton,
                     gameOverMenuButton, gameWonAgainButton,
                     gameWonMenuButton);
  UI::layoutSolverControls(window.getSize(), hudStepButton, hudSolveButton);
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
    UI::layoutSolverControls(targetSize, hudStepButton, hudSolveButton);
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
        UI::layoutSolverControls(resized->size, hudStepButton,
                                 hudSolveButton);
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
          hudStepButton.setHovered(hudStepButton.contains(pos));
          hudSolveButton.setHovered(hudSolveButton.contains(pos));
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
          if (mouseClick->button == sf::Mouse::Button::Left &&
              (hudStepButton.contains(mousePos) ||
               hudSolveButton.contains(mousePos))) {
            if (hudStepButton.contains(mousePos))
              solver.playTurn();
            else
              solver.solve();
            // The solver plays through Game, so apply the same end-of-move
            // bookkeeping as a player click.
            game.checkAndHandleWin();
            if (game.state == GAME_OVER)
              game.elapsedSeconds = game.gameClock.getElapsedTime().asSeconds();
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

    window.clear(Palette::Body);
    UI::drawDevice(window, game.state != MENU);

    if (game.state == MENU) {
      // The menu is a boot screen: no board or HUD behind it.
      UI::drawMenu(window, font, menuStartButton, columnsField, rowsField, sizeError);
    } else {
      UI::drawHUD(window, font, game, hudRestartButton, hudStepButton,
                  hudSolveButton);
      UI::drawBoard(window, font, game);
    }

    if (game.state == GAME_OVER) {
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
