#include "Solver.hpp"
#include <algorithm>
#include <iterator>
#include <numeric>
#include <set>

namespace {
constexpr std::size_t MAX_DERIVED_CONSTRAINTS = 4000;
constexpr double TIE_EPS = 1e-9;
using Poly = std::vector<long double>;

Poly convolve(const Poly &a, const Poly &b) {
  Poly out(a.size() + b.size() - 1, 0.0L);
  for (std::size_t i = 0; i < a.size(); i++)
    if (a[i] != 0)
      for (std::size_t j = 0; j < b.size(); j++)
        out[i + j] += a[i] * b[j];
  return out;
}

// Backtracking state for one component: a single mutable assignment with
// assign/undo, and per-constraint counters so violations are caught at once.
struct Searcher {
  struct LC {
    std::vector<int> cells;
    int need = 0;
    int mines = 0; // assigned mines
    int open = 0;  // unassigned cells
  };
  std::vector<LC> lcs;
  std::vector<std::vector<int>> cellCons;
  std::vector<int> order;
  std::vector<char> value;
  int n = 0;
  int minesNow = 0;
  long nodes = 0;
  long limit = 0;
  bool aborted = false;
  Solver::ComponentSolutions out;

  // Place a value, update counters, report whether all touched constraints
  // can still be satisfied (not too many mines, not too few cells left).
  bool assign(int cell, bool mine) {
    bool ok = true;
    for (int ci : cellCons[cell]) {
      LC &c = lcs[ci];
      c.open--;
      if (mine)
        c.mines++;
      if (c.mines > c.need || c.mines + c.open < c.need)
        ok = false;
    }
    return ok;
  }
  void undo(int cell, bool mine) {
    for (int ci : cellCons[cell]) {
      LC &c = lcs[ci];
      c.open++;
      if (mine)
        c.mines--;
    }
  }

  void dfs(int depth) {
    if (aborted)
      return;
    if (++nodes > limit) {
      aborted = true;
      return;
    }
    if (depth == n) {
      out.ways[minesNow] += 1;
      auto &row = out.mineWays[minesNow];
      if (row.empty())
        row.assign(n, 0.0L);
      for (int j = 0; j < n; j++)
        if (value[j])
          row[j] += 1;
      return;
    }
    const int cell = order[depth];
    for (int v = 0; v <= 1; v++) {
      value[cell] = static_cast<char>(v);
      minesNow += v;
      if (assign(cell, v == 1))
        dfs(depth + 1);
      undo(cell, v == 1);
      minesNow -= v;
      value[cell] = 0;
      if (aborted)
        return;
    }
  }
};
} // namespace

int Solver::Snapshot::neighbours(int cell, int out[8]) const {
  const int c = column(cell), r = row(cell);
  int count = 0;
  for (int dx = -1; dx <= 1; dx++)
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0)
        continue;
      const int nc = c + dx, nr = r + dy;
      if (nc >= 0 && nc < columns && nr >= 0 && nr < rows)
        out[count++] = id(nc, nr);
    }
  return count;
}

int Solver::Snapshot::flagCount() const {
  return static_cast<int>(
      std::count(view.begin(), view.end(), CellView::Flagged));
}

Solver::Solver(Game &currentGame) : game(currentGame) {}

Solver::Snapshot Solver::snapshot() const {
  Snapshot s;
  s.columns = game.columns();
  s.rows = game.rows();
  s.totalMines = game.mineCount(); // the mine counter every player can see
  s.view.assign(s.size(), CellView::Unknown);
  s.number.assign(s.size(), 0);
  for (int c = 0; c < s.columns; c++)
    for (int r = 0; r < s.rows; r++) {
      const Cell &cell = game.grid[c][r];
      const int id = s.id(c, r);
      if (cell.isRevealed) {
        s.view[id] = CellView::Revealed;
        s.number[id] = cell.minesCount;
      } else if (cell.isFlag) {
        s.view[id] = CellView::Flagged;
      }
    }
  return s;
}

// ---------------------------------------------------------------- Step 0

bool Solver::stepFirstClick(const Snapshot &s) {
  if (std::any_of(s.view.begin(), s.view.end(),
                  [](CellView v) { return v == CellView::Revealed; }))
    return false;
  history.clear();
  guesses = 0;
  if (game.isFirstClick) { // mines are placed lazily on the first click
    game.resetGame(0, 0);
    game.isFirstClick = false;
  }
  reveal(0, 0, Step::FirstClick, false);
  return true;
}

// ---------------------------------------------------------------- Step 1

std::vector<Solver::Constraint> Solver::buildConstraints(const Snapshot &s) {
  std::vector<Constraint> out;
  int nb[8];
  for (int id = 0; id < s.size(); id++) {
    if (s.view[id] != CellView::Revealed)
      continue;
    const int count = s.neighbours(id, nb);
    Constraint c;
    int flags = 0;
    for (int k = 0; k < count; k++) {
      if (s.view[nb[k]] == CellView::Unknown)
        c.cells.push_back(nb[k]); // U(X): unknown only
      else if (s.view[nb[k]] == CellView::Flagged)
        flags++;
    }
    if (c.cells.empty())
      continue;
    c.mines = s.number[id] - flags; // R(X)
    if (c.mines < 0 || c.mines > static_cast<int>(c.cells.size()))
      continue; // inconsistent (only possible with wrong flags)
    std::sort(c.cells.begin(), c.cells.end());
    out.push_back(std::move(c));
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

Solver::Deductions Solver::stepBasicRules(const Snapshot &s) {
  std::set<int> safe, mines;
  for (const Constraint &c : buildConstraints(s)) {
    if (c.mines == 0)
      safe.insert(c.cells.begin(), c.cells.end());
    else if (c.mines == static_cast<int>(c.cells.size()))
      mines.insert(c.cells.begin(), c.cells.end());
  }
  return {{safe.begin(), safe.end()}, {mines.begin(), mines.end()}};
}

// ---------------------------------------------------------------- Step 2

Solver::Deductions Solver::stepSubsetRule(const Snapshot &s) {
  std::vector<Constraint> cs = buildConstraints(s);
  std::set<Constraint> known(cs.begin(), cs.end());
  std::set<int> safe, mines;

  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t i = 0; i < cs.size(); i++) {
      for (std::size_t j = 0; j < cs.size(); j++) {
        if (i == j)
          continue;
        const Constraint a = cs[i], b = cs[j]; // copies: cs may grow below
        if (a.cells.size() >= b.cells.size() ||
            !std::includes(b.cells.begin(), b.cells.end(), a.cells.begin(),
                           a.cells.end()))
          continue; // need cells(A) strict subset of cells(B)
        Constraint d;
        std::set_difference(b.cells.begin(), b.cells.end(), a.cells.begin(),
                            a.cells.end(), std::back_inserter(d.cells));
        d.mines = b.mines - a.mines;
        if (d.mines < 0 || d.mines > static_cast<int>(d.cells.size()))
          continue; // contradicts the board; never happens on valid input
        if (d.mines == 0) {
          safe.insert(d.cells.begin(), d.cells.end());
        } else if (d.mines == static_cast<int>(d.cells.size())) {
          mines.insert(d.cells.begin(), d.cells.end());
        } else if (cs.size() < MAX_DERIVED_CONSTRAINTS &&
                   known.insert(d).second) { // skip duplicates
          cs.push_back(d);
          changed = true;
        }
      }
    }
  }
  return {{safe.begin(), safe.end()}, {mines.begin(), mines.end()}};
}

// ---------------------------------------------------------------- Step 4

std::vector<Solver::Component>
Solver::splitComponents(const std::vector<Constraint> &constraints,
                        int cellCount) {
  std::vector<int> parent(cellCount);
  std::iota(parent.begin(), parent.end(), 0);
  auto find = [&](int x) {
    while (parent[x] != x)
      x = parent[x] = parent[parent[x]];
    return x;
  };
  // Two cells are connected when they share a constraint.
  for (const Constraint &c : constraints)
    for (std::size_t k = 1; k < c.cells.size(); k++)
      parent[find(c.cells[k])] = find(c.cells[0]);

  std::vector<int> slot(cellCount, -1);
  std::vector<Component> out;
  std::vector<char> constrained(cellCount, 0);
  for (std::size_t ci = 0; ci < constraints.size(); ci++) {
    const int root = find(constraints[ci].cells[0]);
    if (slot[root] < 0) {
      slot[root] = static_cast<int>(out.size());
      out.emplace_back();
    }
    out[slot[root]].constraints.push_back(static_cast<int>(ci));
    for (int cell : constraints[ci].cells)
      constrained[cell] = 1;
  }
  for (int id = 0; id < cellCount; id++)
    if (constrained[id]) // ascending id keeps each component sorted
      out[slot[find(id)]].cells.push_back(id);
  return out;
}

// ---------------------------------------------------------------- Step 3

Solver::ComponentSolutions
Solver::searchComponent(const std::vector<Constraint> &constraints,
                        const Component &comp, long nodeLimit) {
  Searcher se;
  se.n = static_cast<int>(comp.cells.size());
  se.limit = nodeLimit;
  se.cellCons.assign(se.n, {});
  se.value.assign(se.n, 0);
  for (int ci : comp.constraints) {
    Searcher::LC lc;
    lc.need = constraints[ci].mines;
    for (int g : constraints[ci].cells) {
      const int local = static_cast<int>(
          std::lower_bound(comp.cells.begin(), comp.cells.end(), g) -
          comp.cells.begin());
      lc.cells.push_back(local);
      se.cellCons[local].push_back(static_cast<int>(se.lcs.size()));
    }
    lc.open = static_cast<int>(lc.cells.size());
    se.lcs.push_back(std::move(lc));
  }

  // Order cells breadth-first through shared constraints so that cells of the
  // same constraint sit next to each other in the search order and violations
  // surface early.
  std::vector<char> seen(se.n, 0);
  for (int start = 0; start < se.n; start++) {
    if (seen[start])
      continue;
    seen[start] = 1;
    se.order.push_back(start);
    for (std::size_t head = se.order.size() - 1; head < se.order.size();
         head++) {
      for (int ci : se.cellCons[se.order[head]])
        for (int v : se.lcs[ci].cells)
          if (!seen[v]) {
            seen[v] = 1;
            se.order.push_back(v);
          }
    }
  }

  se.out.ways.assign(se.n + 1, 0.0L);
  se.out.mineWays.assign(se.n + 1, {});
  se.dfs(0);
  se.out.exact = !se.aborted;
  se.out.total = std::accumulate(se.out.ways.begin(), se.out.ways.end(), 0.0L);
  return se.out;
}

Solver::Frontier Solver::analyzeFrontier(const Snapshot &s, long nodeLimit) {
  Frontier f;
  f.constraints = buildConstraints(s);
  std::vector<char> isFrontier(s.size(), 0);
  for (const Constraint &c : f.constraints)
    for (int cell : c.cells)
      isFrontier[cell] = 1;
  for (int id = 0; id < s.size(); id++) {
    if (isFrontier[id])
      f.cells.push_back(id);
    else if (s.view[id] == CellView::Unknown)
      f.interior.push_back(id);
  }
  f.components = splitComponents(f.constraints, s.size());
  for (const Component &comp : f.components)
    f.solutions.push_back(searchComponent(f.constraints, comp, nodeLimit));
  return f;
}

Solver::Deductions Solver::stepExhaustiveSearch(const Frontier &f) {
  Deductions d;
  for (std::size_t k = 0; k < f.components.size(); k++) {
    const ComponentSolutions &sol = f.solutions[k];
    if (!sol.exact || sol.total <= 0)
      continue;
    const Component &comp = f.components[k];
    for (std::size_t j = 0; j < comp.cells.size(); j++) {
      long double asMine = 0;
      for (const auto &row : sol.mineWays)
        if (!row.empty())
          asMine += row[j];
      if (asMine == 0)
        d.safe.push_back(comp.cells[j]);
      else if (asMine == sol.total)
        d.mines.push_back(comp.cells[j]);
    }
  }
  std::sort(d.safe.begin(), d.safe.end());
  std::sort(d.mines.begin(), d.mines.end());
  return d;
}

// ---------------------------------------------------------------- Step 5

Solver::Probabilities Solver::computeProbabilities(const Snapshot &s,
                                                   const Frontier &f) {
  Probabilities pr;
  pr.mineProb.assign(s.size(), -1.0);
  pr.certainSafe.assign(s.size(), 0);
  pr.certainMine.assign(s.size(), 0);

  const int interior = static_cast<int>(f.interior.size());
  pr.interiorCount = interior;
  const int remaining = std::max(0, s.totalMines - s.flagCount());

  std::vector<std::size_t> exact;
  for (std::size_t k = 0; k < f.components.size(); k++) {
    if (f.solutions[k].exact && f.solutions[k].total > 0) {
      exact.push_back(k);
      continue;
    }
    // Search gave up: fall back to the average local density.
    const Component &comp = f.components[k];
    for (int cell : comp.cells) {
      double sum = 0;
      int cnt = 0;
      for (int ci : comp.constraints) {
        const Constraint &c = f.constraints[ci];
        if (std::binary_search(c.cells.begin(), c.cells.end(), cell)) {
          sum += static_cast<double>(c.mines) / c.cells.size();
          cnt++;
        }
      }
      pr.mineProb[cell] = cnt ? sum / cnt : 0.5;
    }
  }

  // C(interior, k) for all k.
  Poly binom(interior + 1, 1.0L);
  for (int k = 0; k < interior; k++)
    binom[k + 1] = binom[k] * (interior - k) / (k + 1);

  std::vector<Poly> polys;
  for (std::size_t k : exact)
    polys.push_back(f.solutions[k].ways);
  Poly total{1.0L};
  for (const Poly &p : polys)
    total = convolve(total, p);

  // Weight of a global frontier-mine count m: the number of ways to place the
  // remaining mines in the interior.
  bool useGlobal = true;
  auto weight = [&](int m) -> long double {
    if (!useGlobal)
      return 1.0L;
    const int k = remaining - m;
    return (k < 0 || k > interior) ? 0.0L : binom[k];
  };
  long double z = 0;
  for (std::size_t m = 0; m < total.size(); m++)
    z += total[m] * weight(static_cast<int>(m));
  if (z <= 0)
    useGlobal = false; // inconsistent total (e.g. wrong user flags): ignore it
  pr.usedGlobalCount = useGlobal;

  for (std::size_t e = 0; e < exact.size(); e++) {
    Poly others{1.0L};
    for (std::size_t o = 0; o < exact.size(); o++)
      if (o != e)
        others = convolve(others, polys[o]);
    const ComponentSolutions &sol = f.solutions[exact[e]];
    // inner[m]: weighted count of the rest of the board given m mines here
    Poly inner(sol.ways.size(), 0.0L);
    for (std::size_t m = 0; m < sol.ways.size(); m++) {
      if (sol.ways[m] == 0)
        continue;
      for (std::size_t t = 0; t < others.size(); t++)
        inner[m] += others[t] * weight(static_cast<int>(m + t));
    }
    const Component &comp = f.components[exact[e]];
    for (std::size_t j = 0; j < comp.cells.size(); j++) {
      long double asMine = 0, asSafe = 0;
      for (std::size_t m = 0; m < sol.ways.size(); m++) {
        if (sol.ways[m] == 0)
          continue;
        const long double mineHere = sol.mineWays[m][j];
        asMine += mineHere * inner[m];
        asSafe += (sol.ways[m] - mineHere) * inner[m];
      }
      const int cell = comp.cells[j];
      const long double both = asMine + asSafe;
      pr.mineProb[cell] = both > 0 ? static_cast<double>(asMine / both) : 0.5;
      pr.certainSafe[cell] = useGlobal && asMine == 0 && asSafe > 0;
      pr.certainMine[cell] = useGlobal && asSafe == 0 && asMine > 0;
    }
  }

  if (interior > 0) {
    long double zi = 0, expected = 0;
    bool anyMine = false, anySafe = false;
    for (std::size_t m = 0; m < total.size(); m++) {
      const long double w = total[m] * weight(static_cast<int>(m));
      if (w <= 0)
        continue;
      const int inInterior = remaining - static_cast<int>(m);
      zi += w;
      expected += w * inInterior;
      anyMine |= inInterior > 0;
      anySafe |= inInterior < interior;
    }
    if (useGlobal && zi > 0) {
      pr.interiorProb = static_cast<double>(expected / zi / interior);
      for (int cell : f.interior) {
        pr.mineProb[cell] = pr.interiorProb;
        pr.certainSafe[cell] = !anyMine;
        pr.certainMine[cell] = !anySafe;
      }
    } else { // no usable total: assume the board-wide density
      const int unknown = interior + static_cast<int>(f.cells.size());
      pr.interiorProb =
          unknown ? std::min(1.0, double(remaining) / unknown) : 0;
      for (int cell : f.interior)
        pr.mineProb[cell] = pr.interiorProb;
    }
  }
  return pr;
}

Solver::GuessResult Solver::stepGuess(const Snapshot &s, const Frontier &f) {
  GuessResult g;
  const Probabilities pr = computeProbabilities(s, f);
  for (int id = 0; id < s.size(); id++) {
    if (pr.certainSafe[id])
      g.certain.safe.push_back(id);
    else if (pr.certainMine[id])
      g.certain.mines.push_back(id);
  }
  if (!g.certain.empty())
    return g;

  int bestInfo = -1;
  int nb[8];
  for (int id = 0; id < s.size(); id++) {
    if (s.view[id] != CellView::Unknown || pr.mineProb[id] < 0)
      continue;
    const int count = s.neighbours(id, nb);
    int info = 0; // unknown neighbours: more of them means more to learn
    for (int k = 0; k < count; k++)
      info += s.view[nb[k]] == CellView::Unknown;
    const double p = pr.mineProb[id];
    if (g.cell < 0 || p < g.probability - TIE_EPS ||
        (p <= g.probability + TIE_EPS && info > bestInfo)) {
      g.cell = id;
      g.probability = p;
      bestInfo = info;
    }
  }
  return g;
}

// ---------------------------------------------------------------- Driver

void Solver::reveal(int column, int row, Step step, bool guess) {
  history.push_back({column, row, false, step, guess});
  game.openCells(column, row);
  game.checkAndHandleWin();
}

int Solver::apply(const Deductions &d, Step step) {
  int changes = 0;
  const Snapshot s = snapshot();
  for (int id : d.mines) {
    const int c = s.column(id), r = s.row(id);
    if (game.grid[c][r].isFlag || game.grid[c][r].isRevealed)
      continue;
    game.toggleFlag(c, r);
    history.push_back({c, r, true, step, false});
    changes++;
  }
  for (int id : d.safe) {
    if (game.state != PLAYING)
      break;
    const int c = s.column(id), r = s.row(id);
    const Cell &cell = game.grid[c][r];
    if (cell.isFlag || cell.isRevealed) // an earlier flood fill got it
      continue;
    reveal(c, r, step, false);
    changes++;
  }
  return changes;
}

bool Solver::playTurn() {
  if (game.state != PLAYING)
    return false;
  const Snapshot s = snapshot();
  if (stepFirstClick(s))
    return true;
  if (apply(stepBasicRules(s), Step::Basic) > 0)
    return true;
  if (apply(stepSubsetRule(s), Step::Subset) > 0)
    return true;
  const Frontier f = analyzeFrontier(s);
  if (apply(stepExhaustiveSearch(f), Step::Search) > 0)
    return true;
  const GuessResult g = stepGuess(s, f);
  if (apply(g.certain, Step::Guess) > 0) // proven by the mine total: no guess
    return true;
  if (g.cell >= 0) {
    guesses++;
    reveal(s.column(g.cell), s.row(g.cell), Step::Guess, true);
    return true;
  }
  return false;
}

namespace {
// First cell in `ids` that is still unrevealed and unflagged, or -1.
int firstUnknown(const std::vector<int> &ids, const Solver::Snapshot &s) {
  for (int id : ids)
    if (s.view[id] == Solver::CellView::Unknown)
      return id;
  return -1;
}
} // namespace

int Solver::stepHint(const Snapshot &s) {
  if (int id = firstUnknown(stepBasicRules(s).safe, s); id >= 0)
    return id;
  if (int id = firstUnknown(stepSubsetRule(s).safe, s); id >= 0)
    return id;
  const Frontier f = analyzeFrontier(s);
  if (int id = firstUnknown(stepExhaustiveSearch(f).safe, s); id >= 0)
    return id;
  return firstUnknown(stepGuess(s, f).certain.safe, s);
}

std::optional<std::pair<int, int>> Solver::findHint() const {
  if (game.state != PLAYING)
    return std::nullopt;
  if (game.isFirstClick)
    return std::make_pair(0, 0);
  const Snapshot s = snapshot();
  const int id = stepHint(s);
  if (id < 0)
    return std::nullopt;
  return std::make_pair(s.column(id), s.row(id));
}

Solver::SolveResult Solver::solve() {
  while (playTurn()) {
  }
  return {game.state == GAME_WON, guesses, static_cast<int>(history.size())};
}
