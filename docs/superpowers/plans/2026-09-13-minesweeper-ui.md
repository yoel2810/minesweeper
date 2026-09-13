# Minesweeper UI States Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add full UI state machine (MENU/Start Game, PLAYING HUD, GAME_OVER, GAME_WON, RESTARTING) with visual overlays and restart/menu navigation.

**Architecture:** Extract game logic from `src/main.cpp:157-207` into `Game` class, create new `UI` module (`src/UI.hpp/cpp`) using pure SFML 3.1 primitives (`sf::Text`, `sf::RectangleShape`, `sf::Font::openFromFile`). Keep `Cell` as data-only, centralize state in `enum GameState` at `src/main.cpp:18`. No ImGui/new deps. Add vendored font to `assets/fonts/`.

**Tech Stack:** C++17, SFML 3.1 (already via `CMakeLists.txt:8-14`), Ninja, 12x12 grid 32px (`src/main.cpp:15-16`)

**Spec:** User request: “add UI to the game? Start Game, Game Over, Game Won, Restarting, etc…” — confirmed choices: Layout A (640x480, board centered, 48px header), HUD with timer+mine counter, instant restart, font vendored as DejaVuSans.

## Global Constraints

- C++17 (`CMakeLists.txt:17`)
- SFML 3.1 via FetchContent (`CMakeLists.txt:10`)
- Window 640x480 `src/main.cpp:158`, board 384x384 (12*32), header 48px, offsetX=(640-384)/2=128, offsetY=48
- Asset path relative: `assets/images/*.png` (`src/main.cpp:22-34`), new `assets/fonts/DejaVuSans.ttf`
- Keep texture dict pattern `src/main.cpp:22`
- No new runtime deps beyond SFML
- `isFirstClick` bug fix: must reset on restart/menu
- Confirm SFML 3 API: `font.openFromFile()`, `event.getIf<sf::Event::MouseButtonPressed>()`, `event.getIf<sf::Event::KeyPressed>()`

---

### Task 1: Font Asset + CMake + Resource Loading

**Files:**
- Create: `assets/fonts/DejaVuSans.ttf`
- Modify: `CMakeLists.txt:16`
- Modify: `src/main.cpp:1-10` (add font loading)

**Interfaces:**
- Consumes: existing texture loading `src/main.cpp:22`
- Produces: `sf::Font globalFont` loaded via `font.openFromFile("assets/fonts/DejaVuSans.ttf")` consumed by UI tasks

- [ ] **Step 1: Add font file**

  Download DejaVuSans.ttf (OFL) and place at `assets/fonts/DejaVuSans.ttf`. If network blocked, copy from system or create placeholder. Verify with `Test-Path assets/fonts/DejaVuSans.ttf`.

- [ ] **Step 2: Update CMakeLists.txt**

  ```cmake
  add_executable(minesweeper src/main.cpp src/Cell.cpp src/Game.cpp src/UI.cpp)
  ```

  Currently `CMakeLists.txt:16` is `add_executable(minesweeper src/main.cpp src/Cell.cpp)`.

- [ ] **Step 3: Add font loading in main.cpp**

  ```cpp
  sf::Font font;
  if (!font.openFromFile("assets/fonts/DejaVuSans.ttf")) {
    std::cerr << "Failed to load font\n";
    return 1;
  }
  ```

  Add near top of `main()` after window creation `src/main.cpp:158`.

- [ ] **Step 4: Build verify**

  Run: `cmake -S . -B build -G Ninja; cmake --build build`
  Expected: build succeeds, exe at `build/bin/minesweeper.exe`

- [ ] **Step 5: Commit**

  ```bash
  git add assets/fonts/DejaVuSans.ttf CMakeLists.txt src/main.cpp
  git commit -m "feat: add font asset and resource loading"
  ```

### Task 2: Extract Game State Machine (Refactor)

**Files:**
- Create: `src/Game.hpp`
- Create: `src/Game.cpp`
- Modify: `src/main.cpp:18-50`, `src/main.cpp:157-207`

**Interfaces:**
- Consumes: `Cell.hpp:3` (`Cell { minesCount, isRevealed, isMine, isFlag }`)
- Produces: `class Game { GameState state; Cell grid[GRID_SIZE][GRID_SIZE]; bool isFirstClick; sf::Clock gameClock; sf::Time elapsed; void reset(int firstI,int firstJ); void restart(); void toMenu(); void open(int i,int j); bool checkWin(); void toggleFlag(int i,int j); int flagCount() const; int mineCount() const; }` — consumed by all later UI tasks

- [ ] **Step 1: Create Game.hpp**

  ```cpp
  #pragma once
  #include "Cell.hpp"
  #include <SFML/System/Clock.hpp>
  #define GRID_SIZE 12 // or include from shared header
  enum GameState { MENU, PLAYING, GAME_OVER, GAME_WON, RESTARTING };
  class Game {
  public:
    GameState state = MENU;
    Cell grid[GRID_SIZE][GRID_SIZE];
    bool isFirstClick = true;
    sf::Clock clock;
    void resetGame(int firstClickI, int firstClickJ);
    void openCells(int i, int j);
    bool checkWinCondition();
    void checkAndHandleWin();
    void restart(); // instant: clear and go to MENU
    void toMenu();
    int flagCount() const;
    void toggleFlag(int i,int j);
  };
  ```

- [ ] **Step 2: Create Game.cpp — move implementations**

  Move `resetGame` `src/main.cpp:38-76`, `openCells` `src/main.cpp:78-110`, `checkWinCondition` `src/main.cpp:112-120`, `checkAndHandleWin` `src/main.cpp:122-127` into `Game.cpp` as methods, replacing `grid`/`currentGameState` globals with `this->grid`/`this->state`. Add `restart(){ toMenu(); }`, `toMenu(){ for all cells grid[i][j]=Cell(); state=MENU; isFirstClick=true; }`.

- [ ] **Step 3: Update main.cpp to use Game**

  Replace globals `currentGameState` `src/main.cpp:20` and `grid` `src/main.cpp:36` with `Game game;` instance. Update call sites: `resetGame`→`game.resetGame`, `openCells`→`game.openCells`, `checkWinCondition`→`game.checkWinCondition`, `grid[col][row]`→`game.grid[col][row]`.

- [ ] **Step 4: Build verify**

  Run: `cmake --build build` — Expected: compiles, game still runs (MENU→PLAYING flow unchanged)

- [ ] **Step 5: Commit**

  ```bash
  git add src/Game.hpp src/Game.cpp src/main.cpp
  git commit -m "refactor: extract Game state machine and fix isFirstClick reset"
  ```

### Task 3: UI Primitives (Button, Overlay)

**Files:**
- Create: `src/UI.hpp`
- Create: `src/UI.cpp`

**Interfaces:**
- Consumes: `sf::Font&`, `sf::RenderWindow&`, `sf::Vector2i`
- Produces: `struct Button { sf::RectangleShape shape; sf::Text label; Button(sf::Font&, std::string, sf::Vector2f pos, sf::Vector2f size); bool contains(sf::Vector2i) const; void setHovered(bool); void draw(sf::RenderWindow&) const; }`, `void drawOverlay(sf::RenderWindow&, sf::Color)`, `void drawCenteredText(sf::RenderWindow&, sf::Text&)` — consumed by Tasks 4-8

- [ ] **Step 1: Write UI.hpp**

  ```cpp
  #pragma once
  #include <SFML/Graphics.hpp>
  struct Button {
    sf::RectangleShape shape;
    sf::Text text;
    sf::Color normal = sf::Color(74,144,226);
    sf::Color hover = sf::Color(53,122,189);
    sf::Color textColor = sf::Color::White;
    Button(sf::Font& font, const std::string& str, sf::Vector2f pos, sf::Vector2f size);
    bool contains(sf::Vector2i p) const;
    void setHovered(bool h);
    void draw(sf::RenderWindow& w) const;
  };
  void drawOverlay(sf::RenderWindow& w, sf::Color c = sf::Color(0,0,0,150));
  ```

- [ ] **Step 2: Implement UI.cpp**

  Button ctor: `shape.setPosition(pos); shape.setSize(size); shape.setFillColor(normal); text=sf::Text(font,str,20); center text via getLocalBounds`. `contains` via `shape.getGlobalBounds().contains(Vector2f(p))`. `setHovered` toggles fill color. `draw` draws shape then text. `drawOverlay` draws fullscreen rect `sf::RectangleShape(Vector2f(window.getSize()))`.

- [ ] **Step 3: Build verify**

  Run: `cmake --build build` — Expected: compiles even if not yet used

- [ ] **Step 4: Commit**

  ```bash
  git add src/UI.hpp src/UI.cpp
  git commit -m "feat: add UI primitives Button and overlay"
  ```

### Task 4: MENU / Start Game Screen

**Files:**
- Modify: `src/UI.hpp`, `src/UI.cpp`
- Modify: `src/main.cpp:157-207`

**Interfaces:**
- Consumes: `Game::state`, `Button`, `sf::Font`
- Produces: `void drawMenu(sf::RenderWindow&, sf::Font&, Game&, Button& startButton)` — shows title + start button

- [ ] **Step 1: Implement drawMenu in UI**

  ```cpp
  void drawMenu(sf::RenderWindow& w, sf::Font& font, Game& game, Button& startButton);
  // inside: drawOverlay(w, Color(180,180,180,0)) or clear color, then:
  // sf::Text title(font,"MINESWEEPER",48); title.setFillColor(Color::Black); center at y=80
  // startButton at pos (220,200) size (200,50) label "Start Game"
  // sf::Text hint(font,"Left click reveal • Right click flag • R restart",14); at y=280
  ```

- [ ] **Step 2: Wire input in main.cpp**

  In event loop `src/main.cpp:168`: if `game.state==MENU` and `event.getIf<sf::Event::MouseButtonPressed>` with `mouseClick->button==Left` and `startButton.contains(mouseClick->position)` → `game.state=PLAYING; game.isFirstClick=true;` (do not place mines yet). Also handle `event.getIf<sf::Event::KeyPressed>` with `code==Enter||Space` → same.

- [ ] **Step 3: Wire rendering**

  In render section `src/main.cpp:202`: `if (game.state==MENU) { window.clear(Color(180,180,180)); renderBoard(window, game); drawOverlay(window, Color(0,0,0,80)); drawMenu(window,font,game,startButton); } else ...`

- [ ] **Step 4: Manual test**

  Launch `build/bin/minesweeper.exe` from repo root — see MENU, click Start, board becomes interactive, first click places mines.

- [ ] **Step 5: Commit**

  ```bash
  git add src/UI.* src/main.cpp
  git commit -m "feat: add MENU Start Game screen"
  ```

### Task 5: PLAYING HUD (Timer, Mine Counter, Restart)

**Files:**
- Modify: `src/UI.hpp`, `src/UI.cpp`
- Modify: `src/main.cpp:129-155`, `src/main.cpp:157-207`

**Interfaces:**
- Consumes: `Game::flagCount()`, `Game::clock`, `Button`
- Produces: `void drawHUD(sf::RenderWindow&, sf::Font&, Game&, Button& restartButton)` — header bar with mine counter, timer, restart

- [ ] **Step 1: Redefine layout constants**

  Add constants `HEADER_H=48`, `BOARD_OFFSET_X=(640-384)/2=128`, `BOARD_OFFSET_Y=48`. Update `renderBoard` signature to `renderBoard(sf::RenderWindow&, Game&)` and position: `sprite.setPosition(Vector2f(i*CELL_SIZE+BOARD_OFFSET_X, j*CELL_SIZE+BOARD_OFFSET_Y))`. Update mouse `col = (mouse.x - BOARD_OFFSET_X)/CELL_SIZE`, `row = (mouse.y - BOARD_OFFSET_Y)/CELL_SIZE` and bounds checks `src/main.cpp:171`.

- [ ] **Step 2: Implement drawHUD**

  ```cpp
  void drawHUD(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartBtn);
  // draw header rect Color(60,60,60) size 640x48 at (0,0)
  // left: sf::Text(font, "Mines: "+std::to_string(totalMines - game.flagCount())) pos (16,12) white
  // center: restartBtn at (304,8) size (32,32) label "R"
  // right: sf::Text(font, "Time: "+std::to_string((int)game.clock.getElapsedTime().asSeconds())+"s") pos (500,12)
  ```

  Add `game.mineCount()` helper counting `isMine`.

- [ ] **Step 3: Wire HUD interactions**

  On left click in PLAYING, if click inside `restartBtn.contains(pos)` with `mouse.y < HEADER_H` → `game.restart()` (instant to MENU). Ensure board clicks ignore header area (`row <0` check handles).

- [ ] **Step 4: Timer logic**

  In `Game::resetGame` restart clock `clock.restart()`. In `drawHUD` compute elapsed only when `state==PLAYING`; otherwise freeze.

- [ ] **Step 5: Build + manual test** — HUD shows updating timer, mine counter decreases on flag, restart button works.

- [ ] **Step 6: Commit**

  ```bash
  git add src/UI.* src/main.cpp src/Game.*
  git commit -m "feat: add PLAYING HUD with timer and mine counter"
  ```

### Task 6: GAME_OVER Overlay

**Files:**
- Modify: `src/Game.cpp:78`, `src/UI.hpp/cpp`, `src/main.cpp`

**Interfaces:**
- Consumes: `Game::state==GAME_OVER`, `Button`
- Produces: `void drawGameOver(sf::RenderWindow&, sf::Font&, Game&, Button& restartBtn, Button& menuBtn)`

- [ ] **Step 1: Reveal mines on game over**

  In `Game::openCells` `src/main.cpp:85` when `isMine` set `state=GAME_OVER` and loop all cells `if (grid[i][j].isMine) grid[i][j].isRevealed=true;`.

- [ ] **Step 2: Implement drawGameOver**

  ```cpp
  void drawGameOver(sf::RenderWindow& w, sf::Font& font, Game& game, Button& restartBtn, Button& menuBtn);
  // drawOverlay(w, Color(0,0,0,150))
  // Title "GAME OVER" red 36pt at centered y=140
  // Subtitle "You hit a mine!" white 16pt y=190
  // restartBtn "Restart" at (220,240) (200,40)
  // menuBtn "Menu" at (220,290) (200,40) gray
  ```

- [ ] **Step 3: Wire input**

  In event loop when `state==GAME_OVER` and left click: if `restartBtn.contains(pos)` → `game.restart()` (to MENU); else if `menuBtn.contains(pos)` → `game.toMenu()`; else `continue` (ignore board clicks `src/main.cpp:174`). Also `KeyPressed` with `R` → `game.restart()`, `Escape` → `game.toMenu()`.

- [ ] **Step 4: Wire rendering**

  After `renderBoard` + `drawHUD`, if `GAME_OVER` call `drawGameOver`.

- [ ] **Step 5: Commit**

  ```bash
  git add src/Game.cpp src/UI.* src/main.cpp
  git commit -m "feat: add GAME_OVER overlay with restart/menu"
  ```

### Task 7: GAME_WON Overlay

**Files:**
- Modify: `src/UI.hpp/cpp`, `src/main.cpp:112-127`

**Interfaces:**
- Consumes: `Game::checkWinCondition()`, `Game::clock`
- Produces: `void drawGameWon(sf::RenderWindow&, sf::Font&, Game&, Button&, Button&)`

- [ ] **Step 1: Implement drawGameWon**

  ```cpp
  void drawGameWon(sf::RenderWindow& w, sf::Font& font, Game& game, Button& againBtn, Button& menuBtn);
  // overlay Color(0,0,0,150)
  // "YOU WON!" green 36pt y=140 Color(46,204,113)
  // "Time: Xs" white y=190
  // againBtn "Play Again" (220,240) (200,40)
  // menuBtn "Menu" (220,290) (200,40)
  ```

- [ ] **Step 2: Ensure win detection**

  Verify `checkAndHandleWin` `src/main.cpp:122` called after `openCells` and after flag toggle if needed. No change needed except method rename.

- [ ] **Step 3: Wire input/render same as GAME_OVER**

  Click handling for `GAME_WON` mirrors GameOver but with won buttons. `R` → restart, `Escape` → menu.

- [ ] **Step 4: Manual test** — force win (temporarily reduce mine density or use debug key) and verify overlay.

- [ ] **Step 5: Commit**

  ```bash
  git add src/UI.* src/main.cpp src/Game.*
  git commit -m "feat: add GAME_WON overlay"
  ```

### Task 8: RESTARTING State + Global Input Handling

**Files:**
- Modify: `src/Game.hpp/cpp`, `src/main.cpp:162-207`, `src/UI.cpp`

**Interfaces:**
- Consumes: `GameState::RESTARTING`
- Produces: `void Game::restart()` instant transition, `void drawRestarting(sf::RenderWindow&, sf::Font&)` (or no-op if instant), global key handling for R/Escape

- [ ] **Step 1: Implement RESTARTING as instant**

  `Game::restart(){ // instant per user choice
    for each cell grid[i][j]=Cell();
    state = MENU;
    isFirstClick = true;
  }`
  `Game::toMenu()` same but explicit. `RESTARTING` enum kept for future animation but `restart()` goes directly to `MENU` (no timer). Add optional `drawRestarting` that just draws "Restarting..." for one frame if needed, but instant means not rendered.

- [ ] **Step 2: Global input handling**

  Add at top of event loop: 
  ```cpp
  if (auto* key = event->getIf<sf::Event::KeyPressed>()) {
    if (key->code == sf::Keyboard::Key::R) game.restart();
    if (key->code == sf::Keyboard::Key::Escape) game.toMenu();
  }
  ```
  Ignore board clicks when `state==RESTARTING` or `state==MENU` outside buttons.

- [ ] **Step 3: Fix isFirstClick and click handling**

  Ensure `game.isFirstClick` reset in `restart`/`toMenu`. Update `src/main.cpp:174` from `if (currentGameState==GAME_OVER||GAME_WON) continue` to `if (game.state==GAME_OVER||GAME_WON||RESTARTING||MENU) handle only UI buttons`.

- [ ] **Step 4: Build verify**

  Run `cmake --build build` and test R/Escape from all states.

- [ ] **Step 5: Commit**

  ```bash
  git add src/Game.* src/UI.* src/main.cpp
  git commit -m "feat: add RESTARTING state and global input handling"
  ```

### Task 9: Polish & Verification (Hover, Final Build)

**Files:**
- Modify: `src/UI.cpp`, `src/main.cpp`

**Interfaces:**
- Consumes: all prior UI
- Produces: hover states, final verified build

- [ ] **Step 1: Button hover**

  Handle `sf::Event::MouseMoved`: get position `auto* moved = event->getIf<sf::Event::MouseMoved>()`, then `button.setHovered(button.contains(moved->position))` for all visible buttons (menu, HUD restart, game over/won). Call each frame.

- [ ] **Step 2: Fix texture loading edge case**

  Ensure `cellTexturesDictionary` loads after window context or lazily — test that `build/bin/minesweeper.exe` still loads `assets/images/*.png` via relative path when cwd is repo root `src/main.cpp:22` (see AGENTS.md). If failure, move to function `loadTextures()` called after window creation.

- [ ] **Step 3: Final build**

  Run: `cmake -S . -B build -G Ninja && cmake --build build`
  Expected: clean build, no warnings, exe runs, all states navigable: MENU→Start→PLAYING→(hit mine)→GAME_OVER→Restart→MENU→Start→Win→GAME_WON→Menu.

- [ ] **Step 4: Update README if needed**

  If window layout or controls changed, update `README.md:34`.

- [ ] **Step 5: Commit**

  ```bash
  git add src/UI.* src/main.cpp README.md
  git commit -m "polish: button hover and final verification"
  ```

