#pragma once
#include "emulator/memory.hpp"
namespace emu {
enum class MemoryPolicy { static_reservation, bounded_first_fit };
struct MemoryEvent {
    Tick time = 0, request = 0;
    std::size_t owner = 0, bytes = 0;
    bool allocate = true;
};
struct MemorySnapshot {
    const char* outcome = "ok";
    std::size_t demand = 0, live = 0, occupied = 0, free = 0, largest = 0;
    std::size_t unused = 0, padding = 0;
};
struct MemoryMetrics {
    Tick events = 0, requests = 0, successes = 0, failures = 0, skipped_frees = 0;
    Tick fragmentation_failures = 0, capacity_failures = 0, descriptor_failures = 0;
    std::size_t peak_demand = 0, peak_live = 0, peak_occupied = 0, peak_unused = 0, peak_padding = 0;
};
class Replay {
public:
    Status init(MemoryPolicy policy, const std::array<std::size_t, max_tasks>& maxima, std::size_t owners, std::size_t budget);
    MemorySnapshot apply(const MemoryEvent& event);
    bool finish();
    MemorySnapshot snapshot() const;
    Status status = Status::invalid_input;
    MemoryMetrics metrics{};
    StaticArena fixed;
    DynamicArena dynamic;
private:
    struct Request { Tick id = 0; std::size_t bytes = 0, handle = 0; bool pending = false, successful = false; };
    std::array<Request, max_tasks> requests{};
    std::array<std::size_t, max_tasks> maxima{};
    std::size_t owners = 0, budget = 0;
    MemoryPolicy policy = MemoryPolicy::static_reservation;
    Tick last_time = 0;
};
}
