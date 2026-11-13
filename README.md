## Overview

Build a Minesweeper game in C++, then create a machine-learning agent that can learn to solve it.

## Goals

- Build a complete Minesweeper implementation from scratch.
- Represent the game state in a form suitable for machine learning.
- Generate training data through gameplay.
- Train and evaluate an agent that learns to make decisions.

## Concepts to Learn

- C++ game architecture
- Grid and state representation
- Probability and uncertainty
- Machine learning fundamentals
- Training data generation
- Model evaluation
- Reinforcement learning or supervised learning

## Milestones

- [V]  Implement the Minesweeper game.
- [V]  Implement win/loss detection and game state management.
- [ ]  Create a non-ML solver as a baseline.
- [ ]  Design the ML problem and state representation.
- [ ]  Generate training data.
- [ ]  Train the first model.
- [ ]  Evaluate the model against the baseline.
- [ ]  Visualize the agent playing.

## Build and run on Windows

Requires MSYS2 UCRT64 with GCC (C++17), CMake 3.28 or newer, and Ninja.
The commands below assume MSYS2 is installed at `C:\msys64`; adjust the paths
if your installation or project folder is elsewhere.

### Git Bash

```bash
cd /c/Users/yoel2/Projects/minesweeper
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/minesweeper.exe
```

### PowerShell

```powershell
cd C:\Users\yoel2\Projects\minesweeper
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake -S . -B build -G Ninja
cmake --build build
.\build\bin\minesweeper.exe
```

### Visual Studio fallback

If Ninja or the MSYS2 compiler is not available, use the Visual Studio 2022
compiler instead. Run these commands from PowerShell in the project root:

```powershell
cd C:\Users\yoel2\Projects\Minesweeper
cmake -S . -B build-vs -G "Visual Studio 17 2022"
cmake --build build-vs --config Release --parallel 1
.\build-vs\bin\Release\minesweeper.exe
```

The first configure downloads SFML 3.1.0 and requires an internet connection;
it can take a while. For subsequent runs, you can skip the configure command
unless the build configuration changes.

The game finds `assets/fonts/` next to the executable or in the repository
above `build/bin` (or `build-vs/bin/<Config>`), so it can be launched from
any directory. Single-config builds (Ninja) default to `Release`; pass
`-DCMAKE_BUILD_TYPE=Debug` to the configure command for a debug build.

Run the tests with `ctest --test-dir build`. `build/bin/solver_bench.exe
[columns rows games [seed [minePercent]]]` plays the solver on random boards
and reports wins, guesses and the slowest turn.

The window can be resized by dragging its border. Press `F11` while playing
or from a menu to toggle fullscreen; press `F11` again to restore the previous
windowed size.

The PATH command applies only to the current terminal session. Run it again
in a new terminal so Windows can find the MSYS2 runtime DLLs, including
`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, and `libwinpthread-1.dll`.
