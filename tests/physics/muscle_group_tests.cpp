#include "spoon/physics/muscles/muscle_group.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace spoon::physics;

int main() {
    std::vector<MuscleUnit> muscles(3);
    MuscleGroupController controller(muscles.size());

    MuscleGroup flexors;
    flexors.role = MuscleGroupRole::agonist;
    flexors.members = {{0, 1.0f}, {1, 0.8f}};
    flexors.set_target_activation(1.0f);

    MuscleGroup extensors;
    extensors.role = MuscleGroupRole::antagonist;
    extensors.members = {{2, 1.0f}};
    extensors.set_target_activation(0.35f);

    MuscleGroup stabilizers;
    stabilizers.role = MuscleGroupRole::stabilizer;
    stabilizers.members = {{1, 0.4f}};
    stabilizers.set_target_activation(0.5f);

    controller.groups = {flexors, extensors, stabilizers};
    for (int i = 0; i < 240; ++i)
        assert(controller.step(muscles, 1.0f / 120.0f));

    assert(muscles[0].activation() > 0.98f);
    assert(muscles[1].activation() > 0.98f);
    assert(muscles[2].activation() > 0.30f);
    assert(controller.groups[0].role == MuscleGroupRole::agonist);
    assert(controller.groups[1].role == MuscleGroupRole::antagonist);
    assert(controller.groups[2].role == MuscleGroupRole::stabilizer);

    // Invalid member indices and weights are ignored safely.
    MuscleGroup malformed;
    malformed.members = {{99, 1.0f}, {0, -2.0f},
                         {1, std::nanf("")}};
    malformed.set_target_activation(1.0f);
    controller.groups.push_back(malformed);
    assert(controller.step(muscles, 1.0f / 120.0f));

    // Changing muscle count requires an explicit setup-time prepare.
    muscles.emplace_back();
    assert(!controller.step(muscles, 1.0f / 120.0f));
    controller.prepare(muscles.size());
    assert(controller.step(muscles, 1.0f / 120.0f));

    // Bad timesteps do not advance the controller.
    const float before = controller.groups[0].activation();
    assert(!controller.step(muscles, 0.0f));
    assert(std::fabs(controller.groups[0].activation() - before) < 1.0e-6f);
    return 0;
}
