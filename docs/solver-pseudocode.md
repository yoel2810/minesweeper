# Minesweeper Solver: Pseudo-code

Implemented in `src/Solver.hpp` / `src/Solver.cpp` (class `Solver`).
Each step is one function; the name in brackets is the C++ method.

## Definitions

```
flagged cell     : flagged by the solver itself (a proven mine)
unknown cell     : not revealed and not flagged -- includes cells carrying a
                   player's flag, since a player's flag may be wrong
U(X)             : unknown cells among the 8 neighbours of revealed number X
R(X)             : number(X) - (flagged neighbours of X)       -- mines still hidden
constraint       : (cells, mines)  "exactly `mines` mines in `cells`"
frontier cell    : unknown cell adjacent to >= 1 revealed number
interior cell    : unknown cell adjacent to no revealed number
```

U and R are recomputed from a fresh `Snapshot` of the game before every
decision, so they are always current after any reveal or flag.

## Main loop  [solve / playTurn = applyPlan(plan(snapshot()))]

A turn is split in two so the GUI can think on a worker thread: `plan` is a
pure function of the snapshot, `applyPlan` changes the game.

```
solve():
    while playTurn(): pass
    return (won?, guesses, moves)

plan(S):                                      -- one decision; earliest step wins
    if no cell of S is revealed:                  return FirstClick -- Step 0
    if stepBasicRules(S)        found any:        return it         -- Step 1
    if stepSubsetRule(S)        found any:        return it         -- Step 2
    F = analyzeFrontier(S)                                          -- Steps 3+4
    if stepExhaustiveSearch(F)  found any:        return it
    G = stepGuess(S, F)                                             -- Step 5
    if G.certain                found any:        return it         -- proven by mine total
    return guess G.cell (if any)

applyPlan(P):                   -- P.deductions = (safe cells, mine cells), all proven
    if new game started: forget history, guesses and proven flags
    FirstClick: reveal corner (0,0)
    for c in mines: flag c (or adopt the player's flag already on it) unless revealed
    for c in safe : reveal c (lifting a wrong player flag) unless already revealed
    guess     : reveal P.guess; guesses += 1
```

After `applyPlan` the loop restarts at Step 0/1, so every new reveal or flag
is fed back through the cheapest rules first.

## Step 0: first click  [plan, Step::FirstClick]

```
if no cell is revealed:
    reveal corner (0,0)        -- Game places mines lazily, keeping the corner
                               -- and its neighbours mine-free
```

## Step 1: basic rules  [stepBasicRules]

```
for every revealed number X with U(X) not empty:
    if R(X) == 0:          every cell in U(X) is SAFE
    if R(X) == |U(X)|:     every cell in U(X) is a MINE
```

Because `apply` followed by a fresh Step 1 pass repeats this until nothing new
is found, this is the "repeat until no new cells" loop.

## Step 2: subset rule  [stepSubsetRule]

```
C = { (U(X), R(X)) for every revealed number X with U(X) not empty }
known = C
repeat until nothing changes:
    for every pair (A, B) in C with cells(A) strictly inside cells(B):
        D = cells(B) - cells(A)
        k = R(B) - R(A)
        if k < 0 or k > |D|:   skip                 -- cannot happen on a valid board
        if k == 0:             D is SAFE
        elif k == |D|:         D is MINES
        elif (D, k) not in known:
            add (D, k) to C and to known            -- derived constraint, deduplicated
                                                    -- (also capped at 4000 constraints)
return all SAFE / MINES found
```

Derived constraints are differences of true constraints, so they never
contradict the numbers on the board.

## Step 4: independent components  [splitComponents]

```
union-find over frontier cells:
    for each constraint: union all its cells
group frontier cells by root -> components
each component keeps the constraints that touch it
```

Two frontier cells are connected when they appear in a common constraint.
Components cannot influence one another (except through the global mine count,
handled in Step 5), so they are searched separately. This avoids exponential
blow-up: cost is the sum of the component costs, not the product.

## Step 3: exhaustive search  [searchComponent, stepExhaustiveSearch]

```
searchComponent(component):
    order = cells in breadth-first order through shared constraints
            -- cells of one constraint end up next to each other,
            -- so a violated constraint fails early
    for each constraint c keep   c.mines (assigned so far)   and   c.open (unassigned cells)
    assignment = one mutable array;  minesNow = 0

    dfs(depth):                                   -- depth-first, no copies
        if node budget exhausted: abort (component marked inexact)
        if depth == |order|:
            ways[minesNow] += 1
            for each cell that is a mine: mineWays[minesNow][cell] += 1
            return
        cell = order[depth]
        for value in (safe, mine):
            assign(cell, value)                   -- update c.mines / c.open of its constraints
            ok = for all touched c:  c.mines <= c.need   and   c.mines + c.open >= c.need
            if ok: dfs(depth + 1)                 -- prune immediately on violation
            undo(cell, value)

stepExhaustiveSearch(frontier):
    for each exact component, for each cell:
        mineSolutions = sum over m of mineWays[m][cell]
        if mineSolutions == 0:             cell is SAFE   (mine in 0 solutions)
        if mineSolutions == total:         cell is a MINE (mine in all solutions)
```

`ways[m]` and `mineWays[m][cell]` are kept per mine count `m` so Step 5 can
weight each combination by how the remaining mines fit in the interior.

## Step 5: probabilities and the guess  [computeProbabilities, stepGuess]

```
T = total_mines - flagged        -- every flag is proven, so T is exact
I = |interior|
weight(m) = C(I, T - m)          -- ways to put the other mines in the interior
                                 -- (0 when T - m < 0 or T - m > I)

total = convolution of ways[] of all exact components       -- ways to use m frontier mines
Z     = sum over m of total[m] * weight(m)

for each frontier cell c in component k:
    others   = convolution of ways[] of all components except k
    inner[m] = sum over t of others[t] * weight(m + t)
    asMine   = sum over m of mineWays[m][c]          * inner[m]
    asSafe   = sum over m of (ways[m] - mineWays[m][c]) * inner[m]
    P(c)     = asMine / (asMine + asSafe)
    asMine == 0 -> certainly safe;   asSafe == 0 -> certainly a mine   (via the mine total)

interior:
    expected = sum over m of total[m] * weight(m) * (T - m) / I
    P(interior cell) = expected / Z / I

stepGuess:
    if any cell is certain (probability 0 or 1): return those, no guess is made
    else choose the unknown cell (frontier OR interior) with the lowest P
         ties (within 1e-9): the cell with the most unknown neighbours
    reveal it and count one guess
```

Numerics: raw counts overflow a `double` (MSVC's `long double`) on big
boards, so each component's `ways`/`mineWays` are divided by its own total and
`weight(m)` is computed as `exp(log C(I, T - m) - max)` over the reachable `m`.
Only ratios enter P(c), so the scaling cancels. "Certainly safe / mine" is
decided from a separate boolean support (which mine counts are possible at
all), never from a floating-point zero, so underflow cannot fake a proof.

If the search ran out of budget for a component, its cells fall back to the
average `R/|U|` of their constraints, and they are never used for proofs.
If the mine total is inconsistent with the board, the global weighting is
switched off and uniform weights are used.

## Correctness rules

* Flags are placed **only** on cells proven to be mines; a guess never flags.
* Player flags are never trusted: they count as unknown cells until the
  solver proves them (then adopted) or proves them wrong (then lifted and the
  cell revealed). The hint follows the same rule, so a wrong flag cannot make
  it point at a mine.
* U contains only unknown cells (revealed and flagged cells are excluded).
* U and R are rebuilt from a new snapshot after every action.
* Derived constraints are differences of true constraints, deduplicated, so
  they cannot contradict the board.
* A game can only be lost on a Step 5 guess (verified by the tests).
