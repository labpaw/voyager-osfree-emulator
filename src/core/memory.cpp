#include "emulator/memory.hpp"
namespace emu {
bool StaticArena::init(const std::array<std::size_t, max_tasks>& maxima, std::size_t owners, std::size_t budget) {
    reserved = payload = lookups = count = 0; admitted = false; regions = {}; data = {};
    if (owners > max_tasks || budget > arena_capacity) return false;
    for (std::size_t i = 0; i < owners; ++i) {
        if (maxima[i] > arena_capacity) return false;
        const auto bytes = aligned(maxima[i]);
        regions[i] = {reserved, maxima[i]};
        reserved += bytes; payload += maxima[i];
    }
    if (reserved > budget) return false;
    count = owners; admitted = true;
    return true;
}
bool StaticArena::access(std::size_t owner, std::size_t offset, std::size_t bytes) const {
    if (!admitted || owner >= count) return false;
    return offset <= regions[owner].capacity && bytes <= regions[owner].capacity - offset;
}
bool StaticArena::write(std::size_t owner, std::size_t offset, std::uint8_t value) {
    ++lookups;
    if (!access(owner, offset, 1)) return false;
    data[regions[owner].offset + offset] = value;
    return true;
}
bool StaticArena::read(std::size_t owner, std::size_t offset, std::uint8_t& value) const {
    if (!access(owner, offset, 1)) return false;
    value = data[regions[owner].offset + offset];
    return true;
}
}
