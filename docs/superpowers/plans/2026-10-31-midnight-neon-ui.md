# Midnight Neon UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Translate the selected Midnight Neon HTML concept into the existing SFML Minesweeper without changing gameplay.

**Architecture:** Add a header-only theme token module and pure visual-selection helpers that can be tested without reading pixels. Move board rendering into the UI layer, render cells from SFML shapes and text instead of PNG textures, then restyle existing controls and overlays around the same tokens.

**Tech Stack:** C++17, SFML 3.1.0 Graphics, CMake/Ninja, CTest

**Spec:** `docs/superpowers/specs/2026-10-31-midnight-neon-ui-design.md`

## Global Constraints

- Use C++17 and the existing SFML 3.1.0 dependency only.
- Keep the bundled `assets/fonts/DejaVuSans.ttf`; do not download resources.
- Preserve board sizes 4x4 through 30x30, the 480x360 minimum window, resizing, and F11 fullscreen.
- Do not change gameplay, mine density, input bindings, or state transitions.
- Preserve unrelated working-tree changes. Do not commit during execution unless the user explicitly requests it because the relevant files are already modified.

## Review Focus

- A 30x30 board at 480x360 must use a positive cell size, remain inside the play area, and omit text that cannot fit rather than overlap cells; Task 1 tests the text-size policy and the existing layout test covers bounds.
- Menu fields at 480x360 must stay within the overlay card and remain keyboard focusable; Task 3 extends the control-layout assertions.
- Flags, mines, empty revealed cells, and numbered cells must never share an ambiguous visual mapping; Task 1 tests every state class and all eight number colors.
- MENU, PLAYING, GAME_OVER, and GAME_WON must retain readable HUD values and correct button styles; Task 3 tests style assignment and state-neutral timer formatting.
- Removing PNG cell textures must not leave any runtime texture construction or asset-path dependency in `main.cpp`; Task 2 verifies this with a source scan and a clean build.

---

### Task 1: Theme Tokens and Testable Cell Visuals

**Files:**
- Create: `src/Theme.hpp`
- Modify: `src/UI.hpp`
- Modify: `src/UI.cpp`
- Modify: `tests/BoardTests.cpp`

**Interfaces:**
- Produces: `namespace Theme` color constants for background, panels, borders, cyan, violet, pink, green, red, text, and muted text.
- Produces: `UI::CellVisual { sf::Color fill, outline, text; std::string glyph; bool raised; }`.
- Produces: `UI::cellVisual(const Cell &cell) -> CellVisual` and `UI::cellTextSize(int cellSize) -> unsigned int`.

- [ ] **Step 1: Add failing theme and visual-policy tests to `tests/BoardTests.cpp`**

Assert the exact Midnight Neon token values from the HTML prototype (`#070b18`, `#0f1730`, `#151f3d`, `#55e6ff`, `#a98bff`, `#ff6eac`, `#7affc7`) and assert:

- closed cell: raised, no glyph;
- closed flagged cell: raised, flag glyph, pink text;
- revealed mine: mine glyph, red/pink treatment;
- revealed empty: no glyph and recessed fill;
- revealed counts 1 through 8: decimal glyphs and a non-background text color;
- `cellTextSize(10) == 0`, `cellTextSize(16) > 0`, and `cellTextSize(32) <= 20`.

- [ ] **Step 2: Run the focused test target and verify the new tests fail**

Run: `cmake --build build --target board_tests`

Expected: FAIL because `Theme.hpp`, `UI::CellVisual`, `UI::cellVisual`, and `UI::cellTextSize` do not exist.

- [ ] **Step 3: Add `src/Theme.hpp` and declare the pure UI helpers**

Use `inline const sf::Color` tokens so the header has one source of truth without adding a compilation unit. Add the exact helper signatures to `src/UI.hpp`.

- [ ] **Step 4: Implement `UI::cellVisual` and `UI::cellTextSize` in `src/UI.cpp`**

Give flags precedence on unrevealed cells, mines precedence on revealed cells, and clamp readable text to the cell interior. Counts outside 1 through 8 return the empty revealed treatment.

- [ ] **Step 5: Build and run the tests**

Run: `cmake --build build --target board_tests; ctest --test-dir build --output-on-failure`

Expected: build succeeds and CTest reports `100% tests passed`.

- [ ] **Step 6: Review the scoped diff checkpoint**

Run: `git diff --check -- src/Theme.hpp src/UI.hpp src/UI.cpp tests/BoardTests.cpp`

Expected: no whitespace errors.

### Task 2: Shape-Based Midnight Neon Board

**Files:**
- Modify: `src/UI.hpp`
- Modify: `src/UI.cpp`
- Modify: `src/main.cpp`
- Test: `tests/BoardTests.cpp`

**Interfaces:**
- Consumes: `Theme` tokens, `UI::cellVisual`, `UI::cellTextSize`, `BoardLayout::fit`, and `Game::grid`.
- Produces: `UI::drawBoard(sf::RenderWindow &window, sf::Font &font, const Game &game) -> void`.

- [ ] **Step 1: Extend the layout tests for the renderer's supported cell-size extremes**

For 4x4, 12x12, and 30x30 boards at 480x360, 640x480, and 1200x700, assert the existing `BoardLayout` result remains positive and that `UI::cellTextSize(layout.cellSize)` is either zero or strictly smaller than the cell size.

- [ ] **Step 2: Run the focused test and verify the new assertion fails if the sizing contract is not met**

Temporarily assert an impossible upper bound first (`cellTextSize(layout.cellSize) >= layout.cellSize`), run `cmake --build build --target board_tests; ctest --test-dir build --output-on-failure`, and observe the named assertion fail; restore the intended assertion before implementation.

- [ ] **Step 3: Declare and implement `UI::drawBoard`**

For each cell, draw a small-gutter rectangle using `CellVisual`; add a one-pixel highlight/shadow pair for raised cells; center number glyphs using the current font; draw flags and mines from SFML primitive shapes so their appearance does not depend on Unicode font coverage. Skip inner detail when the cell is too small to render it safely.

- [ ] **Step 4: Replace the texture renderer in `src/main.cpp`**

Remove texture/sprite includes, `cellTexturesDictionary`, and the file-local `renderBoard`. Call `UI::drawBoard(window, font, game)` and clear the window with `Theme::background`.

- [ ] **Step 5: Verify the texture dependency is gone and the project passes**

Run: `rg "cellTexturesDictionary|assets/images|sf::Texture|sf::Sprite" src/main.cpp; cmake --build build; ctest --test-dir build --output-on-failure`

Expected: `rg` returns no matches, the build succeeds, and CTest reports `100% tests passed`.

- [ ] **Step 6: Review the scoped diff checkpoint**

Run: `git diff --check -- src/UI.hpp src/UI.cpp src/main.cpp tests/BoardTests.cpp`

Expected: no whitespace errors.

### Task 3: Controls, HUD, Menu, and Terminal Overlays

**Files:**
- Modify: `src/UI.hpp`
- Modify: `src/UI.cpp`
- Modify: `src/main.cpp`
- Modify: `tests/BoardTests.cpp`

**Interfaces:**
- Consumes: all `Theme` tokens and the existing `GameState`, `Button`, `NumberField`, and `UI::layoutControls` interfaces.
- Produces: `enum class ButtonStyle { Primary, Secondary, Hud }`, `Button::applyStyle(ButtonStyle style) -> void`, and `UI::formatTime(float elapsedSeconds) -> std::string`.

- [ ] **Step 1: Add failing control-style, time-format, and compact-layout tests**

Assert:

- Primary, Secondary, and Hud styles apply their expected theme fill, hover, outline, and text colors;
- `UI::formatTime(0.f) == "00:00"`, `UI::formatTime(137.f) == "02:17"`, and negative time clamps to `"00:00"`;
- at 480x360, both number fields, the start button, and both terminal buttons remain fully inside the window and below `HEADER_H`.

- [ ] **Step 2: Run the focused target and verify the new tests fail**

Run: `cmake --build build --target board_tests`

Expected: FAIL because `ButtonStyle`, `Button::applyStyle`, and `UI::formatTime` do not exist.

- [ ] **Step 3: Implement button styles, number-field styling, and `UI::formatTime`**

Use theme tokens for normal/hover/outline/text colors. Preserve all hit boxes and editing behavior. Make `Button::setSize` recenter its text so future layout changes cannot separate the label from the shape.

- [ ] **Step 4: Restyle the HUD**

Draw layered navy header surfaces with a bottom accent rule, use muted uppercase labels and cyan values for mines/time, retain a centered 32x32 restart target, and use `UI::formatTime` for the displayed time.

- [ ] **Step 5: Restyle menu, validation, game-over, win, and restarting views**

Draw a centered dark panel with thin accent border; apply the prototype copy hierarchy and spacing; use pink/red for failure and green/cyan for success; retain the current controls, shortcuts, and input focus behavior. At compact sizes, reduce title/subtitle spacing so controls stay visible.

- [ ] **Step 6: Assign the three button styles in `src/main.cpp`**

Apply Primary to Start/Restart/Play Again, Secondary to Menu, and Hud to the centered restart control. Remove the old per-button gray color overrides.

- [ ] **Step 7: Build and run the complete automated suite**

Run: `cmake --build build; ctest --test-dir build --output-on-failure`

Expected: build succeeds and CTest reports `100% tests passed`.

- [ ] **Step 8: Run the game from the repository root for visual and interaction verification**

Run: `./build/bin/minesweeper.exe`

Verify menu typing and Tab/Enter, 4x4/12x12/30x30 starts, reveal and flag contrast, restart, loss and win overlays, Escape, resize to 480x360, and F11 enter/exit. Expected: controls remain visible, board stays centered, and no text overlaps cells.

- [ ] **Step 9: Review the final diff**

Run: `git diff --check; git diff --stat; git status --short`

Expected: no whitespace errors; only the theme/UI implementation, tests, design artifacts, and the already-present user changes appear.
