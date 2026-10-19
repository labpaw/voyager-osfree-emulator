Here’s the cleaned-up version:

---

# Voyager-inspired OS-free emulator

A small C++17 research project looking at cooperative scheduling and fixed memory budgets. It models a single processor using integer ticks, but still runs on a normal host OS. Tasks run to completion. Memory can use either fixed task reservations or a bounded first-fit allocator for comparison. Execution costs are just input values. Nothing is measured from real hardware.

The required implementation and the full experiment batch are done. There are **525 recorded outcomes**: 440 completed runs and 85 expected static-memory admission rejections. No incomplete simulations. The core uses fixed arrays and short linear scans.

## Build

You need a C++17 compiler and Python 3. `CMakeLists.txt` lists the sources, sets C++17 and registers the four test suites. CMake and Matplotlib can live in a local venv (Matplotlib is only needed for the figures).

```sh
python3 -m venv .venv
.venv/bin/python -m pip install cmake matplotlib
.venv/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
.venv/bin/cmake --build build -j 2
.venv/bin/ctest --test-dir build --output-on-failure
build/emulator info
```

Recorded toolchain was Apple Clang 21.0.0 on arm64 macOS, Python 3.9.6, CMake 4.4.4 and Matplotlib 3.9.4. GCC wasn’t used. Full build and environment details are in the batch manifest.

Optional sanitizer build:

```sh
.venv/bin/cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DEMULATOR_SANITIZE=ON -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
.venv/bin/cmake --build build-sanitize -j 2
.venv/bin/ctest --test-dir build-sanitize --output-on-failure
```

## Trying a run

Run everything from the repo root. Output directories must be new. Just change the name if you want to re-run something. Quote paths that contain spaces.

```sh
build/emulator schedule --tasks configs/fixtures/blocking.csv --horizon 10 --budget 16 --out results/local-blocking --run-id blocking
build/emulator memory --trace configs/traces/fragmentation_fixture.csv --policy bounded_first_fit --budget 512 --out results/local-memory --run-id memory
```

In the first example the long job blocks the short task and causes a deadline miss. The second finishes the replay but records a fragmentation-related allocation failure (a completed replay doesn’t mean every allocation succeeded). Use `build/emulator --help` for the small CLI. Schedule mode supports `cooperative_rm` and `static_reservation`. Memory mode picks the memory policy. `--jobs 0` turns off the detailed completion rows without changing the logical metrics.

Exit codes:  
0 = complete  
2 = parsing/host validation  
3 = expected static-budget rejection  
4 = invalid/incomplete core run  
5 = output/filesystem error  

The batch runner treats expected rejections as normal and stops on unexpected failures.

## Experiments

Inputs and a frozen grid are checked in. You can regenerate the prepared sets with `python3 scripts/prepare.py` (they’re deterministic). `configs/frozen.json` locks the inputs used for the full evidence batch.

```sh
.venv/bin/python scripts/run_batch.py --profile smoke --out results/local-smoke
.venv/bin/python scripts/check_csv.py results/local-smoke
.venv/bin/python scripts/analyze.py results/local-smoke --out results/local-smoke/analysis
.venv/bin/python scripts/plot_results.py results/local-smoke/analysis --out results/local-smoke/plots

.venv/bin/python scripts/run_batch.py --profile full --out results/local-full
.venv/bin/python scripts/check_csv.py results/local-full
.venv/bin/python scripts/analyze.py results/local-full --out results/local-full/analysis
.venv/bin/python scripts/plot_results.py results/local-full/analysis --out results/local-full/plots
```

Smoke is a 62-cell subset. Full has 240 load-sweep cells, eight blocking cells, 264 memory cells, four integrated cells, six sensitivity checks and three small validation cases. Each manifest records source/build details, exact commands, input hashes, statuses and raw file hashes. Analysis checks the accounting and hashes before producing the summaries. Rejected cases are kept visible.

To rebuild in a fresh directory (even one with spaces in the path), run the smoke batch and compare the raw files against the saved full evidence:

```sh
.venv/bin/python scripts/reproduce_smoke.py --cmake .venv/bin/cmake --out results/local-fresh
```

This check passed for 62 runs and 216 byte-identical raw files. Core heap/dependency checks can be re-run with `python3 scripts/audit_core.py`.

## Saved evidence

[results/full](results/full/) contains the complete batch (all completed job records and the copied memory traces). Summaries are in [Analysis](results/full/analysis/) and the four figures are in [plots](results/full/plots/).

At the same 25% total utilisation, increasing contiguous low-priority work increased high-priority response time and caused misses. Static reservations were predictable when admitted. Staggered lifetimes let first-fit reuse a much smaller arena. First-fit also produced genuine fragmentation failures. The integrated example behaved as specified and still missed deadlines at the chosen loads. None of this claims flight reliability or a universal schedulability bound.

The Liu–Layland value is only a labelled theoretical reference for **preemptive** rate-monotonic assumptions. It is not a pass/fail threshold for the cooperative case. Payload budgets exclude fixed metadata and do not shrink the compiled backing arrays.

---