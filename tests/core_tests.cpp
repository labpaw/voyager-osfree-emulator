#include "emulator/model.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)
int main() {
    emu::Tick n = 0;
    CHECK(emu::checked_add(2, 3, n) && n == 5);
    CHECK(!emu::checked_add(std::numeric_limits<emu::Tick>::max(), 1, n));
}
