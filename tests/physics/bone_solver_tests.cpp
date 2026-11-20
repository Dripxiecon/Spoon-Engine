#include "spoon/physics/simulation/bone_solver.hpp"

#include <cassert>
#include <cmath>

using namespace spoon::physics;

int main() {
    BoneBody body{};
    body.inverse_mass = 0.5f;
    body.inverse_inertia_local = {1.0f, 1.0f, 1.0f};
    body.apply_force({10.0f, 0.0f, 0.0f});
    body.integrate(0.1f, {});
    assert(std::fabs(body.linear_velocity.x - 0.5f) < 1.0e-4f);
    assert(std::fabs(body.pose.translation.x - 0.05f) < 1.0e-4f);
    assert(body.accumulated_force.length_squared() == 0.0f);

    BoneBody torque_body{};
    torque_body.inverse_mass = 1.0f;
    torque_body.apply_force_at_point({0.0f, 1.0f, 0.0f},
                                     {1.0f, 0.0f, 0.0f});
    assert(std::fabs(torque_body.accumulated_torque.z - 1.0f) < 1.0e-5f);

    BoneSolver solver(2, 1);
    solver.gravity = {};
    solver.constraint_iterations = 12;
    solver.bodies[0] = BoneBody::make_static();
    solver.bodies[1].pose.translation = {1.0f, 0.0f, 0.0f};
    solver.bodies[1].inverse_mass = 1.0f;
    solver.bodies[1].inverse_inertia_local = {1.0f, 1.0f, 1.0f};

    BoneJoint joint{};
    joint.body_a = 0;
    joint.body_b = 1;
    joint.local_anchor_a = {0.0f, 0.0f, 0.0f};
    joint.local_anchor_b = {-1.0f, 0.0f, 0.0f};
    solver.joints[0] = joint;
    solver.bodies[1].pose.translation.x = 1.5f;

    for (int i = 0; i < 10; ++i) solver.step(1.0f / 120.0f);
    const Vec3 anchor_a = solver.bodies[0].pose.transform_point(
        solver.joints[0].local_anchor_a);
    const Vec3 anchor_b = solver.bodies[1].pose.transform_point(
        solver.joints[0].local_anchor_b);
    assert((anchor_b - anchor_a).length() < 0.02f);

    BoneSolver repeat_a(1, 0);
    BoneSolver repeat_b(1, 0);
    repeat_a.gravity = repeat_b.gravity = {};
    repeat_a.bodies[0].apply_force({4.0f, 0.0f, 0.0f});
    repeat_b.bodies[0].apply_force({4.0f, 0.0f, 0.0f});
    repeat_a.step(1.0f / 120.0f);
    repeat_b.step(1.0f / 120.0f);
    assert(std::fabs(repeat_a.bodies[0].pose.translation.x -
                     repeat_b.bodies[0].pose.translation.x) < 1.0e-7f);
    return 0;
}
