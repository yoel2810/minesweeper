# AGENTS.md

## Build & Run (Windows / MSYS2 UCRT64)
- Configure: `cmake -S . -B build -G Ninja`
- Build: `cmake --build build`
- Run from repo root: `./build/bin/minesweeper.exe` — loads `assets/fonts/VT323-Regular.ttf` via relative path; fails if cwd != repo root
- Toolchain: `C:/msys64/ucrt64/bin/c++.exe`, C++17 (`CMakeLists.txt:17`), `CMAKE_EXPORT_COMPILE_COMMANDS ON` (`CMakeLists.txt:4`), Ninja generator
- SFML 3.1.0 via `FetchContent` (`CMakeLists.txt:8-14`, `SFML::Graphics`) — first configure fetches from network and is slow; `CMAKE_RUNTIME_OUTPUT_DIRECTORY` is `build/bin` (`CMakeLists.txt:5`)

## Project Structure
- `src/main.cpp:1-207` — entrypoint, SFML window loop (`640x480`, 60fps), `GameState { MENU, PLAYING, GAME_OVER, GAME_WON }`, global `Cell grid[GRID_SIZE][GRID_SIZE]`, input/render/win logic
- `src/Cell.hpp` / `src/Cell.cpp` — `Cell { minesCount, isRevealed, isMine, isFlag }`
- `assets/fonts/VT323-Regular.ttf` (OFL, license alongside) is the only runtime asset — all visuals are SFML shapes drawn in `src/UI.cpp` ("Pocket LCD" theme, colors in `Palette` in `src/UI.hpp`); `assets/images/*.png` are legacy and unused; `build/`, `.cache/`, `.clangd` index are gitignored
- Single target `minesweeper` (`CMakeLists.txt:16`); no tests, lint, formatter, or CI

## Gotchas & Conventions
- `GRID_SIZE 12` / `CELL_SIZE_PX 32` are macros (`src/main.cpp:15-16`); window size assumes these — change together
- Mine density ~20% (`rand() % 5 == 0` in `src/main.cpp:45`); first click is always safe — `resetGame(col,row)` skips mine at first-click coords (`src/main.cpp:38-43`) gated by `isFirstClick` (`src/main.cpp:160,184`)
- Recursive flood-fill in `openCells` (`src/main.cpp:78-110`); `checkWinCondition` requires all non-mines revealed (`src/main.cpp:112-120`)
- `.clangd:3-8` hardcodes MSYS2 UCRT64 system includes (`C:/msys64/ucrt64/include/c++/16.2.0`); update if toolchain changes
- No `opencode.json` / existing instruction files — this is the sole agent guide
- Current branch is `side-project` (tracks `origin/side-project`); `main` exists separately
