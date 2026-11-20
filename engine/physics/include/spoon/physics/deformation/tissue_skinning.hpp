#pragma once

#include "spoon/physics/deformation/skinned_mesh.hpp"
#include "spoon/physics/deformation/tissue_graph.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

// Adds a tissue node's world-space displacement to one already-skinned vertex.
// Multiple bindings may target the same vertex to blend several tissue nodes.
struct TissueVertexBinding final {
    std::size_t vertex_index{0};
    std::size_t node_index{0};
    Vec3 rest_node_position{};
    float weight{1.0f};
};

class TissueSkinningBridge final {
public:
    // Call after tissue simulation. All bindings are validated before output is
    // modified. Pre-size output_positions to avoid allocation in the frame loop.
    static bool deform(const std::vector<SkinVertex>& vertices,
                       const std::vector<BonePose>& bone_poses,
                       const std::vector<TissueNode>& tissue_nodes,
                       const std::vector<TissueVertexBinding>& bindings,
                       std::vector<Vec3>& output_positions) {
        if (output_positions.size() != vertices.size()) return false;

        for (const TissueVertexBinding& binding : bindings) {
            if (binding.vertex_index >= vertices.size() ||
                binding.node_index >= tissue_nodes.size() ||
                !binding.rest_node_position.is_finite() ||
                !std::isfinite(binding.weight) ||
                binding.weight < 0.0f || binding.weight > 1.0f ||
                !tissue_nodes[binding.node_index].position.is_finite()) {
                return false;
            }
        }

        SkinnedMeshDeformer::deform(vertices, bone_poses, output_positions);
        for (const TissueVertexBinding& binding : bindings) {
            const Vec3 displacement =
                tissue_nodes[binding.node_index].position -
                binding.rest_node_position;
            output_positions[binding.vertex_index] +=
                displacement * binding.weight;
        }
        return true;
    }
};

} // namespace spoon::physics
