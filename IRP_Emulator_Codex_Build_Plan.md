
# Codex build prompt: C++ research emulator

**Project:** Personal project — Voyager-Inspired Software Principles  
**Title:** Voyager-Inspired Software Principles: Implementing and Evaluating in Resource-Constrained and OS-Free Systems  
**Prepared:** 19 October 2026  
**Purpose:** A self-contained implementation brief. Give this file to Codex in the emulator repository and ask it to implement the phases in order.

## 1. Your task

Build a small, understandable C++ research emulator that evaluates cooperative scheduling and static memory allocation under explicit resource limits. Produce working source, meaningful tests, reproducible synthetic experiments, raw CSV data, analysis scripts, and documentation that can be explained and inspected.

Prioritise a correct, data-producing artefact over a general-purpose framework. Implement and validate one phase at a time, continue through the required phases, and report actual results and limitations. Do not stop after proposing another plan. Optional extensions must not delay the required experiments.

Before changing anything, inspect the repository and its instructions. Reuse sound existing work, including any task model, rather than assuming that earlier conversation code exists locally. Keep uploaded references and everything under `sources/` read-only. Put implementation in a writable project directory outside the synced reference files; use the existing emulator repository if available, otherwise create `emulator/`.

The emulator itself must be C++. Python may be used for experiment orchestration, CSV analysis, and plots. Do not substitute a Python or Java scheduler for the C++ artefact.

## 2. Source grounding and decision boundaries

The primary project source is the uploaded project guide (particularly sections covering requirements, architecture, experiments, and outputs). It is a project guide, not independent verification of every bibliographic entry.

### 2.1 Requirements from the project guide

- Investigate cooperative task scheduling without a pre-emptive OS and static memory allocation within a fixed budget.
- Build a custom C++ emulator of an OS-free target environment, hosted on a normal computer.
- Use entirely synthetic, self-generated data.
- Measure CPU utilisation, worst-case response time, deadline miss rate, and report the Liu–Layland reference bound.
- Define scheduler behaviour, yielding, memory restrictions, task sets, measurement, and raw outputs explicitly.
- Start with the scheduler loop and static memory pool; add the wider instrumentation and experiments incrementally.
- Evaluate findings against literature and acknowledge emulation and researcher-selected assumptions.
- Supply source, raw data, analysis scripts/outputs in the final artefact package.

### 2.2 Proposed implementation choices

The specific priority rule, tick convention, C++ standard, GCC/CMake toolchain, task counts, horizons, budgets, seeds, allocator comparison, file layout, and experiment grids below are practical design defaults. Record them in the methodology and change them only with a documented reason. Earlier discussion supports C++, a logical clock, no target OS threads, fixed storage, task sets of 3/5/10 tasks, blocking experiments, and a later dynamic-memory comparison.

The guide does not prescribe an exact NASA computer or require an RTOS implementation. Do not assume any dated week-by-week schedule is still current.

### 2.3 Necessary interpretation of the scheduling benchmark

For `n` independent periodic tasks, define `U = sum(C_i / T_i)` and `U_LL(n) = n * (2^(1/n) - 1)`. The classic sufficient bound concerns a single processor using **pre-emptive rate-monotonic scheduling**, with deadlines equal to periods and the relevant ideal assumptions. It does not certify this cooperative scheduler. Exceeding a sufficient bound does not establish failure. Plot it as a labelled theoretical reference, not as the cooperative system's pass/fail threshold. Any phrase “objective success criterion” must be qualified accordingly.

Primary reference: C. L. Liu and J. W. Layland, “Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment,” *Journal of the ACM*, 20(1), 46–61, 1973, [DOI](https://doi.org/10.1145/321738.321743); [paper hosted by the University of Pennsylvania](https://www.cis.upenn.edu/~lee/07cis505/Lec/liu73scheduling.pdf).

Do not reproduce any guide bibliography as verified. Reference-checking warnings remain unresolved.

## 3. Research questions and evidence

```
| Research question                                                                 | Evidence this artefact should provide                                                                                            | Limit of the conclusion                                                                                        |
| --------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| RQ1: How effective is cooperative task scheduling without an OS?                  | Load sweeps, response times, deadline misses, processor occupancy, and a controlled long-job blocking experiment.                | Behaviour of the declared finite workloads and scheduling model.                                               |
| RQ2: Is static memory allocation a viable alternative to dynamic memory management? | Matched memory-demand traces, budget sweeps, reserved capacity, useful live bytes, failures, and fragmentation where applicable. | Comparison against the implemented allocation strategy and traces.                                             |
| RQ3: How applicable are Voyager-era bare-metal principles today?                  | Synthesis of scheduling and memory trade-offs, implementation constraints, and a small integrated demonstration.                 | Design relevance to comparable constraints; no proof about all modern embedded systems or Voyager reliability. |
```

RQ3 is a reasoned synthesis of RQ1/RQ2 and literature. Do not invent a numerical “Voyager applicability score.” Results may favour different approaches under different conditions.

## 4. Scope and non-goals

### 4.1 Required minimum

1. One logical processor with integer simulated ticks.
2. Periodic task specifications and separate per-release job records.
3. Deterministic fixed-priority cooperative scheduling, with completion as the default yield point.
4. Fixed task/job storage and a statically backed memory arena with preassigned task regions in the primary mode.
5. A bounded runtime allocator comparison on the same memory-demand traces.
6. Reproducible workloads, CSV export, tests, and a modest analysis pipeline.
7. A small integrated cooperative/static example, without a large factorial experiment.

### 4.2 Optional after the required artefact works

- A tick-pre-emptive rate-monotonic comparison using the same engine and workloads.
- Cooperative jobs split at explicit application yield points, if blocking results motivate it.
- Native host-heap allocation as a clearly separate illustration.
- Small horizon/offset sensitivity checks beyond the required checks below.

### 4.3 Do not build

- A Voyager instruction-set emulator, flight-software replica, spacecraft simulation, or historical RAM reconstruction.
- A kernel, bootloader, RTOS port, interrupt controller, device drivers, or real hardware deployment.
- OS threads, processes per task, coroutines, or actual concurrency to simulate scheduling.
- A GUI, web dashboard, database, cloud service, plugin system, general simulation framework, or allocator library.
- EDF, multicore scheduling, locks, priority inheritance, fault injection, energy modelling, cache modelling, or formal verification in the minimum version.
- A complex configuration language, large dependency stack, or elaborate class hierarchy.
- Full project write-ups or fabricated measurements, citations, or results.

## 5. Technical constraints and architecture

### 5.1 Toolchain

- Use C++17, standard library facilities that fit the restrictions, CMake, and CTest.
- GCC is the intended research compiler. Allow Clang for development where available; record the actual compiler and version used. On macOS, check the compiler identity rather than inferring it from a command named `g++`.
- Enable useful warnings and provide an optional AddressSanitizer/UndefinedBehaviorSanitizer build where supported. Collect final logical results from a documented normal build, not sanitizer timings.
- Use Python 3 plus the standard library for orchestration/aggregation; add only Matplotlib if plots need it. Document the environment actually used.
- Do not silently install system packages or change the user's compiler configuration. If a tool is missing, use a suitable installed alternative and record the limitation.

### 5.2 Separate target core and host support

```
| Layer                     | Responsibility                                                                  | Allocation/I/O rules                                                       |
| ------------------------- | ------------------------------------------------------------------------------- | -------------------------------------------------------------------------- |
| Target core               | Clock, release logic, scheduler, fixed jobs, memory policies, counters.         | No host heap, threads, sleep, system time, file access, or console output. |
| Host executable           | Validate input, load fixed core configuration, invoke the core, export results. | Host allocation and file I/O allowed outside core execution.               |
| Experiment/analysis tools | Generate inputs, run batches, check CSVs, aggregate and plot.                   | Normal host facilities allowed; never presented as target behaviour.       |
```

The host computer runs an OS. “OS-free” describes the modelled target's scheduling and resource policy. The core does not emulate instructions or measure real embedded WCET. Task execution costs are supplied modelling inputs.

Use fixed-size records and `std::array` in the core. Avoid `std::vector`, `std::string`, growing containers, `std::function`, and heap-backed polymorphism there. Simple enums/functions and short linear scans are sufficient for 3–10 tasks. Keep an explicit boundary around core execution so output and parsing cannot contaminate allocation checks.

### 5.3 Suggested layout

```text
emulator/
CMakeLists.txt
README.md
include/emulator/ # task/job records, capacities, clock, scheduler, memory, metrics
src/core/ # target-only implementation
src/host/ # CLI, validation, CSV adapter
tests/ # small explicit test executable(s), CTest registration
configs/ # checked-in task sets and experiment manifests
scripts/ # workload generation, batches, CSV checks, analysis
docs/ # design, methodology, decisions, limitations, artefact inventory
results/ # run directories, manifests, raw CSVs, derived outputs
```

Adjust names to existing repository conventions. Do not create one abstraction/file for every small operation. A lightweight assertion-based test executable is sufficient; a third-party test framework is unnecessary.

### 5.4 Fixed capacities and allocation evidence

Start with compile-time capacities such as 16 task specifications, 1,024 concurrent jobs, and a 65,536-byte maximum experimental arena. Permit a smaller configured arena budget up to that maximum. Check these proposed capacities against the largest required workload before freezing the experiment manifest.

- Allocate large core storage in a fixed object with deliberate storage duration; do not hide an unbounded call stack or a large accidental stack allocation.
- Document `sizeof` the core storage, experimental arena capacity, active arena budget, fixed metadata, bounded trace storage, and what stack usage has/has not been measured.
- The arena budget sweep is a **payload-arena budget**, not the whole target's RAM. The executable always reserves its compiled maximum arena; do not report the configured prefix as actual total physical RAM.
- Preserve a fixed bound on task/job/allocator metadata. Do not let a smaller arena cause an unreported increase in metadata elsewhere.
- Add a test-build guard that detects C++ allocations during core execution and audit core objects/dependencies for C allocation calls. Guard all relevant `new` forms used by the build. Report the actual coverage; a guard for `new` alone does not prove interception of `malloc` or library internals.
- A practical combination of a narrow core API, dependency/source audit, allocation tests, and documented evidence is sufficient. Do not build a cross-platform interception framework or claim the whole host process never allocates.

## 6. Task model and precise scheduling semantics

### 6.1 Task specification versus job instance

A task specification contains a stable ID, period `T`, relative deadline `D`, fixed execution cost `C`, release offset `O`, priority, and reserved memory requirement. All timing fields use checked unsigned 64-bit tick arithmetic. Reject zero periods, zero execution costs, zero deadlines, duplicate IDs, invalid offsets, unsupported capacities, and arithmetic overflow before simulation.

A job contains task ID, per-task sequence number, release time `r`, absolute deadline `d = r + D`, first-start time if any, remaining execution, completion time if any, state, and a “miss already counted” flag. Use explicit validity flags for absent timestamps, not a zero sentinel, because a job can start at tick zero.

Default workload assumptions: periodic independent jobs; `D = T`; fixed `C`; no I/O waits, locks, dependencies, caches, interrupts, or release jitter. `C` is assumed execution demand, not measured hardware WCET. Backlog is allowed, including multiple releases of the same task; never overwrite the older job with the new release.

### 6.2 Cooperative policy

Assign higher priority to a shorter period. Resolve equal periods by ascending task ID, and jobs of the same task by release sequence. Smaller numeric priority values mean higher priority. Derive priorities once at setup and write them into the exported task set.

At each dispatch point, select the highest-priority ready job. Once selected, it runs until completion. A newly released higher-priority job waits until that completion. The default yield point is therefore the end of the job. This is a deliberately simple form of cooperative execution; it does not require coroutines.

For the optional segmented variant, yields may occur only at explicitly specified segment boundaries. A per-tick engine must not accidentally turn the primary policy into pre-emption.

### 6.3 Logical clock and boundary ordering

Use a transparent tick loop first. One tick of work occupies `[t, t + 1)`. Do not use sleeps, busy-waits, elapsed host time, or wall-clock timers to advance target time.

At a boundary `t`, use this order:

1. Finalise the work interval that ended at `t`, including any job completion.
2. Release every job with scheduled release `t`, provided `t < H`, where `H` is the release horizon.
3. Mark each still-unfinished job with deadline `<= t` as missed, once only.
4. If no job is running, select the next ready job using the cooperative rule.
5. Execute one tick of that job, or account for an idle tick, then advance time.

Thus completion exactly at the deadline is on time. Releases occur according to their intended times even while another job runs. Store those intended release times; do not reset releases to the next dispatch. Idle periods still advance the clock and process releases.

Model scheduler/allocation overhead as zero target ticks in the required version. Count decisions, scans, and allocator operations separately. Real host bookkeeping time is neither target CPU occupancy nor task execution cost. Any later nonzero overhead model needs an explicit new experiment label.

### 6.4 Horizon, backlog, and failure rules

- Admit releases in `[0, H)` only. At `H`, record the backlog and primary-window processor counters.
- Stop arrivals at `H` and drain all admitted jobs, so every job's response and deadline outcome is eventually known.
- After the last admitted completion, stop; no need to idle to a future deadline for jobs already completed on time.
- Keep main-window occupancy separate from drain work and drain duration. Compute deadline miss rate and response summaries over the full admitted release cohort, including jobs completed during drain.
- Bound the drain using checked arithmetic based on admitted work and the declared zero-overhead model. If a guard is exceeded, mark the run invalid/incomplete and report unfinished jobs.
- On job-capacity exhaustion or clock/counter overflow, stop with an explicit invalid-run status. Never silently drop, merge, overwrite, or count missing jobs as successful.
- Expected memory-budget rejection is an experimental outcome with its own status, distinct from a simulator defect/capacity failure.
- A finite run with no misses is evidence for that run, not proof of indefinite schedulability.

## 7. Memory design and fair comparison

### 7.1 Primary static mode

Use an aligned, statically backed byte arena. Before admitting work, reserve a fixed region for each task using its declared maximum memory requirement and a documented alignment. Keep offsets/handles and metadata in fixed arrays. Validate the sum of aligned reservations against the configured budget before execution.

Once admitted, task regions do not move, grow, or disappear during the run. A task can logically use fewer bytes than its reservation, but the unused capacity remains reserved. There is no runtime call to `malloc`/`new`, no per-job host allocation, and no fallback heap when the arena is full.

Return explicit admission failure when the reservations do not fit. Include any fixed headers, padding, and policy-specific metadata in the appropriate reported category. Check bounds and alignment with tests. Restrict requests to one supported alignment in the minimum version rather than building a general typed allocator.

### 7.2 Required runtime-dynamic comparison

Implement a small first-fit allocator with splitting, freeing, and adjacent-block coalescing inside a second instance of the same bounded byte-arena design. Use fixed-capacity block descriptors and offset handles, reject invalid frees, and report descriptor exhaustion separately from insufficient bytes.

This is **dynamic allocation within a bounded arena**, even though its backing storage is static. The comparison is fixed task reservations versus runtime sharing/reuse and fragmentation. It is not a benchmark of the host's `malloc`, all dynamic allocators, TLSF, or garbage collection. Label the baseline `bounded_first_fit`, never `native_heap`.

The primary cooperative/static configuration still performs no runtime allocation. The comparison core can also remain free of host-heap allocation while dynamically partitioning its arena.

### 7.3 Shared memory-demand trace

For RQ2, isolate memory management from scheduling first. Generate a deterministic trace of `allocate(owner, request_id, bytes)` and `free(request_id)` operations with explicit integer event times/order. Replay exactly the same trace under both memory policies and every relevant budget.

Each owner has at most one live request in the minimum trace model. Static mode reserves that owner's declared maximum request for the entire run. Dynamic mode allocates each request on demand and frees it at its prescribed end. This supplies a coherent comparison of the same application demand under persistent reservations versus temporal reuse. Per-job allocations with arbitrary overlapping lifetimes are outside this minimum trace contract.

If a dynamic allocation fails, keep replaying subsequent events; a later free for that failed request is a recorded skipped free, not an invalid-free defect. If static admission fails, report admission rejection and do not invent a runtime request success rate for an execution that never began. Show policy feasibility and failure stage alongside request-level counts.

Include constant-size demand, alternating lifetimes/variable sizes, and one hand-designed fragmentation case. Publish traces, generator version, seeds, ordering, owner maxima, and policy labels. Never substitute unrelated random streams between policies or adjust a trace after seeing which policy wins.

### 7.4 Memory measurements

At each event track budget `B`, live requested bytes `L`, occupied arena bytes `Q`, total free bytes `F = B - Q`, and largest contiguous free block `Fmax`. Keep policy metadata outside the arena visible as a separate fixed overhead rather than pretending it is free.

- **Static reservation:** aligned bytes committed at setup, constant after admission.
- **Unused reserved payload:** sum of owners' declared maximum requested bytes minus currently live requested bytes. Keep reservation padding separate. Report at each event and as a peak; do not infer it by subtracting unrelated peaks.
- **Alignment/header overhead:** charged occupied bytes minus corresponding requested capacity, separately from unused task reservations.
- **Dynamic peak occupancy:** maximum `Q`, including charged alignment/padding within the arena.
- **External fragmentation indicator:** `1 - Fmax/F` when `F > 0`; emit `NA` when `F = 0`. Report it only for the dynamic arena.
- **Fragmentation-related failure:** aligned request exceeds the largest fitting block despite sufficient aggregate free capacity under the declared metadata/alignment model.
- **Capacity failure:** insufficient total capacity; distinguish metadata-capacity failures.
- **Operation effort:** blocks inspected, splits, coalesces, and fixed-region lookups. These are deterministic algorithmic counts, not nanoseconds or target WCET.

For static mode, use `NA` for external fragmentation instead of treating unreserved space as a reusable dynamic heap. For dynamic mode, do not call all free bytes “wasted memory.” Both strategies may have unused capacity for different reasons.

### 7.5 Small integrated demonstration

After isolated experiments work, run a cooperative task set with admitted fixed task regions, and check every scripted memory access against the owner's region. Use a budget below and above static admission demand. Show deterministic success/rejection and unchanged schedule for successful admissions when memory access adds zero modelled CPU ticks.

Do not claim allocator latency affects deadline results: the minimum model does not charge it as execution time. A joint scheduling/allocator timing experiment would need a separate explicit cost model.

## 8. Metrics and accounting

Use ticks for target time and bytes for memory. Store exact counters/timestamps; use floating point only for derived ratios and summaries. Emit `NA` for undefined statistics, never zero or infinity disguised as a measurement.

```
| Metric                   | Definition and reporting rule                                                                                                                                                 |
| ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Requested utilisation    | `sum(C_i/T_i)` using the actual rounded integer task parameters; retain the requested generator target separately.                                                            |
| Observed CPU utilisation | Busy ticks in `[0,H)` divided by `H`. Report idle ticks too; drain is excluded.                                                                                               |
| Response time            | For each completed admitted job, `completion - release`, including drain completions.                                                                                         |
| Observed WCRT            | Maximum response time in the admitted cohort, per task and run. Call it “observed maximum response time” in conclusions; finite simulation is not a formal upper-bound proof. |
| Waiting time             | `first_start - release`; in the default non-pre-emptive model, response time equals waiting plus execution cost.                                                              |
| Deadline miss            | A job that is unfinished at its deadline after boundary completions are processed. Count it once, even if it remains late for many ticks.                                     |
| Deadline miss rate       | Missed jobs divided by all jobs released in `[0,H)`, with drain resolving their outcomes. Invalid/incomplete runs must not receive a normal success rate.                     |
| Lateness/tardiness       | Signed `completion - deadline`, or nonnegative `max(0, completion - deadline)` respectively. Avoid unsigned underflow.                                                        |
| Backlog                  | Queued plus running jobs at `H`, peak concurrent jobs, and any unfinished jobs at an invalid stop.                                                                            |
| Dispatches               | Count job selections. Keep scans/decision calls separate; checking a running job every tick is not a new dispatch.                                                            |
| Memory metrics           | Section 7 definitions, with static admission failures and dynamic runtime failures kept distinct.                                                                             |
| Liu–Layland reference    | Export `U_LL(n)` and the scheduler/assumption label described in section 2.3.                                                                                                 |
```

The default simulation has no target overhead, so `busy_ticks + idle_ticks = H`. Also require `released = completed` after a valid full drain, and `on_time + missed = released`. Completed execution ticks equal the sum of admitted job costs. Reconcile per-task totals with run totals.

Host wall-clock runtime may be recorded for practical batch sizing, separately from all target metrics. Repeating identical deterministic inputs is a reproducibility check; it is not an independent sample for confidence intervals.

## 9. Required experiment programme

Implement a tiny smoke profile first, then freeze a full experiment manifest. Use the following proposed values unless a validated capacity/runtime reason requires an explicit revision. Never silently skip failed runs or alter the grid to obtain favourable results.

### 9.1 E0 — hand-checkable validation cases

Include:

- A single periodic job stream with known idle intervals and response time equal to its execution cost.
- Simultaneous releases, equal-period ties, exact-deadline completion, and a one-tick-late completion.
- Release of a high-priority task while a lower-priority job is running.
- Multiple outstanding jobs of one task and a finite overloaded run.
- Successful static admission, one-byte-below required capacity, and an alignment boundary.
- Dynamic allocation, split, free, coalesce, exhaustion, and fragmentation examples.

One specific cooperative blocking fixture is: task A has `T=D=5`, `C=1`, `O=1`; task B has `T=D=20`, `C=7`, `O=0`. At tick 0, B starts; A's first release is tick 1 with deadline 6. B completes at 7 and A's first job completes at 8, so that A job misses. Check the actual timeline rather than copying these expected numbers into the metrics implementation. Record both tasks' costs and offsets in a small trace.

### 9.2 E1 — utilisation sweep for RQ1

- Task counts: `3, 5, 10`.
- Target utilisation grid: `0.20, 0.40, 0.60, 0.70, 0.80, 0.90, 1.00, 1.10`.
- Ten workload seeds, `1..10`, per task-count/load cell: 240 required primary runs before sensitivity checks.
- Select periods from a documented small set, initially `{50, 100, 200, 400, 800}` ticks. Repeat periods are permitted and priorities remain deterministic.
- Use positive random weights, normalised to allocate target utilisation among tasks; set `C_i` from each share times `T_i`, rounded to a positive integer. Prefer nearest-integer rounding and save the exact algorithm.
- Publish both nominal and achieved utilisation. Specify a tolerance such as absolute `0.02`; use bounded deterministic retries, or mark the requested cell unavailable if it cannot be constructed. Never conceal rounding errors or use unbounded rejection loops.
- Keep the underlying periods and weights paired across the load sweep for each task-count/seed family. The same exported task sets must feed every scheduling policy comparison.
- Default offsets are zero and deadlines equal periods. State that this is a restricted workload family, not a representative sample of all embedded workloads.
- Use `H = 100,000` ticks, followed by drain. The maximum suggested period is 800, so this covers many releases without an expensive long run.
- Use ample admitted static memory, for example 256 bytes per task with a 4,096-byte active arena budget, so memory does not determine RQ1 outcomes.

Before the full batch, check backlog requirements against fixed capacities at `U=1.10` for all task counts. Increase the declared compile-time job capacity if needed and rerun validation before freezing the manifest; do not change overload semantics.

For sensitivity, rerun at least one low-load, one near-transition, and one overloaded workload at `H=200,000`. Also run one deterministic staggered-offset version of those workloads. Label these separately; they test sensitivity and do not establish an exhaustive phase analysis.

Produce per-task and per-run response, miss, occupancy, and backlog results. Show actual utilisation on the x-axis. Show the theoretical reference only with its stated assumptions. Do not call the observed first miss a universal cooperative utilisation limit.

### 9.3 E2 — long-job blocking for RQ1

Start from an explicit small task set with a short-period, high-priority task and a longer-period, low-priority task. Use the E0 blocking fixture as a correctness anchor.

For the experiment sweep, hold total low-priority utilisation approximately fixed while increasing its contiguous execution length: for example use low-priority pairs `(C,T) = (2,40), (4,80), (8,160), (12,240)`. Keep the high-priority workload fixed, record actual load, and give it an offset that releases it after the long task starts. Run a matching all-zero-offset condition to show the effect of release phase.

Measure the high-priority task's waiting time, observed maximum response, and misses; record the long task's maximum contiguous occupancy. State that period and release frequency change in this controlled construction, even though low-priority utilisation stays constant. Save all parameters and traces for representative cases. Do not attribute changes solely to total utilisation.

If segmented yields or pre-emption are implemented later, rerun these exact workloads as a labelled extension. Do not replace the primary results.

### 9.4 E3 — memory budget and lifetime sweep for RQ2

Use five owners and three trace families:

1. **Constant demand:** stable requests showing the predictable case for static reservations.
2. **Staggered variable demand:** owners need up to 512 bytes each, but peaks occur at different times, allowing temporal reuse.
3. **Fragmentation pressure:** variable sizes and lifetimes designed to create nonadjacent holes; include an explicit small manually checked fixture as well as longer seeded traces.

Use 100 bounded allocation/free cycles per owner, ten seeds for generated trace families, and active budgets `512, 1024, 1536, 2048, 2560, 4096` bytes. Replay both `static_reservation` and `bounded_first_fit`. A deterministic family may use one canonical trace instead of ten identical copies; do not treat duplicate data as independent replication.

Declare owner maxima from the published trace contract before replay. Keep one-live-request-per-owner semantics and deterministic same-time ordering. For long fragmentation traces, choose sizes/lifetimes that actually exercise holes; verify that a reported fragmentation case has aggregate free space but no fitting contiguous block.

Compare admission feasibility, request outcomes when admitted, peak useful demand, occupied bytes, unused reserved payload, overhead, free-block structure, and operation effort. Include budgets where both succeed as well as where one fails. If a proposed trace does not fragment, report that result and retain the explicit fragmentation fixture rather than inventing fragmentation values.

### 9.5 E4 — integrated evidence for RQ3

Run a five-task cooperative/static example at moderate load with declared task regions and at least one higher-load variant. Use one admitted and one rejected memory budget. Publish scheduler and memory outcomes with the same run identifiers. This confirms the two principles coexist in one implementation; it does not establish performance on a physical embedded processor.

Write a short evidence note linking observations to conditions: short bounded jobs versus long blocking, sufficient static capacity versus changing live demand, and simplicity versus flexibility. Discuss missing hardware, interrupts, caches, peripheral timing, energy, and real fault behaviour as limitations.

### 9.6 Optional E5 — pre-emptive comparison

If required phases are complete and this is a small change, add `preemptive_rm` using the same one-tick core, jobs, releases, priorities, zero-overhead assumption, and inputs. Reconsider the running job at tick boundaries, preserving remaining execution. Add explicit tests for pre-emption and resume. Compare paired outcomes; do not call this an RTOS benchmark or assign invented context-switch cost.

If omitted, document that no implemented pre-emptive comparator exists. Literature/reference comparisons still need the caveats already specified.

## 10. Outputs, CLI, and reproducibility

### 10.1 Keep the interface small

Provide a CLI capable of running an exported task set and selecting scheduler, memory policy/budget, horizon, and output directory. Provide a separate trace-replay command or mode for E3. A simple line-based task CSV and a documented manifest format are enough; no interactive prompts or YAML framework are needed.

Provide copyable commands in the README for configure/build, CTest, one scheduling example, one memory replay, the smoke batch, the full batch, and analysis. Use the actual executable/script names you implement. Ensure paths with spaces work.

Use nonzero exit codes for invalid input, simulator defects, I/O errors, and incomplete execution. Define how a batch records expected budget rejection as an experimental result and continues. Fail the batch on unexpected simulation errors unless explicitly collecting diagnostics.

### 10.2 Raw and derived files

At minimum produce:

```
| File                     | Essential content                                                                                                                                                                                 |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `manifest.json`          | Schema version, experiment IDs, input paths/hashes, seeds, capacities, horizon, policies, compiler/build/platform, revision or source hash, and exact commands.                                   |
| `tasksets.csv`           | Task-set ID, seed/family, task ID, priority, period, deadline, execution cost, offset, reserved bytes, nominal/achieved utilisation.                                                              |
| `runs.csv`               | Run/input IDs, policies, budget, horizon, status/reason, released/completed/on-time/missed counts, window busy/idle ticks, backlog, drain duration, observed max response, reference-bound value. |
| `task_metrics.csv`       | Run/task ID, counts, response and waiting summaries, maximum response, misses, and units.                                                                                                         |
| `jobs.csv`               | Run/task/job ID, release, absolute deadline, first start, completion, response, tardiness, missed flag.                                                                                           |
| `memory_trace.csv`       | Trace ID, seed, event index/time, owner/request ID, operation, bytes, owner maximum. This is the input, not a reconstructed approximation.                                                        |
| `memory_events.csv`      | Run/event IDs, outcome/reason, requested/live/occupied/reserved/free bytes, largest free block, operation counts, fragmentation indicator if applicable.                                          |
| `memory_runs.csv`        | Policy/budget/status, admission outcome, valid request counts/failures, skipped frees, peak metrics and accounting scope.                                                                         |
| `summary.csv` and plots  | Derived experiment summaries and labelled figures; preserve all raw files.                                                                                                                        |
| `README`/data dictionary | Units, formulas, missing-value rules, status enums, schemas, and commands.                                                                                                                        |
```

Store source references and limitations in documentation, not as invented columns of “verified” claims. Add event traces for small validation/blocking examples; do not dump a per-tick row for every full-batch tick.

Retain all job outcomes without unbounded core logging: a core step can return a bounded event/result buffer, then the host writes it between steps. Drain the buffer before advancing again. Host output must not change logical time or scheduler choices. Treat buffer overflow as an error, never silent data loss. Do not retain millions of completed jobs in core arrays.

### 10.3 Reproduction rules

- Use a named deterministic PRNG and explicit seed; save the generated task sets and traces so reproduction does not depend on library distribution details.
- Separate stable results from volatile metadata such as host runtime and creation timestamp. Identical inputs/build must give identical logical rows and counters.
- Create a new run directory for each batch. Do not overwrite prior raw data, hand-edit outcomes, or present a smoke batch as the full matrix.
- Include source revision if under version control; otherwise record a source-bundle hash.
- Write CSV with consistent escaping, stable column ordering, adequate numeric precision, and a documented schema version.
- Check CSV row counts and accounting invariants before making plots. Failure/incomplete rows stay visible and are excluded from normal success summaries with counts reported.
- Plot miss rate and observed maximum response versus achieved utilisation; high-priority response versus contiguous low-priority cost; and memory feasibility/occupancy versus budget. Keep policy labels and units explicit.
- Across generated task sets, show descriptive median/range or quartiles and number of distinct inputs. Do not manufacture confidence from repeated identical runs. If doing inference later, define the population and sampling scheme first.
- Every figure/table should identify the input batch and be reproducible from saved raw data. Include brief findings with numeric evidence only after runs finish.

## 11. Implementation phases and exit checks

```
| Phase                    | Build                                                                                           | Exit check before continuing                                                             |
| ------------------------ | ----------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| 0 — inspect and scaffold | Inspect current code/references; record decisions; create minimal build and one test.           | Clean configure/build/test works; no synced reference is changed.                        |
| 1 — scheduling core      | Task/job model, clock, releases, cooperative selection, completion and drain, minimal counters. | Hand-calculated timelines, exact-deadline boundary, idle and overlap tests pass.         |
| 2 — static memory        | Fixed task regions, alignment/budget checks, ownership/access checks, core allocation evidence. | Valid admission, deterministic rejection, bounds and allocation tests pass.              |
| 3 — measurement/export   | Counters, job events, CSV output, run statuses and manifest.                                    | Raw rows reconcile with counters and reproduce exactly for a saved fixture.              |
| 4 — RQ1 experiments      | Seeded task generator, E1/E2, small smoke profile, capacity check.                              | Smoke and representative overload runs are valid; achieved load and backlog are visible. |
| 5 — RQ2 comparison       | Bounded first-fit allocator, shared demand traces, E3.                                          | Allocation/coalescing/fragmentation fixtures and matched-input checks pass.              |
| 6 — evidence batch       | Freeze manifest, run full E1–E4 plus required sensitivity checks, aggregate and plot.           | Every planned cell has a result/status; no silent exclusions; invariants pass.           |
| 7 — handoff              | README, methodology/design/limitations, data dictionary, artefact inventory and package script. | A fresh output directory can reproduce the smoke evidence using documented commands.     |
```

Build core behaviour before broad instrumentation. Produce one small, valid raw result as soon as phases 1–3 are ready. Do not postpone the required data to add optional features. If a full batch is blocked by the execution environment, finish the runnable pipeline, state exactly what was run, and leave commands/manifests for the outstanding batch; never fill missing results with examples.

## 12. Required verification

Use tests that establish model behaviour and accounting, not tests that merely duplicate internal implementation details.

**Scheduler:** hand-computed single/multiple-task cases, deterministic priority ties, release during execution without pre-emption, exact-deadline success, one miss counted only once, idle advancement, overlapping jobs, overload/backlog/drain, and integer-overflow/input rejection.

**Memory:** exact-fit and too-small static budgets, alignment, owner boundary violation, dynamic split/reuse/coalesce, full free-space recovery after successful frees, duplicate/invalid free rejection, skipped frees after failed requests, actual fragmentation failure, descriptor exhaustion, and reset independence between runs.

**Integration:** no core host-heap allocation in the tested paths, deterministic replay, equivalent logical results with detailed event output enabled/disabled, identical input hashes across policy pairs, fixed-capacity overflow reported rather than dropped, CSV totals matching core totals, and analysis handling rejection/incomplete rows without reporting them as successes.

Run CTest on normal builds and sanitizer checks where available. Run the smoke batch and independent CSV accounting validation before the full experiment set. A small hand-worked timeline is the correctness oracle; do not validate the scheduler solely against another implementation written from the same code structure.

## 13. Acceptance criteria

The required implementation is complete only when:

- [ ] C++ source builds with the recorded toolchain and documented commands.
- [ ] The primary scheduler is cooperative, deterministic, single-processor, and uses logical ticks.
- [ ] Core storage is bounded and no target host-heap fallback exists; allocation evidence and coverage limitations are documented.
- [ ] Task and job records are distinct; release times, priority ties, deadlines, backlog, drain, and invalid-run semantics match this brief.
- [ ] Static reservations have enforced ownership, alignment, and admission limits.
- [ ] The runtime-dynamic comparison is implemented and accurately labelled; matched traces and budget accounting are available.
- [ ] Required correctness tests pass; any unavailable compiler/sanitizer check is explicitly recorded.
- [ ] E1–E4 and required sensitivity checks have raw outputs or are explicitly listed as outstanding. Outstanding required runs mean the research evidence is not yet complete.
- [ ] Every planned experiment has an inspectable status; failed/rejected cases are retained and distinguished.
- [ ] Response, misses, CPU occupancy, memory figures, and run totals can be checked against raw records.
- [ ] Stable logical output reproduces from saved inputs; independent workload seeds are distinguished from reruns.
- [ ] Analysis scripts produce labelled summaries/figures without editing raw data.
- [ ] The theoretical benchmark is qualified, finite observed maxima are not sold as proofs, and there are no unsupported hardware/RTOS/flight-reliability claims.
- [ ] README, design/methodology notes, data dictionary, limitations, actual commands, and artefact inventory are supplied.
- [ ] Final delivery states what was implemented, what was run, the key observed findings if available, and optional omissions.

No acceptance criterion requires the cooperative scheduler or static memory to outperform a baseline. A correctly reproduced limitation is a valid research finding.

## 14. Handoff and what to keep simple

Provide a concise `docs/methodology.md` describing operational definitions, independent/dependent variables, controlled factors, workloads, memory comparison, repeated seeds, validation, and threats to validity. Provide `docs/decisions.md` for departures from this brief, and `docs/limitations.md` for known boundaries and omitted extensions. Keep these factual implementation notes.

The artefact package should include source, configurations, tests, scripts, raw data, derived outputs, environment/build instructions, and an inventory. Exclude local compiler build directories, caches, duplicate batches, and generated dependency folders.

Prefer linear scans, fixed arrays, integer ticks, a simple first-fit baseline, a short CLI, and a few clear scripts. Do not optimise tick processing until a measured batch-runtime problem justifies it. Do not add response-time analysis formulae, arbitrary task segmentation, a sophisticated allocator, or an RTOS merely to make the artefact look advanced. Explain the completed model clearly enough that a reader can trace a job from release to output row and a memory request from demand to outcome.

Start by inspecting the repository, then implement phase 0 and the smallest correct task/clock/scheduler example. Continue through the required phases without asking for approval at every routine step. Seek clarification only when an unresolved issue materially changes the research question or requires information that cannot be inferred; continue independent useful work meanwhile.