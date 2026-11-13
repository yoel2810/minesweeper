// Solver tests: no framework, exits 1 on the first failed require().
#include "Game.hpp"
#include "Solver.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <string>

using View = Solver::CellView;

static void require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

// ------------------------------------------------------------ brute force

// Every assignment of mines to the unknown cells that satisfies all revealed
// numbers. "noTotal" ignores the global mine count, "withTotal" enforces it.
struct Tally {
  long solutions = 0;
  std::vector<long> asMine; // per unknown cell
};
struct Brute {
  std::vector<int> unknown; // cell ids
  Tally noTotal, withTotal;
};

static bool bruteForce(const Solver::Snapshot &s, Brute &b, int maxUnknown) {
  for (int id = 0; id < s.size(); id++)
    if (s.view[id] == View::Unknown)
      b.unknown.push_back(id);
  const int u = static_cast<int>(b.unknown.size());
  if (u == 0 || u > maxUnknown)
    return false;
  auto indexOf = [&](int id) {
    return static_cast<int>(
        std::find(b.unknown.begin(), b.unknown.end(), id) - b.unknown.begin());
  };
  struct Rule {
    std::uint32_t mask;
    int need;
  };
  std::vector<Rule> rules;
  int nb[8];
  for (int id = 0; id < s.size(); id++) {
    if (s.view[id] != View::Revealed)
      continue;
    Rule r{0, s.number[id]};
    const int count = s.neighbours(id, nb);
    for (int k = 0; k < count; k++) {
      if (s.view[nb[k]] == View::Unknown)
        r.mask |= 1u << indexOf(nb[k]);
      else if (s.view[nb[k]] == View::Flagged)
        r.need--;
    }
    if (r.mask)
      rules.push_back(r);
  }
  const int remaining = s.totalMines - s.flagCount();
  b.noTotal.asMine.assign(u, 0);
  b.withTotal.asMine.assign(u, 0);
  for (std::uint32_t m = 0; m < (1u << u); m++) {
    bool ok = true;
    for (const Rule &r : rules)
      if (__builtin_popcount(m & r.mask) != r.need) {
        ok = false;
        break;
      }
    if (!ok)
      continue;
    const bool totalOk = __builtin_popcount(m) == remaining;
    b.noTotal.solutions++;
    if (totalOk)
      b.withTotal.solutions++;
    for (int i = 0; i < u; i++)
      if (m >> i & 1) {
        b.noTotal.asMine[i]++;
        if (totalOk)
          b.withTotal.asMine[i]++;
      }
  }
  return true;
}

static bool certainSafe(const Tally &t, int i) {
  return t.solutions > 0 && t.asMine[i] == 0;
}
static bool certainMine(const Tally &t, int i) {
  return t.solutions > 0 && t.asMine[i] == t.solutions;
}

// ------------------------------------------------------------ scenarios

// A real game state: random mines, a safe opening, a few extra safe reveals
// and some correctly flagged mines.
static void randomScenario(Game &g, std::mt19937 &rng, int cols, int rows) {
  g.seed(static_cast<std::uint32_t>(rng()));
  g.start(cols, rows);
  const int fc = rng() % cols, fr = rng() % rows;
  g.reveal(fc, fr);
  const int extra = rng() % 6;
  for (int k = 0; k < extra; k++) {
    const int c = rng() % cols, r = rng() % rows;
    if (!g.cell(c, r).isMine && !g.cell(c, r).isRevealed)
      g.reveal(c, r);
  }
  for (int c = 0; c < cols; c++)
    for (int r = 0; r < rows; r++)
      if (g.cell(c, r).isMine && rng() % 2)
        g.toggleFlag(c, r);
}

// The board as a player with correct flags sees it: unlike
// Solver::snapshot(), every flag on the board counts.
static Solver::Snapshot snapshotWithFlags(const Game &g) {
  Solver::Snapshot s;
  s.columns = g.columns();
  s.rows = g.rows();
  s.totalMines = g.mineCount();
  s.view.assign(s.size(), View::Unknown);
  s.number.assign(s.size(), 0);
  for (int c = 0; c < s.columns; c++)
    for (int r = 0; r < s.rows; r++) {
      const Cell &cell = g.cell(c, r);
      if (cell.isRevealed) {
        s.view[s.id(c, r)] = View::Revealed;
        s.number[s.id(c, r)] = cell.minesCount;
      } else if (cell.isFlag) {
        s.view[s.id(c, r)] = View::Flagged;
      }
    }
  return s;
}

static std::set<int> asSet(const std::vector<int> &v) {
  return {v.begin(), v.end()};
}

// ------------------------------------------------------------ tests

static void testHandBuiltSubsetPattern() {
  // Top row revealed 1 2 2 1, the row below unknown (classic 1-2-2-1).
  // No single number decides anything, but subsets do: cells (1,1) and (2,1)
  // must be mines.
  Solver::Snapshot s;
  s.columns = 4;
  s.rows = 4;
  s.totalMines = 4;
  s.view.assign(16, View::Unknown);
  s.number.assign(16, 0);
  const int top[4] = {1, 2, 2, 1};
  for (int c = 0; c < 4; c++) {
    s.view[s.id(c, 0)] = View::Revealed;
    s.number[s.id(c, 0)] = top[c];
  }
  const auto basic = Solver::stepBasicRules(s);
  require(basic.empty(), "basic rules should find nothing in 1-2-2-1");
  const auto subset = Solver::stepSubsetRule(s);
  require(subset.safe.empty(), "subset rule marked something safe in 1-2-2-1");
  require(asSet(subset.mines) == std::set<int>({s.id(1, 1), s.id(2, 1)}),
          "subset rule should flag (1,1) and (2,1)");
  // The search must find at least everything the subset rule found, plus the
  // outer cells of the row, which are safe.
  const auto f = Solver::analyzeFrontier(s);
  const auto search = Solver::stepExhaustiveSearch(f);
  require(asSet(search.mines) == asSet(subset.mines),
          "search disagrees with subset on mines");
  require(asSet(search.safe) ==
              std::set<int>({s.id(0, 1), s.id(3, 1)}),
          "search should prove (0,1) and (3,1) safe");
  require(f.components.size() == 1, "1-2-2-1 is a single component");
}

static void testComponentsSplit() {
  // Two revealed numbers far apart never share an unknown cell.
  Solver::Snapshot s;
  s.columns = 9;
  s.rows = 4;
  s.totalMines = 2;
  s.view.assign(36, View::Unknown);
  s.number.assign(36, 0);
  s.view[s.id(0, 0)] = View::Revealed;
  s.number[s.id(0, 0)] = 1;
  s.view[s.id(8, 3)] = View::Revealed;
  s.number[s.id(8, 3)] = 1;
  const auto f = Solver::analyzeFrontier(s);
  require(f.components.size() == 2, "far numbers must be separate components");
  require(f.cells.size() == 3 + 3, "frontier size");
  require(f.interior.size() == 36 - 2 - 6, "interior size");
}

static void testAgainstBruteForce() {
  std::mt19937 rng(12345);
  int checked = 0, subsetFinds = 0, searchBeyondBasic = 0, certainByTotal = 0;
  for (int attempt = 0; attempt < 4000 && checked < 400; attempt++) {
    const int cols = 4 + rng() % 3, rows = 4 + rng() % 2;
    Game g;
    randomScenario(g, rng, cols, rows);
    if (g.state() != GameState::PLAYING)
      continue;
    const Solver::Snapshot s = snapshotWithFlags(g);
    Brute b;
    if (!bruteForce(s, b, 18))
      continue;
    checked++;
    require(b.noTotal.solutions > 0, "real layout must be a solution");
    auto indexOf = [&](int id) {
      return static_cast<int>(
          std::find(b.unknown.begin(), b.unknown.end(), id) -
          b.unknown.begin());
    };

    // Basic and subset rules must be sound (never wrong), not complete.
    const auto basic = Solver::stepBasicRules(s);
    const auto subset = Solver::stepSubsetRule(s);
    for (const auto *d : {&basic, &subset}) {
      for (int id : d->safe)
        require(certainSafe(b.noTotal, indexOf(id)), "unsound safe deduction");
      for (int id : d->mines)
        require(certainMine(b.noTotal, indexOf(id)), "unsound mine deduction");
    }
    // Subset rule must subsume the basic rules' outcome only where it can:
    // it is at least sound; count how often it beats Step 1.
    for (int id : subset.safe)
      if (!asSet(basic.safe).count(id))
        subsetFinds++;
    for (int id : subset.mines)
      if (!asSet(basic.mines).count(id))
        subsetFinds++;

    // Search must be exactly the set of frontier cells that are certain
    // under the number constraints alone.
    const auto f = Solver::analyzeFrontier(s);
    const auto search = Solver::stepExhaustiveSearch(f);
    std::set<int> expectSafe, expectMine;
    for (int id : f.cells) {
      const int i = indexOf(id);
      if (certainSafe(b.noTotal, i))
        expectSafe.insert(id);
      if (certainMine(b.noTotal, i))
        expectMine.insert(id);
    }
    require(asSet(search.safe) == expectSafe, "search safe set != brute force");
    require(asSet(search.mines) == expectMine,
            "search mine set != brute force");
    for (int id : search.safe)
      if (!asSet(basic.safe).count(id) && !asSet(subset.safe).count(id))
        searchBeyondBasic++;

    // Probabilities (with the mine total) must equal brute-force frequencies,
    // and certainty flags must match exactly.
    const auto pr = Solver::computeProbabilities(s, f);
    require(pr.usedGlobalCount, "global count must be consistent here");
    for (int id : b.unknown) {
      const int i = indexOf(id);
      const double expected =
          static_cast<double>(b.withTotal.asMine[i]) / b.withTotal.solutions;
      require(std::fabs(pr.mineProb[id] - expected) < 1e-9,
              "probability mismatch at cell " + std::to_string(id) + ": " +
                  std::to_string(pr.mineProb[id]) + " vs " +
                  std::to_string(expected));
      require(static_cast<bool>(pr.certainSafe[id]) ==
                  certainSafe(b.withTotal, i),
              "certainSafe flag mismatch");
      require(static_cast<bool>(pr.certainMine[id]) ==
                  certainMine(b.withTotal, i),
              "certainMine flag mismatch");
      if (certainSafe(b.withTotal, i) && !certainSafe(b.noTotal, i))
        certainByTotal++;
    }

    // The guess must pick a minimum-probability unknown cell, and only when
    // nothing is certain.
    const auto guess = Solver::stepGuess(s, f);
    if (!guess.certain.empty()) {
      for (int id : guess.certain.safe)
        require(certainSafe(b.withTotal, indexOf(id)), "bad certain safe");
      for (int id : guess.certain.mines)
        require(certainMine(b.withTotal, indexOf(id)), "bad certain mine");
    } else {
      require(guess.cell >= 0 && s.view[guess.cell] == View::Unknown,
              "guess must be an unknown cell");
      double best = 2;
      for (int id : b.unknown)
        best = std::min(best, static_cast<double>(
                                  b.withTotal.asMine[indexOf(id)]) /
                                  b.withTotal.solutions);
      require(std::fabs(guess.probability - best) < 1e-9,
              "guess is not the lowest-probability cell");
    }
  }
  require(checked >= 200, "too few brute-force scenarios generated");
  std::cout << "brute-force scenarios: " << checked
            << ", subset-only finds: " << subsetFinds
            << ", search beyond rules: " << searchBeyondBasic
            << ", certain only via mine total: " << certainByTotal << '\n';
  require(subsetFinds > 0, "subset rule never found anything beyond Step 1");
}

// Full automatic games: every non-guess move must be correct, a loss can only
// follow a guess, and every flag on the board is on a real mine.
static void testFullGames(int cols, int rows, int games, unsigned seed,
                          bool expectSomeWins) {
  int wins = 0, totalGuesses = 0, deductions = 0;
  for (int n = 0; n < games; n++) {
    Game g;
    g.seed(seed + n);
    g.start(cols, rows);
    Solver solver(g);
    const auto result = solver.solve();
    require(g.state() == GameState::GAME_WON || g.state() == GameState::GAME_OVER,
            "solve() left the game unfinished");
    require(result.won == (g.state() == GameState::GAME_WON), "result.won mismatch");
    int guessMoves = 0;
    for (const auto &m : solver.history) {
      const Cell &cell = g.cell(m.column, m.row);
      if (m.guess) {
        guessMoves++;
        continue;
      }
      deductions++;
      if (m.flag)
        require(cell.isMine, "WRONG FLAG: solver flagged a safe cell");
      else
        require(!cell.isMine, "WRONG REVEAL: solver revealed a mine "
                              "without guessing");
    }
    require(guessMoves == result.guesses, "guess count mismatch");
    for (int c = 0; c < cols; c++)
      for (int r = 0; r < rows; r++)
        if (g.cell(c, r).isFlag)
          require(g.cell(c, r).isMine, "board holds a wrong flag");
    if (g.state() == GameState::GAME_OVER)
      require(!solver.history.empty() && solver.history.back().guess,
              "game lost without a guess");
    else
      require(g.checkWinCondition(), "win without all safe cells revealed");
    wins += result.won;
    totalGuesses += result.guesses;
  }
  std::cout << cols << "x" << rows << ": " << wins << "/" << games
            << " won, " << totalGuesses << " guesses, " << deductions
            << " proven moves, 0 wrong\n";
  if (expectSomeWins)
    require(wins > 0, "solver never wins");
}

static void testFirstClickIsCorner() {
  Game g;
  g.seed(7);
  g.start(8, 8);
  Solver solver(g);
  require(solver.playTurn(), "first turn did nothing");
  require(solver.history.size() == 1 && solver.history[0].column == 0 &&
              solver.history[0].row == 0 && !solver.history[0].guess,
          "first move must be a non-guess corner click");
  require(g.minesPlaced() && g.cell(0, 0).isRevealed,
          "corner not opened via the game's first-click convention");
}

// A new game (restart) must not inherit the previous game's history.
static void testNewGameResetsSolver() {
  Game g;
  g.seed(3);
  g.start(8, 8);
  Solver solver(g);
  solver.solve();
  require(!solver.history.empty(), "solver did nothing");
  g.restart();
  require(solver.playTurn(), "no turn after restart");
  require(solver.history.size() == 1 && solver.guesses == 0,
          "solver kept the previous game's history");
}

// Wrong player flags next to revealed numbers must never make the hint point
// at a mine, nor make the solver open a mine without guessing.
static void testWrongPlayerFlags() {
  std::mt19937 rng(777);
  int wrongFlags = 0;
  for (int k = 0; k < 200; k++) {
    Game g;
    randomScenario(g, rng, 9, 9);
    if (g.state() != GameState::PLAYING)
      continue;
    // Flag a few safe unknown cells that touch a revealed number.
    for (int c = 0; c < 9; c++)
      for (int r = 0; r < 9; r++) {
        const Cell &cell = g.cell(c, r);
        if (cell.isMine || cell.isRevealed || cell.isFlag || rng() % 3)
          continue;
        bool touches = false;
        for (int dc = -1; dc <= 1; dc++)
          for (int dr = -1; dr <= 1; dr++)
            touches |= g.inBounds(c + dc, r + dr) &&
                       g.cell(c + dc, r + dr).isRevealed;
        if (touches && g.toggleFlag(c, r))
          wrongFlags++;
      }

    Solver solver(g);
    if (const auto hint = solver.findHint()) {
      const Cell &cell = g.cell(hint->first, hint->second);
      require(!cell.isMine, "hint trusted a wrong flag and pointed at a mine");
      require(!cell.isFlag, "hint pointed at a flagged cell");
    }
    solver.solve();
    require(g.state() != GameState::PLAYING, "solver stalled on wrong flags");
    for (const auto &m : solver.history)
      if (!m.guess)
        require(m.flag == g.cell(m.column, m.row).isMine,
                "solver made a wrong non-guess move with wrong flags around");
    if (g.state() == GameState::GAME_OVER)
      require(solver.history.back().guess, "lost without a guess");
  }
  require(wrongFlags > 50, "too few wrong flags placed to test anything");
}

// Large boards produce huge solution counts and binomials; probabilities must
// stay finite (raw counts overflow a double, which is MSVC's long double).
static void testProbabilitiesStayFinite() {
  // 30x30: isolated revealed 4s, each ringed by 8 unknown cells (70 ways
  // each), on a 4-cell grid so the blocks are separate components and a
  // large interior remains.
  Solver::Snapshot s;
  s.columns = 30;
  s.rows = 30;
  s.view.assign(s.size(), View::Unknown);
  s.number.assign(s.size(), 0);
  int blocks = 0;
  for (int c = 1; c < 30; c += 4)
    for (int r = 1; r < 30; r += 4) {
      if (c + 1 >= 30 || r + 1 >= 30)
        continue;
      s.view[s.id(c, r)] = View::Revealed;
      s.number[s.id(c, r)] = 4;
      blocks++;
    }
  s.totalMines = blocks * 4 + 200;
  const Solver::Frontier f = Solver::analyzeFrontier(s);
  const Solver::Probabilities pr = Solver::computeProbabilities(s, f);
  require(pr.usedGlobalCount, "a consistent mine total was ignored");
  for (int id = 0; id < s.size(); id++) {
    if (s.view[id] != View::Unknown)
      continue;
    require(std::isfinite(pr.mineProb[id]) && pr.mineProb[id] >= 0.0 &&
                pr.mineProb[id] <= 1.0,
            "probability is not a finite value in [0, 1]");
    require(!pr.certainSafe[id] && !pr.certainMine[id],
            "nothing is certain on this board");
  }
  // Every ring cell is symmetric: 4 of 8, so exactly one half.
  require(std::abs(pr.mineProb[s.id(0, 0)] - 0.5) < 1e-9,
          "ring cell probability should be 0.5");
}

static void testHintFindsSearchSafeCell() {
  // Same 1-2-2-1 board: the hint must be one of the outer cells the search
  // proves safe, never a mine.
  Solver::Snapshot s;
  s.columns = 4;
  s.rows = 4;
  s.totalMines = 4;
  s.view.assign(16, View::Unknown);
  s.number.assign(16, 0);
  const int top[4] = {1, 2, 2, 1};
  for (int c = 0; c < 4; c++) {
    s.view[s.id(c, 0)] = View::Revealed;
    s.number[s.id(c, 0)] = top[c];
  }
  const int hint = Solver::stepHint(s);
  require(hint == s.id(0, 1) || hint == s.id(3, 1),
          "hint should be a proven-safe cell of 1-2-2-1");
}

static void testNoHintOnEmptyBoard() {
  // Nothing revealed and half the board mined: no cell is provably safe.
  Solver::Snapshot s;
  s.columns = 4;
  s.rows = 4;
  s.totalMines = 8;
  s.view.assign(16, View::Unknown);
  s.number.assign(16, 0);
  require(Solver::stepHint(s) == -1,
          "no cell is provable on an empty board, so no hint");
}

static void testHintNeverMine() {
  std::mt19937 rng(42);
  for (int k = 0; k < 300; k++) {
    Game g;
    randomScenario(g, rng, 8, 8);
    Solver solver(g);
    const auto hint = solver.findHint();
    if (!hint)
      continue;
    const Cell &c = g.cell(hint->first, hint->second);
    require(!c.isMine, "hint pointed at a mine");
    require(!c.isRevealed && !c.isFlag, "hint pointed at a non-unknown cell");
  }
}

int main() {
  testHandBuiltSubsetPattern();
  testComponentsSplit();
  testFirstClickIsCorner();
  testNewGameResetsSolver();
  testWrongPlayerFlags();
  testProbabilitiesStayFinite();
  testHintFindsSearchSafeCell();
  testNoHintOnEmptyBoard();
  testHintNeverMine();
  testAgainstBruteForce();
  testFullGames(8, 8, 300, 1000, true);
  testFullGames(12, 12, 150, 5000, true);
  testFullGames(30, 16, 10, 9000, false);
  std::cout << "Solver checks passed\n";
}
