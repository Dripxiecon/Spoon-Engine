#include "spoon/physics/muscles/muscle_attachment.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace spoon::physics;

int main() {
    std::vector<BonePose> poses(2);
    poses[0].translation = {0.0f, 0.0f, 0.0f};
    poses[1].translation = {1.0f, 0.0f, 0.0f};

    MuscleSettings settings{};
    settings.max_force = 100.0f;
    settings.optimal_length = 1.0f;
    settings.force_length_width = 0.5f;
    settings.passive_slack_length = 2.0f;
    MuscleUnit muscle(settings);
    muscle.set_activation(1.0f);
    muscle.step_activation(1.0f);

    MuscleAttachment attachment{};
    attachment.origin_bone = 0;
    attachment.origin_local = {0.0f, 1.0f, 0.0f};
    attachment.insertion_bone = 1;
    attachment.insertion_local = {0.0f, 1.0f, 0.0f};

    const auto forces = evaluate_muscle_attachment(muscle, attachment, poses);
    assert(forces.valid);
    assert(forces.tension > 90.0f);
    assert(std::fabs(forces.origin_force.x - forces.tension) < 1.0e-3f);
    assert(std::fabs(forces.insertion_force.x + forces.tension) < 1.0e-3f);
    assert(std::fabs(forces.origin_torque.z + forces.tension) < 1.0e-2f);
    assert(std::fabs(forces.insertion_torque.z - forces.tension) < 1.0e-2f);

    attachment.insertion_bone = 9;
    assert(!evaluate_muscle_attachment(muscle, attachment, poses).valid);
    return 0;
}
