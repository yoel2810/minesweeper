#pragma once
#include "Game.hpp"
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
#include <vector>

// Automatic Minesweeper solver. See docs/solver-pseudocode.md for the algorithm.
//
// The solver only looks at information a human player sees: which cells are
// revealed, their numbers, which cells are flagged, and the total mine count.
// It acts through Game (reveal / toggleFlag), like a player, and never changes
// the game rules. Every step is a function of an immutable Snapshot, so the
// reasoning can be tested without a Game.
class Solver {
public:
  enum class CellView : std::uint8_t { Unknown, Flagged, Revealed };
  enum class Step { FirstClick, Basic, Subset, Search, Guess };

  // Read-only copy of the visible board. Cell id = column * rows + row.
  struct Snapshot {
    int columns = 0;
    int rows = 0;
    int totalMines = 0; // total mines on the board (flagged or not)
    std::vector<CellView> view;
    std::vector<int> number; // valid for Revealed cells

    int size() const { return columns * rows; }
    int id(int column, int row) const { return column * rows + row; }
    int column(int id) const { return id / rows; }
    int row(int id) const { return id % rows; }
    int neighbours(int id, int out[8]) const; // in-bounds 8-neighbourhood
    int flagCount() const;
    bool operator==(const Snapshot &o) const {
      return columns == o.columns && rows == o.rows &&
             totalMines == o.totalMines && view == o.view && number == o.number;
    }
    bool operator!=(const Snapshot &o) const { return !(*this == o); }
  };

  // "Exactly `mines` mines among `cells`" (cells sorted ascending, all unknown).
  struct Constraint {
    std::vector<int> cells;
    int mines = 0;
    bool operator<(const Constraint &o) const {
      return cells != o.cells ? cells < o.cells : mines < o.mines;
    }
    bool operator==(const Constraint &o) const {
      return cells == o.cells && mines == o.mines;
    }
  };

  struct Deductions {
    std::vector<int> safe;  // proven safe: reveal
    std::vector<int> mines; // proven mines: flag
    bool empty() const { return safe.empty() && mines.empty(); }
  };

  // A connected group of frontier cells that share constraints (Step 4).
  struct Component {
    std::vector<int> cells;       // sorted cell ids
    std::vector<int> constraints; // indices into Frontier::constraints
  };

  // Result of enumerating every valid mine assignment of one component.
  struct ComponentSolutions {
    bool exact = true; // false when the node budget ran out
    long double total = 0;
    std::vector<long double> ways; // ways[m]: solutions using m mines
    // mineWays[m][j]: solutions with m mines where local cell j is a mine
    // (inner vector empty when ways[m] == 0)
    std::vector<std::vector<long double>> mineWays;
  };

  struct Frontier {
    std::vector<Constraint> constraints; // original, from revealed numbers
    std::vector<int> cells;              // frontier cells
    std::vector<int> interior;           // unknown cells with no revealed neighbour
    std::vector<Component> components;
    std::vector<ComponentSolutions> solutions; // parallel to components
  };

  struct Probabilities {
    bool usedGlobalCount = true;
    int interiorCount = 0;
    double interiorProb = 0;
    std::vector<double> mineProb;  // per cell id; -1 when not unknown
    std::vector<char> certainSafe; // proven safe (incl. via mine total)
    std::vector<char> certainMine; // proven mine (incl. via mine total)
  };

  struct GuessResult {
    Deductions certain; // deductions only the global mine count can prove
    int cell = -1;      // cell to guess when nothing is certain
    double probability = 0;
  };

  struct Move {
    int column;
    int row;
    bool flag; // true: flagged, false: revealed
    Step step;
    bool guess; // true only for Step 5 guesses
  };

  struct SolveResult {
    bool won = false;
    int guesses = 0;
    int moves = 0;
  };

  Game &game;
  std::vector<Move> history; // moves of the current game
  int guesses = 0;

  // One turn's decision, computed from a Snapshot alone.
  struct Plan {
    Step step = Step::FirstClick;
    Deductions deductions; // proven cells to flag / reveal
    int guess = -1;        // cell id to guess when nothing is proven
  };

  explicit Solver(Game &currentGame);

  // One decision: take the first step that finds something and apply it.
  // Returns false when the game is over or nothing can be done.
  // Equivalent to applyPlan(plan(snapshot())).
  bool playTurn();
  // Run turns until the game is won or lost.
  SolveResult solve();

  // The board as the solver sees it: unproven player flags count as unknown.
  Snapshot snapshot() const;
  // Thinking half of a turn: pure, touches no shared state, so it can run on
  // a worker thread while the game keeps drawing.
  static Plan plan(const Snapshot &s);
  // Acting half: applies `p` to the game. The caller must make sure the board
  // still matches the snapshot the plan was made from. Returns true if it acted.
  bool applyPlan(const Plan &p);
  // flaggedCells()[id]: the cell carries a flag (the player's or the solver's).
  std::vector<char> flaggedCells() const;

  // Step 1: R(X)==0 -> safe, R(X)==|U(X)| -> mines.
  static Deductions stepBasicRules(const Snapshot &s);
  // Step 2: subset rule with derived constraints.
  static Deductions stepSubsetRule(const Snapshot &s);
  // Steps 3+4: enumerate each independent frontier component.
  static Deductions stepExhaustiveSearch(const Frontier &f);
  // Step 5: probabilities and the best guess.
  static GuessResult stepGuess(const Snapshot &s, const Frontier &f);
  // Hint: first unrevealed cell provably safe (no guessing), or -1. Reads only.
  // Cells with skip[id] set are never returned (empty skip: none skipped).
  static int stepHint(const Snapshot &s, const std::vector<char> &skip = {});
  // Hint for the live game: (column, row) of a proven-safe cell, or nullopt.
  // Before the first click the corner is returned, since it is always safe.
  std::optional<std::pair<int, int>> findHint() const;

  // Building blocks (public so tests can exercise them).
  static std::vector<Constraint> buildConstraints(const Snapshot &s);
  // Step 4
  static std::vector<Component>
  splitComponents(const std::vector<Constraint> &constraints, int cellCount);
  // Step 3
  static ComponentSolutions
  searchComponent(const std::vector<Constraint> &constraints,
                  const Component &component, long nodeLimit);
  static Frontier analyzeFrontier(const Snapshot &s, long nodeLimit = 2000000);
  static Probabilities computeProbabilities(const Snapshot &s,
                                            const Frontier &f);

private:
  // Flags the solver proved itself in the current game. A player's flag may
  // be wrong, so snapshot() treats every other flag as an unknown cell.
  std::set<std::pair<int, int>> provenFlags;
  std::uint64_t trackedGame = 0; // Game::gameNumber() the state above is for

  void syncWithGame(); // new game started: forget history, guesses, flags
  bool isProvenFlag(int column, int row) const;
  int applyDeductions(const Deductions &d, Step step);
  void reveal(int column, int row, Step step, bool guess);
};
