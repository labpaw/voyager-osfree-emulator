#include "io.hpp"
#include "export.hpp"
#include "memory_cli.hpp"
#include "info.hpp"
#include "emulator/scheduler.hpp"
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "info") { host::print_info(); return 0; }
        if (argc < 2 || (std::string(argv[1]) != "schedule" && std::string(argv[1]) != "memory")) throw std::runtime_error("usage: emulator schedule --tasks FILE --horizon T --budget B --out DIR [--run-id ID] [--jobs 0|1]");
        std::map<std::string, std::string> args;
        for (int i = 2; i < argc; i += 2) {
            if (i + 1 == argc) throw std::runtime_error("option needs value");
            const std::string key = argv[i];
            if (key != "--tasks" && key != "--horizon" && key != "--budget" && key != "--out" && key != "--run-id" && key != "--jobs" && key != "--trace" && key != "--policy") throw std::runtime_error("unknown option: " + key);
            if (!args.emplace(key, argv[i+1]).second) throw std::runtime_error("duplicate option");
        }
        if (std::string(argv[1]) == "memory") return host::run_memory(args);
        if (args.count("--trace") || args.count("--policy")) throw std::runtime_error("unsupported schedule option");
        for (auto key : {"--tasks","--horizon","--budget","--out"}) if (!args.count(key)) throw std::runtime_error(std::string("missing ") + key);
        const std::string id = args.count("--run-id") ? args.at("--run-id") : "single";
        if (!host::valid_id(id)) throw std::runtime_error("run ID must contain letters, digits, underscores or hyphens");
        bool detailed = true;
        if (args.count("--jobs")) {
            if (args.at("--jobs") != "0" && args.at("--jobs") != "1") throw std::runtime_error("--jobs expects 0 or 1");
            detailed = args.at("--jobs") == "1";
        }
        auto config = host::load_tasks(args.at("--tasks"));
        config.horizon = host::number(args.at("--horizon")); config.budget = host::size_number(args.at("--budget"));
        static emu::Scheduler core;
        core.init(config);
        const auto out = std::filesystem::path(args.at("--out"));
        if (std::filesystem::exists(out)) throw std::runtime_error("output directory already exists");
        std::filesystem::create_directories(out);
        auto jobs = host::output((out / "jobs.csv").string());
        jobs << "run_id,task_id,sequence,release,deadline,first_start,completion,response,waiting,tardiness,missed\n";
        while (core.status == emu::Status::ready) {
            const auto event = core.step();
            if (event.valid && detailed) {
                const auto& j = event.job;
                jobs << id << ',' << core.config.tasks[j.task].id << ',' << j.sequence << ',' << j.release << ',' << j.deadline << ',' << j.first_start << ',' << j.completion << ',' << j.completion-j.release << ',' << j.first_start-j.release << ',' << (j.completion > j.deadline ? j.completion-j.deadline : 0) << ',' << j.missed << '\n';
            }
        }
        jobs.close();
        host::export_schedule(core, out, id, detailed);
        std::cout << id << ": " << emu::status_name(core.status) << ", completed=" << core.metrics.completed << ", missed=" << core.metrics.missed << '\n';
        if (core.status == emu::Status::memory_rejected) return 3;
        return core.status == emu::Status::complete ? 0 : 4;
    } catch (const std::ios_base::failure& e) { std::cerr << e.what() << '\n'; return 5;
    } catch (const std::filesystem::filesystem_error& e) { std::cerr << e.what() << '\n'; return 5;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
