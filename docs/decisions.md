# Decisions

- Implementation started on 19 October 2026. Git dates from 17 August to 19 October are assigned retrospective milestone dates requested by the project owner, not a contemporaneous record of work.
- The build plan is the only supplied reference. No separate project guide or bibliography was available locally. It is kept unchanged.
- Keep the core separate from parsing, files, and experiment scripts. Use a tick loop and linear scans first.
- Development compiler: Apple Clang 21.0.0 on arm64 macOS. GCC was not found in the initial environment. CMake and Matplotlib use a local Python virtual environment; system packages are unchanged.
