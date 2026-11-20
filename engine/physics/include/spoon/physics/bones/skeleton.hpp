#pragma once

#include "spoon/physics/bones/bone_pose.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace spoon::physics {

struct BoneNode {
    // -1 marks a root. Every other value must refer to an element in the same skeleton.
    std::int32_t parent{-1};
    BonePose local_pose{};
};

inline Quaternion multiply(const Quaternion& a, const Quaternion& b) {
    return Quaternion{
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
    }.normalized();
}

inline Vec3 component_multiply(const Vec3& a, const Vec3& b) {
    return {a.x*b.x, a.y*b.y, a.z*b.z};
}

// Evaluates poses without allocating. Output and state must already match bones.size().
// Returns false for invalid parents, cycles, or non-finite local transforms.
inline bool evaluate_skeleton(const std::vector<BoneNode>& bones,
                              std::vector<BonePose>& world_poses,
                              std::vector<std::uint8_t>& state) {
    const std::size_t count = bones.size();
    if (world_poses.size() != count || state.size() != count) return false;

    for (std::size_t i = 0; i < count; ++i) {
        state[i] = 0;
        if (!bones[i].local_pose.is_finite()) return false;
        const std::int32_t parent = bones[i].parent;
        if (parent < -1 || (parent >= 0 &&
            static_cast<std::size_t>(parent) >= count)) return false;
        if (parent == static_cast<std::int32_t>(i)) return false;
    }

    std::size_t completed = 0;
    while (completed < count) {
        bool made_progress = false;
        for (std::size_t i = 0; i < count; ++i) {
            if (state[i] == 2) continue;
            const std::int32_t parent = bones[i].parent;
            if (parent >= 0 && state[static_cast<std::size_t>(parent)] != 2)
                continue;

            const BonePose& local = bones[i].local_pose;
            if (parent < 0) {
                world_poses[i] = local;
            } else {
                const BonePose& parent_world = world_poses[static_cast<std::size_t>(parent)];
                world_poses[i].translation = parent_world.transform_point(local.translation);
                world_poses[i].rotation = multiply(parent_world.rotation, local.rotation);
                // Component scale is an efficient approximation. A rotated child beneath
                // non-uniform parent scale can require shear, which BonePose cannot represent.
                world_poses[i].scale = component_multiply(parent_world.scale, local.scale);
            }
            if (!world_poses[i].is_finite()) return false;
            state[i] = 2;
            ++completed;
            made_progress = true;
        }
        if (!made_progress) return false; // Remaining nodes form a cycle.
    }
    return true;
}

} // namespace spoon::physics
