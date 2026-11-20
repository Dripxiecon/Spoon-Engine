#pragma once

#include "spoon/physics/muscles/muscle_group.hpp"
#include "spoon/physics/simulation/bone_solver.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

struct MuscleActuator {
    std::size_t muscle_index{0};
    MuscleAttachment attachment{};
};

// Connects grouped muscle activation to the force accumulators in BoneSolver.
// Configure vectors and call prepare() before entering the simulation loop.
class MuscleRig final {
public:
    explicit MuscleRig(std::size_t muscle_count = 0,
                       std::size_t actuator_count = 0)
        : muscles(muscle_count), actuators(actuator_count),
          groups(muscle_count), pose_scratch_(), force_scratch_() {}

    std::vector<MuscleUnit> muscles;
    std::vector<MuscleActuator> actuators;
    MuscleGroupController groups;

    bool prepare(const BoneSolver& solver) {
        pose_scratch_.resize(solver.bodies.size());
        force_scratch_.resize(actuators.size());
        groups.prepare(muscles.size());
        return true;
    }

    // Advances activation and accumulates attachment forces. Call solver.step(dt)
    // afterwards. No memory is allocated here when setup sizes remain unchanged.
    bool step(BoneSolver& solver, float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f ||
            pose_scratch_.size() != solver.bodies.size() ||
            force_scratch_.size() != actuators.size())
            return false;

        // Validate every reference before changing activation or force accumulators.
        for (const MuscleActuator& actuator : actuators) {
            if (actuator.muscle_index >= muscles.size() ||
                actuator.attachment.origin_bone >= solver.bodies.size() ||
                actuator.attachment.insertion_bone >= solver.bodies.size() ||
                !actuator.attachment.origin_local.is_finite() ||
                !actuator.attachment.insertion_local.is_finite())
                return false;
        }
        for (std::size_t i = 0; i < solver.bodies.size(); ++i) {
            if (!solver.bodies[i].pose.is_finite() ||
                !solver.bodies[i].linear_velocity.is_finite() ||
                !solver.bodies[i].angular_velocity.is_finite())
                return false;
            pose_scratch_[i] = solver.bodies[i].pose;
        }

        if (!groups.step(muscles, delta_time)) return false;

        for (std::size_t i = 0; i < actuators.size(); ++i) {
            const MuscleActuator& actuator = actuators[i];
            const BoneBody& origin = solver.bodies[actuator.attachment.origin_bone];
            const BoneBody& insertion = solver.bodies[actuator.attachment.insertion_bone];
            const Vec3 origin_position =
                pose_scratch_[actuator.attachment.origin_bone].transform_point(
                    actuator.attachment.origin_local);
            const Vec3 insertion_position =
                pose_scratch_[actuator.attachment.insertion_bone].transform_point(
                    actuator.attachment.insertion_local);
            const Vec3 origin_velocity = point_velocity(origin, origin_position);
            const Vec3 insertion_velocity = point_velocity(insertion, insertion_position);
            force_scratch_[i] = evaluate_muscle_attachment(
                muscles[actuator.muscle_index], actuator.attachment,
                pose_scratch_, origin_velocity, insertion_velocity);
            if (!force_scratch_[i].valid) return false;
        }

        for (std::size_t i = 0; i < actuators.size(); ++i) {
            const MuscleActuator& actuator = actuators[i];
            solver.add_muscle_forces(force_scratch_[i],
                                     actuator.attachment.origin_bone,
                                     actuator.attachment.insertion_bone);
        }
        return true;
    }

private:
    static Vec3 point_velocity(const BoneBody& body, const Vec3& world_point) {
        return body.linear_velocity +
               cross(body.angular_velocity, world_point - body.pose.translation);
    }

    std::vector<BonePose> pose_scratch_;
    std::vector<MuscleAttachmentForces> force_scratch_;
};

} // namespace spoon::physics
