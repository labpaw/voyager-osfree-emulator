#include "allocation_guard.hpp"
#include <cstdlib>
#include <new>
bool watch_allocations = false;
std::size_t watched_allocations = 0;
namespace {
void* get(std::size_t n, std::size_t alignment = 0) {
    if (watch_allocations) ++watched_allocations;
    void* p = nullptr;
    if (alignment) {
        if (posix_memalign(&p, alignment, n ? n : 1) != 0) p = nullptr;
    } else p = std::malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
}
void* operator new(std::size_t n) { return get(n); }
void* operator new[](std::size_t n) { return get(n); }
void* operator new(std::size_t n, std::align_val_t a) { return get(n, static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return get(n, static_cast<std::size_t>(a)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept { try { return get(n); } catch (...) { return nullptr; } }
void* operator new[](std::size_t n, const std::nothrow_t& t) noexcept { return ::operator new(n, t); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { try { return get(n, static_cast<std::size_t>(a)); } catch (...) { return nullptr; } }
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t& t) noexcept { return ::operator new(n, a, t); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
