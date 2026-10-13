# Methodology

The target is a single logical processor. The host OS, compiler and Python scripts are experimental support. All inputs are synthetic. Tick costs are supplied execution demands, not measured WCET. A tick is [t,t+1); no host time advances the model. Scheduler and memory operation costs are zero target ticks.

## Scheduling experiment

Tasks have stable uint32 IDs and uint64 period, deadline, cost and offset. Job records are separate and may overlap for the same task. Inputs reject zero timing parameters, offset >= period, duplicate IDs, unsupported capacities and overflow. Priorities rank shorter periods first, then smaller task IDs; same-task jobs rank by sequence. Jobs yield on completion only.

At each boundary the engine finalises completion, admits releases at that tick if t<H, marks unfinished deadlines <=t once, dispatches if idle, and runs one tick. A completion exactly at its deadline succeeds. Idle ticks still process arrivals. At H it records remaining jobs, stops arrivals, then drains the cohort. A conservative checked H+total_admitted_work bound limits the drain. A fixed-storage failure marks the run incomplete rather than dropping work.

E1 varies task count (3,5,10), nominal utilisation (0.20,0.40,0.60,0.70,0.80,0.90,1.00,1.10) and ten seeds per family: 240 primary cells. H=100,000. Periods come from {50,100,200,400,800}; deadlines equal periods, offsets are zero, reservations are 256 bytes/task and budget 4,096. Achieved U=sum(C/T) uses rounded costs.

Generator v1 is xorshift32 with shifts 13,17,5 and masking to 32 bits, initial state seed*1009+n*9176. Candidate periods use next()%5 and positive weights use 1+next()%1000. Cost is max(1,round_half_up(load_percent*weight*period/(100*sum_weights)))). Integer arithmetic implements the rounding. A candidate family is accepted only if all eight achieved loads differ from their targets by <=0.02, with at most 1,000 attempts. Periods and weights are then paired across all loads; the saved `family_attempt` is zero-based. Failure to construct a family stops preparation explicitly; it does not omit a cell.

E2 holds the short task at C=1,T=D=5 and uses long (C,T) pairs (2,40),(4,80),(8,160),(12,240). The short task offset is either zero or one. H=10,000, with 2,000 short-task releases each. Long-task utilisation stays 0.05; period and release count change with contiguous cost. All job intervals are retained.

Sensitivity repeats five-task seed-1 loads 0.20,0.80,1.10 at H=200,000, with zero and staggered offsets. For task index i=0..4, staggered offset=floor(i*T_i/5). Six cases test horizon/phase sensitivity, not all phases. E0 supplies three small saved timelines and the C++ tests add exact boundaries, overload and capacity fixtures.

## Memory experiment

E3 replays five-owner traces through `static_reservation` and `bounded_first_fit`, at active budgets 512,1024,1536,2048,2560,4096 bytes. Each owner has at most one logically pending request, including a failed allocation awaiting its scripted free. Request IDs are globally unique within a trace. Every request eventually has a free; frees after failures are skipped and counted. Owner maxima are declared by the trace contract before replay. Same-time events preserve insertion order, and explicit event indices fix replay order.

There is one canonical constant trace, ten staggered traces, ten generated fragmentation traces, and one canonical manual fragmentation fixture. Generated families use 100 cycles per owner (500 requests/1,000 events per trace). No tenfold replication of the constant trace is counted as independent evidence. Including the fixture gives 22 traces and 264 policy/budget runs.

Memory generator v1 initialises xorshift32 with seed*104729. For each seed it generates the staggered family before fragmentation from the same continuing PRNG stream. Staggered events allocate owner i at 40*cycle+7*i and free five ticks later, using 64*(1+next()%8) bytes, maxima 512 each. Fragmentation cycles allocate four owners with sizes 64*(1+next()%4), free alternating owners, then request 64*(4+next()%5) for the fifth. Remaining requests are freed before the next cycle; maxima are 512 each, including owners whose generated sizes only reach 256. Constant requests are 256 bytes with maxima 256. The fixture declares maxima [128,128,128,128,192]. These contracts deliberately expose contrasting reservation/reuse conditions.

Static regions are aligned to eight bytes and persist for the replay. Dynamic first-fit splits aligned blocks, rejects insufficient capacity or descriptor exhaustion, and coalesces adjacent free blocks. Thirty-two fixed block descriptors are outside the payload arena. No host allocation is used by either core policy. No latency comparison with malloc, RTOS or other allocators is intended.

At each event, useful live bytes L count successful requests only, while demand includes failed requests still logically pending. Occupied bytes Q include charged padding; free bytes F=B-Q. Static unused reservation=sum(owner maxima)-L, with alignment padding separate. Dynamic external fragmentation is 1-largest_free/F for F>0, else NA. A fragmentation failure needs enough aggregate free capacity but no fitting contiguous block. These are deterministic algorithm counts and layouts, not elapsed-time measurements.

## Integrated example and analysis

E4 uses the five-task seed-1 families at nominal 0.60 and 1.00. Each has budgets 1,024 and 1,536 against 1,280 bytes of reservations. Every dispatch writes and reads the first and last byte of the task's region. Access checks add zero target ticks. Unit tests verify equivalent schedule metrics with and without scripted memory accesses.

For completed runs, miss rate=missed/released, response=completion-release, waiting=first_start-release, tardiness=max(0,completion-deadline), and CPU occupancy=busy_ticks_in_[0,H)/H. Responses/misses include drain completions; occupancy excludes the drain. All admitted jobs eventually complete. Observed maxima are finite cohort statistics, not proven upper bounds. See the data dictionary for raw fields.

Analysis verifies raw hashes, job/task/run accounting, trace identity and every memory event before summarising. Summaries use descriptive median and min–max over distinct saved inputs. Invalid/pending rows stay counted and are excluded from success statistics. Static admission rejection has no runtime success rate. Tests exercise these exclusions explicitly. Repeating a run is a determinism check, not another independent sample; no confidence intervals or population inference are claimed.

The Liu–Layland value n*(2^(1/n)-1) is labelled a preemptive rate-monotonic sufficient reference, with independent periodic tasks, deadlines equal periods and ideal processor assumptions. It does not certify completion-yield cooperative execution. The paper's model and bound were checked against [Liu and Layland (1973), sections 3–5](https://www.cis.upenn.edu/~lee/07cis505/Lec/liu73scheduling.pdf), DOI [10.1145/321738.321743](https://doi.org/10.1145/321738.321743). No other guide bibliography was verified.

## Validation and reproducibility

Normal and ASan/UBSan CTest runs precede the batch; a source/dependency audit and test-only global new guard cover tested core paths. A 36-case capacity preflight found a 1,024-job limit insufficient, so the final fixed capacity is 4,096; maximum observed concurrency was 1,038. Capacity remains fixed across the complete final matrix. Manifest inputs were frozen before the full run.

The saved full batch has 240 E1, eight E2, 264 E3, four E4, six sensitivity and three E0 runs: 525 planned cells. The 62-cell smoke is explicitly a subset, and its earlier 1,024-job build was sufficient for those inputs. Full results use 4,096. Manifests record the actual source revision/hash, binary hash, compiler/configuration, platform, Python, input/output hashes and exact commands. Final artifact source can add analysis/documentation after the measurement revision; the measurement revision remains identified. There is no random use of host time and no hand-edited result data.
