# Custom board sizes

## Intent and scope

Allow players to choose rectangular board dimensions before starting a game.
The user chose editable number fields for columns and rows. Keep the existing
C++17/SFML implementation, approximately 20% mine density, and safe first reveal.
Mine density controls, difficulty presets, and persistent settings are out of scope.

## Approaches

Number fields allow direct entry of exact dimensions; increment/decrement buttons
are simpler but require repeated clicks. The user selected number fields.

## Proposed behavior

- Add labeled Columns and Rows fields to the menu, initially containing 12.
- Accept dimensions from 4 through 30 inclusive, independently for each field.
  These limits are an implementation proposal to keep boards usable in the window.
- Clicking a field focuses it; support digits, Backspace, Tab to switch fields,
  and Enter to start. Show a visible focus indicator.
- Empty or out-of-range values show an explanatory message and prevent starting;
  do not silently clamp the player's input.
- Keep the selected dimensions when returning to the menu or restarting.
- Keep the 640 by 480 window and header. Center the board below the header,
  using cells no larger than 32 pixels and shrinking them as needed to fit.
  Rendering and mouse hit testing share the same board layout calculations.
- Replace the fixed square grid with storage sized by the selected columns and
  rows; update mine placement, neighbors, reveals, counters, and win detection.
- Reject clicks outside the board before converting pixel coordinates to cells.

## Implementation and validation

Preserve existing unrelated edits in README.md, UI.cpp, and the unfinished solver.
Use focused game-logic checks for rectangular boards, boundary dimensions,
first-click safety, neighbor counts, and win detection. Build with the existing
CMake/Ninja toolchain. Check input validation and board layout at default and
maximum dimensions, and report any inability to exercise the graphical UI.

## Approval

Approved by the user on 2026-10-31 ("looking good").
