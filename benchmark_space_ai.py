#!/usr/bin/env python3
"""
Same methodology as benchmark_space.py, but compares the three single-file-
output variants from the Assignment 6 prompt instead of the Assignment 5
variants:
  - fox.cpp      original: one solutionN.txt per puzzle (own ofstream member)
  - claude.cpp   Claude Opus 5.5's rewrite: all puzzles share one solutions.txt
                 (ofstream& reference member)
  - copilot.cpp  Microsoft Copilot's rewrite: all puzzles share one solutions.txt
                 (ofstream& reference member)

The point is to check whether going from N owned ofstream buffers to 1
shared-by-reference ofstream changed anything about peak memory use.

Usage:
    python3 benchmark_space_ai.py
"""

import sys

from benchmark import BLANK_COUNTS, PUZZLE5, ROOT, build_blank_order, write_puzzle5
from benchmark_space import fit_linear_slope, fmt_bytes, run_once_memory

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
                peak = run_once_memory(binary)
                results[name].append(peak)
                if peak is None:
                    gave_up.add(name)
                    print(f"  {name:{name_w}s}: TIMEOUT")
                else:
                    print(f"  {name:{name_w}s}: {fmt_bytes(peak)} peak RSS")
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
            v = results[name][i]
            cell = "timeout" if v is None else fmt_bytes(v)
            row.append(f"{cell:>{col_w}s}")
        print(" | ".join(row))

    print("\nLinear fit of peak RSS vs blank count (bytes added per extra")
    print("blank cell):")
    for name, values in results.items():
        slope = fit_linear_slope(BLANK_COUNTS, values)
        if slope is None:
            print(f"  {name:{name_w}s}: not enough non-timeout data points")
        else:
            print(f"  {name:{name_w}s}: {slope:+.1f} bytes/blank")


if __name__ == "__main__":
    main()
