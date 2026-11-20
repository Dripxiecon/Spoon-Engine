#pragma once

#include "spoon/physics/bones/bone_pose.hpp"
#include "spoon/physics/deformation/tissue_graph.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

// A tissue node that follows a bone-local point while transmitting motion
// into neighboring dynamic tissue nodes through TissueGraph links.
struct TissueBinding final {
    std::size_t node_index{0};
    std::size_t bone_index{0};
    Vec3 bone_local_anchor{};
    Vec3 previous_target{};
    bool initialized{false};
};

class TissueBindingSet final {
public:
    explicit TissueBindingSet(std::size_t binding_count = 0)
        : bindings(binding_count) {}

    std::vector<TissueBinding> bindings;

    // Call after bone poses are evaluated and before TissueGraph::step().
    // Bound nodes become kinematic anchors; their velocity is retained as
    // target motion so link damping can transmit bone movement into tissue.
    bool update_anchors(TissueGraph& graph,
                       const std::vector<BonePose>& bone_poses,
                       float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f) return false;

        for (const TissueBinding& binding : bindings) {
            if (binding.node_index >= graph.nodes.size() ||
                binding.bone_index >= bone_poses.size() ||
                !binding.bone_local_anchor.is_finite() ||
                !bone_poses[binding.bone_index].is_finite()) {
                return false;
            }
        }

        for (TissueBinding& binding : bindings) {
            const Vec3 target =
                bone_poses[binding.bone_index].transform_point(
                    binding.bone_local_anchor);
            if (!target.is_finite()) return false;

            TissueNode& node = graph.nodes[binding.node_index];
            node.inverse_mass = 0.0f;
            node.accumulated_force = {};
            if (binding.initialized) {
                node.velocity = (target - binding.previous_target) / delta_time;
            } else {
                node.velocity = {};
                binding.initialized = true;
            }
            node.position = target;
            binding.previous_target = target;
        }
        return true;
    }
};

} // namespace spoon::physics
