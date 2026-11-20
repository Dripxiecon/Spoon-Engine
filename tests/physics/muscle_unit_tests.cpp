#include "spoon/physics/muscles/muscle_unit.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace spoon::physics;

int main() {
    MuscleUnit muscle;
    muscle.set_activation(1.0f);
    for (int i = 0; i < 120; ++i) muscle.step_activation(1.0f / 120.0f);
    assert(muscle.activation() > 0.99f);

    const auto forces = muscle.evaluate({0.0f, 0.0f, 0.0f}, {0.30f, 0.0f, 0.0f});
    assert(forces.tension > 100.0f);
    assert(forces.origin.x > 0.0f);
    assert(forces.insertion.x < 0.0f);
    assert(std::abs(forces.origin.x + forces.insertion.x) < 1.0e-5f);

    muscle.set_activation(0.0f);
    for (int i = 0; i < 120; ++i) muscle.step_activation(1.0f / 120.0f);
    assert(muscle.activation() < 0.001f);

    const auto slack = muscle.evaluate({0.0f, 0.0f, 0.0f}, {0.20f, 0.0f, 0.0f});
    assert(slack.tension < 0.01f);
    std::cout << "Spoon muscle unit tests passed\n";
}
