#include "spoon/physics/deformation/muscle_geometry.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace spoon::physics;

int main() {
    const auto contracted = MuscleGeometry::evaluate(1.0f, 0.5f, 0.1f, 1.0f, 2.0f);
    assert(std::abs(contracted.length_scale - 0.5f) < 1.0e-5f);
    assert(contracted.radius_scale > 1.0f);
    assert(contracted.radius_scale <= 2.0f);

    const auto stretched = MuscleGeometry::evaluate(1.0f, 2.0f, 0.1f, 1.0f, 2.0f);
    assert(stretched.radius_scale < 1.0f);

    const auto rigid = MuscleGeometry::evaluate(1.0f, 0.5f, 0.1f, 0.0f, 2.0f);
    assert(std::abs(rigid.radius_scale - 1.0f) < 1.0e-5f);

    std::cout << "Spoon muscle geometry tests passed\n";
}
