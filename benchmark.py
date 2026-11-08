#!/usr/bin/env python3
"""
Benchmarks three Sudoku solvers against each other:
  - lucas.cpp     original brute-force solver
  - fox.cpp       bitmask + MRV-optimized solver, but recordSolution()
                  formats and writes every solution grid TWICE per
                  solution (once to std::cout, once to the file)
  - fox_mod.cpp   same bitmask + MRV search as fox.cpp, with the
                  duplicate std::cout grid print removed from
                  recordSolution() so each solution is written once

by generating puzzle5.txt variants with an increasing number of
blanked-out cells and timing how long each compiled program takes to
process puzzle1..5.txt.

puzzle1.txt..puzzle4.txt are left untouched (they're small, fixed
work); only puzzle5.txt is regenerated each round, so the change in
run time from round to round is attributable to puzzle5's growing
search space.

Usage:
    python3 benchmark.py
"""

import math
import random
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PUZZLE5 = ROOT / "puzzle5.txt"

# A known valid, fully-solved 9x9 Sudoku grid. Blanking cells out of a
# valid solution guarantees every generated puzzle is itself consistent
# (no clashing givens), so all observed time growth comes from search
# space size, not from early duplicate-detection exits.
SOLVED_GRID = [
    [5, 3, 4, 6, 7, 8, 9, 1, 2],
    [6, 7, 2, 1, 9, 5, 3, 4, 8],
    [1, 9, 8, 3, 4, 2, 5, 6, 7],
    [8, 5, 9, 7, 6, 1, 4, 2, 3],
    [4, 2, 6, 8, 5, 3, 7, 9, 1],
    [7, 1, 3, 9, 2, 4, 8, 5, 6],
    [9, 6, 1, 5, 3, 7, 2, 8, 4],
    [2, 8, 7, 4, 1, 9, 6, 3, 5],
    [3, 4, 5, 2, 8, 6, 1, 7, 9],
]

SOLVERS = {
    "lucas.cpp (brute force)": ROOT / "lucas_bin",
    "fox.cpp (bitmask+MRV, dup print)": ROOT / "fox_bin",
    "fox_mod.cpp (bitmask+MRV, fixed)": ROOT / "fox_mod_bin",
}

# Number of cells (out of 81) to blank out, in increasing order. Below
# ~55 blanks all solvers finish in well under a second; lucas.cpp and
# fox.cpp fall off an exponential cliff right around 60-62 blanks for
# this fixed cell ordering (calibrated by hand beforehand), so it's
# sampled densely there. fox_mod.cpp's duplicate-I/O fix lets it keep
# going well past that, so a few higher counts are included to show
# where it, in turn, hits its own wall.
BLANK_COUNTS = [0, 20, 35, 45, 50, 55, 58, 60, 61, 62, 63, 64, 65, 66, 68, 70]

TIMEOUT_SECONDS = 20.0
RNG_SEED = 42


def build_blank_order():
    """A fixed random permutation of the 81 cell positions, so that the
    set of blanked cells at count n is a subset of the set at count n+1
    (monotonically harder puzzles)."""
    positions = [(r, c) for r in range(9) for c in range(9)]
    random.Random(RNG_SEED).shuffle(positions)
    return positions


def write_puzzle5(blank_order, n_blanks):
    grid = [row[:] for row in SOLVED_GRID]
    for (r, c) in blank_order[:n_blanks]:
        grid[r][c] = 0
    lines = []
    for row in grid:
        lines.append(" ".join("_" if v == 0 else str(v) for v in row) + " ")
    PUZZLE5.write_text("\n".join(lines) + "\n")


def run_once(binary):
    start = time.perf_counter()
    try:
        subprocess.run(
            [str(binary)],
            cwd=ROOT,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        return None
    return time.perf_counter() - start


def fit_exponential_base(blanks, times):
    """Rough fit of time ~ base**blanks via linear regression on
    ln(time) vs blanks. Returns None if fewer than 2 usable points."""
    pts = [(b, t) for b, t in zip(blanks, times) if t is not None and t > 0]
    if len(pts) < 2:
        return None
    xs = [p[0] for p in pts]
    ys = [math.log(p[1]) for p in pts]
    n = len(xs)
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    num = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))
    den = sum((x - mean_x) ** 2 for x in xs)
    if den == 0:
        return None
    slope = num / den
    return math.exp(slope)


def main():
    for binary in SOLVERS.values():
        if not binary.exists():
            print(f"error: {binary.name} not found, build it first "
                  f"(g++ -O2 -std=c++17 -o {binary.name} {binary.stem.replace('_bin', '')}.cpp)")
            sys.exit(1)

    backup = PUZZLE5.read_bytes() if PUZZLE5.exists() else None
    blank_order = build_blank_order()

    results = {name: [] for name in SOLVERS}
    gave_up = set()

    name_w = max(len(n) for n in SOLVERS)

    try:
        for n_blanks in BLANK_COUNTS:
            write_puzzle5(blank_order, n_blanks)
            print(f"\n=== puzzle5.txt with {n_blanks} blank cells "
                  f"({81 - n_blanks} givens) ===")
            for name, binary in SOLVERS.items():
                if name in gave_up:
                    results[name].append(None)
                    print(f"  {name:{name_w}s}: skipped (already timed out "
                          f"at a smaller blank count)")
                    continue
                elapsed = run_once(binary)
                results[name].append(elapsed)
                if elapsed is None:
                    gave_up.add(name)
                    print(f"  {name:{name_w}s}: TIMEOUT (> {TIMEOUT_SECONDS:.0f}s)")
                else:
                    print(f"  {name:{name_w}s}: {elapsed:9.4f} s")
    finally:
        if backup is not None:
            PUZZLE5.write_bytes(backup)
        else:
            PUZZLE5.unlink(missing_ok=True)

    col_w = max(len(n) for n in SOLVERS) + 2
    header = f"{'blanks':>7} | " + " | ".join(f"{n:>{col_w}s}" for n in SOLVERS)
    print("\n" + "=" * len(header))
    print(header)
    print("-" * len(header))
    for i, n_blanks in enumerate(BLANK_COUNTS):
        row = [f"{n_blanks:>7}"]
        for name in SOLVERS:
            t = results[name][i]
            row.append(f"{'timeout':>{col_w}s}" if t is None else f"{t:>{col_w}.4f}")
        print(" | ".join(row))

    print("\nApproximate exponential growth base (time ~ base**blanks),")
    print("fit by linear regression of ln(time) vs blank count:")
    for name, times in results.items():
        base = fit_exponential_base(BLANK_COUNTS, times)
        if base is None:
            print(f"  {name:{name_w}s}: not enough non-timeout data points")
        else:
            print(f"  {name:{name_w}s}: base ~= {base:.3f}")


if __name__ == "__main__":
    main()
