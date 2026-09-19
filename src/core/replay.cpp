#include "emulator/replay.hpp"
#include <algorithm>
namespace emu {
Status Replay::init(MemoryPolicy p, const std::array<std::size_t,max_tasks>& limits, std::size_t n, std::size_t bytes) {
    policy = p; maxima = limits; owners = n; budget = bytes;
    requests = {}; metrics = {}; last_time = 0;
    status = Status::invalid_input;
    if (!n || n > max_tasks || bytes > arena_capacity) return status;
    for (std::size_t i = 0; i < n; ++i) if (!limits[i] || limits[i] > arena_capacity) return status;
    if (p == MemoryPolicy::static_reservation) {
        if (!fixed.init(limits,n,bytes)) return status = Status::memory_rejected;
    } else if (!dynamic.init(bytes)) return status;
    status = Status::ready;
    const auto initial = snapshot();
    metrics.peak_occupied = initial.occupied; metrics.peak_unused = initial.unused; metrics.peak_padding = initial.padding;
    return status;
}
MemorySnapshot Replay::snapshot() const {
    MemorySnapshot s;
    std::size_t allocated_payload = 0;
    for (std::size_t i = 0; i < owners; ++i) {
        if (requests[i].pending) s.demand += requests[i].bytes;
        if (requests[i].successful) s.live += requests[i].bytes;
        allocated_payload += maxima[i];
    }
    if (policy == MemoryPolicy::static_reservation) {
        s.occupied = fixed.reserved; s.free = budget-s.occupied;
        s.unused = allocated_payload-s.live; s.padding = fixed.reserved-allocated_payload;
        s.largest = 0; // Not a reusable heap; exported as NA.
    } else {
        s.occupied = dynamic.occupied(); s.free = dynamic.free_bytes(); s.largest = dynamic.largest_free();
        s.padding = s.occupied-s.live;
    }
    return s;
}
MemorySnapshot Replay::apply(const MemoryEvent& event) {
    if (status != Status::ready) return {};
    if (event.owner >= owners || event.time < last_time) { status = Status::invalid_input; return {}; }
    last_time = event.time;
    auto& r = requests[event.owner];
    const char* outcome = "ok";
    if (event.allocate) {
        if (r.pending || !event.bytes || event.bytes > maxima[event.owner]) { status = Status::invalid_input; return {}; }
        ++metrics.requests;
        AllocationResult result{Allocation::ok, fixed.regions[event.owner].offset};
        if (policy == MemoryPolicy::bounded_first_fit) result = dynamic.allocate(event.bytes);
        else ++fixed.lookups;
        r = {event.request, event.bytes, result.offset, true, result.reason == Allocation::ok};
        if (r.successful) ++metrics.successes;
        else {
            ++metrics.failures;
            if (result.reason == Allocation::fragmentation) ++metrics.fragmentation_failures;
            if (result.reason == Allocation::capacity) ++metrics.capacity_failures;
            if (result.reason == Allocation::descriptors) ++metrics.descriptor_failures;
        }
        outcome = allocation_name(result.reason);
    } else {
        if (!r.pending || r.id != event.request || event.bytes != 0) { status = Status::invalid_input; return {}; }
        if (!r.successful) { ++metrics.skipped_frees; outcome = "skipped_free"; }
        else if (policy == MemoryPolicy::bounded_first_fit && dynamic.release(r.handle) != Allocation::ok) {
            status = Status::invalid_input; return {};
        }
        r = {};
    }
    ++metrics.events;
    auto s = snapshot(); s.outcome = outcome;
    metrics.peak_demand = std::max(metrics.peak_demand,s.demand);
    metrics.peak_live = std::max(metrics.peak_live,s.live);
    metrics.peak_occupied = std::max(metrics.peak_occupied,s.occupied);
    metrics.peak_unused = std::max(metrics.peak_unused,s.unused);
    metrics.peak_padding = std::max(metrics.peak_padding,s.padding);
    return s;
}
bool Replay::finish() {
    if (status != Status::ready) return false;
    for (std::size_t i = 0; i < owners; ++i) if (requests[i].pending) { status = Status::invalid_input; return false; }
    status = Status::complete; return true;
}
}
