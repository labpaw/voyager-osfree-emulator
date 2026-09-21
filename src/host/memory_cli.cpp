#include "memory_cli.hpp"
#include "io.hpp"
#include "emulator/replay.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
namespace host {
int run_memory(const std::map<std::string,std::string>& args) {
    for (const auto& a : args) if (a.first != "--trace" && a.first != "--policy" && a.first != "--budget" && a.first != "--out" && a.first != "--run-id") throw std::runtime_error("unsupported memory option: " + a.first);
    for (auto key : {"--trace","--policy","--budget","--out"}) if (!args.count(key)) throw std::runtime_error(std::string("missing ")+key);
    const auto& policy_name = args.at("--policy");
    if (policy_name != "static_reservation" && policy_name != "bounded_first_fit") throw std::runtime_error("unsupported memory policy");
    const auto policy = policy_name == "static_reservation" ? emu::MemoryPolicy::static_reservation : emu::MemoryPolicy::bounded_first_fit;
    const std::string id = args.count("--run-id") ? args.at("--run-id") : "single";
    if (!valid_id(id)) throw std::runtime_error("invalid run ID");
    const auto input = read_csv(args.at("--trace"), {"event_index","time","owner","request_id","operation","bytes","owner_maximum"});
    if (input.empty() || input.size() > 1000000) throw std::runtime_error("unsupported trace size");
    std::array<std::size_t,emu::max_tasks> maxima{};
    std::array<emu::Tick,emu::max_tasks> pending{};
    std::array<bool,emu::max_tasks> active{};
    std::set<emu::Tick> used_ids;
    std::vector<emu::MemoryEvent> events;
    std::size_t owners = 0;
    emu::Tick time = 0;
    for (const auto& row : input) {
        emu::MemoryEvent e;
        if (number(row[0]) != events.size()) throw std::runtime_error("nonsequential event index");
        e.time = number(row[1]); e.owner = size_number(row[2]); e.request = number(row[3]);
        e.bytes = size_number(row[5]); const auto maximum = size_number(row[6]);
        if (e.time < time || e.owner >= emu::max_tasks || !maximum || maximum > emu::arena_capacity) throw std::runtime_error("invalid trace contract");
        time = e.time;
        owners = std::max(owners,e.owner+1);
        if (maxima[e.owner] && maxima[e.owner] != maximum) throw std::runtime_error("owner maximum changed");
        maxima[e.owner] = maximum;
        if (row[4] == "allocate") {
            e.allocate = true;
            if (active[e.owner] || !e.bytes || e.bytes > maximum || !used_ids.insert(e.request).second) throw std::runtime_error("invalid allocation lifetime");
            pending[e.owner] = e.request; active[e.owner] = true;
        } else if (row[4] == "free") {
            e.allocate = false;
            if (!active[e.owner] || pending[e.owner] != e.request || e.bytes) throw std::runtime_error("invalid free lifetime");
            active[e.owner] = false;
        } else throw std::runtime_error("unknown trace operation");
        events.push_back(e);
    }
    for (std::size_t i = 0; i < owners; ++i) if (!maxima[i] || active[i]) throw std::runtime_error("missing owner or unfinished trace");
    static emu::Replay core;
    const auto budget = size_number(args.at("--budget"));
    core.init(policy,maxima,owners,budget);
    if (core.status == emu::Status::invalid_input) throw std::runtime_error("invalid memory configuration");
    const auto out = std::filesystem::path(args.at("--out"));
    if (std::filesystem::exists(out)) throw std::runtime_error("output directory already exists");
    std::filesystem::create_directories(out);
    std::filesystem::copy_file(args.at("--trace"),out / "memory_trace.csv");
    auto csv = output((out / "memory_events.csv").string());
    csv << std::setprecision(17);
    csv << "run_id,event_index,time,owner,request_id,operation,bytes,outcome,demand,live,occupied,free,largest_free,unused_reserved,padding,external_fragmentation,blocks_inspected,splits,coalesces,lookups\n";
    for (std::size_t i = 0; i < events.size() && core.status == emu::Status::ready; ++i) {
        const auto& e = events[i]; const auto s = core.apply(e);
        csv << id << ',' << i << ',' << e.time << ',' << e.owner << ',' << e.request << ',' << (e.allocate ? "allocate" : "free") << ',' << e.bytes << ',' << s.outcome << ',' << s.demand << ',' << s.live << ',' << s.occupied << ',' << s.free << ',';
        if (policy == emu::MemoryPolicy::bounded_first_fit) csv << s.largest << ",NA," << s.padding << ',';
        else csv << "NA," << s.unused << ',' << s.padding << ',';
        if (policy == emu::MemoryPolicy::bounded_first_fit && s.free) csv << 1.0-static_cast<double>(s.largest)/static_cast<double>(s.free);
        else csv << "NA";
        csv << ',' << (policy == emu::MemoryPolicy::bounded_first_fit ? core.dynamic.blocks_inspected : 0) << ',' << (policy == emu::MemoryPolicy::bounded_first_fit ? core.dynamic.splits : 0) << ',' << (policy == emu::MemoryPolicy::bounded_first_fit ? core.dynamic.coalesces : 0) << ',' << (policy == emu::MemoryPolicy::static_reservation ? core.fixed.lookups : 0) << '\n';
    }
    csv.close();
    if (core.status == emu::Status::ready) core.finish();
    const auto& m = core.metrics;
    auto summary = output((out / "memory_runs.csv").string());
    summary << "run_id,policy,budget,status,events,requests,successes,failures,skipped_frees,fragmentation_failures,capacity_failures,descriptor_failures,peak_demand,peak_live,peak_occupied,peak_unused_reserved,peak_padding,policy_metadata_bytes,replay_storage_bytes\n";
    summary << id << ',' << policy_name << ',' << budget << ',' << emu::status_name(core.status) << ',' << m.events << ',' << m.requests << ',' << m.successes << ',' << m.failures << ',' << m.skipped_frees << ',' << m.fragmentation_failures << ',' << m.capacity_failures << ',' << m.descriptor_failures << ',';
    if (core.status == emu::Status::memory_rejected) summary << "NA,NA,NA,NA,NA,";
    else summary << m.peak_demand << ',' << m.peak_live << ',' << m.peak_occupied << ',' << (policy == emu::MemoryPolicy::static_reservation ? std::to_string(m.peak_unused) : "NA") << ',' << m.peak_padding << ',';
    summary << (policy == emu::MemoryPolicy::static_reservation ? sizeof(emu::StaticArena) : sizeof(emu::DynamicArena))-emu::arena_capacity << ',' << sizeof(emu::Replay) << '\n';
    summary.close();
    std::cout << id << ": " << emu::status_name(core.status) << ", failed requests=" << m.failures << '\n';
    if (core.status == emu::Status::memory_rejected) return 3;
    return core.status == emu::Status::complete ? 0 : 4;
}
}
