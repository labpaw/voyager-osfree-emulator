# Decisions

- Implementation started on 19 October 2026. Git dates from 17 August to 19 October are assigned retrospective milestone dates requested by the project owner, not a contemporaneous record of work.
- The build plan is the only supplied reference. No separate project guide or bibliography was available locally. It is kept unchanged.
- Keep the core separate from parsing, files, and experiment scripts. Use a tick loop and linear scans first.
- Development compiler: Apple Clang 21.0.0 on arm64 macOS. GCC was not found in the initial environment. CMake and Matplotlib use a local Python virtual environment; system packages are unchanged.

- Capacity preflight at U=1.10 found `e1_n5_s5_u110` exhausted 1,024 active jobs. Increased the fixed job array to 4,096 and reran all 30 overload cells plus six sensitivity cases before freezing. The earlier saved smoke batch used 1,024 and was valid for all its selected inputs. Full evidence uses 4,096. This changes metadata storage, not overload/release semantics.

- Cleanup retains one full evidence batch and one deliverable archive. Duplicate exploratory/smoke outputs, local trial batches, compiler builds, the rebuildable virtual environment and preview exports were removed. Historical source/result commits are preserved.
