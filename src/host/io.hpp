#pragma once
#include "emulator/model.hpp"
#include <fstream>
#include <string>
#include <vector>
namespace host {
using Rows = std::vector<std::vector<std::string>>;
Rows read_csv(const std::string& path, const std::vector<std::string>& header);
emu::Tick number(const std::string& text);
std::size_t size_number(const std::string& text);
emu::Config load_tasks(const std::string& path);
std::ofstream output(const std::string& path);
bool valid_id(const std::string& text);
}
