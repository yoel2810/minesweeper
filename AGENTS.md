# AGENTS.md

## Build, Run & Test (Windows / MSYS2 UCRT64)
- Configure: `cmake -S . -B build -G Ninja` — single-config builds default to `Release` (`CMakeLists.txt`); add `-DCMAKE_BUILD_TYPE=Debug` for a debug build
- Build: `cmake --build build` — builds `minesweeper`, `solver_bench`, `board_tests`, `solver_tests`
- Run: `./build/bin/minesweeper.exe` — finds `assets/fonts/VT323-Regular.ttf` next to the exe, in `..`, `../..`, `../../..` (repo root from `build/bin` or `build-vs/bin/<Config>`) or the cwd (`findAsset` in `src/App.cpp`), so any cwd works
- Test: `ctest --test-dir build`, or run `build/bin/board_tests.exe` / `build/bin/solver_tests.exe` directly. Both exit 1 on the first failed `require`. `board_tests` loads `assets/fonts/DejaVuSans.ttf` by relative path, so run it from the repo root (CTest sets `WORKING_DIRECTORY`)
- Bench: `build/bin/solver_bench.exe [columns rows games [seed [minePercent]]]` — wins, guesses and the slowest solver turn
- Toolchain: `C:/msys64/ucrt64/bin/c++.exe`, C++17, Ninja. Warnings are on for all project targets (`-Wall -Wextra -Wpedantic -Wshadow`, `/W4` on MSVC) via the `project_options` interface target; keep the build warning-free
- SFML 3.1.0 via `FetchContent` (Graphics + Audio; network off). The first configure fetches from the network and is slow. Binaries go to `build/bin`
- Visual Studio also works: `cmake -S . -B build-vs -G "Visual Studio 17 2022"` (note: there `long double` is only a `double`)

## Project Structure
- CMake targets: `minesweeper_core` (static: `Game.cpp`, `Solver.cpp`, no SFML) → `minesweeper_ui` (static: `UI.cpp`, + `SFML::Graphics`) → `minesweeper` (`main.cpp`, `App.cpp`, `Audio.cpp`, `app.rc`, + `SFML::Audio`, Threads). `solver_bench` and `solver_tests` link only the core; `board_tests` links the UI library
- `src/main.cpp` — just constructs `App` and runs it
- `src/App.hpp` / `src/App.cpp` — window (640x480 default, resizable, min 640x480, 60 fps, F11 borderless fullscreen via `DisplaySettings`), event routing (`handleEvent` → `handleMenuEvent` / `handleKey` / `handleClick` / `handleBoardClick`), `update`, `draw`. Keys: `R` restart (same settings), `Esc` menu, `M` mute, `Tab`/`Enter` in the menu. Sets the window icon from the exe resource after every window (re)creation
- `src/Game.hpp` / `src/Game.cpp` — rules and board state. `enum class GameState { MENU, PLAYING, GAME_OVER, GAME_WON }`. Grid is private (`cell(c, r)`, `grid()` read-only, indexed `[column][row]`); change it only through `start`, `restart`, `toMenu`, `reveal`, `toggleFlag`. Owns its `std::mt19937` (`seed()`), the timer (`std::chrono`), the exploded cell, and `gameNumber()` (bumps on every `start`)
- `src/BoardSettings.hpp` — size limits (4–30, default 12), mine percent limits (5–50), `Difficulty` presets, `mineCountFor`, `parseBoardDimension` / `parseMinePercent`, `BoardLayout::fit` / `cellAt`
- `src/Cell.hpp` — plain struct `Cell { minesCount, isRevealed, isMine, isFlag }`
- `src/UI.hpp` / `src/UI.cpp` — all drawing with SFML shapes ("Pocket LCD" theme, colors in `Palette`), `Button`, `NumberField`, `Controls` (every on-screen control; `relayout(size)`, `updateHover(state, mouse)`, `focusField`), `UI::layout*` (kept separately for tests), `UI::draw*`
- `src/Audio.hpp` / `src/Audio.cpp` — synthesized sound effects (no audio files); silent if no device
- `src/DisplaySettings.hpp` — windowed/fullscreen state and the F11 latch
- `src/Solver.hpp` / `src/Solver.cpp` — full solver, algorithm in `docs/solver-pseudocode.md`. A turn is `applyPlan(plan(snapshot()))`: `plan` is static and pure (the GUI runs it on a worker thread with `std::async`), `applyPlan` acts through `Game::reveal` / `toggleFlag`. `findHint` / `stepHint` give a proven-safe cell for the `?` key
- `tests/BoardTests.cpp` — `NumberField`, layout, `Button` label centering, `Controls` hover, `DisplaySettings`, parsing, and `Game` (sizes, mine counts, seeding, first click, flags, timer, restart, chord, win/loss)
- `tests/SolverTests.cpp` — solver steps against a brute-force reference, full games, hints, wrong player flags, new-game reset, numeric overflow
- `installer/minesweeper.iss` — Inno Setup script (copies the exe and font)
- `assets/fonts/VT323-Regular.ttf` (OFL) is the game font; `assets/fonts/DejaVuSans.ttf` is test-only; `assets/images/*.png` are legacy and unused

## Gotchas & Conventions
- Mines are placed lazily by `Game::reveal` on the first click: the click and its 8 neighbours are always mine-free, so the opening is a 0 and flood-fills. Flagging is refused before that (nothing is known yet), and the timer shows 0 until then
- `mineCountFor(columns, rows, percent)` = round(cells × percent / 100), at least 1, capped at `cells - 9`; it does not depend on where the first click lands, so `Game::mineCount()` reports it before mines exist and the HUD counter is right from the start
- `Game::reveal` on a revealed number chords (opens unflagged neighbours when the adjacent flags match). It returns the number of newly revealed cells (used for sound)
- The solver never trusts player flags: `Solver::snapshot()` treats them as unknown unless the solver proved them (`provenFlags`). It adopts player flags it proves and lifts ones it proves wrong. Tests that want a snapshot with all flags build it themselves (`snapshotWithFlags` in `SolverTests.cpp`)
- The solver resets its history, guesses and proven flags when `Game::gameNumber()` changes
- App drops a worker result if `solver.snapshot()` no longer equals the snapshot it was computed from (the player moved meanwhile)
- Probabilities are computed with scaled weights (see "Numerics" in `docs/solver-pseudocode.md`) so large boards cannot overflow, even where `long double` is a `double`
- `App` seeds the game from `std::random_device`; tests and the bench call `Game::seed`, so their layouts are deterministic. Nothing uses `rand()`
- `.clangd` hardcodes MSYS2 UCRT64 system includes (`C:/msys64/ucrt64/include/c++/16.2.0`); update it if the toolchain changes
- No formatter or CI; this file is the sole agent guide
- `main` is the default branch
