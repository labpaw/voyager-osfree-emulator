# Artifact inventory

The primary evidence is `results/full`. It was collected from source revision `845f9bffb7862b0cbe68999319b5deca78df8b6c` (capacity freeze), using the normal Release binary identified in its manifest. Later commits add summaries, figures, documentation, packaging and reproduction checks. Raw data have not been edited.

| Location | Contents |
| --- | --- |
| `IRP_Emulator_Codex_Build_Plan.md` | Original supplied plan, unchanged |
| `CMakeLists.txt`, `include/emulator`, `src/core` | C++17 models, checked task setup, scheduler, static and first-fit arenas, trace replay |
| `src/host` | Numeric CSV parser, CLI, streaming exports and build/storage information |
| `tests` | Hand-worked scheduling/memory checks, allocation guard, CLI validation, deterministic replay, analysis exclusions |
| `configs/fixtures`, `configs/generated`, `configs/tasksets.csv` | Small cases and all saved scheduling inputs |
| `configs/traces` | 22 shared demand traces with exact operation order and declared maxima |
| `configs/experiments.json`, `configs/frozen.json` | 525-cell grid, capacities and frozen input hashes |
| `scripts` | Input preparation, batches, accounting, analysis, figures, allocation/stack audits, fresh smoke reproduction and archive packaging |
| `docs` | Decisions, design, method, timelines, findings, schemas, limitations and verification evidence |
| `results/full/manifest.json`, `tasksets.csv` | Full batch provenance, all cell statuses and E1 input catalogue |
| `results/full/e0`, `e1`, `e2`, `e3`, `e4`, `sensitivity` | 1,836 raw CSV files for 525 outcomes |
| `results/full/analysis` | Seven derived files: flat scheduling/memory rows, status counts, three summary CSVs and analysis provenance |
| `results/full/plots` | Four PNG figures and plotting provenance |

Full cell accounting:

| Group | Planned | Complete | Static admission rejection | Incomplete |
| --- | ---: | ---: | ---: | ---: |
| E0 validation | 3 | 3 | 0 | 0 |
| E1 utilisation | 240 | 240 | 0 | 0 |
| E2 blocking | 8 | 8 | 0 | 0 |
| E3 memory, including manual fixture | 264 | 181 | 83 | 0 |
| E4 integrated | 4 | 2 | 2 | 0 |
| Sensitivity | 6 | 6 | 0 | 0 |
| Total | 525 | 440 | 85 | 0 |

The raw files include 1,163,718 admitted/completed scheduling job records. Memory replays can be complete while containing allocation failures; inspect successes/failures and their reasons rather than equating complete with every request succeeding.

The repository also retains `results/e0-blocking`, the first pre-integration scheduling export, and `results/smoke`, the earlier validated 62-cell subset with a sufficient 1,024-job build. These historical results are not additional independent evidence and are excluded from the final archive to avoid duplicating the full batch. Full evidence and the final executable use 4,096 jobs. `docs/verification/fresh-smoke.json` records reproduction with that final capacity: 62 runs and 216 identical raw files matched the corresponding full-batch cells.

The archive builder includes the paths above and adds `ARTIFACT_MANIFEST.json`, containing every file's size/hash and the packaging source revision/dirty state. It verifies all archive members after creation. ZIP entry timestamps are normalised to the ZIP epoch for reproducibility; they are not measurement dates. The archive excludes `.git`, build products, `.venv`, Python caches, local prototype batches and duplicate saved smoke evidence. Rebuild from the included source using README commands; no precompiled binary is required.

All four CTest suites passed on normal and ASan/UBSan builds. The source/symbol allocation audit passed, the C++ new guard saw no allocations during covered core tests, and the overload preflight had maximum concurrency 1,038/4,096. Per-function stack estimates and sizeof reports are preserved with their coverage limitations. The supplied plan and any external reference files were not modified.
