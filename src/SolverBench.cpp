// Runs the solver automatically on full games and reports win/loss and the
// number of guesses. Usage: solver_bench [columns rows games [seed]]
#include "Game.hpp"
#include "Solver.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  const int columns = argc > 1 ? std::atoi(argv[1]) : 12;
  const int rows = argc > 2 ? std::atoi(argv[2]) : 12;
  const int games = argc > 3 ? std::atoi(argv[3]) : 20;
  const unsigned seed = argc > 4 ? static_cast<unsigned>(std::atoi(argv[4])) : 1;

  int wins = 0, totalGuesses = 0;
  for (int n = 0; n < games; n++) {
    std::srand(seed + n);
    Game game;
    if (!game.start(columns, rows)) {
      std::cerr << "Board size must be " << MIN_BOARD_SIZE << "-"
                << MAX_BOARD_SIZE << " per side\n";
      return 1;
    }
    Solver solver(game);
    const auto result = solver.solve();
    std::cout << "game " << n + 1 << ": " << (result.won ? "WON " : "LOST")
              << "  guesses=" << result.guesses << "  moves=" << result.moves
              << '\n';
    wins += result.won;
    totalGuesses += result.guesses;
  }
  std::cout << "---\n"
            << columns << "x" << rows << ": won " << wins << "/" << games
            << ", avg guesses per game "
            << (games ? static_cast<double>(totalGuesses) / games : 0.0)
            << '\n';
}
