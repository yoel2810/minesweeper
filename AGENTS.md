# AGENTS.md

## Build, Run & Test (Windows / MSYS2 UCRT64)
- Configure: `cmake -S . -B build -G Ninja`
- Build: `cmake --build build` (builds both `minesweeper` and `board_tests`)
- Run from repo root: `./build/bin/minesweeper.exe` — loads `assets/fonts/VT323-Regular.ttf` via relative path; fails if cwd != repo root
- Test from repo root: `./build/bin/board_tests.exe` (or `ctest --test-dir build`) — prints `Board checks passed`, exits 1 on the first failed `require`; loads `assets/fonts/DejaVuSans.ttf`, so cwd must be repo root (CTest sets this via `WORKING_DIRECTORY`, `CMakeLists.txt:31`)
- Toolchain: `C:/msys64/ucrt64/bin/c++.exe`, C++17 (`cxx_std_17`), `CMAKE_EXPORT_COMPILE_COMMANDS ON` (`CMakeLists.txt:4`), Ninja generator
- SFML 3.1.0 via `FetchContent` (`CMakeLists.txt:12-18`, `SFML::Graphics` only; audio/network disabled) — first configure fetches from network and is slow; binaries go to `build/bin` (`CMakeLists.txt:5`)

## Project Structure
- `src/main.cpp` — entrypoint, SFML window loop (`640x480` default, resizable, min `640x480`, 60fps), menu/HUD buttons, input handling, F11 fullscreen toggle (`DisplaySettings`), keys: `R` restart, `Esc` menu, `Tab`/`Enter` in menu fields
- `src/Game.hpp` / `src/Game.cpp` — `Game` class: `GameState { MENU, PLAYING, GAME_OVER, GAME_WON, RESTARTING }`, `grid` is `std::vector<std::vector<Cell>>` indexed `grid[column][row]`, mine generation (`resetGame`), flood-fill (`openCells`), win check, flags, timer
- `src/BoardSettings.hpp` — board size limits (`MIN_BOARD_SIZE 4`, `MAX_BOARD_SIZE 30`, `DEFAULT_BOARD_SIZE 12`), `CELL_SIZE_PX`, `HEADER_H`, `BOARD_PADDING`, `parseBoardDimension`, `BoardLayout::fit` (scales cells to fit the window) and `cellAt` (pixel → cell)
- `src/Cell.hpp` / `src/Cell.cpp` — `Cell { minesCount, isRevealed, isMine, isFlag }`
- `src/UI.hpp` / `src/UI.cpp` — all visuals drawn with SFML shapes ("Pocket LCD" theme); colors in `namespace Palette`, `Button`, `NumberField`, `UI::layoutControls`, `UI::style*`
- `src/DisplaySettings.hpp` — windowed/fullscreen state
- `src/Solver.hpp` / `src/Solver.cpp` — `Solver` holds a `Game&`; `playTurn()` / `solve()` are stubs. Compiled into `minesweeper` only (not `board_tests`). Triggered by the HUD `>` (one turn) and `>>` (full solve) buttons placed by `UI::layoutSolverControls`; `main.cpp` runs the win/loss bookkeeping after each call
- `tests/BoardTests.cpp` — `board_tests` target: plain `main()` with a `require(cond, msg)` helper, no framework; covers `NumberField` input, `UI::layoutControls` positions, `DisplaySettings`/F11 latch, `parseBoardDimension`, and `Game` sizing, first-click safety, mine counts, and win detection
- `assets/fonts/VT323-Regular.ttf` (OFL, license alongside) is the game font; `assets/fonts/DejaVuSans.ttf` is used only by tests; `assets/images/*.png` are legacy and unused
- `build/`, `out/`, `build-vs/`, `.cache/`, `.clangd`, IDE dirs are gitignored

## Gotchas & Conventions
- Board size is chosen in the menu (columns × rows, 4–30 each) and passed to `Game::start`; layout adapts to window size via `BoardLayout::fit` — no hardcoded grid size anymore
- Difficulty is chosen in the menu: EASY 10% / MEDIUM 15% (default) / HARD 20% (`presetMinePercent` in `BoardSettings.hpp`) or CUSTOM (typed in the `MINES %` field, 5–50; typing there auto-selects CUSTOM). The percentage goes to `Game::start(columns, rows, minePercent)` and is stored in `Game::minePercent`
- Mines are placed lazily on the first click: `main.cpp` calls `game.resetGame(col,row)` when `game.isFirstClick` is set. `resetGame` places an exact count, `mineCountFor` = round(cells × percent / 100), capped at the cells outside the safe area; the first click and its 8 neighbours are always mine-free, so the opening cell is a 0 and flood-fills
- `rand()` is seeded once with `std::time(nullptr)` at the top of `main()` in `src/main.cpp`; `board_tests` does not seed, so its layouts are deterministic
- Recursive flood-fill in `Game::openCells`; `checkWinCondition` requires all non-mines revealed
- `.clangd:3-8` hardcodes MSYS2 UCRT64 system includes (`C:/msys64/ucrt64/include/c++/16.2.0`); update if toolchain changes
- No lint, formatter, or CI; this file is the sole agent guide
- Current branch is `side-project` (tracks `origin/side-project`); `main` is the default branch
