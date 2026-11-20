#include "spoon/physics/muscles/muscle_rig.hpp"

#include <cassert>
#include <cmath>

using namespace spoon::physics;

int main() {
    BoneSolver solver(2, 0);
    solver.gravity = {};
    solver.bodies[0].pose.translation = {0.0f, 0.0f, 0.0f};
    solver.bodies[1].pose.translation = {1.0f, 0.0f, 0.0f};
    solver.bodies[0].inverse_mass = 1.0f;
    solver.bodies[1].inverse_mass = 1.0f;
    solver.bodies[0].inverse_inertia_local = {1.0f, 1.0f, 1.0f};
    solver.bodies[1].inverse_inertia_local = {1.0f, 1.0f, 1.0f};

    MuscleSettings settings{};
    settings.max_force = 100.0f;
    settings.optimal_length = 1.0f;
    settings.force_length_width = 0.5f;
    settings.passive_slack_length = 2.0f;

    MuscleRig rig(1, 1);
    rig.muscles[0].set_settings(settings);
    rig.actuators[0].muscle_index = 0;
    rig.actuators[0].attachment.origin_bone = 0;
    rig.actuators[0].attachment.origin_local = {0.0f, 0.5f, 0.0f};
    rig.actuators[0].attachment.insertion_bone = 1;
    rig.actuators[0].attachment.insertion_local = {0.0f, 0.5f, 0.0f};

    MuscleGroup flexor;
    flexor.role = MuscleGroupRole::agonist;
    flexor.members = {{0, 1.0f}};
    flexor.set_target_activation(1.0f);
    rig.groups.groups.push_back(flexor);

    assert(rig.prepare(solver));
    for (int i = 0; i < 240; ++i) {
        assert(rig.step(solver, 1.0f / 120.0f));
        solver.step(1.0f / 120.0f);
    }

    assert(rig.muscles[0].activation() > 0.98f);
    assert(solver.bodies[0].linear_velocity.x > 0.0f);
    assert(solver.bodies[1].linear_velocity.x < 0.0f);

    BoneSolver static_solver(2, 0);
    static_solver.gravity = {};
    static_solver.bodies[0] = BoneBody::make_static();
    static_solver.bodies[1].pose.translation = {1.0f, 0.0f, 0.0f};
    MuscleRig static_rig(1, 1);
    static_rig.muscles[0].set_settings(settings);
    static_rig.actuators[0].attachment.origin_bone = 0;
    static_rig.actuators[0].attachment.insertion_bone = 1;
    MuscleGroup static_group;
    static_group.members = {{0, 1.0f}};
    static_group.set_target_activation(1.0f);
    static_rig.groups.groups.push_back(static_group);
    assert(static_rig.prepare(static_solver));
    assert(static_rig.step(static_solver, 1.0f / 120.0f));
    assert(static_solver.bodies[0].accumulated_force.length_squared() == 0.0f);
    assert(static_solver.bodies[1].accumulated_force.length_squared() > 0.0f);

    const Vec3 force_before = static_solver.bodies[1].accumulated_force;
    static_rig.actuators[0].attachment.insertion_bone = 99;
    assert(!static_rig.step(static_solver, 1.0f / 120.0f));
    assert((static_solver.bodies[1].accumulated_force - force_before).length_squared()
           < 1.0e-8f);

    return 0;
}
