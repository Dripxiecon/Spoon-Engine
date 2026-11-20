#pragma once

#include "spoon/physics/bones/bone_pose.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace spoon::physics {

struct SkinInfluence {
    std::size_t bone_index{0};
    float weight{0.0f};
    Vec3 bind_local_position{};
};

struct SkinVertex {
    Vec3 bind_position{};
    std::array<SkinInfluence, 4> influences{};
};

class SkinnedMeshDeformer final {
public:
    // Each influence stores the vertex position in that bone's bind-local space.
    // Weights are normalized at evaluation, so malformed totals do not scale vertices.
    static Vec3 deform_vertex(const SkinVertex& vertex,
                              const std::vector<BonePose>& bone_poses) {
        Vec3 accumulated{};
        float total_weight = 0.0f;

        for (const SkinInfluence& influence : vertex.influences) {
            if (influence.weight <= 0.0f || !std::isfinite(influence.weight) ||
                influence.bone_index >= bone_poses.size() ||
                !influence.bind_local_position.is_finite() ||
                !bone_poses[influence.bone_index].is_finite()) {
                continue;
            }

            accumulated += bone_poses[influence.bone_index].transform_point(
                influence.bind_local_position) * influence.weight;
            total_weight += influence.weight;
        }

        if (total_weight <= 1.0e-8f) return vertex.bind_position;
        return accumulated / total_weight;
    }

    static void deform(const std::vector<SkinVertex>& vertices,
                       const std::vector<BonePose>& bone_poses,
                       std::vector<Vec3>& output_positions) {
        output_positions.resize(vertices.size());
        for (std::size_t index = 0; index < vertices.size(); ++index)
            output_positions[index] = deform_vertex(vertices[index], bone_poses);
    }
};

} // namespace spoon::physics
