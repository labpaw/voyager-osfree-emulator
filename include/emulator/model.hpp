#pragma once
#include <cstddef>
#include <cstdint>

namespace emu {
using Tick = std::uint64_t;
constexpr std::size_t max_tasks = 16;
constexpr std::size_t max_jobs = 1024;
constexpr std::size_t arena_capacity = 65536;
constexpr std::size_t alignment = 8;
bool checked_add(Tick a, Tick b, Tick& result);
}

#include <array>
namespace emu {
struct Task {
    std::uint32_t id = 0;
    Tick period = 0, deadline = 0, cost = 0, offset = 0;
    std::size_t memory = 0, priority = 0;
};
struct Config {
    std::array<Task, max_tasks> tasks{};
    std::size_t count = 0;
    Tick horizon = 0;
    std::size_t budget = 4096;
};
enum class Status { ready, complete, invalid_input, job_capacity, arithmetic_overflow, memory_rejected, access_error };
const char* status_name(Status status);
Status validate(const Config& config, Tick& work_bound, Tick& jobs_bound);
}
