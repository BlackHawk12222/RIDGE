#ifndef VEX_LOGIC_HPP
#define VEX_LOGIC_HPP

#include <cstdint>

namespace VexLogic {
bool wait(double duration, int units);
void yield_ms(std::uint32_t ms = 1);
}

#endif
