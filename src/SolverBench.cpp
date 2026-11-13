// Runs the solver automatically on full games and reports win/loss, the
// number of guesses and the slowest single turn (the GUI runs one turn per
// frame, so that is the worst frame stall).
// Usage: solver_bench [columns rows games [seed [minePercent]]]
#include "Game.hpp"
#include "Solver.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  const int columns = argc > 1 ? std::atoi(argv[1]) : 12;
  const int rows = argc > 2 ? std::atoi(argv[2]) : 12;
  const int games = argc > 3 ? std::atoi(argv[3]) : 20;
  const unsigned seed = argc > 4 ? static_cast<unsigned>(std::atoi(argv[4])) : 1;
  const int percent = argc > 5 ? std::atoi(argv[5]) : DEFAULT_MINE_PERCENT;

  using Clock = std::chrono::steady_clock;
  int wins = 0, totalGuesses = 0;
  double slowestMs = 0;
  for (int n = 0; n < games; n++) {
    Game game;
    game.seed(seed + n);
    if (!game.start(columns, rows, percent)) {
      std::cerr << "Board size must be " << MIN_BOARD_SIZE << "-"
                << MAX_BOARD_SIZE << " per side, mines " << MIN_MINE_PERCENT
                << "-" << MAX_MINE_PERCENT << "%\n";
      return 1;
    }
    Solver solver(game);
    double gameSlowestMs = 0;
    for (;;) {
      const auto begin = Clock::now();
      const bool acted = solver.playTurn();
      gameSlowestMs = std::max(
          gameSlowestMs,
          std::chrono::duration<double, std::milli>(Clock::now() - begin).count());
      if (!acted)
        break;
    }
    const bool won = game.state() == GameState::GAME_WON;
    std::cout << "game " << n + 1 << ": " << (won ? "WON " : "LOST")
              << "  guesses=" << solver.guesses
              << "  moves=" << solver.history.size()
              << "  slowest turn=" << gameSlowestMs << " ms\n";
    wins += won;
    totalGuesses += solver.guesses;
    slowestMs = std::max(slowestMs, gameSlowestMs);
  }
  std::cout << "---\n"
            << columns << "x" << rows << " @" << percent << "%: won " << wins
            << "/" << games << ", avg guesses per game "
            << (games ? static_cast<double>(totalGuesses) / games : 0.0)
            << ", slowest turn " << slowestMs << " ms\n";
}
