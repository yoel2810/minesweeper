#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Audio.hpp"
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
#include <string>
#include <vector>

// The window's title-bar and taskbar icon: the same .ico embedded in the exe
// (resource ID 1, see src/app.rc). SFML cannot read .ico files itself.
void applyWindowIcon(sf::RenderWindow &window) {
#ifdef _WIN32
  HWND hwnd = static_cast<HWND>(window.getNativeHandle());
  HINSTANCE instance = GetModuleHandleW(nullptr);
  HICON big = LoadIconW(instance, MAKEINTRESOURCEW(1));
  HICON small = static_cast<HICON>(LoadImageW(
      instance, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
  if (big)
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big));
  if (small)
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
                 reinterpret_cast<LPARAM>(small));
#else
  (void)window;
#endif
}

int main() {
  // Seed rand() so mine layouts differ between runs.
  std::srand(static_cast<unsigned>(std::time(nullptr)));

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(640, 480)), "Minesweeper",
                          sf::Style::Default);
  window.setFramerateLimit(60);
  window.setMinimumSize(sf::Vector2u(640, 480));
  applyWindowIcon(window);
  DisplaySettings displaySettings;

  sf::Font font;
  if (!font.openFromFile("assets/fonts/VT323-Regular.ttf")) {
    std::cerr << "Failed to load font assets/fonts/VT323-Regular.ttf\n";
    return 1;
  }

  Audio audio;
  Game game;
  game.toMenu();
  Solver solver(game);
  // ">>" plays the solver turn by turn, one turn every SOLVER_TURN_SECONDS, so
  // the solution can be followed. A second press (or any board click) stops it.
  constexpr float SOLVER_TURN_SECONDS = 0.15f;
  bool solverRunning = false;
  sf::Clock solverTimer;
  // Cell the solver acted on last; outlined on the board.
  std::optional<std::pair<int, int>> solverMark;
  // Cell the hint key pointed at; outlined on the board until the next move.
  std::optional<std::pair<int, int>> hintMark;
  // Set when the hint key found nothing provable; the key flashes until it fades.
  std::optional<sf::Clock> noHintSince;
  constexpr float NO_HINT_FLASH_SECONDS = 0.6f;
  // Shared end-of-move bookkeeping after the solver plays through Game.
  auto afterSolverTurn = [&]() {
    hintMark.reset();
    if (!solver.history.empty()) {
      const auto &last = solver.history.back();
      solverMark = std::make_pair(last.column, last.row);
    }
    game.checkAndHandleWin();
    if (game.state == GAME_OVER)
      game.elapsedSeconds = game.gameClock.getElapsedTime().asSeconds();
    if (game.state != PLAYING)
      solverRunning = false;
  };

  // Create buttons
  // The pixel font should stay crisp rather than being filtered.
  font.setSmooth(false);

  Button menuStartButton(font, "START GAME", sf::Vector2f(220.f, 275.f),
                         sf::Vector2f(200.f, 50.f), 30);
  NumberField columnsField(font, "Columns", {180.f, 190.f});
  NumberField rowsField(font, "Rows", {340.f, 190.f});
  // Difficulty presets fill the MINES % field; typing in it selects CUSTOM.
  NumberField minesField(font, "Mines %", {500.f, 190.f});
  Difficulty difficulty = Difficulty::Medium;
  std::vector<Button> difficultyButtons;
  difficultyButtons.reserve(DIFFICULTY_COUNT);
  for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    difficultyButtons.emplace_back(font,
                                   difficultyName(static_cast<Difficulty>(i)),
                                   sf::Vector2f(0.f, 0.f),
                                   UI::DIFFICULTY_BUTTON_SIZE, 24);
  std::string sizeError;
  auto selectDifficulty = [&](Difficulty selected) {
    difficulty = selected;
    if (selected != Difficulty::Custom)
      minesField.value = std::to_string(presetMinePercent(selected));
    UI::styleDifficultyButtons(difficultyButtons, difficulty);
  };
  selectDifficulty(difficulty);
  auto focusField = [&](NumberField *field) {
    columnsField.focus(field == &columnsField);
    rowsField.focus(field == &rowsField);
    minesField.focus(field == &minesField);
  };
  auto startGame = [&]() {
    const auto columns = parseBoardDimension(columnsField.value);
    const auto rows = parseBoardDimension(rowsField.value);
    if (!columns || !rows) {
      sizeError = "Enter columns and rows from 4 to 30.";
      return;
    }
    const auto minePercent = parseMinePercent(minesField.value);
    if (!minePercent) {
      sizeError = "Enter a mine percentage from 5 to 50.";
      return;
    }
    game.start(*columns, *rows, *minePercent);
    audio.play(Sfx::Click);
    sizeError.clear();
    focusField(nullptr);
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
  // Hint key: "?" marks one cell the solver can prove safe.
  Button hudHintButton(font, "?", sf::Vector2f(224.f, 20.f),
                       sf::Vector2f(32.f, 32.f), 26);
  hudHintButton.shadowOffset = 3.f;
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
  UI::styleRubber(hudHintButton);
  UI::styleScreenPrimary(gameOverRestartButton);
  UI::styleScreenSecondary(gameOverMenuButton);
  UI::styleScreenPrimary(gameWonAgainButton);
  UI::styleScreenSecondary(gameWonMenuButton);

  UI::layoutControls(window.getSize(), menuStartButton, columnsField,
                     rowsField, hudRestartButton, gameOverRestartButton,
                     gameOverMenuButton, gameWonAgainButton,
                     gameWonMenuButton);
  UI::layoutSolverControls(window.getSize(), hudHintButton, hudStepButton,
                           hudSolveButton);
  UI::layoutDifficultyControls(window.getSize(), minesField, difficultyButtons);
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
      window.setMinimumSize(sf::Vector2u(640, 480));
    window.setView(sf::View(sf::FloatRect({}, sf::Vector2f(targetSize))));
    UI::layoutControls(targetSize, menuStartButton, columnsField, rowsField,
                       hudRestartButton, gameOverRestartButton,
                       gameOverMenuButton, gameWonAgainButton,
                       gameWonMenuButton);
    UI::layoutSolverControls(targetSize, hudHintButton, hudStepButton,
                             hudSolveButton);
    UI::layoutDifficultyControls(targetSize, minesField, difficultyButtons);
  };

  GameState lastState = game.state;
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
        UI::layoutSolverControls(resized->size, hudHintButton, hudStepButton,
                                 hudSolveButton);
        UI::layoutDifficultyControls(resized->size, minesField,
                                     difficultyButtons);
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
          const auto previousMines = minesField.value;
          columnsField.enter(text->unicode);
          rowsField.enter(text->unicode);
          minesField.enter(text->unicode);
          if (minesField.value != previousMines)
            selectDifficulty(Difficulty::Custom);
          if (columnsField.value != previousColumns ||
              rowsField.value != previousRows || minesField.value != previousMines)
            sizeError.clear();
        }
        if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
          if (key->code == sf::Keyboard::Key::Tab) {
            // Cycle COLUMNS -> ROWS -> MINES %.
            if (columnsField.focused)
              focusField(&rowsField);
            else if (rowsField.focused)
              focusField(&minesField);
            else
              focusField(&columnsField);
          } else if (key->code == sf::Keyboard::Key::Enter) {
            startGame();
          }
          // Menu typing must not trigger gameplay shortcuts.
          continue;
        }
      }

      // Global key handling
      if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::M) {
          audio.toggleMute();
        } else if (key->code == sf::Keyboard::Key::R) {
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
          for (Button &button : difficultyButtons)
            button.setHovered(button.contains(pos));
        } else if (game.state == PLAYING) {
          hudRestartButton.setHovered(hudRestartButton.contains(pos));
          hudStepButton.setHovered(hudStepButton.contains(pos));
          hudSolveButton.setHovered(hudSolveButton.contains(pos));
          hudHintButton.setHovered(hudHintButton.contains(pos));
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
            minesField.focus(minesField.contains(mousePos));
            for (int i = 0; i < DIFFICULTY_COUNT; ++i) {
              if (!difficultyButtons[i].contains(mousePos))
                continue;
              const auto selected = static_cast<Difficulty>(i);
              selectDifficulty(selected);
              sizeError.clear();
              // CUSTOM jumps straight into the percentage field.
              focusField(selected == Difficulty::Custom ? &minesField : nullptr);
            }
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
              hudHintButton.contains(mousePos)) {
            hintMark = solver.findHint();
            noHintSince.reset();
            audio.play(hintMark ? Sfx::Hint : Sfx::NoHint);
            if (!hintMark)
              noHintSince = sf::Clock{};
            continue;
          }
          if (mouseClick->button == sf::Mouse::Button::Left &&
              hudStepButton.contains(mousePos)) {
            solverRunning = false;
            if (solver.playTurn())
              afterSolverTurn();
            continue;
          }
          if (mouseClick->button == sf::Mouse::Button::Left &&
              hudSolveButton.contains(mousePos)) {
            solverRunning = !solverRunning;
            solverTimer.restart();
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
          solverRunning = false; // a player click takes over from the solver
          solverMark.reset();
          hintMark.reset();

          if (mouseClick->button == sf::Mouse::Button::Left) {
            std::cout << "Left click detected at: " << col << ", " << row
                      << std::endl;
            // A click on an already revealed cell chords (see Game::chordReveal).
            const bool chord = game.grid[col][row].isRevealed;
            if (!chord) {
              if (game.grid[col][row].isFlag)
                continue;
              if (game.isFirstClick) {
                game.resetGame(col, row);
                game.isFirstClick = false;
              }
            }
            const int revealedBefore = game.revealedCount();
            if (chord)
              game.chordReveal(col, row);
            else
              game.openCells(col, row);
            game.checkAndHandleWin();
            if (game.state == PLAYING) {
              const int opened = game.revealedCount() - revealedBefore;
              if (opened > 1)
                audio.play(Sfx::Flood);
              else if (opened == 1)
                audio.play(Sfx::Reveal);
            }
            if (game.state == GAME_OVER) {
              game.elapsedSeconds = game.gameClock.getElapsedTime().asSeconds();
            } else if (game.state == GAME_WON) {
              // elapsed set in checkAndHandleWin
            }
          } else if (mouseClick->button == sf::Mouse::Button::Right) {
            std::cout << "Right click detected at: " << col << ", " << row
                      << std::endl;
            const int flagsBefore = game.flagCount();
            game.toggleFlag(col, row);
            if (game.flagCount() > flagsBefore)
              audio.play(Sfx::Flag);
            else if (game.flagCount() < flagsBefore)
              audio.play(Sfx::Unflag);
          }
        }
      }
    }

    // Win/lose sounds fire on the state change, whoever caused it (player or
    // solver).
    if (game.state != lastState) {
      if (game.state == GAME_OVER)
        audio.play(Sfx::Mine);
      else if (game.state == GAME_WON)
        audio.play(Sfx::Win);
      lastState = game.state;
    }

    // RESTARTING is instant, but handle transition if it was set directly
    if (game.state == RESTARTING) {
      game.toMenu();
    }

    // Animated solver: one turn per interval while ">>" is active.
    if (game.state != PLAYING) {
      solverRunning = false;
      solverMark.reset();
      hintMark.reset();
    }
    if (solverRunning &&
        solverTimer.getElapsedTime().asSeconds() >= SOLVER_TURN_SECONDS) {
      solverTimer.restart();
      if (solver.playTurn())
        afterSolverTurn();
      else
        solverRunning = false;
    }

    // While flashing, the hint key shows LCD colors (inverted) to say "no hint".
    const bool hintFlashing =
        noHintSince &&
        noHintSince->getElapsedTime().asSeconds() < NO_HINT_FLASH_SECONDS;
    UI::styleRubber(hudHintButton);
    if (hintFlashing) {
      hudHintButton.normal = Palette::Lightest;
      hudHintButton.hover = Palette::Lightest;
      hudHintButton.textColor = Palette::Darkest;
      hudHintButton.shadow = sf::Color::Transparent;
    }

    window.clear(Palette::Body);
    UI::drawDevice(window, game.state != MENU);

    if (game.state == MENU) {
      // The menu is a boot screen: no board or HUD behind it.
      UI::drawMenu(window, font, menuStartButton, columnsField, rowsField,
                   minesField, difficultyButtons, sizeError);
    } else {
      UI::drawHUD(window, font, game, hudRestartButton, hudHintButton,
                  hudStepButton, hudSolveButton);
      UI::drawBoard(window, font, game, solverMark, hintMark);
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

    if (game.state != MENU) {
      sf::Text soundLabel(font, audio.isMuted() ? "M: SOUND OFF" : "M: SOUND ON",
                          18);
      soundLabel.setFillColor(Palette::Darkest);
      const auto bounds = soundLabel.getLocalBounds();
      soundLabel.setPosition(
          {static_cast<float>(window.getSize().x) - bounds.size.x - 10.f,
           static_cast<float>(window.getSize().y) - bounds.size.y - 14.f});
      window.draw(soundLabel);
    }

    window.display();
  }
  return 0;
}
