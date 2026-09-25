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
    bool read(std::size_t owner, std::size_t offset, std::uint8_t& value);
    std::size_t reserved = 0, payload = 0, lookups = 0;
    std::array<Region, max_tasks> regions{};
private:
    alignas(alignment) std::array<std::uint8_t, arena_capacity> data{};
    std::size_t count = 0;
    bool admitted = false;
};
}
namespace emu {
constexpr std::size_t max_blocks = 32;
enum class Allocation { ok, invalid_request, capacity, fragmentation, descriptors, invalid_free };
const char* allocation_name(Allocation reason);
struct Block { std::size_t offset = 0, bytes = 0; bool used = false; };
struct AllocationResult { Allocation reason = Allocation::invalid_request; std::size_t offset = 0; };
class DynamicArena {
public:
    bool init(std::size_t budget);
    AllocationResult allocate(std::size_t bytes);
    Allocation release(std::size_t offset);
    std::size_t free_bytes() const;
    std::size_t largest_free() const;
    std::size_t occupied() const { return budget - free_bytes(); }
    std::size_t blocks_inspected = 0, splits = 0, coalesces = 0;
private:
    alignas(alignment) std::array<std::uint8_t, arena_capacity> data{};
    std::array<Block, max_blocks> blocks{};
    std::size_t count = 0, budget = 0;
};
}
