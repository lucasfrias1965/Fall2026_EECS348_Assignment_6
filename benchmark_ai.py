#!/usr/bin/env python3
"""
Same methodology as benchmark.py, but compares the three single-file-output
variants from the Assignment 6 prompt instead of the Assignment 5 variants:
  - fox.cpp      original: one solutionN.txt per puzzle
  - claude.cpp   Claude Opus 5.5's rewrite: all puzzles share one solutions.txt
  - copilot.cpp  Microsoft Copilot's rewrite: all puzzles share one solutions.txt

The point is to check whether switching from N output streams to 1 shared
stream changed anything about how run time scales with search difficulty
(it shouldn't -- the change is only to I/O plumbing, not the search).

Usage:
    python3 benchmark_ai.py
"""

import sys
from pathlib import Path

from benchmark import (
    BLANK_COUNTS,
    PUZZLE5,
    ROOT,
    TIMEOUT_SECONDS,
    build_blank_order,
    fit_exponential_base,
    run_once,
    write_puzzle5,
)

SOLVERS = {
    "fox.cpp (per-puzzle solutionN.txt)": ROOT / "fox_cmp_bin",
    "claude.cpp (shared solutions.txt)": ROOT / "claude_bin",
    "copilot.cpp (shared solutions.txt)": ROOT / "copilot_bin",
}


def main():
    for binary in SOLVERS.values():
        if not binary.exists():
            print(f"error: {binary.name} not found, build it first")
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
