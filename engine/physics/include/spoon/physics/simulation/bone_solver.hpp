#pragma once

#include "spoon/physics/muscles/muscle_attachment.hpp"
#include "spoon/physics/simulation/bone_body.hpp"
#include "spoon/physics/simulation/fixed_step_clock.hpp"
#include "spoon/physics/simulation/joint_constraint.hpp"

#include <cmath>
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
        if (!std::isfinite(delta_time) || delta_time <= 0.0f) return;
        for (BoneBody& body : bodies) body.integrate(delta_time, gravity);

        const int iterations = constraint_iterations < 1 ? 1 : constraint_iterations;
        for (BoneJoint& joint : joints) joint.reset();
        for (int iteration = 0; iteration < iterations; ++iteration) {
            for (BoneJoint& joint : joints) {
                if (joint.body_a < bodies.size() && joint.body_b < bodies.size())
                    joint.solve(bodies[joint.body_a], bodies[joint.body_b], delta_time);
            }
        }
    }
};

} // namespace spoon::physics
