#pragma once
#include "emulator/scheduler.hpp"
#include <filesystem>
#include <string>
namespace host { void export_schedule(const emu::Scheduler& s, const std::filesystem::path& out, const std::string& id, bool detailed); }
