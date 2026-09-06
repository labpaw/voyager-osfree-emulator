# Small correctness examples

Ticks describe intervals [t,t+1). Completion is processed before deadline checking.

- `single.csv`, H=12: run [0,2), [5,7), [10,12). Six busy and six idle ticks, three responses of two ticks, no misses.
- `blocking.csv`, H=10: B (ID 2) runs [0,7). A releases at 1 and 6. The first A job starts at 7 and completes at 8, missing deadline 6. The second runs [8,9) and meets deadline 11. Only one miss, despite the first job being late at two boundaries.
- `ties.csv`, H=1: ID 2 runs [0,1), then ID 8 drains [1,2). IDs settle equal periods regardless of input order.
- One task T=D=2,C=3,H=6: releases 0,2,4, completions 3,6,9. All miss once, backlog at 6 is one, drain is three ticks. Response sum is 12.

The job rows themselves give the execution intervals in this completion-yield model. Full batches do not emit per-tick traces.
