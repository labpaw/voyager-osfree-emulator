#include "emulator/model.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#define CHECK(x) do { if (!(x)) { std::cerr << __FILE__ << ':' << __LINE__ << ": " #x "\n"; std::exit(1); } } while (false)
void validation() {
    emu::Config c; c.count = 1; c.horizon = 20;
    c.tasks[0] = {1, 5, 5, 1, 0, 8, 0};
    emu::Tick w = 0, j = 0;
    CHECK(emu::validate(c, w, j) == emu::Status::ready && w == 4 && j == 4);
    c.tasks[0].period = 0;
    CHECK(emu::validate(c, w, j) == emu::Status::invalid_input);
    c.tasks[0].period = 5; c.count = 2; c.tasks[1] = c.tasks[0];
    CHECK(emu::validate(c, w, j) == emu::Status::invalid_input);
    c.count = 1; c.tasks[0].deadline = std::numeric_limits<emu::Tick>::max();
    CHECK(emu::validate(c, w, j) == emu::Status::arithmetic_overflow);
}
int main() {
    validation();
    emu::Tick n = 0;
    CHECK(emu::checked_add(2, 3, n) && n == 5);
    CHECK(!emu::checked_add(std::numeric_limits<emu::Tick>::max(), 1, n));
}
