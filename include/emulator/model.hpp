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
