# Findings from the saved batch

These numbers come from `results/full/manifest.json` and the checked CSVs in `results/full/analysis/`. They describe this model and these finite inputs. Figures are in `results/full/plots/`.

The full batch has 525 planned outcomes: 440 completed replays/simulations and 85 expected memory admission rejections. There are no incomplete runs. Scheduling admitted and completed 1,163,718 jobs in total, including E0, E2, sensitivity, and E4. This is not 525 independent samples: policies and budgets reuse inputs deliberately.

## Scheduling

All 30 synchronous E1 task sets at nominal U=0.20 met every deadline (achieved U=0.19000–0.21875). At nominal U=0.40, 24 of 30 had no misses. At U=1.10 none of 30 had zero misses. The median miss fractions at U=1.10 were 0.49783 for three tasks, 0.44349 for five, and 0.15531 for ten. These are fractions of jobs, not fractions of task sets. Different task counts change periods, weights and job counts, so this is not evidence that adding tasks improves scheduling.

There is no single observed cooperative utilisation threshold. Some medians are nonmonotonic as integer costs change, and the task sets use harmonic periods. The Liu–Layland line is only a reference for preemptive rate-monotonic scheduling under its assumptions; this core is cooperative. Exceeding that sufficient bound does not prove failure, and being below it does not certify this policy. See [Liu and Layland (1973), sections 3–5](https://www.cis.upenn.edu/~lee/07cis505/Lec/liu73scheduling.pdf).

In E2 the total utilisation stayed at 0.25. With short-task offset 1, contiguous long costs 2,4,8,12 gave short-task maximum responses 2,4,8,12 ticks, and deadline misses 0,0,63,84 out of 2,000 releases each. With all offsets zero, the corresponding responses were 1,1,5,9 and misses 0,0,0,42. Long-task periods also changed (40,80,160,240); this construction isolates a useful blocking contrast without claiming execution length was the only changed parameter. See `blocking_summary.csv`.

Sensitivity matters. For the five-task seed-1 U=0.20 case at H=200,000, zero offsets gave no misses, while staggered offsets gave 250 misses out of 9,250 releases. Its observed maximum response actually fell from 91 to 60 ticks: the run maximum alone does not identify which task's deadline is threatened. The U=1.10 zero-offset case drained for 19,000 ticks with an observed maximum response of 47,814 ticks. These six checks are not an exhaustive phase study.

## Memory

The constant trace has five simultaneous 256-byte requests. Static reservations need 1,280 aligned bytes and are rejected at budgets 512 and 1,024, then admitted with no request failures at every larger tested budget. First-fit starts at all budgets but fails 300/500 requests at 512 and 100/500 at 1,024. Successful replay admission therefore does not imply that every request succeeds.

The staggered traces declare 512 bytes per owner and never overlap requests. All ten first-fit traces complete without failures at a 512-byte budget, with peak occupancy 512 bytes. Static reservations require 2,560 bytes; they are rejected below that budget. At 2,560 both policies succeed, but static occupancy stays at 2,560 bytes while first-fit peaks at 512. Static's unused reservation is reported per event separately from padding. These workloads intentionally favour reuse; they do not represent every application.

The hand fixture at budget 512 frees alternating 128-byte blocks. Its 192-byte request fails with aggregate free capacity 256 and largest free block 128, establishing an actual fragmentation failure. Across ten generated fragmentation traces, first-fit fragmentation failures had median 31 (range 20–37) at budget 512 and median 38 (30–48) at 1,024. At 1,536 all generated fragmentation traces had zero request failures. The larger-budget count can exceed the smaller one because earlier capacity failures change later occupied layouts; inspect both failure reasons rather than assuming monotonic fragmentation counts.

The comparison charges no allocator or scheduler overhead ticks. Operation counts describe the implemented algorithms, not measured latency. The backing arrays and metadata remain compiled into the executable even when a smaller active prefix is selected.

## Integrated example and interpretation

Five tasks reserve 1,280 bytes. Budgets 1,024 reject admission at both loads, while 1,536 admits both. The nominal 0.60 case completed 4,625 jobs with 875 misses; the nominal 1.00 case completed 4,625 with 2,936 misses and 125 drain ticks. Each successful run performed 18,500 checked memory operations (four per job). Bounded memory and completion-yield scheduling coexist, but neither is a guarantee that selected deadlines will be met.

For RQ3, the implementation illustrates the value of inspectable bounded storage and simple dispatch rules, and their costs: memory must be provisioned for declared maxima, while cooperative response depends on contiguous execution and release phase. This is a design inference from RQ1/RQ2, not a measurement of Voyager or a numerical applicability score. Hardware, interrupts, cache effects, peripherals, energy and fault behaviour remain outside the model.
