#pragma once
#include <cstddef>

namespace frozen_py {

constexpr const char* main_py = R"PY_EOF(
from vex import *

brain = Brain()

motor = Motor(Ports.PORT12, GearSetting.RATIO_18_1, True)
rotation = Rotation(Ports.PORT7, True)
timer = Timer()

start1 = timer.time()
motor.spin(FORWARD)
end1 = timer.time()

print("time diff = %d"%((end1 - start1)))

brain.screen.print("HI")
)PY_EOF";

struct PyModule {
    const char* name;
    const char* source;
};

constexpr size_t MODULE_COUNT = 1;
constexpr PyModule MODULES[] = {
    {"main.py", main_py},
};

} // namespace frozen_py
