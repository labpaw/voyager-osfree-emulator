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
#include <algorithm>
namespace emu {
const char* allocation_name(Allocation r) {
    switch (r) {
    case Allocation::ok: return "ok";
    case Allocation::invalid_request: return "invalid_request";
    case Allocation::capacity: return "capacity";
    case Allocation::fragmentation: return "fragmentation";
    case Allocation::descriptors: return "descriptors";
    case Allocation::invalid_free: return "invalid_free";
    }
    return "unknown";
}
bool DynamicArena::init(std::size_t bytes) {
    data = {}; blocks = {}; count = 0; budget = 0;
    blocks_inspected = splits = coalesces = 0;
    if (bytes > arena_capacity) return false;
    budget = bytes;
    if (bytes) { blocks[0] = {0, bytes, false}; count = 1; }
    return true;
}
std::size_t DynamicArena::free_bytes() const {
    std::size_t total = 0;
    for (std::size_t i = 0; i < count; ++i) if (!blocks[i].used) total += blocks[i].bytes;
    return total;
}
std::size_t DynamicArena::largest_free() const {
    std::size_t largest = 0;
    for (std::size_t i = 0; i < count; ++i) if (!blocks[i].used) largest = std::max(largest, blocks[i].bytes);
    return largest;
}
AllocationResult DynamicArena::allocate(std::size_t request) {
    if (!request || request > arena_capacity) return {Allocation::invalid_request, 0};
    const auto bytes = aligned(request);
    for (std::size_t i = 0; i < count; ++i) {
        ++blocks_inspected;
        const auto block = blocks[i];
        if (block.used || block.bytes < bytes) continue;
        if (block.bytes > bytes) {
            if (count == max_blocks) return {Allocation::descriptors, 0};
            for (std::size_t j = count; j > i+1; --j) blocks[j] = blocks[j-1];
            blocks[i+1] = {block.offset+bytes, block.bytes-bytes, false};
            ++count; ++splits;
        }
        blocks[i] = {block.offset, bytes, true};
        return {Allocation::ok, block.offset};
    }
    return {free_bytes() >= bytes ? Allocation::fragmentation : Allocation::capacity, 0};
}
}
namespace emu {
Allocation DynamicArena::release(std::size_t offset) {
    for (std::size_t i = 0; i < count; ++i) {
        ++blocks_inspected;
        if (blocks[i].offset != offset || !blocks[i].used) continue;
        blocks[i].used = false;
        // Merge all adjacent holes, including either neighbour of this block.
        for (std::size_t j = 0; j+1 < count;) {
            if (!blocks[j].used && !blocks[j+1].used) {
                blocks[j].bytes += blocks[j+1].bytes;
                for (std::size_t k = j+1; k+1 < count; ++k) blocks[k] = blocks[k+1];
                --count; ++coalesces;
            } else ++j;
        }
        return Allocation::ok;
    }
    return Allocation::invalid_free;
}
}
