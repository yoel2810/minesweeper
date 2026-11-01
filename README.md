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

how to run:
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/minesweeper.exe

or

cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
Start-Process .\build-vs\bin\Release\minesweeper.exe
