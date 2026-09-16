#include "emulator/model.hpp"
#include "emulator/scheduler.hpp"
#include "emulator/memory.hpp"
#include <cstdlib>
#include <new>
#include "allocation_guard.hpp"
#include <iostream>
#include <limits>
#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)
void validation() {
    emu::Config c; c.count = 1; c.horizon = 20;
    c.tasks[0] = {1, 5, 5, 1, 0, 8, 0};
    emu::Tick w = 0, j = 0;
    CHECK(emu::validate(c, w, j) == emu::Status::ready && w == 4 && j == 4);
    c.tasks[0].period = 0;
    CHECK(emu::validate(c, w, j) == emu::Status::invalid_input);
    c.tasks[0].period = 5; c.count = 2; c.tasks[1] = c.tasks[0];
    CHECK(emu::validate(c, w, j) == emu::Status::invalid_input);
    c.count = 1; c.tasks[0].deadline = std::numeric_limits<emu::Tick>::max();
    CHECK(emu::validate(c, w, j) == emu::Status::arithmetic_overflow);
}
void single_stream() {
    static emu::Scheduler s;
    emu::Config c; c.count = 1; c.horizon = 12;
    c.tasks[0] = {1, 5, 5, 2, 0, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    unsigned events = 0;
    while (s.status == emu::Status::ready) {
        const auto e = s.step();
        if (e.valid) { ++events; CHECK(e.job.completion - e.job.release == 2); }
    }
    CHECK(s.status == emu::Status::complete && events == 3);
    CHECK(s.metrics.busy == 6 && s.metrics.idle == 6 && s.metrics.missed == 0);
}
void backlog_and_drain() {
    static emu::Scheduler s;
    emu::Config c; c.count = 1; c.horizon = 6;
    c.tasks[0] = {1, 2, 2, 3, 0, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    unsigned events = 0;
    while (s.status == emu::Status::ready) {
        auto e = s.step();
        if (e.valid) { CHECK(e.job.sequence == events++); CHECK(e.job.missed); }
    }
    CHECK(s.metrics.released == 3 && s.metrics.completed == 3 && events == 3);
    CHECK(s.metrics.missed == 3 && s.metrics.backlog == 1);
    CHECK(s.metrics.end == 9 && s.metrics.busy == 6 && s.metrics.drain_busy == 3);
    CHECK(s.task_metrics[0].response_sum == 12 && s.metrics.max_response == 5);
}
void boundaries_and_blocking() {
    static emu::Scheduler s;
    emu::Config c; c.count = 1; c.horizon = 1;
    c.tasks[0] = {1, 5, 5, 5, 0, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    while (s.status == emu::Status::ready) s.step();
    CHECK(s.metrics.missed == 0 && s.metrics.end == 5);
    c.tasks[0].cost = 6;
    CHECK(s.init(c) == emu::Status::ready);
    while (s.status == emu::Status::ready) s.step();
    CHECK(s.metrics.missed == 1 && s.metrics.end == 6);
    c.count = 2; c.horizon = 10;
    c.tasks[0] = {1, 5, 5, 1, 1, 0, 0};
    c.tasks[1] = {2, 20, 20, 7, 0, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    unsigned events = 0;
    while (s.status == emu::Status::ready) {
        const auto e = s.step();
        if (!e.valid) continue;
        if (events == 0) CHECK(e.job.task == 1 && e.job.first_start == 0 && e.job.completion == 7);
        if (events == 1) CHECK(e.job.task == 0 && e.job.release == 1 && e.job.first_start == 7 && e.job.completion == 8 && e.job.missed);
        ++events;
    }
    CHECK(events == 3 && s.metrics.missed == 1);
}
void capacity_and_idle() {
    static emu::Scheduler s;
    emu::Config c; c.count = 1; c.horizon = 3;
    c.tasks[0] = {1, 10, 10, 1, 5, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    while (s.status == emu::Status::ready) s.step();
    CHECK(s.metrics.idle == 3 && s.metrics.completed == 0);
    c.horizon = 1030; c.tasks[0] = {1, 1, 1, 2000, 0, 0, 0};
    CHECK(s.init(c) == emu::Status::ready);
    while (s.status == emu::Status::ready) s.step();
    CHECK(s.status == emu::Status::job_capacity && s.metrics.released == emu::max_jobs);
    CHECK(s.metrics.unfinished == emu::max_jobs && s.metrics.completed == 0);
}
void static_regions() {
    static emu::StaticArena arena;
    std::array<std::size_t, emu::max_tasks> sizes{}; sizes[0] = 9; sizes[1] = 8;
    CHECK(arena.init(sizes, 2, 24));
    CHECK(arena.reserved == 24 && arena.payload == 17 && arena.regions[1].offset == 16);
    CHECK(!arena.init(sizes, 2, 23));
    CHECK(!arena.access(0, 0, 1));
    CHECK(arena.init(sizes, 2, 24));
    CHECK(arena.write(0, 8, 42));
    CHECK(!arena.write(0, 9, 99) && !arena.access(2, 0, 1));
    CHECK(!arena.access(0, 8, 2) && !arena.access(0, std::numeric_limits<std::size_t>::max(), 1));
    std::uint8_t value = 0;
    CHECK(arena.read(0, 8, value) && value == 42);
    CHECK(arena.read(1, 0, value) && value == 0);
    static emu::Scheduler sched;
    emu::Config c; c.count = 1; c.horizon = 5; c.budget = 7;
    c.tasks[0] = {1, 5, 5, 1, 0, 8, 0};
    CHECK(sched.init(c) == emu::Status::memory_rejected && sched.metrics.released == 0);
}
void first_fit_split() {
    static emu::DynamicArena arena;
    CHECK(arena.init(32));
    auto a = arena.allocate(9); auto b = arena.allocate(16);
    CHECK(a.reason == emu::Allocation::ok && a.offset == 0);
    CHECK(b.reason == emu::Allocation::ok && b.offset == 16);
    CHECK(arena.occupied() == 32 && arena.free_bytes() == 0 && arena.splits == 1);
    CHECK(arena.allocate(1).reason == emu::Allocation::capacity);
    CHECK(arena.allocate(0).reason == emu::Allocation::invalid_request);
}
void reuse_and_coalesce() {
    static emu::DynamicArena arena;
    CHECK(arena.init(64));
    auto a = arena.allocate(16); auto b = arena.allocate(16); auto c = arena.allocate(16);
    CHECK(arena.release(b.offset) == emu::Allocation::ok);
    CHECK(arena.release(b.offset) == emu::Allocation::invalid_free);
    auto d = arena.allocate(8); CHECK(d.offset == b.offset);
    CHECK(arena.release(7) == emu::Allocation::invalid_free);
    CHECK(arena.release(a.offset) == emu::Allocation::ok);
    CHECK(arena.release(c.offset) == emu::Allocation::ok);
    CHECK(arena.release(d.offset) == emu::Allocation::ok);
    CHECK(arena.free_bytes() == 64 && arena.largest_free() == 64 && arena.coalesces == arena.splits);
    CHECK(arena.init(32) && arena.occupied() == 0 && arena.blocks_inspected == 0);
}
int main() {
    reuse_and_coalesce();
    first_fit_split();
    watch_allocations = true;
    static_regions();
    capacity_and_idle();
    boundaries_and_blocking();
    backlog_and_drain();
    single_stream();
    validation();
    emu::Config ties; ties.count = 3;
    ties.tasks[0] = {8, 10, 10, 1, 0, 0, 0};
    ties.tasks[1] = {2, 10, 10, 1, 0, 0, 0};
    ties.tasks[2] = {9, 5, 5, 1, 0, 0, 0};
    emu::assign_priorities(ties);
    CHECK(ties.tasks[0].priority == 2 && ties.tasks[1].priority == 1 && ties.tasks[2].priority == 0);
    emu::Tick n = 0;
    CHECK(emu::checked_add(2, 3, n) && n == 5);
    CHECK(!emu::checked_add(std::numeric_limits<emu::Tick>::max(), 1, n));
    watch_allocations = false;
    CHECK(watched_allocations == 0);
    watch_allocations = true;
    void* p = ::operator new(8);
    void* q = ::operator new[](16, std::align_val_t(16), std::nothrow);
    watch_allocations = false;
    ::operator delete(p); ::operator delete[](q, std::align_val_t(16));
    CHECK(watched_allocations == 2);
}
