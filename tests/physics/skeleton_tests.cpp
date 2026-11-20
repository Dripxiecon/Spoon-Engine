#include "spoon/physics/bones/skeleton.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace spoon::physics;

int main() {
    std::vector<BoneNode> bones(2);
    bones[0].parent = -1;
    bones[0].local_pose.translation = {2.0f, 0.0f, 0.0f};
    bones[1].parent = 0;
    bones[1].local_pose.translation = {0.0f, 3.0f, 0.0f};

    std::vector<BonePose> world(2);
    std::vector<std::uint8_t> state(2);
    assert(evaluate_skeleton(bones, world, state));
    assert(std::fabs(world[1].translation.x - 2.0f) < 1.0e-5f);
    assert(std::fabs(world[1].translation.y - 3.0f) < 1.0e-5f);

    bones[0].parent = 1;
    bones[1].parent = 0;
    assert(!evaluate_skeleton(bones, world, state));

    bones[0].parent = -1;
    bones[1].parent = 7;
    assert(!evaluate_skeleton(bones, world, state));
    return 0;
}
