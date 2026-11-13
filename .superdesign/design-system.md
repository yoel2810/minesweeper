# Minesweeper — Design Brief (shared across all 3 directions)

## Product context
A desktop Minesweeper game written in C++17 with SFML 3.1 (not a web app). The HTML mockups are
visual targets that will be re-implemented with SFML primitives, so every design must be buildable from:
- filled rectangles and rounded rectangles (solid colors, optional outline), circles, lines
- solid-color hard drop shadows (an offset rectangle) — NO blur, NO gradients, NO glassmorphism, NO images/photos
- ONE bundled TTF font per design (a Google Font is fine; pick one and use it everywhere)
- text at a few sizes; no icon fonts — flags and mines are drawn from simple shapes (triangle + line for a flag, circle + spikes for a mine)

## Window & layout
- Default window 640x480 (4:3), resizable, min 480x360, F11 fullscreen. Mock at 960x720 (1.5x scale).
- Top HUD bar: mines-remaining counter (left), restart button (center), elapsed-time counter (right).
- Board centered below the HUD, square cells, scales to fit. Boards range 4x4 to 30x30; mock a 12x12 board.
- Board margin of at least 16px (at 1x) around the board.

## Screens to show (all in one mock page, as 4 framed 960x720 "screens" stacked vertically, each labelled)
1. MENU — game title, two numeric input fields "Columns" and "Rows" (one shown focused), a "Start Game" primary button, a small inline validation message ("Enter columns and rows from 4 to 30."), board dimmed/hidden behind.
2. PLAYING — 12x12 mid-game board: a mix of hidden cells, revealed empty cells, revealed numbers 1–5 (each number its own distinct color), 3–4 flags, one hovered hidden cell. HUD shows e.g. 019 mines left and 0:47.
3. GAME OVER — board with all mines revealed, the clicked mine highlighted, a centered modal/panel: "Game Over", time, primary "Restart" and secondary "Menu" buttons.
4. VICTORY — solved board, centered panel: "You Win!", time, primary "Play Again" and secondary "Menu" buttons.

## Interaction states to make visible
Hidden cell, hovered hidden cell, revealed cell, flagged cell, mine, exploded mine, button normal/hover, input normal/focused.

## Quality bar
Numbers must be instantly readable at 24px cells. Hidden vs revealed must be distinguishable at a glance without color alone (use depth/border/fill contrast). Avoid generic "dashboard" looks.
