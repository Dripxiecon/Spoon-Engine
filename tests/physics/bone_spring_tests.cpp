#include "spoon/physics/bones/bone_spring.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using spoon::physics::BoneSpring;
using spoon::physics::BoneSpringSettings;
using spoon::physics::Vec3;

int main() {
    BoneSpring spring(BoneSpringSettings{80.0f, 14.0f, 1.0f, 50.0f});
    spring.teleport({0.0f, 0.0f, 0.0f});
    spring.set_target({1.0f, 0.0f, 0.0f});

    for (int i = 0; i < 600; ++i) spring.step(1.0f / 120.0f);
    assert(std::abs(spring.state().position.x - 1.0f) < 0.02f);
    assert(spring.state().position.is_finite());

    spring.add_impulse({0.0f, 4.0f, 0.0f});
    assert(spring.state().velocity.y > 0.0f);

    spring.set_active(false);
    assert(spring.state().velocity.length_squared() == 0.0f);
    assert((spring.state().position - spring.state().target).length_squared() == 0.0f);

    spring.step(-1.0f);
    assert(spring.state().position.is_finite());

    std::cout << "Spoon physics foundation tests passed\n";
    return 0;
}
