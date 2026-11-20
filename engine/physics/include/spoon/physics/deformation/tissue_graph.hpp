#pragma once

#include "spoon/physics/math/vec3.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

struct TissueNode final {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 accumulated_force{};
    float inverse_mass{1.0f};
    bool enabled{true};

    bool is_dynamic() const {
        return enabled && std::isfinite(inverse_mass) && inverse_mass > 0.0f;
    }

    void apply_force(const Vec3& force) {
        if (is_dynamic() && force.is_finite()) accumulated_force += force;
    }

    void apply_impulse(const Vec3& impulse) {
        if (is_dynamic() && impulse.is_finite())
            velocity += impulse * inverse_mass;
    }
};

struct TissueLink final {
    std::size_t node_a{0};
    std::size_t node_b{0};
    float rest_length{0.0f};
    float compliance{0.0f};
    float damping{0.08f};
    float accumulated_lambda{0.0f};
    bool enabled{true};

    void reset() { accumulated_lambda = 0.0f; }
};

class TissueGraph final {
public:
    explicit TissueGraph(std::size_t node_count = 0,
                         std::size_t link_count = 0)
        : nodes(node_count), links(link_count), predicted_positions(node_count),
          previous_positions(node_count) {}

    std::vector<TissueNode> nodes;
    std::vector<TissueLink> links;
    Vec3 gravity{};
    int constraint_iterations{6};

    bool apply_impulse(std::size_t node_index, const Vec3& impulse) {
        if (node_index >= nodes.size() || !impulse.is_finite()) return false;
        nodes[node_index].apply_impulse(impulse);
        return true;
    }

    std::size_t apply_radial_impulse(const Vec3& center, float radius,
                                     float strength) {
        if (!center.is_finite() || !std::isfinite(radius) || radius <= 0.0f ||
            !std::isfinite(strength)) return 0;
        std::size_t affected = 0;
        const float radius_squared = radius * radius;
        for (TissueNode& node : nodes) {
            if (!node.is_dynamic()) continue;
            const Vec3 offset = node.position - center;
            const float distance_squared = offset.length_squared();
            if (distance_squared > radius_squared || distance_squared <= 1.0e-12f)
                continue;
            const float distance = std::sqrt(distance_squared);
            const float falloff = 1.0f - distance / radius;
            node.apply_impulse((offset / distance) * (strength * falloff));
            ++affected;
        }
        return affected;
    }

    void step(float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f ||
            !gravity.is_finite()) return;

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            TissueNode& node = nodes[i];
            previous_positions[i] = node.position;
            predicted_positions[i] = node.position;
            if (!node.is_dynamic()) {
                node.accumulated_force = {};
                continue;
            }
            const Vec3 acceleration = gravity +
                node.accumulated_force * node.inverse_mass;
            node.velocity += acceleration * delta_time;
            predicted_positions[i] += node.velocity * delta_time;
            node.accumulated_force = {};
        }

        for (TissueLink& link : links) link.reset();
        const int iterations = std::max(1, constraint_iterations);
        for (int iteration = 0; iteration < iterations; ++iteration) {
            for (TissueLink& link : links) solve_link(link, delta_time);
        }

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            TissueNode& node = nodes[i];
            if (node.is_dynamic()) {
                node.position = predicted_positions[i];
                node.velocity = (node.position - previous_positions[i]) /
                                delta_time;
            }
        }

        for (const TissueLink& link : links) apply_link_damping(link);
    }

private:
    std::vector<Vec3> predicted_positions;
    std::vector<Vec3> previous_positions;

    void solve_link(TissueLink& link, float delta_time) {
        if (!link.enabled || link.node_a >= nodes.size() ||
            link.node_b >= nodes.size() || link.node_a == link.node_b ||
            !std::isfinite(link.rest_length) || link.rest_length < 0.0f ||
            !std::isfinite(link.compliance) || link.compliance < 0.0f)
            return;

        TissueNode& a = nodes[link.node_a];
        TissueNode& b = nodes[link.node_b];
        const float weight_a = a.is_dynamic() ? a.inverse_mass : 0.0f;
        const float weight_b = b.is_dynamic() ? b.inverse_mass : 0.0f;
        const float weight_sum = weight_a + weight_b;
        if (weight_sum <= 0.0f) return;

        const Vec3 delta = predicted_positions[link.node_b] -
                           predicted_positions[link.node_a];
        const float distance_squared = delta.length_squared();
        if (!std::isfinite(distance_squared) || distance_squared <= 1.0e-12f)
            return;
        const float distance = std::sqrt(distance_squared);
        const Vec3 direction = delta / distance;
        const float constraint = distance - link.rest_length;
        const float alpha = link.compliance / (delta_time * delta_time);
        const float denominator = weight_sum + alpha;
        if (denominator <= 1.0e-12f || !std::isfinite(denominator)) return;

        const float delta_lambda =
            (-constraint - alpha * link.accumulated_lambda) / denominator;
        link.accumulated_lambda += delta_lambda;
        predicted_positions[link.node_a] -=
            direction * (weight_a * delta_lambda);
        predicted_positions[link.node_b] +=
            direction * (weight_b * delta_lambda);
    }

    void apply_link_damping(const TissueLink& link) {
        if (!link.enabled || link.node_a >= nodes.size() ||
            link.node_b >= nodes.size() || link.node_a == link.node_b ||
            !std::isfinite(link.damping)) return;
        const float damping = std::clamp(link.damping, 0.0f, 1.0f);
        if (damping <= 0.0f) return;

        TissueNode& a = nodes[link.node_a];
        TissueNode& b = nodes[link.node_b];
        const float weight_a = a.is_dynamic() ? a.inverse_mass : 0.0f;
        const float weight_b = b.is_dynamic() ? b.inverse_mass : 0.0f;
        const float weight_sum = weight_a + weight_b;
        if (weight_sum <= 0.0f) return;

        const Vec3 delta = b.position - a.position;
        const float distance_squared = delta.length_squared();
        if (distance_squared <= 1.0e-12f) return;
        const Vec3 direction = delta / std::sqrt(distance_squared);
        const float relative_speed =
            (b.velocity.x - a.velocity.x) * direction.x +
            (b.velocity.y - a.velocity.y) * direction.y +
            (b.velocity.z - a.velocity.z) * direction.z;
        const float impulse = damping * relative_speed / weight_sum;
        if (a.is_dynamic()) a.velocity += direction * (weight_a * impulse);
        if (b.is_dynamic()) b.velocity -= direction * (weight_b * impulse);
    }
};

} // namespace spoon::physics
