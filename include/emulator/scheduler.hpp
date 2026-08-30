#pragma once
#include "emulator/model.hpp"
#include "emulator/memory.hpp"
namespace emu {
struct Job {
    std::size_t task = 0;
    Tick sequence = 0, release = 0, deadline = 0, remaining = 0;
    Tick first_start = 0, completion = 0;
    bool started = false, missed = false;
};
struct Completion { bool valid = false; Job job{}; };
struct TaskMetrics {
    Tick released = 0, completed = 0, missed = 0;
    Tick response_sum = 0, wait_sum = 0, max_response = 0, max_wait = 0;
};
struct Metrics {
    Tick released = 0, completed = 0, missed = 0;
    Tick busy = 0, idle = 0, drain_busy = 0, end = 0;
    Tick dispatches = 0, scans = 0, max_response = 0;
    std::size_t backlog = 0, peak_jobs = 0, unfinished = 0;
};
class Scheduler {
public:
    Status init(const Config& input);
    Completion step(); // One boundary and at most one interval; host drains each returned event.
    Config config{};
    Metrics metrics{};
    std::array<TaskMetrics, max_tasks> task_metrics{};
    Status status = Status::invalid_input;
    Tick now = 0;
    StaticArena memory;
private:
    std::array<Job, max_jobs> jobs{};
    std::array<Tick, max_tasks> next_release{}, sequence{};
    std::array<bool, max_tasks> has_release{};
    std::size_t count = 0, running = max_jobs;
    Tick stop_bound = 0;
};
}
