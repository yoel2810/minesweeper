# Responsive Window and Fullscreen Design

## Intent

Let players resize the Minesweeper window and switch between windowed and fullscreen play without distorting the interface or breaking mouse input.

## Goals

- Make the window resizable, with a practical minimum size.
- Recalculate the board, HUD, menus, overlays, and buttons from the current drawable size.
- Toggle borderless fullscreen with F11 and restore the previous windowed size on the next toggle.
- Keep board textures square and capped at their native 32-pixel size; larger windows add surrounding space instead of enlarging and blurring cells.
- Show the F11 shortcut in the on-screen help.

## Non-goals and constraints

- Board dimensions, game rules, and saved state do not change when the window mode changes.
- The existing SFML 3.1 dependency and 640x480 initial size remain.
- Recreating the SFML window for fullscreen must restore the 60 FPS limit and minimum-size constraint.

## Approaches

Two approaches were considered. Scaling a fixed 640x480 scene would require letterboxing and could blur the bitmap cells. The selected responsive approach computes positions from the current window size, keeps text and cells crisp, and uses the same computed board layout for rendering and hit testing.

## Behavior

`BoardLayout::fit` accepts the current drawable width and height. UI draw functions center their content and size the HUD from those same dimensions. Buttons and number fields are repositioned whenever the window size changes or the window is recreated. F11 recreates the window using the desktop video mode in borderless fullscreen, or the remembered windowed size and standard resizable style when returning. Existing gameplay continues uninterrupted.

## Verification

Automated layout checks cover multiple window sizes, board bounds, centering, and hit testing. The complete test executable and application build must pass. A launch check will confirm that the app opens from the repository root.
