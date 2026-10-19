# Project guide: what was built and where everything lives

This is a practical walkthrough of the completed project. It explains the implementation, each maintained source file, the experiment inputs, and the saved outputs. Read [the README](../README.md) for installation and copyable build commands.

## 1. What the project actually does

The project is a C++17 research emulator for two questions:

1. What happens when periodic tasks share a processor and each task must finish its current job before another job can run?
2. What happens when memory is reserved permanently for each task, compared with sharing a fixed memory arena at runtime?

It uses a simulated clock measured in integer ticks. It does not wait for real seconds or run the modelled tasks in operating-system threads. A task's execution cost is a number supplied in its input file. For example, cost 7 means that a job needs seven simulated execution ticks.

The word “OS-free” describes the target model. The emulator itself runs as a normal program on the host computer. This is not a Voyager instruction-set emulator, a kernel, an RTOS, or a physical spacecraft simulation.

The main implemented features are:

- Periodic task releases, separate job records, and deterministic cooperative scheduling.
- Fixed task/job storage, overflow checks, deadlines, backlog measurement and completion of admitted jobs after arrivals stop.
- Aligned static task memory regions with admission and owner-boundary checks.
- A bounded first-fit allocator with splitting, freeing, coalescing and explicit failure reasons.
- Shared memory-demand traces replayed under both memory policies.
- A command-line program that exports job, task, run and memory-event CSVs.
- Deterministic workload generation, automated experiment batches, independent accounting checks, summaries and plots.
- Tests, allocation checks, sanitizer checks, reproduction records and an evidence-package builder.

## 2. Folder structure

The maintained project currently looks like this. Repeated generated inputs and result directories are shown using patterns rather than listing thousands of files.

```text
Emulator/
├── .git/                              Git history and remote configuration
├── .gitignore                         Generated/local files Git should ignore
├── CMakeLists.txt                     Build targets, compiler options and tests
├── IRP_Emulator_Codex_Build_Plan.md    Original supplied build plan, unchanged
├── README.md                          Setup, example commands and main links
│
├── include/emulator/                  C++ core declarations and data structures
│   ├── model.hpp
│   ├── scheduler.hpp
│   ├── memory.hpp
│   └── replay.hpp
│
├── src/
│   ├── core/                          Simulation and memory-policy implementation
│   │   ├── model.cpp
│   │   ├── scheduler.cpp
│   │   ├── memory.cpp
│   │   └── replay.cpp
│   └── host/                          Command-line input and file output
│       ├── main.cpp
│       ├── io.hpp
│       ├── io.cpp
│       ├── export.hpp
│       ├── export.cpp
│       ├── memory_cli.hpp
│       ├── memory_cli.cpp
│       ├── info.hpp
│       ├── info.cpp
│       └── build_info.hpp.in
│
├── configs/                           Saved experiment inputs and plans
│   ├── experiments.json
│   ├── frozen.json
│   ├── tasksets.csv
│   ├── fixtures/                      3 small scheduling examples
│   │   ├── single.csv
│   │   ├── blocking.csv
│   │   └── ties.csv
│   ├── generated/                     258 generated scheduling input CSVs
│   │   ├── e1_n*_s*_u*.csv
│   │   ├── e2_c*_o*.csv
│   │   ├── sensitivity_u*_zero.csv
│   │   ├── sensitivity_u*_staggered.csv
│   │   └── e4_u*_b*.csv
│   └── traces/                        22 memory-demand input CSVs
│       ├── constant.csv
│       ├── staggered_s*.csv
│       ├── fragmentation_s*.csv
│       └── fragmentation_fixture.csv
│
├── tests/                             Correctness and integration checks
│   ├── core_tests.cpp
│   ├── allocation_guard.hpp
│   ├── allocation_guard.cpp
│   ├── cli_tests.py
│   ├── repro_tests.py
│   └── analysis_tests.py
│
├── scripts/                           Input preparation and research tools
│   ├── workloads.py
│   ├── memory_traces.py
│   ├── prepare.py
│   ├── run_batch.py
│   ├── check_capacity.py
│   ├── check_csv.py
│   ├── analyze.py
│   ├── plot_results.py
│   ├── audit_core.py
│   ├── stack_usage.py
│   ├── reproduce_smoke.py
│   └── package.py
│
├── docs/                              Explanations and recorded verification
│   ├── project-guide.md               This walkthrough
│   ├── results-explained.md            Plain-language interpretation of results
│   ├── design.md
│   ├── methodology.md
│   ├── data-dictionary.md
│   ├── decisions.md
│   ├── timelines.md
│   ├── findings.md
│   ├── limitations.md
│   ├── inventory.md
│   ├── python-requirements.txt
│   └── verification/
│       ├── build.json
│       ├── normal-tests.txt
│       ├── sanitizer-tests.txt
│       ├── core-audit.txt
│       ├── stack-usage.txt
│       ├── capacity.csv
│       └── fresh-smoke.json
│
├── results/full/                      The retained complete evidence batch
│   ├── manifest.json
│   ├── tasksets.csv
│   ├── e0/<run_id>/
│   ├── e1/<run_id>/
│   ├── e2/<run_id>/
│   ├── e3/<run_id>/
│   ├── e4/<run_id>/
│   ├── sensitivity/<run_id>/
│   ├── analysis/
│   └── plots/
│
└── artifacts/                         Local deliverables; ignored by Git
    └── voyager-emulator-evidence.zip
```

The `.venv/`, `build/` and `build-sanitize/` directories were removed during cleanup. They are recreated by the README commands. CMake also creates `build/generated/build_info.hpp`, `build/emulator`, `build/core_tests` and its own build-system files. These generated files are not maintained source.

## 3. The three parts of the implementation

| Part | Location | Responsibility |
| --- | --- | --- |
| Target core | `include/emulator/`, `src/core/` | Model ticks, tasks, jobs, scheduling and bounded memory policies. No file output, threads, sleep or host-heap fallback. |
| Host program | `src/host/` | Read arguments and CSVs, validate host input, call the core, and write output files between core calls. Host memory allocation is allowed here. |
| Research tools | `scripts/` | Prepare inputs, run the executable repeatedly, check results, summarise data and create figures/packages. Python does not implement the scheduler. |

A `.hpp` file declares the types and methods another C++ file can use. Its `.cpp` file contains their implementation. The headers and implementations are separated for ordinary C++ organisation; they are not separate programs.

## 4. Core C++ files

| File | What it does |
| --- | --- |
| `include/emulator/model.hpp` | Defines `Tick`, task/configuration records, run statuses and fixed capacities. Declares input validation, checked addition and priority assignment. |
| `src/core/model.cpp` | Implements those helpers. Rejects invalid timing, duplicate task IDs and unsafe arithmetic. Calculates conservative admitted-work/count bounds. Assigns shorter-period tasks higher priority, breaking equal-period ties by task ID. |
| `include/emulator/scheduler.hpp` | Defines a released `Job`, the one-completion result returned by a step, per-task/run metrics and the `Scheduler` object. Holds the bounded active-job array and release state. |
| `src/core/scheduler.cpp` | Implements scheduler initialisation and the tick loop: complete work, release jobs, count deadline misses, dispatch if idle, and execute one interval. Tracks busy/idle time, backlog, responses and drain work. Performs scripted task-region reads/writes when dispatching a job with reserved memory. |
| `include/emulator/memory.hpp` | Declares eight-byte alignment, fixed owner regions, `StaticArena`, allocation outcomes, fixed block descriptors and `DynamicArena`. |
| `src/core/memory.cpp` | Reserves static regions and enforces owner boundaries. Implements first-fit allocation, aligned splitting, exact-offset free validation, adjacent-hole coalescing, free-space measurements and operation counters. |
| `include/emulator/replay.hpp` | Defines memory policies, input events, post-event snapshots, replay metrics and the `Replay` object. Keeps bounded per-owner pending-request state. |
| `src/core/replay.cpp` | Applies the same sequence of allocations/frees to the chosen policy. Tracks logical demand separately from successful live allocations. Records allocation failures and skips the later free of a failed request. Checks that replay ends with no pending lifetimes. |

The declared capacities are 16 tasks, 4,096 simultaneously active jobs, a 65,536-byte arena and 32 dynamic block descriptors. Selecting a smaller arena budget limits its usable prefix; it does not make the compiled backing array smaller. `Replay` contains two backing arenas, while `Scheduler` contains one.

The original 1,024-job limit was increased after an overloaded workload exhausted it. The final preflight observed at most 1,038 active jobs. That decision is recorded in [decisions.md](decisions.md).

## 5. Host C++ files

| File | What it does |
| --- | --- |
| `src/host/main.cpp` | Program entry point. Handles `schedule`, `memory`, `info` and `--help`. For scheduling, loads configuration, calls the core repeatedly and writes completed job rows. Selects exit codes and reports errors. |
| `src/host/io.hpp` | Declares numeric CSV reading, checked integer parsing, task loading, output stream creation and run-ID validation. |
| `src/host/io.cpp` | Implements those helpers. Checks exact input headers/row widths, invalid or oversized integers and supported task counts. Input fields are integers or fixed operation names; a general quoted-CSV parser is unnecessary for these schemas. |
| `src/host/export.hpp` | Declares scheduling summary export. |
| `src/host/export.cpp` | Writes `runs.csv`, `task_metrics.csv` and exported `tasks.csv`. Calculates ratios and means outside the core. Uses `NA` where a statistic is undefined. |
| `src/host/memory_cli.hpp` | Declares the memory-replay command handler. |
| `src/host/memory_cli.cpp` | Validates a complete trace, including event order, unique request IDs, owner maxima and matching frees. Calls `Replay`, copies the exact input trace and writes memory event/run CSVs. |
| `src/host/info.hpp` | Declares build/storage information output. |
| `src/host/info.cpp` | Prints JSON describing compiler/build settings, capacities, alignment and `sizeof` storage figures. The batch runner records this output in its manifest. |
| `src/host/build_info.hpp.in` | A CMake input template. CMake fills in compiler and configuration values to create the generated build-information header. It is not compiled directly. |

## 6. How one scheduling run works

```text
Task CSV + command-line settings
    -> main.cpp and io.cpp
    -> Scheduler::init()
       -> validate timing/arithmetic
       -> assign priorities
       -> admit static task memory
    -> repeated Scheduler::step() calls
       -> return at most one completed job per call
    -> host writes jobs.csv between calls
    -> export.cpp writes task and run summaries
```

A task is a recurring specification; a job is one release of that task. For example, period 5 means a new job arrives every five ticks, even if an older job is unfinished. Older jobs are not overwritten.

The policy is cooperative with completion as the yield point. A newly released high-priority job waits while another job is running. At each tick boundary the scheduler:

1. Finalises the preceding interval and removes a job if it completed.
2. Releases jobs scheduled for that boundary, provided the release is before the horizon `H`.
3. Counts still-unfinished jobs whose deadlines have arrived, once each.
4. Chooses the highest-priority ready job if no job is running.
5. Executes one tick, or accounts for an idle tick, and advances the clock.

Completion exactly at a deadline is on time because completion is processed first. At `H`, the scheduler stops admitting arrivals and records the backlog. It then drains every admitted job, allowing response times and deadline outcomes to be known. Drain execution is kept separate from CPU occupancy in `[0,H)`.

For the small blocking fixture, the low-priority job runs from tick 0 to 7. The high-priority job released at tick 1 must wait until tick 7, then completes at tick 8. Its deadline was tick 6, so it misses. This timeline is an explicit correctness example, not a value hard-coded into the metrics.

## 7. How one memory run works

```text
Memory trace + policy + arena budget
    -> memory_cli.cpp validates input lifetimes
    -> Replay::init()
       -> StaticArena reserves owner maxima, or
       -> DynamicArena starts with free bounded space
    -> Replay::apply() for each ordered event
       -> allocation/free outcome and memory snapshot
    -> host writes memory_events.csv
    -> Replay::finish() and memory_runs.csv
```

`static_reservation` reserves an aligned region for each owner's declared maximum before replay begins. If all reservations do not fit, admission is rejected and there are no runtime request results. When admitted, the regions stay reserved even when owners are not using them.

`bounded_first_fit` searches its fixed descriptor array for the first sufficiently large free block. It splits a block when needed and merges adjacent holes after freeing. This is runtime allocation inside a bounded arena; it is not host `malloc`.

Failures distinguish insufficient aggregate bytes, fragmented free space and exhausted block descriptors. A failed allocation still has a logical request lifetime in the trace; its later free is recorded as `skipped_free`.

In the manual fragmentation fixture, four 128-byte allocations fill a 512-byte arena. Freeing alternating blocks leaves 256 bytes free, but only in two separate 128-byte holes. A 192-byte request therefore fails through fragmentation.

The model charges zero execution ticks for scheduling, allocation and memory accesses. Operation counters describe algorithmic work, not measured hardware latency.

## 8. Configuration and input files

| Location | What it contains |
| --- | --- |
| `configs/fixtures/single.csv` | A periodic task with predictable execution and idle intervals. |
| `configs/fixtures/blocking.csv` | The two-task example where a long running job delays a higher-priority arrival. |
| `configs/fixtures/ties.csv` | Equal-period tasks used to check the task-ID tie rule. |
| `configs/generated/e1_*.csv` | 240 task sets: 3/5/10 tasks, eight target loads and ten seeds. |
| `configs/generated/e2_*.csv` | Eight blocking conditions: four long-job costs and two release phases. |
| `configs/generated/sensitivity_*.csv` | Six doubled-horizon conditions using zero or staggered offsets. |
| `configs/generated/e4_*.csv` | Four integrated conditions: two loads and an admitted/rejected memory budget for each. |
| `configs/traces/constant.csv` | One canonical trace of five owners making simultaneous fixed-size requests. |
| `configs/traces/staggered_s*.csv` | Ten seeded variable-demand traces whose request lifetimes are staggered. |
| `configs/traces/fragmentation_s*.csv` | Ten seeded traces designed to create holes in the dynamic arena. |
| `configs/traces/fragmentation_fixture.csv` | The small manually checked fragmentation example. |
| `configs/tasksets.csv` | A combined catalogue of E1 task parameters, priorities, seeds and nominal/achieved utilisation. Other experiments' inputs are identified separately in the plan. |
| `configs/experiments.json` | The complete 525-cell plan: input paths, parameters, IDs, groups and which runs belong to smoke. |
| `configs/frozen.json` | Hashes of the approved plan/catalogue/input files and the build capacities at freeze. The runner checks these before executing. It is not a compiler binary. |

Task CSV columns are `task_id,period,deadline,cost,offset,reserved_bytes`. Memory trace columns are `event_index,time,owner,request_id,operation,bytes,owner_maximum`. The full field definitions are in [data-dictionary.md](data-dictionary.md).

## 9. What each Python script does

| Script | Purpose | Main output or effect |
| --- | --- | --- |
| `workloads.py` | Implements the deterministic xorshift32 generator, paired scheduling families, bounded rounding retries and CSV-writing helpers. | Used by preparation and tests; not a scheduler. |
| `memory_traces.py` | Constructs constant, staggered and fragmentation demand traces, with fixed ordering and declared owner maxima. | Memory input files under `configs/traces/`. |
| `prepare.py` | Assembles E0–E4 and sensitivity definitions and writes all generated inputs. | `configs/generated/`, `configs/traces/`, `tasksets.csv`, `experiments.json`. |
| `run_batch.py` | Selects smoke/full cells, checks frozen inputs, invokes the C++ executable and validates each outcome. Saves pending/completed/rejected/error statuses, commands and hashes. | A new result batch with per-run folders and `manifest.json`. |
| `check_capacity.py` | Runs the 30 overloaded E1 inputs plus six sensitivity cases to check active-job capacity, then freezes validated inputs. | `docs/verification/capacity.csv` and `configs/frozen.json`. This is deliberate revalidation, not a routine viewing command. |
| `check_csv.py` | Independently reconciles jobs, tasks, CPU ticks, memory events and run totals. Separates raw run files from derived analysis exports. | Validation output; no changes to raw data. |
| `analyze.py` | Verifies raw hashes/accounting and produces descriptive summaries. Keeps rejections/incomplete statuses visible and excludes invalid outcomes from success statistics. | A new analysis directory containing CSV summaries and `analysis.json`. |
| `plot_results.py` | Uses Matplotlib to graph achieved utilisation, blocking and memory outcomes. Labels sample counts and the theoretical reference. | Four PNGs and `plots.json` in a new directory. |
| `audit_core.py` | Checks core source and undefined library symbols for prohibited heap/host dependencies. | An audit report; complements the test allocation guard. |
| `stack_usage.py` | Compiles core files in a temporary directory using `-fstack-usage`. | Per-function compiler stack estimates; not measured total runtime stack consumption. |
| `reproduce_smoke.py` | Makes a fresh build in a path containing spaces, runs CTest, executes smoke, checks CSVs, analyses them and compares every raw smoke file with the saved full batch. | A new local reproduction folder, command log and `verification.json`. |
| `package.py` | Collects source, configurations, tests, documentation and the full evidence batch into a ZIP. Adds and verifies a per-file hash inventory. | A new archive, or verification of an existing archive with `--check`. |

Preparation writes inputs; capacity validation can replace the freeze file. New experiment/analysis/package output paths must not already exist. When repeating a run, choose a fresh output name. Existing measured raw data should be preserved.

## 10. Tests and recorded checks

| File | What it checks |
| --- | --- |
| `tests/core_tests.cpp` | Hand-calculated scheduling behaviour, exact/late deadlines, priority rules, blocking, idle time, overlapping releases, drain, capacity failure, static bounds, allocator splitting/reuse/coalescing, fragmentation, descriptor exhaustion and replay lifetimes. |
| `tests/allocation_guard.hpp` | Declares the test allocation-counting switch and counter. |
| `tests/allocation_guard.cpp` | Replaces relevant C++ `new`/`delete` forms in the test executable, including arrays, alignment and nothrow forms, so allocations during covered core tests can be counted. It does not change the normal emulator binary. |
| `tests/cli_tests.py` | Invokes the executable with valid and invalid inputs, output paths containing spaces, memory budgets and malformed lifetimes. Checks that existing outputs are not overwritten. |
| `tests/repro_tests.py` | Checks deterministic input generation, repeated output equality, logging enabled/disabled equivalence, shared trace identity and actual dispatch tie order. |
| `tests/analysis_tests.py` | Checks exclusion/counting of incomplete or pending outcomes and ensures derived memory summaries are not mistaken for raw single-run files. |

CMake exposes four CTest suites: `core`, `cli`, `reproducibility` and `analysis`. Normal and AddressSanitizer/UndefinedBehaviorSanitizer checks passed during implementation. The later cleanup changed the Python validator and ran its analysis regression plus full CSV validation; the C++ model was unchanged.

The allocation guard covers tested C++ allocation paths. The source/symbol audit provides additional evidence. Neither claims that the entire host process never allocates or that every C-library allocation is intercepted.

`docs/verification/` preserves these records:

| File | Meaning |
| --- | --- |
| `build.json` | Recorded compiler/build identity, fixed capacities, alignment and storage sizes. |
| `normal-tests.txt` | Normal-build CTest outcome record. |
| `sanitizer-tests.txt` | Sanitizer-build CTest outcome record. |
| `core-audit.txt` | Source/symbol audit outcome and undefined symbols. |
| `stack-usage.txt` | Compiler estimates for each core function's stack use. |
| `capacity.csv` | Results of the 36-case capacity preflight. |
| `fresh-smoke.json` | Fresh-build commands/settings and the successful 62-run, 216-file reproduction comparison. |

These are records of the checks that were run, not files required to execute the scheduler.

## 11. What is saved under results/full

The retained full batch has 525 planned outcomes:

| Group | Purpose | Runs | Completed | Admission rejected |
| --- | --- | ---: | ---: | ---: |
| E0 | Small saved correctness examples | 3 | 3 | 0 |
| E1 | Utilisation sweep | 240 | 240 | 0 |
| E2 | Long-job blocking and release phase | 8 | 8 | 0 |
| E3 | Memory traces, policies and budgets, including the manual fixture | 264 | 181 | 83 |
| E4 | Scheduling and static memory together | 4 | 2 | 2 |
| Sensitivity | Longer horizon and staggered offsets | 6 | 6 | 0 |
| Total | | 525 | 440 | 85 |

There are no incomplete full-batch simulations. A completed memory replay can still contain allocation failures; “complete” means that replay finished and its outcomes were recorded. A static admission rejection is an expected budget result, not a crash.

### Scheduling run folder

For example, `results/full/e2/e2_c8_o1/` contains:

| File | What it tells you |
| --- | --- |
| `tasks.csv` | The exact task parameters and assigned priorities. |
| `jobs.csv` | Each admitted job's release, deadline, first start, completion, response, wait, tardiness and miss flag. |
| `task_metrics.csv` | Per-task counts, sums, means and observed maxima. |
| `runs.csv` | One run-level row: status, occupancy, releases/completions/misses, backlog, drain, utilisation reference and memory/access counts. |

### Memory run folder

For example, `results/full/e3/e3_fragmentation_fixture_s0_b512_bounded_first_fit/` contains:

| File | What it tells you |
| --- | --- |
| `memory_trace.csv` | An exact copy of the input trace, including requests that failed. |
| `memory_events.csv` | Each event's outcome and post-event live/demand/occupied/free bytes, padding, fragmentation and operation counters. Rejected admission leaves only its header. |
| `memory_runs.csv` | One run-level row: status, successes/failures/skipped frees, failure reasons, peaks and storage accounting. |

There are 1,836 raw per-run CSV files and 1,163,718 admitted/completed scheduling job records in the full batch.

The batch-level `manifest.json` records the source revision/content hash, compiler/build/platform, binary hash, input parameters and hashes, commands, statuses and raw output hashes. It lets a result be traced back to the exact measurement configuration. Later documentation and validation maintenance do not change that recorded measurement revision. The batch-level `tasksets.csv` is the saved E1 input catalogue.

### Derived analysis files

`results/full/analysis/` contains seven files:

| File | Purpose |
| --- | --- |
| `schedule_runs.csv` | Scheduling run summaries combined with experiment/input metadata. |
| `memory_runs.csv` | Memory run summaries combined across conditions. This contains many runs, unlike a raw run's single-row file with the same name. |
| `status_counts.csv` | Counts of every planned status by group/type. |
| `summary.csv` | Scheduling medians and ranges by declared conditions, with input/outcome counts. |
| `memory_summary.csv` | Memory admission, request-failure and occupancy summaries by family, policy and budget. |
| `blocking_summary.csv` | High-priority response, waiting and misses for each E2 condition. |
| `analysis.json` | Input manifest hash, analysis-script hash, measurement revision and outcome totals. |

`results/full/plots/` contains `miss_rate.png`, `max_response.png`, `blocking.png`, `memory.png` and the plotting provenance file `plots.json`.

These summaries and figures are derived from raw data. They are not additional independent experiments. Repeating the same deterministic input is a reproduction check, not another independent sample.

## 12. Documentation map

| File | When to read it |
| --- | --- |
| `README.md` at the project root | You want to install, compile, run a fixture or execute a batch. |
| `docs/project-guide.md` | You want the file/folder walkthrough and the connection between components. |
| `docs/design.md` | You want the implementation/storage boundary, job-array mechanics and allocation evidence. |
| `docs/methodology.md` | You want exact experiment controls, generation rules, metrics and interpretation assumptions. |
| `docs/data-dictionary.md` | You want every raw/derived column, status and accounting convention explained. |
| `docs/decisions.md` | You want the recorded implementation choices, capacity change, cleanup and history-date explanation. |
| `docs/timelines.md` | You want small scheduling examples that can be checked by hand. |
| `docs/results-explained.md` | You want the results, tables and graphs explained step by step in plain language. |
| `docs/findings.md` | You want numerical observations from the completed batch and their interpretation. |
| `docs/limitations.md` | You want what was not modelled, what was not verified and possible future work. |
| `docs/inventory.md` | You want a delivery checklist and the evidence/package contents. |
| `docs/python-requirements.txt` | You want the recorded Python package/version snapshot from the development environment. |

## 13. What was done during the build and cleanup

The implementation started from the supplied plan in an otherwise empty project folder. Git was initialised and connected to the provided GitHub repository. The work added the C++ build, core records and validation, cooperative scheduler, static memory, runtime allocator, host CLI/CSV exports, tests, input generators, experiment batches, independent checks, analysis, plots, documentation and packaging.

The full required matrix and sensitivity cases were executed. The evidence was checked, and a fresh smoke reproduction matched all 216 raw output files for its 62 selected runs against the full batch. The original build milestones and subsequent maintenance changes are preserved in Git. The retrospective milestone-date convention is documented in the decisions note.

Cleanup removed compiler builds, the local Python environment, caches, trial batches, earlier duplicate smoke/example exports and preview packages/plots. Their removal does not remove the saved full evidence, source or tests. The final archive was rebuilt. Duplicate historical tracked outputs remain recoverable from Git history.

A cleanup validation also found and fixed a Python discovery issue: after analysis existed, `check_csv.py` could mistake the combined derived `memory_runs.csv` for a raw single-run file. It now recognises the analysis directory marker, with a regression check. All 1,836 retained raw files matched their original manifest hashes after cleanup.

## 14. What to keep, what can be recreated, and where to start

Keep `include/`, `src/`, `tests/`, `scripts/`, the saved configuration/freeze files, documentation and `results/full/`. These contain the implementation and reproducible research evidence. Avoid hand-editing measured CSV rows or treating changes to frozen inputs as if they belonged to the original batch.

Build folders, `.venv/`, Python caches, new `results/local-*` trial batches and ZIP exports can be recreated. The `artifacts/` directory is ignored by Git; the archive is a local deliverable, while the source and full evidence are tracked in the repository. `package.py` includes this guide automatically because it includes `docs/`.

For understanding the code, a useful reading order is:

1. `include/emulator/model.hpp`: what a task and configuration contain.
2. `include/emulator/scheduler.hpp`, then `src/core/scheduler.cpp`: how releases become jobs and run to completion.
3. `include/emulator/memory.hpp`, then `src/core/memory.cpp`: how fixed regions and first-fit blocks work.
4. `src/core/replay.cpp`: how memory lifetimes are replayed under each policy.
5. `src/host/main.cpp`, then the export/CLI helpers: how the core connects to files.
6. `scripts/run_batch.py`, `check_csv.py`, `analyze.py`: how individual runs become checked evidence.
7. `docs/findings.md` alongside the summary CSVs and figures: what the saved results actually show.

To inspect the retained raw data without rebuilding C++, run from the repository root:

```sh
python3 scripts/check_csv.py results/full
```

To compile or run new experiments, recreate the environment and follow the README. There is no optional preemptive scheduler, application yield segmentation, GUI or hardware port in the current implementation. The Liu–Layland line is a labelled preemptive theoretical reference; it does not certify this cooperative scheduler. All timing conclusions remain conclusions about the declared finite model and inputs.
