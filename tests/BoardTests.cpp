#include "Game.hpp"
#include "BoardSettings.hpp"
#include "DisplaySettings.hpp"
#include "UI.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

void require(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}

int main() {
  sf::Font font;
  require(font.openFromFile("assets/fonts/DejaVuSans.ttf"), "Test font missing");
  NumberField field(font, "Columns", {180.f, 190.f});
  field.enter('7');
  require(field.value == "12", "Unfocused field accepts input");
  field.focus(true);
  field.enter('3');
  field.enter('0');
  require(field.value == "30", "Typing did not replace selected value");
  field.enter('9');
  field.enter('a');
  field.enter('\r');
  require(field.value == "30", "Field accepts excess digits or nonnumeric text");
  field.enter('\b');
  require(field.value == "3", "Backspace did not remove last digit");
  field.focus(false);
  field.focus(true);
  field.enter('\b');
  require(field.value.empty(), "Backspace did not clear selected value");
  field.enter('\b');
  require(!parseBoardDimension(field.value), "Empty field is valid");

  NumberField movableField(font, "Movable", {10.f, 20.f});
  movableField.setPosition({70.f, 90.f});
  require(movableField.shape.getPosition() == sf::Vector2f(70.f, 90.f) &&
              movableField.label.getPosition() == sf::Vector2f(70.f, 65.f) &&
              movableField.text.getPosition() == sf::Vector2f(82.f, 85.f),
          "Moving a number field separated its parts");

  Button startButton(font, "Start", {0.f, 0.f}, {200.f, 50.f});
  NumberField layoutColumns(font, "Columns", {0.f, 0.f});
  NumberField layoutRows(font, "Rows", {0.f, 0.f});
  Button hudButton(font, "R", {0.f, 0.f}, {32.f, 32.f});
  Button overRestart(font, "Restart", {0.f, 0.f}, {200.f, 42.f});
  Button overMenu(font, "Menu", {0.f, 0.f}, {200.f, 42.f});
  Button wonAgain(font, "Again", {0.f, 0.f}, {200.f, 42.f});
  Button wonMenu(font, "Menu", {0.f, 0.f}, {200.f, 42.f});
  Button hintButton(font, "?", {0.f, 0.f}, {32.f, 32.f});
  Button stepButton(font, ">", {0.f, 0.f}, {32.f, 32.f});
  Button solveButton(font, ">>", {0.f, 0.f}, {32.f, 32.f});
  NumberField layoutMines(font, "Mines %", {0.f, 0.f});
  std::vector<Button> difficultyButtons;
  for (int i = 0; i < DIFFICULTY_COUNT; ++i)
    difficultyButtons.emplace_back(font, difficultyName(static_cast<Difficulty>(i)),
                                   sf::Vector2f(), UI::DIFFICULTY_BUTTON_SIZE);
  for (const auto viewport : {sf::Vector2u(480, 360), sf::Vector2u(1200, 700)}) {
    UI::layoutControls(viewport, startButton, layoutColumns, layoutRows,
                       hudButton, overRestart, overMenu, wonAgain, wonMenu);
    UI::layoutSolverControls(viewport, hintButton, stepButton, solveButton);
    require(hintButton.shape.getPosition() ==
                    sf::Vector2f(static_cast<float>(viewport.x) / 2.f - 96.f, 20.f) &&
                hintButton.shape.getPosition().x < stepButton.shape.getPosition().x,
            "hint key is not left of the solver keys");
    UI::layoutDifficultyControls(viewport, layoutMines, difficultyButtons);
    const float centerX = static_cast<float>(viewport.x) / 2.f;
    const float centerY =
        HEADER_H + (static_cast<float>(viewport.y) - HEADER_H) / 2.f;
    require(startButton.shape.getPosition() ==
                sf::Vector2f(centerX - 100.f, centerY + 32.f),
            "Start button is not centered after resize");
    require(layoutColumns.shape.getPosition() ==
                    sf::Vector2f(centerX - 200.f, centerY - 84.f) &&
                layoutRows.shape.getPosition() ==
                    sf::Vector2f(centerX - 60.f, centerY - 84.f) &&
                layoutMines.shape.getPosition() ==
                    sf::Vector2f(centerX + 80.f, centerY - 84.f),
            "Menu fields are not centered after resize");
    const float rowLeft = difficultyButtons.front().shape.getPosition().x;
    const auto &last = difficultyButtons.back().shape;
    require(rowLeft >= 0.f &&
                last.getPosition().x + last.getSize().x <=
                    static_cast<float>(viewport.x) &&
                rowLeft - centerX ==
                    centerX - (last.getPosition().x + last.getSize().x),
            "Difficulty row is not centered or does not fit");
    for (std::size_t i = 1; i < difficultyButtons.size(); ++i)
      require(difficultyButtons[i].shape.getPosition().y == centerY - 32.f &&
                  difficultyButtons[i].shape.getPosition().x >
                      difficultyButtons[i - 1].shape.getPosition().x +
                          UI::DIFFICULTY_BUTTON_SIZE.x,
              "Difficulty buttons overlap or are not in one row");
    require(hudButton.shape.getPosition() == sf::Vector2f(centerX - 16.f, 20.f),
            "HUD button is not centered after resize");
    require(stepButton.shape.getPosition() == sf::Vector2f(centerX - 56.f, 20.f) &&
                solveButton.shape.getPosition() ==
                    sf::Vector2f(centerX + 24.f, 20.f),
            "Solver buttons do not flank the HUD restart button");
    require(overRestart.shape.getPosition() ==
                    sf::Vector2f(centerX - 100.f, centerY + 10.f) &&
                overMenu.shape.getPosition() ==
                    sf::Vector2f(centerX - 100.f, centerY + 62.f) &&
                wonAgain.shape.getPosition() == overRestart.shape.getPosition() &&
                wonMenu.shape.getPosition() == overMenu.shape.getPosition(),
            "Terminal controls are not centered after resize");
  }

  {
    // A resized button keeps its label centered.
    Button resized(font, "OK", {10.f, 10.f}, {40.f, 20.f});
    resized.setSize({200.f, 60.f});
    const sf::FloatRect label = resized.text.getGlobalBounds();
    const float labelCenter = label.position.x + label.size.x / 2.f;
    require(std::abs(labelCenter - 110.f) <= 1.f,
            "Resized button label is not centered");
  }

  {
    // Hover only follows the buttons of the current screen; leaving a
    // screen un-hovers its buttons.
    Controls controls(font);
    controls.relayout({640, 480});
    const auto onHint = sf::Vector2i(controls.hudHint.shape.getPosition()) +
                        sf::Vector2i(5, 5);
    controls.updateHover(GameState::PLAYING, onHint);
    require(controls.hudHint.hovered, "HUD button not hovered while playing");
    controls.updateHover(GameState::MENU, onHint);
    require(!controls.hudHint.hovered, "Hidden HUD button stayed hovered");
  }

  {
    // A degenerate window still gives a usable cell size.
    const auto tiny = BoardLayout::fit(30, 30, 10, 10);
    require(tiny.cellSize >= 1, "Cell size dropped to zero");
  }

  DisplaySettings display;
  display.toggle({900, 600});
  require(display.fullscreen && display.windowedSize == sf::Vector2u(900, 600) &&
              display.targetSize({1920, 1080}) == sf::Vector2u(1920, 1080),
          "Entering fullscreen did not remember the windowed size");
  display.toggle({1920, 1080});
  require(!display.fullscreen &&
              display.targetSize({1920, 1080}) == sf::Vector2u(900, 600),
          "Leaving fullscreen did not restore the windowed size");
  display.toggle({1024, 768});
  require(display.fullscreen &&
              display.windowedSize == sf::Vector2u(1024, 768),
          "Later fullscreen toggle did not update the remembered size");
  require(display.windowStyle() == sf::Style::None &&
              display.windowState() == sf::State::Windowed,
          "Fullscreen configuration is not a borderless desktop window");

  DisplaySettings toggleLatch;
  require(toggleLatch.pressToggleKey(), "First F11 press was ignored");
  require(!toggleLatch.pressToggleKey(), "Repeated F11 press toggled again");
  toggleLatch.releaseToggleKey();
  require(toggleLatch.pressToggleKey(), "F11 did not rearm after release");

  for (const auto &value : {"", "0", "3", "31", "9999999999", "a", "-4", "4.5"})
    require(!parseBoardDimension(value), "Invalid dimension accepted");
  require(parseBoardDimension("4") == 4, "Minimum rejected");
  require(parseBoardDimension("30") == 30, "Maximum rejected");

  for (const auto &value : {"", "0", "4", "51", "99", "a", "-5", "100"})
    require(!parseMinePercent(value), "Invalid mine percentage accepted");
  require(parseMinePercent("5") == 5 && parseMinePercent("50") == 50,
          "Mine percentage bounds rejected");
  require(presetMinePercent(Difficulty::Easy) < presetMinePercent(Difficulty::Medium) &&
              presetMinePercent(Difficulty::Medium) < presetMinePercent(Difficulty::Hard),
          "Difficulty presets are not increasing");
  for (const auto preset : {Difficulty::Easy, Difficulty::Medium, Difficulty::Hard})
    require(parseMinePercent(std::to_string(presetMinePercent(preset))).has_value(),
            "Difficulty preset outside the custom range");
  require(presetMinePercent(Difficulty::Medium) == DEFAULT_MINE_PERCENT,
          "Default density is not Medium");

  {
    Game densityGame;
    require(!densityGame.start(12, 12, MIN_MINE_PERCENT - 1) &&
                !densityGame.start(12, 12, MAX_MINE_PERCENT + 1),
            "Invalid mine percentage accepted by Game::start");
    // Exact mine counts: percentage of all cells, rounded.
    for (const auto [columns, rows, percent, expected] :
         {std::tuple{10, 10, 10, 10}, std::tuple{10, 10, 15, 15},
          std::tuple{10, 10, 20, 20}, std::tuple{30, 16, 21, 101},
          std::tuple{30, 30, 50, 450}, std::tuple{4, 4, 5, 1}}) {
      require(densityGame.start(columns, rows, percent), "Valid density rejected");
      require(densityGame.minePercent() == percent, "Density not stored");
      require(densityGame.mineCount() == expected,
              "Mine counter before the first click differs from the placed count");
      densityGame.placeMines(0, 0);
      require(densityGame.mineCount() == expected, "Wrong number of mines placed");
      require(!densityGame.cell(0, 0).isMine && !densityGame.cell(1, 1).isMine,
              "Mine placed in the safe opening");
    }
    // A tiny dense board caps mines at the cells outside a 3x3 opening, the
    // same count wherever the first click lands.
    for (const auto click : {std::pair{0, 0}, std::pair{1, 1}, std::pair{3, 0}}) {
      require(densityGame.start(4, 4, 50), "Dense tiny board rejected");
      require(densityGame.mineCount() == 7, "Dense board count not shown up front");
      densityGame.placeMines(click.first, click.second);
      require(densityGame.mineCount() == 7, "Dense board not capped to free cells");
    }
  }

  {
    // Same seed, same board; a different seed gives a different board.
    auto layoutFor = [](std::uint32_t seed) {
      Game seeded;
      seeded.seed(seed);
      seeded.start(16, 16);
      seeded.placeMines(8, 8);
      std::vector<bool> mines;
      for (int x = 0; x < 16; ++x)
        for (int y = 0; y < 16; ++y)
          mines.push_back(seeded.cell(x, y).isMine);
      return mines;
    };
    require(layoutFor(7) == layoutFor(7), "Seeded mine layout not reproducible");
    require(layoutFor(7) != layoutFor(8), "Seed does not change the layout");
  }

  {
    // Restart keeps the board settings and starts a fresh game.
    Game restartGame;
    restartGame.start(9, 7, 20);
    const auto firstGame = restartGame.gameNumber();
    restartGame.reveal(4, 3);
    restartGame.restart();
    require(restartGame.state() == GameState::PLAYING, "Restart left PLAYING");
    require(restartGame.columns() == 9 && restartGame.rows() == 7 &&
                restartGame.minePercent() == 20,
            "Restart lost the board settings");
    require(!restartGame.minesPlaced() && restartGame.revealedCount() == 0,
            "Restart kept the old board");
    require(restartGame.gameNumber() != firstGame, "Restart is not a new game");
    restartGame.toMenu();
    require(restartGame.state() == GameState::MENU, "toMenu did not reach MENU");
  }

  {
    // Pausing freezes the timer and refuses moves; resuming continues.
    Game paused;
    paused.start(9, 7, 20);
    require(!paused.resume(), "resume worked while not paused");
    paused.reveal(4, 3);
    require(paused.pause() && paused.state() == GameState::PAUSED,
            "pause did not reach PAUSED");
    require(!paused.pause(), "pause worked twice");
    const float frozen = paused.elapsedSeconds();
    const int revealed = paused.revealedCount();
    require(paused.reveal(0, 0) == 0 && !paused.toggleFlag(0, 0) &&
                paused.revealedCount() == revealed,
            "a move was accepted while paused");
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    require(paused.elapsedSeconds() == frozen, "timer ran while paused");
    require(paused.resume() && paused.state() == GameState::PLAYING,
            "resume did not reach PLAYING");
    require(paused.elapsedSeconds() >= frozen &&
                paused.elapsedSeconds() < frozen + 0.05f,
            "resume lost or gained time");
  }

  {
    // Nothing can be flagged before the first click, and the timer waits for it.
    Game early;
    early.start(8, 8);
    require(!early.toggleFlag(2, 2) && early.flagCount() == 0,
            "Flag placed before the first click");
    require(early.elapsedSeconds() == 0.f, "Timer runs before the first click");
    require(early.reveal(0, 0) > 0 && early.minesPlaced(),
            "First click did not place mines and open cells");
    require(early.cell(0, 0).minesCount == 0, "First click is not an opening");
  }

  {
    // Losing records the exploded cell and stops the timer.
    Game lose;
    lose.start(4, 4);
    lose.loadMines({{3, 3}});
    lose.reveal(3, 3);
    require(lose.state() == GameState::GAME_OVER, "Mine did not end the game");
    require(lose.explodedColumn() == 3 && lose.explodedRow() == 3,
            "Exploded cell not recorded");
    const float frozen = lose.elapsedSeconds();
    require(lose.elapsedSeconds() == frozen, "Timer still running after a loss");
    require(lose.reveal(0, 0) == 0, "Reveal accepted after the game ended");
  }

  Game game;
  require(game.columns() == 12 && game.rows() == 12, "Wrong default size");
  require(!game.start(3, 12) && !game.start(12, 31), "Invalid game size accepted");
  for (const auto size : {std::pair{4, 4}, std::pair{4, 30},
                          std::pair{30, 4}, std::pair{12, 12}, std::pair{30, 30}}) {
    const auto [columns, rows] = size;
    require(game.start(columns, rows), "Valid game size rejected");
    require(static_cast<int>(game.grid().size()) == columns &&
                static_cast<int>(game.grid()[0].size()) == rows,
            "Storage dimensions do not match");
    game.placeMines(columns - 1, rows - 1);
    require(!game.cell(columns - 1, rows - 1).isMine, "First click is unsafe");
    int mines = 0;
    for (int x = 0; x < columns; ++x) {
      for (int y = 0; y < rows; ++y) {
        const Cell &cell = game.cell(x, y);
        if (cell.isMine) { ++mines; continue; }
        int neighbors = 0;
        for (int nx = x - 1; nx <= x + 1; ++nx)
          for (int ny = y - 1; ny <= y + 1; ++ny)
            if (nx >= 0 && nx < columns && ny >= 0 && ny < rows)
              neighbors += game.cell(nx, ny).isMine;
        require(cell.minesCount == neighbors, "Incorrect adjacent mine count");
      }
    }
    require(game.mineCount() == mines, "Incorrect mine count");

    // Open the safe cells one at a time: the game is won exactly when the
    // last one opens, never before.
    for (int x = 0; x < columns; ++x)
      for (int y = 0; y < rows; ++y) {
        if (game.cell(x, y).isMine || game.cell(x, y).isRevealed)
          continue;
        require(game.state() == GameState::PLAYING, "Win ignores unopened cells");
        game.reveal(x, y);
        require((game.state() == GameState::GAME_WON) == game.checkWinCondition(),
                "Win state does not match the board");
      }
    require(game.state() == GameState::GAME_WON, "Win not detected");

    game.restart();
    require(game.columns() == columns && game.rows() == rows, "Restart lost size");
    require(game.revealedCount() == 0 && game.flagCount() == 0,
            "Restart did not clear board");
    game.reveal(-1, 0);
    game.toggleFlag(columns, rows);
    game.reveal(0, 0);
    require(game.toggleFlag(columns - 1, rows - 1) ||
                game.cell(columns - 1, rows - 1).isRevealed,
            "Cannot flag final cell");
    if (game.cell(columns - 1, rows - 1).isFlag) {
      const int before = game.revealedCount();
      require(game.reveal(columns - 1, rows - 1) == 0 &&
                  game.revealedCount() == before,
              "Revealed flagged cell");
    }

    // Chord: a revealed 1 with its single mine flagged opens the rest.
    if (columns == 4 && rows == 4) {
      game.start(4, 4);
      game.loadMines({{0, 0}});
      require(game.reveal(1, 1) == 1, "Number cell did not open alone");
      require(game.reveal(1, 1) == 0, "Chord fired without flags");
      game.toggleFlag(0, 0);
      require(game.reveal(1, 1) == 14, "Chord ignored matching flags");
      require(game.revealedCount() == 15 && !game.cell(0, 0).isRevealed,
              "Chord revealed wrong cells");
      require(game.state() == GameState::GAME_WON, "Safe chord did not win");

      // A wrong flag makes the chord hit the mine.
      game.start(4, 4);
      game.loadMines({{0, 0}});
      game.reveal(1, 1);
      game.toggleFlag(2, 0);
      game.reveal(1, 1);
      require(game.state() == GameState::GAME_OVER &&
                  game.explodedColumn() == 0 && game.explodedRow() == 0,
              "Chord with a wrong flag did not explode the mine");
      game.start(4, 4);
    }


    for (const auto viewport : {std::pair{480u, 360u}, std::pair{640u, 480u},
                                std::pair{1200u, 700u}, std::pair{700u, 1200u}}) {
      const auto [width, height] = viewport;
      const auto layout = BoardLayout::fit(columns, rows, width, height);
      require(layout.cellSize <= 32 && layout.cellSize > 0, "Invalid cell size");
      require(layout.x >= 0 && layout.y >= HEADER_H &&
                  layout.x + columns * layout.cellSize <= static_cast<int>(width) &&
                  layout.y + rows * layout.cellSize <= static_cast<int>(height),
              "Board does not fit resized viewport");
      require(layout.x ==
                  (static_cast<int>(width) - columns * layout.cellSize) / 2 &&
                  layout.y == HEADER_H +
                      (static_cast<int>(height) - HEADER_H -
                       rows * layout.cellSize) /
                          2,
              "Board is not centered in resized viewport");
      require(!layout.cellAt(layout.x - 1, layout.y) &&
                  !layout.cellAt(layout.x, layout.y - 1) &&
                  !layout.cellAt(layout.x + columns * layout.cellSize, layout.y) &&
                  !layout.cellAt(layout.x, layout.y + rows * layout.cellSize),
              "Outside click accepted");
      require(layout.cellAt(layout.x, layout.y) == std::pair{0, 0},
              "First cell missed");
      require(layout.cellAt(layout.x + columns * layout.cellSize - 1,
                            layout.y + rows * layout.cellSize - 1) ==
                  std::pair{columns - 1, rows - 1},
              "Last cell missed");
    }
  }
  std::cout << "Board checks passed\n";
}
