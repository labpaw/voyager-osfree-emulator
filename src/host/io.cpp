#include "io.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>
namespace host {
static std::vector<std::string> split(const std::string& line) {
    // Input schemas have numeric fields only, so quoted fields aren't needed.
    std::vector<std::string> row;
    std::size_t start = 0;
    for (;;) {
        const auto end = line.find(',', start);
        row.push_back(line.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) return row;
        start = end + 1;
    }
}
Rows read_csv(const std::string& path, const std::vector<std::string>& header) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot read " + path);
    std::string line;
    if (!std::getline(file, line)) throw std::runtime_error("empty CSV: " + path);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (split(line) != header) throw std::runtime_error("wrong CSV header: " + path);
    Rows rows;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto row = split(line);
        if (row.size() != header.size()) throw std::runtime_error("wrong CSV width: " + path);
        rows.push_back(row);
    }
    if (file.bad()) throw std::runtime_error("CSV read failed: " + path);
    return rows;
}
emu::Tick number(const std::string& text) {
    emu::Tick result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) throw std::runtime_error("invalid unsigned integer: " + text);
    return result;
}
std::size_t size_number(const std::string& text) {
    const auto n = number(text);
    if (n > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("size overflow");
    return static_cast<std::size_t>(n);
}
emu::Config load_tasks(const std::string& path) {
    const auto rows = read_csv(path, {"task_id","period","deadline","cost","offset","reserved_bytes"});
    emu::Config c;
    if (rows.empty() || rows.size() > emu::max_tasks) throw std::runtime_error("unsupported task count");
    for (const auto& row : rows) {
        const auto id = number(row[0]);
        if (id > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("task ID overflow");
        c.tasks[c.count++] = {static_cast<std::uint32_t>(id), number(row[1]), number(row[2]), number(row[3]), number(row[4]), size_number(row[5]), 0};
    }
    return c;
}
std::ofstream output(const std::string& path) {
    std::ofstream file;
    file.exceptions(std::ios::badbit | std::ios::failbit);
    file.open(path);
    return file;
}
bool valid_id(const std::string& text) {
    if (text.empty()) return false;
    for (char c : text) if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_' && c != '-') return false;
    return true;
}
}
