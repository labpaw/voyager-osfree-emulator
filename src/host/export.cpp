#include "export.hpp"
#include "io.hpp"
#include <cmath>
#include <iomanip>
namespace host {
void export_schedule(const emu::Scheduler& s, const std::filesystem::path& out, const std::string& id, bool detailed) {
    const auto& m = s.metrics;
    double utilisation = 0;
    for (std::size_t i = 0; i < s.config.count; ++i) {
        const auto& t = s.config.tasks[i];
        if (t.period) utilisation += static_cast<double>(t.cost) / static_cast<double>(t.period);
    }
    const double n = static_cast<double>(s.config.count);
    auto runs = output((out / "runs.csv").string());
    runs << std::setprecision(17);
    runs << "run_id,scheduler,memory_policy,budget,horizon,status,released,completed,on_time,missed,busy,idle,backlog,peak_jobs,drain_ticks,drain_busy,unfinished,max_response,miss_rate,cpu_utilisation,actual_utilisation,ll_reference,dispatches,scans,required_static_bytes,reserved_payload,padding,accesses,detailed_jobs\n";
    runs << id << ",cooperative_rm,static_reservation," << s.config.budget << ',' << s.config.horizon << ',' << emu::status_name(s.status) << ',' << m.released << ',' << m.completed << ',';
    if (s.status == emu::Status::complete) runs << m.released-m.missed; else runs << "NA";
    runs << ',' << m.missed << ',' << m.busy << ',' << m.idle << ',' << m.backlog << ',' << m.peak_jobs << ',' << (m.end > s.config.horizon ? m.end-s.config.horizon : 0) << ',' << m.drain_busy << ',' << m.unfinished << ',';
    if (m.completed) runs << m.max_response; else runs << "NA";
    runs << ',';
    if (s.status == emu::Status::complete && m.released) runs << static_cast<double>(m.missed)/static_cast<double>(m.released); else runs << "NA";
    runs << ',';
    if (s.status == emu::Status::complete) runs << static_cast<double>(m.busy)/static_cast<double>(s.config.horizon); else runs << "NA";
    runs << ',' << utilisation << ',' << n*(std::pow(2.0,1.0/n)-1.0) << ',' << m.dispatches << ',' << m.scans << ',' << s.memory.reserved << ',' << s.memory.payload << ',' << s.memory.reserved-s.memory.payload << ',' << s.memory.lookups << ',' << detailed << '\n';
    runs.close();
    auto tasks = output((out / "task_metrics.csv").string());
    tasks << std::setprecision(17);
    tasks << "run_id,task_id,released,completed,missed,response_sum,waiting_sum,max_response,max_waiting,mean_response,mean_waiting\n";
    auto inputs = output((out / "tasks.csv").string());
    inputs << "task_id,priority,period,deadline,cost,offset,reserved_bytes\n";
    for (std::size_t i = 0; i < s.config.count; ++i) {
        const auto& t = s.config.tasks[i]; const auto& tm = s.task_metrics[i];
        inputs << t.id << ',' << t.priority << ',' << t.period << ',' << t.deadline << ',' << t.cost << ',' << t.offset << ',' << t.memory << '\n';
        tasks << id << ',' << t.id << ',' << tm.released << ',' << tm.completed << ',' << tm.missed << ',' << tm.response_sum << ',' << tm.wait_sum << ',';
        if (tm.completed) tasks << tm.max_response << ',' << tm.max_wait << ',' << static_cast<double>(tm.response_sum)/static_cast<double>(tm.completed) << ',' << static_cast<double>(tm.wait_sum)/static_cast<double>(tm.completed);
        else tasks << "NA,NA,NA,NA";
        tasks << '\n';
    }
    tasks.close(); inputs.close();
}
}
