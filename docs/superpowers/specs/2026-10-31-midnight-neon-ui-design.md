# Midnight Neon UI Design

## Intent

Replace the current plain gray Minesweeper presentation with the selected
Midnight Neon direction: a focused dark arcade interface using navy surfaces,
cyan information, violet secondary accents, and restrained glow. The visual
upgrade should make the game feel deliberate and modern without changing how
it plays.

## Goals

- Apply the prototype's visual language to the menu, HUD, board, buttons,
  number fields, and win/loss overlays.
- Keep important information immediately readable at the existing 640x480
  default size and across the current resizable/fullscreen layouts.
- Replace the legacy raster-cell appearance with SFML-drawn cells so borders,
  rounding, colors, and scaling remain consistent at custom board sizes.
- Preserve clear hover/focus feedback and distinct colors for neighboring-mine
  counts, flags, mines, success, and failure.

## Non-goals

- No gameplay, mine-density, keyboard-control, board-sizing, or state-flow
  changes.
- No new settings screen, animation system, audio, online features, or saved
  high scores.
- No dependency or font download. The existing bundled DejaVu Sans font stays.

## Constraints

- C++17 and SFML 3.1.0 only.
- Continue loading all runtime resources relative to the repository root.
- Support boards from 4x4 through 30x30, the 480x360 minimum window, and F11
  fullscreen.
- Keep text and controls usable when the available window is too small for the
  generous spacing shown in the HTML prototype.

## Approaches Considered

1. **Classic Refined:** warm paper, serif typography, and traditional raised
   cells. Lowest visual risk, but less distinctive.
2. **Midnight Neon:** layered navy panels, cyan/violet accents, geometric cells,
   and subtle glow. Selected because it gives the game the strongest identity
   while retaining high contrast.
3. **Garden Playful:** sage, cream, and terracotta with softer shapes. Friendly,
   but farther from the focused puzzle-game tone.

## Selected Implementation

- Define a centralized Midnight Neon palette and shared drawing helpers in the
  UI layer instead of scattering raw colors through `main.cpp` and `UI.cpp`.
- Draw the board cells with SFML shapes and text. Closed cells use layered navy
  fills and brighter borders; revealed cells recede; flags and mines use vivid
  accent colors; number colors follow a readable cyan/violet/pink progression.
- Restyle the 48-pixel HUD as a deeper layered bar with compact label/value
  groups and a centered rounded restart control. Keep the same information and
  hit targets.
- Restyle menu and terminal-state overlays as centered dark panels with a thin
  accent border, clearer spacing, and neon primary/secondary buttons.
- Add only lightweight visual depth (outlines, offset shadows, restrained glow
  approximations). Avoid shader effects so rendering remains simple and stable.
- Update the legacy image dependency only where the new shape-based board makes
  it unnecessary; existing image files may remain in the repository unused.

## Verification

- Build the game and run the existing CTest suite.
- Exercise menu input, start, reveal, flag, restart, win/loss overlays, Escape,
  resizing, minimum window size, and fullscreen.
- Check representative small, default, and large boards for legible cell text
  and consistent alignment.
