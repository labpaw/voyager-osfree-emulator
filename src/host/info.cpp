#include "info.hpp"
#include "build_info.hpp"
#include "emulator/scheduler.hpp"
#include "emulator/replay.hpp"
#include <iostream>
#include <iomanip>
namespace host {
void print_info() {
    std::cout << "{\"schema_version\":1,\"compiler_id\":" << std::quoted(IRP_COMPILER_ID)
              << ",\"compiler_version\":" << std::quoted(IRP_COMPILER_VERSION)
              << ",\"compiler_path\":" << std::quoted(IRP_COMPILER_PATH)
              << ",\"build_type\":" << std::quoted(IRP_BUILD_TYPE)
              << ",\"flags\":" << std::quoted(IRP_BUILD_FLAGS)
              << ",\"sanitizers\":" << std::quoted(IRP_SANITIZE)
              << ",\"tasks\":" << emu::max_tasks << ",\"jobs\":" << emu::max_jobs
              << ",\"blocks\":" << emu::max_blocks << ",\"arena_bytes\":" << emu::arena_capacity
              << ",\"alignment\":" << emu::alignment
              << ",\"scheduler_storage_bytes\":" << sizeof(emu::Scheduler)
              << ",\"scheduler_nonarena_bytes\":" << sizeof(emu::Scheduler)-emu::arena_capacity
              << ",\"replay_storage_bytes\":" << sizeof(emu::Replay)
              << ",\"replay_nonarena_bytes\":" << sizeof(emu::Replay)-2*emu::arena_capacity
              << ",\"static_metadata_bytes\":" << sizeof(emu::StaticArena)-emu::arena_capacity
              << ",\"dynamic_metadata_bytes\":" << sizeof(emu::DynamicArena)-emu::arena_capacity << "}\n";
}
}
