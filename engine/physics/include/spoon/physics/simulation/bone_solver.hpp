#pragma once

#include "spoon/physics/simulation/bone_body.hpp"
#include "spoon/physics/simulation/fixed_step_clock.hpp"
#include "spoon/physics/simulation/joint_constraint.hpp"

#include <cstddef>
#include <vector>

namespace spoon::physics {

class BoneSolver final {
public:
    explicit BoneSolver(std::size_t body_count = 0, std::size_t joint_count = 0)
        : bodies(body_count), joints(joint_count) {}

    std::vector<BoneBody> bodies;
    std::vector<BoneJoint> joints;
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    int constraint_iterations{6};

    bool add_muscle_forces(const MuscleAttachmentForces& forces,
                           std::size_t origin_body,
                           std::size_t insertion_body) {
        if (!forces.valid || origin_body >= bodies.size() ||
            insertion_body >= bodies.size()) return false;
        bodies[origin_body].apply_force_at_point(forces.origin_force,
                                                 forces.origin_position);
        bodies[insertion_body].apply_force_at_point(forces.insertion_force,
                                                    forces.insertion_position);
        return true;
    }

    void step(float delta_time) {
        if (delta_time <= 0.0f) return;
        for (BoneBody& body : bodies) {
            if (body.enabled && body.is_dynamic()) {
                body.linear_velocity += (gravity + body.accumulated_force *
                    body.inverse_mass) * delta_time;
                body.angular_velocity +=
                    body.inverse_inertia_world(body.accumulated_torque) * delta_time;
                body.pose.translation += body.linear_velocity * delta_time;
                const Quaternion omega{0.0f, body.angular_velocity.x,
                                       body.angular_velocity.y, body.angular_velocity.z};
                const Quaternion derivative = multiply(omega, body.pose.rotation);
                const float half_delta = 0.5f * delta_time;
                body.pose.rotation = Quaternion{
                    body.pose.rotation.w + derivative.w * half_delta,
                    body.pose.rotation.x + derivative.x * half_delta,
                    body.pose.rotation.y + derivative.y * half_delta,
                    body.pose.rotation.z + derivative.z * half_delta
                }.normalized();
            }
            body.clear_accumulators();
        }

        const int iterations = constraint_iterations < 1 ? 1 : constraint_iterations;
        for (BoneJoint& joint : joints) {
            if (joint.body_a >= bodies.size() || joint.body_b >= bodies.size())
                continue;
            joint.reset();
        }
        for (int iteration = 0; iteration < iterations; ++iteration) {
            for (BoneJoint& joint : joints) {
                if (joint.body_a < bodies.size() && joint.body_b < bodies.size())
                    joint.solve(bodies[joint.body_a], bodies[joint.body_b], delta_time);
            }
        }
    }
};

} // namespace spoon::physics
