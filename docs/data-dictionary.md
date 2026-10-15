# Data dictionary (schema 1)

Time and execution counts use integer ticks; payload and storage sizes use bytes. IDs are identifiers, not measurements. Ratios are dimensionless. CSVs use comma separators, one header row, LF endings and numeric fields or restricted identifiers/enums; input quoted fields, blank records and comments are unsupported. Output IDs are restricted to ASCII letters/digits/underscore/hyphen, so no output field needs quoting. `NA` means undefined or inapplicable, never a measured zero.

Each run lives under `<batch>/<group>/<run_id>/`. Scheduling files are `tasks.csv`, `jobs.csv`, `task_metrics.csv`, `runs.csv`. Memory files are `memory_trace.csv`, `memory_events.csv`, `memory_runs.csv`. Preserve the raw directories; aggregation writes a separate new directory. A rejected run still has headers and a status row.

## Inputs and provenance

- Task input: `task_id,period,deadline,cost,offset,reserved_bytes`. Deadline is relative, cost is supplied execution demand, and offset is the first release. Exported `tasks.csv` adds `priority`, with zero highest.
- `tasksets.csv`: taskset ID, seed, task count, zero-based candidate family attempt, nominal/achieved U, task ID/priority and exact task parameters. This is the 240-cell E1 input catalogue, not a list of every E0/E2/E4/sensitivity task set. The full manifest identifies those other exact input files.
- Memory trace: `event_index` is zero-based order; `time` is integer event time; `owner` is zero-based; `request_id` is unique within that trace; `operation` is allocate/free; `bytes` is requested payload for allocation and zero for free; `owner_maximum` is constant per owner. The copied trace is exactly the input bytes, not reconstructed from successful events. One live logical request per owner includes a failed allocation until its free.
- `manifest.json`: schema, profile, generator, source revision/content hash, dirty-worktree flag, platform/Python, actual executable build info/hash, plan hash, original working directory/command, and every planned run. Each entry contains parameters, seed where applicable, input path/hash, status, exact command, output path and hashes of all raw files. Unexpected errors have a reason; unexecuted entries remain pending. Commands retain original absolute paths where used; README commands reproduce from a new checkout.
- `configs/frozen.json`: plan, E1 catalogue and all input hashes plus build capacities at freeze. `configs/experiments.json` is the executable grid. These are separate from result rows.

## Scheduling outputs

`jobs.csv` retains every completion in completion order. `run_id`, `task_id`, `sequence` identify a job (sequence starts at zero per task). `release` is its intended arrival, `deadline` is absolute release+relative deadline, `first_start` and `completion` are valid timestamps, including zero. `response=completion-release`, `waiting=first_start-release`, `tardiness=max(0,completion-deadline)`, `missed` is 0/1. No unsigned negative lateness is exported. A completion at the deadline is on time. The nonpreemptive execution interval is [first_start,completion).

`task_metrics.csv` contains released/completed/missed counts, response/waiting sums, observed maxima and arithmetic means over the admitted cohort after drain. For a task without completions, means and maxima are NA. Run totals reconcile with sums of per-task counts; run maximum is the maximum job response.

`runs.csv` fields:

| Fields | Meaning |
| --- | --- |
| run_id, scheduler, memory_policy, budget, horizon | Identity and declared controls; budget is active payload arena prefix |
| status | Core outcome, described below |
| released, completed, on_time, missed | Cohort counts; on_time is NA if incomplete/rejected |
| busy, idle | Intervals in [0,H), with busy+idle=H for complete runs |
| backlog, peak_jobs | Unfinished jobs at H after boundary completions; peak simultaneous active jobs |
| drain_ticks, drain_busy | Time past H until last completion; executed work after H, distinct from main-window occupancy |
| unfinished | Jobs still active at an invalid stop, zero for a complete drain |
| max_response | Observed cohort maximum in ticks, NA with no completions |
| miss_rate | missed/released after a complete drain, otherwise NA (also NA for no releases) |
| cpu_utilisation | busy/H for complete runs only |
| actual_utilisation | Sum of declared C/T, distinct from measured occupancy and generator target |
| ll_reference | n*(2^(1/n)-1), theoretical preemptive RM reference only |
| dispatches, scans | Job selections; active job records inspected during selection (includes the initial candidate) |
| required_static_bytes | Sum of aligned task requirements, including when admission is rejected; a requirement, not successful Q for a rejected run |
| reserved_payload, padding | Sum of task maxima and their alignment padding |
| accesses | Region lookups for scripted reads/writes; four per dispatch with a nonzero region |
| detailed_jobs | 1 if completion rows were written; 0 intentionally leaves only the jobs header |

Full batch runs retain detailed jobs. The logging-independence test explicitly exercises the disabled setting. No per-tick million-row logging is needed.

## Memory outputs

`memory_events.csv` starts with run/event/time/owner/request/operation/bytes, matching input events exactly. All measurements are post-event snapshots:

| Fields | Meaning |
| --- | --- |
| outcome | ok, capacity, fragmentation, descriptors, or skipped_free |
| demand | Bytes logically requested by all pending lifetimes, including failed requests |
| live | Useful payload of successful pending allocations only |
| occupied | Q: aligned reservations in static mode; allocated aligned blocks in first-fit |
| free | B-Q; static unreserved space is not available for runtime sharing |
| largest_free | Largest contiguous dynamic hole, zero if none; NA for static |
| unused_reserved | Sum(owner maxima)-live for static; NA for dynamic |
| padding | Static Q-sum(maxima); dynamic Q-live. No in-arena headers are charged by this implementation |
| external_fragmentation | Dynamic 1-largest_free/free when free>0, else NA; always NA for static |
| blocks_inspected | Cumulative descriptor inspections in first-fit allocation and free scans |
| splits, coalesces | Cumulative split/adjacent-hole merge counts |
| lookups | Static allocation region lookups; no runtime allocator in that policy |

`memory_runs.csv` has identity/policy/budget/status, event count, allocation request/success/failure counts, skipped frees and failure reasons. Capacity means insufficient total aligned free bytes; fragmentation means enough total free bytes but no fitting block; descriptors means a needed split cannot be represented. The latter is distinct from byte capacity. A free following failure is skipped rather than sent an invalid handle.

`peak_demand`, `peak_live`, `peak_occupied`, `peak_unused_reserved`, `peak_padding` are independent maxima of event snapshots, with the admitted initial static state also considered. Do not subtract unrelated peaks to calculate unused space. For static rejection these peaks are NA, requests/events remain zero, and no request success fraction is defined. Static admission feasibility is assessed separately.

`policy_metadata_bytes` is sizeof(the selected arena object)-65,536; it includes descriptors/regions/counters/padding outside payload, not the whole replay. `replay_storage_bytes` is sizeof Replay, including both compiled arenas and shared fixed metadata. `emulator info` also supplies full Scheduler/Replay sizes, non-arena bytes, alignment and capacities. These numbers are compiler/ABI-specific; host I/O containers and total process RAM are outside this accounting.

## Status and exits

Core statuses are `complete`, `memory_rejected`, `invalid_input`, `job_capacity`, `arithmetic_overflow`, `access_error`; `ready` exists only during execution. Rejection is expected in budget sweeps. Capacity exhaustion, invalid access or arithmetic failure are incomplete/invalid results and never silently successful. Normal CLI exports invalid core status rows where execution was set up, but host parsing/trace validation failures can stop before an output directory exists.

CLI exit 0 means complete, 2 means argument/parser/host validation failure, 3 means expected static budget rejection, 4 means invalid/incomplete core outcome (including invalid scheduler configuration), 5 means output stream/filesystem failure. Input file open/parse errors use 2. The batch permits 0/3 only, checks their status rows, saves unexpected errors and stops. Directories must be new to protect prior raw data.

## Derived outputs

`analysis/schedule_runs.csv` and `analysis/memory_runs.csv` flatten raw summaries with group/seed/input metadata, preserving rejections. `status_counts.csv` counts every planned status. `summary.csv` groups scheduling inputs by task count/nominal load/phase/contiguous cost/budget; `memory_summary.csv` groups family/policy/budget. Statistics use completed runs only and include median/min/max, distinct input count, completed/rejected/incomplete counts. Memory admission_fraction is admitted traces/all planned traces when no incomplete outcome exists, while request_failure_rate is defined only for admitted traces. These answer different questions. E0 aggregates can combine small fixtures and are for accounting, not inference.

`blocking_summary.csv` gives the ID-1 high-priority response/wait/miss counts for each E2 condition. `analysis.json` records raw manifest hash, analysis script hash, revision and outcome counts. Plot metadata identifies the batch and script hash. Figures use actual utilisation, ticks or bytes as labelled; range bars describe different inputs, not confidence intervals. `docs/verification/capacity.csv` contains a separate preflight, not additional independent workload samples.
