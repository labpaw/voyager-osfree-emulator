#pragma once
#include "emulator/model.hpp"
namespace emu {
constexpr std::size_t aligned(std::size_t n) { return (n + alignment - 1) / alignment * alignment; }
struct Region { std::size_t offset = 0, capacity = 0; };
class StaticArena {
public:
    bool init(const std::array<std::size_t, max_tasks>& maxima, std::size_t owners, std::size_t budget);
    bool access(std::size_t owner, std::size_t offset, std::size_t bytes) const;
    bool write(std::size_t owner, std::size_t offset, std::uint8_t value);
    bool read(std::size_t owner, std::size_t offset, std::uint8_t& value) const;
    std::size_t reserved = 0, payload = 0, lookups = 0;
    std::array<Region, max_tasks> regions{};
private:
    alignas(alignment) std::array<std::uint8_t, arena_capacity> data{};
    std::size_t count = 0;
    bool admitted = false;
};
}
