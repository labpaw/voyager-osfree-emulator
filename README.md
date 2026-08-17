# Voyager-inspired OS-free emulator

A small C++17 project for trying cooperative scheduling and fixed memory budgets.
It models a target with one processor and a logical clock, while running on a normal host OS.
The supplied execution costs are inputs, not hardware measurements.

Implementation follows `IRP_Emulator_Codex_Build_Plan.md`. See `docs/decisions.md` for choices made along the way.
