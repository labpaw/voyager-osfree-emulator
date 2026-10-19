# Voyager-inspired OS-free emulator

A small C++17 research project for trying cooperative scheduling and fixed memory budgets. It models one processor with integer ticks, while running on a normal host OS. Tasks run until completion; memory uses fixed task reservations or a bounded first-fit comparison. Execution costs are supplied inputs, not hardware measurements.

The required implementation and full experiment batch are complete. There are **525 recorded outcomes: 440 completed runs and 85 expected static-memory admission rejections**, with no incomplete simulations. The code uses fixed arrays and short linear scans in the target core.

## Build

You need a C++17 compiler and Python 3. `CMakeLists.txt` lists the C++ source files, selects C++17 and registers the four test suites. It is needed by the build commands below. CMake and Matplotlib can live in a local virtual environment; Matplotlib is only needed for figures.

```sh
python3 -m venv .venv
.venv/bin/python -m pip install cmake matplotlib
.venv/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
.venv/bin/cmake --build build -j 2
.venv/bin/ctest --test-dir build --output-on-failure
build/emulator info
```

The recorded toolchain is Apple Clang 21.0.0 on arm64 macOS, Python 3.9.6, CMake 4.4.4 and Matplotlib 3.9.4. GCC was not used. Build and environment information is recorded in the full batch manifest.

Optional sanitizer build:

```sh
.venv/bin/cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DEMULATOR_SANITIZE=ON -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
.venv/bin/cmake --build build-sanitize -j 2
.venv/bin/ctest --test-dir build-sanitize --output-on-failure
```

## Try a run

Run commands from the repository root. Every output directory must be new; change its name to repeat a run. Quote paths containing spaces.

```sh
build/emulator schedule --tasks configs/fixtures/blocking.csv --horizon 10 --budget 16 --out results/local-blocking --run-id blocking
build/emulator memory --trace configs/traces/fragmentation_fixture.csv --policy bounded_first_fit --budget 512 --out results/local-memory --run-id memory
```

The first example has one deadline miss caused by the long job blocking the short task. The second finishes replay but records a fragmentation-related allocation failure; a completed replay does not mean every allocation succeeded. Use `build/emulator --help` for the small CLI. Schedule mode supports `cooperative_rm` and `static_reservation`; memory mode selects either memory policy. `--jobs 0` can disable detailed completion rows without changing logical metrics.

Exit codes: 0 complete, 2 parsing/host validation, 3 expected static-budget rejection, 4 invalid/incomplete core run, 5 output/filesystem error. The batch runner handles expected rejections and stops on unexpected failures.

## Experiments

Inputs and a frozen grid are checked in. Preparation can be repeated with `python3 scripts/prepare.py`; generated task sets and traces are deterministic. `configs/frozen.json` protects the inputs used for the full evidence batch.

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

Smoke is a 62-cell subset. Full includes 240 load-sweep cells, eight blocking cells, 264 memory cells, four integrated cells, six sensitivity checks and three small validation cases. Each manifest records source/build details, exact commands, input hashes, statuses and raw file hashes. Analysis checks accounting and hashes before producing descriptive summaries. Rejected cases stay visible.

To rebuild in a fresh directory whose path contains spaces, run the smoke batch and compare all its raw files against saved full evidence:

```sh
.venv/bin/python scripts/reproduce_smoke.py --cmake .venv/bin/cmake --out results/local-fresh
```

This check passed for 62 runs and 216 byte-identical raw files. Core heap/dependency checks can be repeated with `python3 scripts/audit_core.py`.

## Saved evidence

[results/full](results/full/) contains the complete batch, including all completed job records and copied memory traces. [Analysis](results/full/analysis/) holds the summaries and [plots](results/full/plots/) holds four figures.

At the same 25% total utilisation, increasing contiguous low-priority work increased high-priority response and caused misses. Static reservations were predictable when admitted, while staggered lifetimes let first-fit reuse a much smaller arena. First-fit also showed genuine fragmentation failures. The integrated example worked as specified and still missed deadlines at the selected loads. None of this establishes flight reliability or a universal schedulability limit.

The Liu–Layland value is a labelled theoretical reference for **preemptive** rate-monotonic assumptions, not a cooperative pass/fail threshold. Payload budgets exclude fixed metadata and do not shrink the compiled backing arrays.

The original build history has 45 logical milestones with assigned dates spanning 17 August–19 October 2026. These are retrospective dates requested for the personal project; implementation and measurements were performed on 19 October. Optional preemptive scheduling, segmented yields and hardware validation remain future work.
