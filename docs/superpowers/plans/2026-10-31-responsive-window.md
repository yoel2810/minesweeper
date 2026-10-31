# Responsive Window and Fullscreen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Minesweeper interface respond to window resizing and toggle borderless fullscreen with F11 while preserving the active game.

**Architecture:** Pass the drawable size into the existing board layout calculation and derive all UI positions from the current window center. Keep fullscreen state in a small testable value type; `main.cpp` recreates the SFML window from that state and reapplies runtime settings.

**Tech Stack:** C++17, SFML 3.1, CMake/CTest

**Spec:** `docs/superpowers/specs/2026-10-31-responsive-window-design.md`

## Global Constraints

- Keep SFML 3.1 and the initial 640x480 window size.
- Keep cells square and no larger than 32 pixels.
- Preserve game state across resize and fullscreen transitions.
- Restore the 60 FPS limit and minimum 480x360 size after recreating a window.
- Preserve unrelated uncommitted work already present in the worktree.

## Review Focus

- A 30x30 board must remain wholly below the HUD at the 480x360 minimum size.
- Wide and tall windows must center the board without moving hit targets away from rendered cells.
- Resizing while a menu or terminal overlay is open must keep controls centered and clickable.
- Repeated F11 toggles must restore the most recent windowed size and keep the game state.
- Fullscreen recreation must retain the framerate limit and return to a resizable, closable window.

---

### Task 1: Size-aware board layout

**Files:**
- Modify: `src/BoardSettings.hpp`
- Modify: `tests/BoardTests.cpp`

**Interfaces:**
- Produces: `BoardLayout::fit(int columns, int rows, unsigned int width, unsigned int height) -> BoardLayout`

- [x] **Step 1: Write failing layout tests**

Add checks for 480x360, 640x480, 1200x700, and 700x1200 viewports. Assert positive cell size, the 32-pixel cap, centering, bounds below `HEADER_H`, and correct first/last-cell hit testing.

- [x] **Step 2: Run the test and verify RED**

Run: `cmake --build build --target board_tests; .\build\bin\board_tests.exe`

Expected: compilation fails because the four-argument `BoardLayout::fit` overload does not exist.

- [x] **Step 3: Implement the size-aware calculation**

Change `BoardLayout::fit` to accept the drawable width and height, calculate the largest square cell that fits below the fixed header, cap it at `CELL_SIZE_PX`, and center the board in the remaining area.

- [x] **Step 4: Run the test and verify GREEN**

Run: `cmake --build build --target board_tests; .\build\bin\board_tests.exe`

Expected: `Board checks passed` and exit code 0.

### Task 2: Responsive controls and overlays

**Files:**
- Modify: `src/UI.hpp`
- Modify: `src/UI.cpp`
- Modify: `src/main.cpp`
- Modify: `tests/BoardTests.cpp`

**Interfaces:**
- Consumes: `BoardLayout::fit(int, int, unsigned int, unsigned int)`
- Produces: `NumberField::setPosition(sf::Vector2f) -> void`
- Produces: `UI::layoutControls(sf::Vector2u, Button&, NumberField&, NumberField&, Button&, Button&, Button&, Button&, Button&) -> void`

- [x] **Step 1: Write failing responsive-control tests**

Assert that `NumberField::setPosition` moves its shape, label, and text together. Call `UI::layoutControls` for 480x360 and 1200x700 and assert the restart button, menu fields, start button, and terminal buttons are centered within each viewport.

- [x] **Step 2: Run the test and verify RED**

Run: `cmake --build build --target board_tests; .\build\bin\board_tests.exe`

Expected: compilation fails because `NumberField::setPosition` and `UI::layoutControls` do not exist.

- [x] **Step 3: Implement responsive control placement**

Add the two declared interfaces, call `UI::layoutControls` initially and after every `sf::Event::Resized`, and pass `window.getSize()` into every board-layout call. Calculate overlay, HUD width, text centers, and right-aligned timer positions from `w.getSize()` in all UI draw functions.

- [x] **Step 4: Run the test and verify GREEN**

Run: `cmake --build build --target board_tests; .\build\bin\board_tests.exe`

Expected: `Board checks passed` and exit code 0.

### Task 3: Fullscreen toggle and final verification

**Files:**
- Create: `src/DisplaySettings.hpp`
- Modify: `src/main.cpp`
- Modify: `tests/BoardTests.cpp`
- Modify: `README.md`

**Interfaces:**
- Produces: `DisplaySettings::toggle(sf::Vector2u currentSize) -> void`
- Produces: `DisplaySettings::targetSize(sf::Vector2u desktopSize) const -> sf::Vector2u`

- [x] **Step 1: Write failing display-state tests**

Assert that the first toggle records the current window size and selects the desktop size, the second toggle selects the recorded size, and later toggles update the remembered windowed size.

- [x] **Step 2: Run the test and verify RED**

Run: `cmake --build build --target board_tests; .\build\bin\board_tests.exe`

Expected: compilation fails because `DisplaySettings.hpp` does not exist.

- [x] **Step 3: Implement display state and F11 recreation**

Implement the value type, handle F11 globally, recreate the window with `sf::Style::None` and `sf::State::Fullscreen` or `sf::Style::Default` and `sf::State::Windowed`, then reapply the 60 FPS limit, minimum size in windowed mode, view, and responsive control layout. Add the F11 shortcut to the menu hint and README controls.

- [x] **Step 4: Run automated verification**

Run: `cmake --build build; ctest --test-dir build --output-on-failure`

Expected: build exit code 0 and 100% of tests pass.

- [x] **Step 5: Launch from the repository root**

Run: `.\build\bin\minesweeper.exe`

Verify: window opens at 640x480; resizing keeps the board and controls centered; clicks align with cells; F11 enters fullscreen; a second F11 restores the windowed size; the current game remains active.

- [x] **Step 6: Review the final diff**

Run: `git diff --check; git diff -- src/BoardSettings.hpp src/DisplaySettings.hpp src/UI.hpp src/UI.cpp src/main.cpp tests/BoardTests.cpp README.md`

Expected: no whitespace errors and only the planned responsive-window changes mixed into the existing files.
