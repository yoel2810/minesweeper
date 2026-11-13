#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "App.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <system_error>
#include <tuple>
#include <vector>

namespace {
constexpr char FONT_FILE[] = "assets/fonts/VT323-Regular.ttf";
const sf::Vector2u MIN_WINDOW_SIZE{640, 480};

std::filesystem::path executableDir() {
#ifdef _WIN32
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0)
      return {};
    if (length < buffer.size()) {
      buffer.resize(length);
      return std::filesystem::path(buffer).parent_path();
    }
    buffer.resize(buffer.size() * 2);
  }
#else
  std::error_code error;
  const auto exe = std::filesystem::read_symlink("/proc/self/exe", error);
  return error ? std::filesystem::path() : exe.parent_path();
#endif
}

// Finds a bundled asset wherever the game is started from: next to the exe
// (installed), the working directory, or the repo root above build/bin or
// build-vs/bin/<Config>.
std::filesystem::path findAsset(const std::filesystem::path &relative) {
  const auto exeDir = executableDir();
  std::vector<std::filesystem::path> roots;
  if (!exeDir.empty())
    roots = {exeDir, exeDir / "..", exeDir / ".." / "..", exeDir / ".." / ".." / ".."};
  roots.push_back(std::filesystem::current_path());
  std::error_code error;
  for (const auto &root : roots)
    if (std::filesystem::exists(root / relative, error))
      return root / relative;
  return relative;
}

// The window's title-bar and taskbar icon: the same .ico embedded in the exe
// (resource ID 1, see src/app.rc). SFML cannot read .ico files itself.
void applyWindowIcon(sf::RenderWindow &window) {
#ifdef _WIN32
  HWND hwnd = static_cast<HWND>(window.getNativeHandle());
  HINSTANCE instance = GetModuleHandleW(nullptr);
  // LR_SHARED: the system owns the icons, so they never need destroying.
  for (const auto [kind, metricX, metricY] :
       {std::tuple{ICON_BIG, SM_CXICON, SM_CYICON},
        std::tuple{ICON_SMALL, SM_CXSMICON, SM_CYSMICON}}) {
    const HANDLE icon = LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                   GetSystemMetrics(metricX),
                                   GetSystemMetrics(metricY), LR_SHARED);
    if (icon)
      SendMessageW(hwnd, WM_SETICON, kind, reinterpret_cast<LPARAM>(icon));
  }
#else
  (void)window;
#endif
}
} // namespace

App::App() { game.seed(std::random_device{}()); }

int App::run() {
  const auto fontPath = findAsset(FONT_FILE);
  if (!font.openFromFile(fontPath)) {
    std::cerr << "Failed to load font " << fontPath.string() << '\n';
    return 1;
  }
  font.setSmooth(false); // the pixel font should stay crisp
  controls.emplace(font);
  selectDifficulty(difficulty);
  createWindow(display.windowedSize);

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent())
      handleEvent(*event);
    update();
    draw();
  }
  return 0;
}

// ------------------------------------------------------------------ window

void App::createWindow(sf::Vector2u size) {
  window.create(sf::VideoMode(size), "Minesweeper", display.windowStyle(),
                display.windowState());
  window.setFramerateLimit(60);
  if (display.fullscreen)
    window.setPosition({0, 0});
  else
    window.setMinimumSize(MIN_WINDOW_SIZE);
  applyWindowIcon(window); // a recreated window loses its icon
  window.setView(sf::View(sf::FloatRect({}, sf::Vector2f(size))));
  controls->relayout(size);
}

void App::toggleFullscreen() {
  display.toggle(window.getSize());
  createWindow(display.targetSize(sf::VideoMode::getDesktopMode().size));
}

// ------------------------------------------------------------------ input

void App::handleEvent(const sf::Event &event) {
  if (event.is<sf::Event::Closed>()) {
    window.close();
    return;
  }
  if (const auto *resized = event.getIf<sf::Event::Resized>()) {
    window.setView(sf::View(sf::FloatRect({}, sf::Vector2f(resized->size))));
    controls->relayout(resized->size);
    return;
  }
  if (const auto *key = event.getIf<sf::Event::KeyPressed>();
      key && key->code == sf::Keyboard::Key::F11) {
    if (display.pressToggleKey())
      toggleFullscreen();
    return;
  }
  if (const auto *key = event.getIf<sf::Event::KeyReleased>();
      key && key->code == sf::Keyboard::Key::F11) {
    display.releaseToggleKey();
    return;
  }

  if (game.state() == GameState::MENU) {
    handleMenuEvent(event);
    return;
  }
  if (const auto *key = event.getIf<sf::Event::KeyPressed>())
    handleKey(key->code);
  if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>())
    handleClick(click->button, click->position);
}

void App::handleMenuEvent(const sf::Event &event) {
  Controls &c = *controls;
  if (const auto *text = event.getIf<sf::Event::TextEntered>()) {
    const auto before = c.columns.value + '/' + c.rows.value + '/' + c.mines.value;
    const auto minesBefore = c.mines.value;
    c.columns.enter(text->unicode);
    c.rows.enter(text->unicode);
    c.mines.enter(text->unicode);
    if (c.mines.value != minesBefore)
      selectDifficulty(Difficulty::Custom);
    if (c.columns.value + '/' + c.rows.value + '/' + c.mines.value != before)
      menuError.clear();
  } else if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
    // Menu typing must not trigger gameplay shortcuts.
    if (key->code == sf::Keyboard::Key::Tab) { // COLUMNS -> ROWS -> MINES %
      if (c.columns.focused)
        c.focusField(&c.rows);
      else if (c.rows.focused)
        c.focusField(&c.mines);
      else
        c.focusField(&c.columns);
    } else if (key->code == sf::Keyboard::Key::Enter) {
      startGame();
    }
  } else if (const auto *click = event.getIf<sf::Event::MouseButtonPressed>();
             click && click->button == sf::Mouse::Button::Left) {
    const sf::Vector2i at = click->position;
    c.columns.focus(c.columns.contains(at));
    c.rows.focus(c.rows.contains(at));
    c.mines.focus(c.mines.contains(at));
    for (int i = 0; i < DIFFICULTY_COUNT; ++i) {
      if (!c.difficulty[i].contains(at))
        continue;
      const auto selected = static_cast<Difficulty>(i);
      selectDifficulty(selected);
      menuError.clear();
      // CUSTOM jumps straight into the percentage field.
      c.focusField(selected == Difficulty::Custom ? &c.mines : nullptr);
    }
    if (c.menuStart.contains(at))
      startGame();
  }
}

void App::handleKey(sf::Keyboard::Key key) {
  switch (key) {
  case sf::Keyboard::Key::M:
    audio.toggleMute();
    break;
  case sf::Keyboard::Key::P:
    togglePause();
    break;
  case sf::Keyboard::Key::R:
    game.restart();
    break;
  case sf::Keyboard::Key::Escape:
    game.toMenu();
    break;
  default:
    break;
  }
}

void App::handleClick(sf::Mouse::Button button, sf::Vector2i position) {
  const Controls &c = *controls;
  const bool left = button == sf::Mouse::Button::Left;
  if (left && game.state() != GameState::MENU && c.hudMute.contains(position)) {
    audio.toggleMute();
    return;
  }
  if (left && c.hudPause.contains(position) &&
      (game.state() == GameState::PLAYING || game.state() == GameState::PAUSED)) {
    togglePause();
    return;
  }
  switch (game.state()) {
  case GameState::PAUSED:
    if (left && c.hudRestart.contains(position))
      game.restart();
    break;
  case GameState::PLAYING:
    if (left && c.hudRestart.contains(position)) {
      game.restart();
    } else if (left && c.hudHint.contains(position)) {
      requestHint();
    } else if (left && c.hudStep.contains(position)) {
      solverRunning = false;
      requestSolverTurn();
    } else if (left && c.hudSolve.contains(position)) {
      solverRunning = !solverRunning;
      solverTimer.restart();
    } else {
      handleBoardClick(button, position);
    }
    break;
  case GameState::GAME_OVER:
  case GameState::GAME_WON: {
    const bool lost = game.state() == GameState::GAME_OVER;
    if (left && (lost ? c.overRestart : c.wonAgain).contains(position))
      game.restart();
    else if (left && (lost ? c.overMenu : c.wonMenu).contains(position))
      game.toMenu();
    break;
  }
  case GameState::MENU:
    break;
  }
}

void App::handleBoardClick(sf::Mouse::Button button, sf::Vector2i position) {
  const auto size = window.getSize();
  const auto layout = BoardLayout::fit(game.columns(), game.rows(), size.x, size.y);
  const auto cell = layout.cellAt(position.x, position.y);
  if (!cell)
    return;
  const auto [column, row] = *cell;
  solverRunning = false; // a player click takes over from the solver
  clearMarks();

  if (button == sf::Mouse::Button::Left) {
    const int opened = game.reveal(column, row);
    if (game.state() == GameState::PLAYING && opened > 0)
      audio.play(opened > 1 ? Sfx::Flood : Sfx::Reveal);
  } else if (button == sf::Mouse::Button::Right) {
    if (game.toggleFlag(column, row))
      audio.play(game.cell(column, row).isFlag ? Sfx::Flag : Sfx::Unflag);
  }
}

// ------------------------------------------------------------------ actions

void App::togglePause() {
  if (!game.pause())
    game.resume();
  audio.play(Sfx::Click);
}

void App::selectDifficulty(Difficulty selected) {
  difficulty = selected;
  if (selected != Difficulty::Custom)
    controls->mines.value = std::to_string(presetMinePercent(selected));
  UI::styleDifficultyButtons(controls->difficulty, difficulty);
}

void App::startGame() {
  Controls &c = *controls;
  const auto columns = parseBoardDimension(c.columns.value);
  const auto rows = parseBoardDimension(c.rows.value);
  if (!columns || !rows) {
    menuError = "Enter columns and rows from 4 to 30.";
    return;
  }
  const auto minePercent = parseMinePercent(c.mines.value);
  if (!minePercent) {
    menuError = "Enter a mine percentage from 5 to 50.";
    return;
  }
  game.start(*columns, *rows, *minePercent);
  audio.play(Sfx::Click);
  menuError.clear();
  c.focusField(nullptr);
}

namespace {
template <class T> bool isReady(const std::future<T> &future) {
  return future.valid() &&
         future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
} // namespace

void App::requestHint() {
  if (pendingHint.valid() || game.state() != GameState::PLAYING)
    return;
  if (!game.minesPlaced()) { // nothing to think about: the corner is safe
    hintMark = CellPos{0, 0};
    noHintSince.reset();
    audio.play(Sfx::Hint);
    return;
  }
  hintBasis = solver.snapshot();
  hintSkip = solver.flaggedCells();
  pendingHint = std::async(std::launch::async,
                           [s = hintBasis, skip = hintSkip] {
                             return Solver::stepHint(s, skip);
                           });
}

void App::requestSolverTurn() {
  if (pendingPlan.valid() || game.state() != GameState::PLAYING)
    return;
  planBasis = solver.snapshot();
  pendingPlan = std::async(std::launch::async,
                           [s = planBasis] { return Solver::plan(s); });
}

void App::collectSolverWork() {
  if (isReady(pendingHint)) {
    const int id = pendingHint.get();
    if (game.state() == GameState::PLAYING && solver.snapshot() == hintBasis &&
        solver.flaggedCells() == hintSkip) {
      if (id >= 0)
        hintMark = CellPos{hintBasis.column(id), hintBasis.row(id)};
      else
        hintMark.reset();
      audio.play(hintMark ? Sfx::Hint : Sfx::NoHint);
      if (hintMark)
        noHintSince.reset();
      else
        noHintSince = sf::Clock{};
    }
  }
  if (isReady(pendingPlan)) {
    const Solver::Plan plan = pendingPlan.get();
    if (game.state() != GameState::PLAYING || solver.snapshot() != planBasis)
      return; // the board changed while the solver was thinking
    if (!solver.applyPlan(plan)) {
      solverRunning = false;
      return;
    }
    hintMark.reset();
    if (!solver.history.empty())
      solverMark = CellPos{solver.history.back().column, solver.history.back().row};
  }
}

void App::clearMarks() {
  solverMark.reset();
  hintMark.reset();
}

// ------------------------------------------------------------------ frame

void App::update() {
  // Win/lose sounds fire on the state change, whoever caused it. Any state
  // change or new game also stops the solver and clears the board marks.
  if (game.state() != lastState || game.gameNumber() != lastGame) {
    if (game.state() == GameState::GAME_OVER)
      audio.play(Sfx::Mine);
    else if (game.state() == GameState::GAME_WON)
      audio.play(Sfx::Win);
    lastState = game.state();
    lastGame = game.gameNumber();
    solverRunning = false;
    clearMarks();
  }

  collectSolverWork();
  if (solverRunning && !pendingPlan.valid() &&
      solverTimer.getElapsedTime().asSeconds() >= SOLVER_TURN_SECONDS) {
    solverTimer.restart();
    requestSolverTurn();
  }

  controls->updateHover(game.state(), sf::Mouse::getPosition(window));

  // While flashing, the hint key shows LCD colors (inverted) to say "no hint".
  Button &hint = controls->hudHint;
  UI::styleRubber(hint);
  if (noHintSince &&
      noHintSince->getElapsedTime().asSeconds() < NO_HINT_FLASH_SECONDS) {
    hint.normal = hint.hover = Palette::Lightest;
    hint.textColor = Palette::Darkest;
    hint.shadow = sf::Color::Transparent;
  }
}

void App::draw() {
  window.clear(Palette::Body);
  UI::drawDevice(window, game.state() != GameState::MENU);

  switch (game.state()) {
  case GameState::MENU:
    // The menu is a boot screen: no board or HUD behind it.
    UI::drawMenu(window, font, *controls, menuError);
    break;
  case GameState::PLAYING:
  case GameState::PAUSED:
  case GameState::GAME_OVER:
  case GameState::GAME_WON:
    UI::drawHUD(window, font, game, *controls, audio.isMuted());
    if (game.state() == GameState::PAUSED)
      UI::drawPaused(window, font, game);
    else
      UI::drawBoard(window, font, game, solverMark, hintMark);
    if (game.state() == GameState::GAME_OVER)
      UI::drawGameOver(window, font, game, *controls);
    else if (game.state() == GameState::GAME_WON)
      UI::drawGameWon(window, font, game, *controls);
    break;
  }

  window.display();
}
