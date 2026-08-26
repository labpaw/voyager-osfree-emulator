#include "emulator/model.hpp"
#include "emulator/scheduler.hpp"
#include <cstdlib>
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
int main() {
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
}
