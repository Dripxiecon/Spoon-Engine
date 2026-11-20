#pragma once

#include "spoon/physics/bones/bone_pose.hpp"
#include "spoon/physics/muscles/muscle_unit.hpp"

#include <cstddef>
#include <vector>

namespace spoon::physics {

struct MuscleAttachment {
    std::size_t origin_bone{0};
    Vec3 origin_local{};
    std::size_t insertion_bone{0};
    Vec3 insertion_local{};
};

struct MuscleAttachmentForces {
    Vec3 origin_position{};
    Vec3 insertion_position{};
    Vec3 origin_force{};
    Vec3 insertion_force{};
    Vec3 origin_torque{};
    Vec3 insertion_torque{};
    float tension{0.0f};
    bool valid{false};
};

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x};
}

inline MuscleAttachmentForces evaluate_muscle_attachment(
    const MuscleUnit& muscle,
    const MuscleAttachment& attachment,
    const std::vector<BonePose>& world_poses,
    const Vec3& origin_velocity = {},
    const Vec3& insertion_velocity = {}) {
    MuscleAttachmentForces result{};
    if (attachment.origin_bone >= world_poses.size() ||
        attachment.insertion_bone >= world_poses.size() ||
        !attachment.origin_local.is_finite() ||
        !attachment.insertion_local.is_finite() ||
        !origin_velocity.is_finite() || !insertion_velocity.is_finite())
        return result;

    const BonePose& origin_bone = world_poses[attachment.origin_bone];
    const BonePose& insertion_bone = world_poses[attachment.insertion_bone];
    if (!origin_bone.is_finite() || !insertion_bone.is_finite()) return result;

    result.origin_position = origin_bone.transform_point(attachment.origin_local);
    result.insertion_position = insertion_bone.transform_point(attachment.insertion_local);
    const MuscleForces muscle_forces = muscle.evaluate(
        result.origin_position, result.insertion_position,
        origin_velocity, insertion_velocity);
    result.origin_force = muscle_forces.origin;
    result.insertion_force = muscle_forces.insertion;
    result.origin_torque = cross(result.origin_position - origin_bone.translation,
                                 result.origin_force);
    result.insertion_torque = cross(result.insertion_position - insertion_bone.translation,
                                    result.insertion_force);
    result.tension = muscle_forces.tension;
    result.valid = result.origin_position.is_finite() &&
                   result.insertion_position.is_finite() &&
                   result.origin_force.is_finite() &&
                   result.insertion_force.is_finite() &&
                   result.origin_torque.is_finite() &&
                   result.insertion_torque.is_finite() &&
                   std::isfinite(result.tension);
    return result;
}

} // namespace spoon::physics
