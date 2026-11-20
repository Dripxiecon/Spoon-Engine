#include "spoon/physics/simulation/fixed_step_clock.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using spoon::physics::FixedStepClock;

int main() {
    FixedStepClock clock(1.0f / 120.0f, 8, 0.25f);
    assert(clock.consume(1.0f / 60.0f) == 2);
    assert(clock.consume(-1.0f) == 0);
    assert(clock.consume(1.0f) == 8);
    assert(clock.interpolation_alpha() >= 0.0f);
    assert(clock.interpolation_alpha() < 1.0f);
    clock.reset();
    assert(clock.interpolation_alpha() == 0.0f);
    std::cout << "Spoon fixed-step clock tests passed\n";
}
