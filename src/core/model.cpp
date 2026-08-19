#include "emulator/model.hpp"
#include <limits>
namespace emu {
bool checked_add(Tick a, Tick b, Tick& result) {
    if (b > std::numeric_limits<Tick>::max() - a) return false;
    result = a + b;
    return true;
}
}
