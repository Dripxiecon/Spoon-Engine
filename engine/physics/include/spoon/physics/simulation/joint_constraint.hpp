#pragma once

#include "spoon/physics/simulation/bone_body.hpp"

#include <cstddef>
#include <cmath>

namespace spoon::physics {

struct BoneJoint final {
    std::size_t body_a{0};
    std::size_t body_b{0};
    Vec3 local_anchor_a{};
    Vec3 local_anchor_b{};
    float compliance{0.0f};
    Vec3 accumulated_lambda{};
    bool enabled{true};

    void reset() { accumulated_lambda = {}; }

    bool solve(BoneBody& a, BoneBody& b, float delta_time) {
        if (!enabled || !std::isfinite(delta_time) || delta_time <= 0.0f ||
            !local_anchor_a.is_finite() || !local_anchor_b.is_finite() ||
            !std::isfinite(compliance) || compliance < 0.0f) return false;

        const Vec3 anchor_a = a.pose.transform_point(local_anchor_a);
        const Vec3 anchor_b = b.pose.transform_point(local_anchor_b);
        const Vec3 error = anchor_b - anchor_a;
        if (!error.is_finite()) return false;

        const float alpha = compliance / (delta_time * delta_time);
        const Vec3 axes[3] = {{1.0f, 0.0f, 0.0f},
                              {0.0f, 1.0f, 0.0f},
                              {0.0f, 0.0f, 1.0f}};
        float* lambda_components[3] = {
            &accumulated_lambda.x, &accumulated_lambda.y, &accumulated_lambda.z
        };

        for (int i = 0; i < 3; ++i) {
            const Vec3 axis = axes[i];
            const Vec3 ra = anchor_a - a.pose.translation;
            const Vec3 rb = anchor_b - b.pose.translation;
            const Vec3 angular_a = a.inverse_inertia_world(cross_vec(ra, axis));
            const Vec3 angular_b = b.inverse_inertia_world(cross_vec(rb, axis));
            const float effective_mass =
                a.inverse_mass + b.inverse_mass +
                dot(cross_vec(angular_a, ra), axis) +
                dot(cross_vec(angular_b, rb), axis);
            const float denominator = effective_mass + alpha;
            if (!std::isfinite(denominator) || denominator <= 1.0e-9f) continue;

            const float delta_lambda =
                (-dot(error, axis) - alpha * (*lambda_components[i])) / denominator;
            *lambda_components[i] += delta_lambda;
            const Vec3 impulse = axis * delta_lambda;
            apply_position_impulse(a, anchor_a, impulse * -1.0f);
            apply_position_impulse(b, anchor_b, impulse);
        }
        return true;
    }

private:
    static float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    static Vec3 cross_vec(const Vec3& a, const Vec3& b) {
        return {a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x};
    }

    static void apply_position_impulse(BoneBody& body,
                                       const Vec3& world_anchor,
                                       const Vec3& impulse) {
        if (!body.is_dynamic()) return;
        body.pose.translation += impulse * body.inverse_mass;
        const Vec3 angular_impulse =
            body.inverse_inertia_world(cross_vec(world_anchor - body.pose.translation,
                                                  impulse));
        const float angle = angular_impulse.length();
        if (std::isfinite(angle) && angle > 1.0e-8f) {
            const Vec3 axis = angular_impulse / angle;
            const float half = 0.5f * angle;
            const Quaternion delta{std::cos(half), axis.x*std::sin(half),
                                   axis.y*std::sin(half), axis.z*std::sin(half)};
            body.pose.rotation = multiply(delta, body.pose.rotation).normalized();
        }
    }
};

} // namespace spoon::physics
