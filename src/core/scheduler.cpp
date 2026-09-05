#include "emulator/scheduler.hpp"
#include <algorithm>
namespace emu {
Status Scheduler::init(const Config& input) {
    config = input;
    metrics = {}; task_metrics = {}; sequence = {}; jobs = {};
    count = 0; running = max_jobs; now = 0;
    Tick work = 0, admitted = 0;
    status = validate(config, work, admitted);
    if (status != Status::ready) return status;
    stop_bound = config.horizon + work;
    assign_priorities(config);
    std::array<std::size_t, max_tasks> maxima{};
    for (std::size_t i = 0; i < config.count; ++i) maxima[i] = config.tasks[i].memory;
    if (!memory.init(maxima, config.count, config.budget)) return status = Status::memory_rejected;
    for (std::size_t i = 0; i < config.count; ++i) {
        next_release[i] = config.tasks[i].offset;
        has_release[i] = next_release[i] < config.horizon;
    }
    return status;
}
Completion Scheduler::step() {
    Completion result;
    if (status != Status::ready) return result;
    if (running != max_jobs && jobs[running].remaining == 0) {
        auto& job = jobs[running];
        job.completion = now;
        result = {true, job};
        auto& tm = task_metrics[job.task];
        ++metrics.completed; ++tm.completed;
        const Tick response = now - job.release;
        const Tick wait = job.first_start - job.release;
        tm.response_sum += response; tm.wait_sum += wait;
        tm.max_response = std::max(tm.max_response, response);
        tm.max_wait = std::max(tm.max_wait, wait);
        metrics.max_response = std::max(metrics.max_response, response);
        jobs[running] = jobs[--count];
        running = max_jobs;
    }
    for (std::size_t i = 0; i < config.count; ++i) {
        if (!has_release[i] || next_release[i] != now) continue;
        if (count == max_jobs) {
            status = Status::job_capacity; metrics.unfinished = count; metrics.end = now;
            return result;
        }
        const auto& task = config.tasks[i];
        jobs[count++] = {i, sequence[i]++, now, now + task.deadline, task.cost, 0, 0, false, false};
        ++metrics.released; ++task_metrics[i].released;
        metrics.peak_jobs = std::max(metrics.peak_jobs, count);
        // Don't compute the next release beyond the finite admission window.
        has_release[i] = task.period < config.horizon - now;
        if (has_release[i]) next_release[i] = now + task.period;
    }
    for (std::size_t i = 0; i < count; ++i) {
        auto& job = jobs[i];
        if (!job.missed && job.deadline <= now) {
            job.missed = true; ++metrics.missed; ++task_metrics[job.task].missed;
        }
    }
    if (now == config.horizon) metrics.backlog = count;
    if (now >= config.horizon && count == 0) {
        status = Status::complete; metrics.end = now;
        return result;
    }
    if (now >= stop_bound) {
        status = Status::arithmetic_overflow; metrics.unfinished = count; metrics.end = now;
        return result;
    }
    if (running == max_jobs && count) {
        running = 0;
        for (std::size_t i = 0; i < count; ++i) {
            ++metrics.scans;
            const auto& a = jobs[i]; const auto& b = jobs[running];
            const auto pa = config.tasks[a.task].priority, pb = config.tasks[b.task].priority;
            if (pa < pb || (pa == pb && a.sequence < b.sequence)) running = i;
        }
        auto& job = jobs[running];
        job.started = true; job.first_start = now;
        ++metrics.dispatches;
    }
    if (running != max_jobs) {
        --jobs[running].remaining;
        if (now < config.horizon) ++metrics.busy; else ++metrics.drain_busy;
    } else if (now < config.horizon) ++metrics.idle;
    ++now;
    return result;
}
}
