#include "emulator/model.hpp"
#include <limits>
namespace emu {
bool checked_add(Tick a, Tick b, Tick& result) {
    if (b > std::numeric_limits<Tick>::max() - a) return false;
    result = a + b;
    return true;
}
}
namespace emu {
const char* status_name(Status s) {
    switch (s) {
    case Status::ready: return "ready";
    case Status::complete: return "complete";
    case Status::invalid_input: return "invalid_input";
    case Status::job_capacity: return "job_capacity";
    case Status::arithmetic_overflow: return "arithmetic_overflow";
    case Status::memory_rejected: return "memory_rejected";
    case Status::access_error: return "access_error";
    }
    return "unknown";
}
Status validate(const Config& c, Tick& work, Tick& jobs) {
    work = jobs = 0;
    if (!c.count || c.count > max_tasks || !c.horizon || c.budget > arena_capacity) return Status::invalid_input;
    const Tick limit = std::numeric_limits<Tick>::max();
    for (std::size_t i = 0; i < c.count; ++i) {
        const auto& t = c.tasks[i];
        if (!t.period || !t.deadline || !t.cost || t.offset >= t.period || t.memory > arena_capacity) return Status::invalid_input;
        for (std::size_t j = 0; j < i; ++j) if (c.tasks[j].id == t.id) return Status::invalid_input;
        if (t.offset >= c.horizon) continue;
        const Tick count = 1 + (c.horizon - 1 - t.offset) / t.period;
        const Tick last = t.offset + (count - 1) * t.period;
        Tick unused = 0;
        if (!checked_add(last, t.deadline, unused) || count > limit / t.cost ||
            !checked_add(work, count * t.cost, work) || !checked_add(jobs, count, jobs)) return Status::arithmetic_overflow;
    }
    Tick end = 0;
    if (!checked_add(c.horizon, work, end) || (jobs && end > limit / jobs) ||
        (work && work > limit / max_jobs)) return Status::arithmetic_overflow;
    return Status::ready;
}
}
