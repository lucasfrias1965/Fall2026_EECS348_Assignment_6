#!/usr/bin/env python3
"""
Same methodology as benchmark.py, but measures peak memory (space
complexity) instead of wall-clock time: lucas.cpp (brute force) vs
fox.cpp (bitmask+MRV, double-prints every solution) vs fox_mod.cpp
(bitmask+MRV, single print) as puzzle5.txt gets increasingly sparse.

Peak memory is measured as the child process's own ru_maxrss, read
directly off os.wait4() after fork+exec'ing the solver — this avoids
wrapping in an external `time`/`ps` process (which can misattribute
memory, or leave an orphaned solver running past a timeout since
signals aren't always forwarded through a wrapper).

On macOS, ru_maxrss is already in bytes; on Linux it is in KB, so the
value is normalized to bytes for both.

Usage:
    python3 benchmark_space.py
"""

import math
import os
import platform
import resource
import signal
import sys
import time

from benchmark import (
    BLANK_COUNTS,
    PUZZLE5,
    ROOT,
    SOLVERS,
    TIMEOUT_SECONDS,
    build_blank_order,
    write_puzzle5,
)

RSS_UNIT = 1 if platform.system() == "Darwin" else 1024  # bytes per ru_maxrss unit


def run_once_memory(binary):
    """Fork+exec binary (stdout/stderr to /dev/null, cwd=ROOT), wait for
    it with a timeout, and return its peak RSS in bytes, or None on
    timeout (the child is SIGKILLed and reaped either way)."""
    pid = os.fork()
    if pid == 0:
        devnull = os.open(os.devnull, os.O_WRONLY)
        os.dup2(devnull, 1)
        os.dup2(devnull, 2)
        os.chdir(str(ROOT))
        try:
            os.execv(str(binary), [str(binary)])
        finally:
            os._exit(127)

    start = time.perf_counter()
    while True:
        wpid, status, rusage = os.wait4(pid, os.WNOHANG)
        if wpid == pid:
            return rusage.ru_maxrss * RSS_UNIT
        if time.perf_counter() - start > TIMEOUT_SECONDS:
            os.kill(pid, signal.SIGKILL)
            os.wait4(pid, 0)
            return None
        time.sleep(0.01)


def fmt_bytes(n):
    return f"{n / (1024 * 1024):.2f} MiB"


def fit_linear_slope(blanks, values):
    """Slope of a plain linear fit of value vs blank count (bytes per
    extra blank), to see whether memory grows at all with difficulty."""
    pts = [(b, v) for b, v in zip(blanks, values) if v is not None]
    if len(pts) < 2:
        return None
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    n = len(xs)
    mean_x = sum(xs) / n
    mean_y = sum(ys) / n
    num = sum((x - mean_x) * (y - mean_y) for x, y in zip(xs, ys))
    den = sum((x - mean_x) ** 2 for x in xs)
    if den == 0:
        return None
    return num / den


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
                    print(f"  {name:{name_w}s}: TIMEOUT (> {TIMEOUT_SECONDS:.0f}s)")
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
    print("blank cell -- near zero means memory does NOT scale with search")
    print("difficulty, unlike time):")
    for name, values in results.items():
        slope = fit_linear_slope(BLANK_COUNTS, values)
        if slope is None:
            print(f"  {name:{name_w}s}: not enough non-timeout data points")
        else:
            print(f"  {name:{name_w}s}: {slope:+.1f} bytes/blank")


if __name__ == "__main__":
    main()
