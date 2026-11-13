# Hint Button Implementation Plan

> **For agentic workers:** Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A HUD hint button that outlines one cell the solver can prove is safe, and flashes the button when no such cell exists.

**Architecture:** `Solver::stepHint` reuses the existing deduction steps (basic, subset, exhaustive search, mine-total) on a read-only snapshot and returns the first unrevealed safe cell or -1. `Solver::findHint` wraps it for the live game. `main.cpp` stores the hinted cell and draws it through `UI::drawBoard`; the HUD gets a third button.

**Tech Stack:** C++17, SFML 3.1.0, plain-`main` test executables.

**Spec:** Approved in conversation; summarized here.

## Global Constraints
- A hint never reveals, flags, or changes the game. It only highlights.
- A hint never points at a guess: only proven-safe cells, or the first-click corner (always safe).
- Hint outline color is `Palette::Dark`, distinct from the solver's `Palette::Darkest` mark.
- "No hint" feedback: the hint button inverts (LCD colors) for 0.6 s.

## Review Focus
- Board where the mine total alone proves a safe cell: hint must return it (covered by `stepGuess().certain`).
- Hint pressed before the first click: must return the corner, not "no hint".
- Hint pressed after the game ends: nothing is drawn and nothing is flashed.

---

### Task 1: Solver hint logic

**Files:**
- Modify: `src/Solver.hpp` (declare `stepHint`, `findHint`; add `<optional>`, `<utility>`)
- Modify: `src/Solver.cpp` (define both)
- Test: `tests/SolverTests.cpp`

**Interfaces:**
- Produces: `static int Solver::stepHint(const Snapshot &s)` — unrevealed proven-safe cell id, or -1.
- Produces: `std::optional<std::pair<int,int>> Solver::findHint() const` — `(column, row)` or nullopt.

- [ ] **Step 1: Write failing tests** in `tests/SolverTests.cpp`:
  - `testHintFindsSearchSafeCell`: 1-2-2-1 snapshot; `Solver::stepHint(s)` is `s.id(0,1)` or `s.id(3,1)`.
  - `testNoHintOnEmptyBoard`: 4x4 all unknown, `totalMines = 8`; `stepHint(s) == -1`.
  - `testHintNeverMine`: 300 `randomScenario` 8x8 games; any `findHint()` result is unrevealed and not a mine.
  Add all three to `main()`.
- [ ] **Step 2: Run** `cmake --build build --target solver_tests && ./build/bin/solver_tests.exe` — expect compile failure (`stepHint` undefined).
- [ ] **Step 3: Implement** in `Solver.hpp`/`Solver.cpp`:
  - `stepHint`: try `stepBasicRules`, `stepSubsetRule`, `stepExhaustiveSearch(analyzeFrontier(s))`, then `stepGuess(s, f).certain`; return the first `safe` id whose view is `Unknown`, else -1.
  - `findHint`: nullopt unless `game.state == PLAYING`; if `game.isFirstClick` return `(0,0)`; else snapshot, `stepHint`, convert id via `column`/`row`.
- [ ] **Step 4: Run** `./build/bin/solver_tests.exe` — expect `Solver checks passed`.
- [ ] **Step 5: Commit** (only when the user asks).

### Task 2: Hint button, outline, and no-hint flash

**Files:**
- Modify: `src/UI.hpp`, `src/UI.cpp`: `layoutSolverControls` takes a third `Button &hintButton` placed at `centerX - 96`; `drawBoard` takes `std::optional<std::pair<int,int>> hinted = std::nullopt`, drawn as an inset `Palette::Dark` outline.
- Modify: `src/main.cpp`: create `hudHintButton` ("?", same size as other HUD keys), style with `styleRubber`, update all three `layoutSolverControls` call sites, hover, click handling (`hintMark = solver.findHint()`; on nullopt set `noHintSince = sf::Clock{}`), clear `hintMark` wherever `solverMark` is cleared, and flash colors while `noHintSince` is under 0.6 s.
- Test: `tests/BoardTests.cpp` only if `layoutSolverControls` is covered there (check with grep).

- [ ] **Step 1: Build** `cmake --build build` — expect success.
- [ ] **Step 2: Manual check** `./build/bin/minesweeper.exe` from repo root: press `?` on a fresh board (outlines corner), press it repeatedly mid-game, and confirm the flash on a board with no provable cell.
- [ ] **Step 3: Run all tests** `ctest --test-dir build` — expect both pass.
- [ ] **Step 4: Commit** (only when the user asks).
