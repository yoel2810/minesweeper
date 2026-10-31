# Custom Board Sizes Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Play rectangular boards using numeric dimensions entered in the menu.

**Architecture:** Game owns dynamic column-major storage and validated dimensions. A shared board layout controls rendering and hit testing. Menu fields handle bounded numeric text and focus.

**Tech Stack:** C++17, SFML 3.1, CMake/Ninja.

**Spec:** ../specs/2026-10-31-custom-board-sizes-design.md (approved by user).

## Global Constraints

- Dimensions 4 through 30 inclusive; default 12 by 12.
- Keep 640 by 480 window, cells at most 32 pixels, safe first reveal, approximately 20% mines.
- Preserve unrelated working changes and retain dimensions across restarts.

## Review Focus

- Rectangles in both orientations must address their last row and column correctly.
- Empty, nonnumeric, or out-of-range inputs cannot start a game.
- Clicks immediately outside every board edge must be ignored.
- Editing fields must not activate the global restart shortcut.
- Resizing and restarting must clear flags and revealed cells.

## Task 1: Dynamic game and layout

Files: src/Game.hpp, src/Game.cpp, src/BoardSettings.hpp, tests/BoardTests.cpp, CMakeLists.txt.

- [x] Add focused tests for dimensions, mine counts, safe first clicks, neighbors, win detection, restart, parsing, and board-edge hit tests.
- [x] Add Game::start(int columns, int rows), columns()/rows() accessors, and dynamic grid storage; update all bounds and loops.
- [x] Add BoardLayout::fit(int columns, int rows) and cellAt(int x, int y), plus parseBoardDimension(const std::string&).
- [x] Configure and run the CTest board test target; verify all checks pass.

## Task 2: Numeric menu fields and integration

Files: src/UI.hpp, src/UI.cpp, src/main.cpp.

- [x] Add NumberField with focus, digit entry, Backspace, validation, and drawing.
- [x] Add Columns and Rows fields, Tab focus switching, Enter/start-button validation, and inline error text.
- [x] Render and hit-test using BoardLayout; scale sprite textures to the chosen cell size.
- [x] Build the app and check menu event routing, selected-size retention, and default/maximum layouts.
- [x] Review the diff, run CTest, and record any graphical verification limitations.

## Verification record

- `cmake --build build`: passed.
- `ctest --test-dir build --output-on-failure`: 1/1 test passed, covering game logic, layout edges, dimension parsing, and numeric field editing.
- `git diff --check`: passed (existing Windows line-ending warnings only).
- Independent code review completed; Enter-key validation-message clearing was fixed.
- Menu event routing and layouts were checked in code and automated checks; manual graphical interaction was not performed.
- Changes remain in the existing working tree, uncommitted, preserving the user's unrelated edits.
