#pragma once

#include <algorithm>
#include <cmath>

namespace spoon::physics {

struct MuscleGeometryState {
    float length_scale{1.0f};
    float radius_scale{1.0f};
    float radius{0.0f};
};

class MuscleGeometry final {
public:
    // Approximates bulging during contraction. This is a visual/deformation
    // target, not a claim that the muscle is a fully simulated fluid volume.
    static MuscleGeometryState evaluate(float rest_length,
                                        float current_length,
                                        float base_radius,
                                        float volume_preservation = 0.85f,
                                        float max_bulge = 1.8f) {
        MuscleGeometryState result{};
        if (!std::isfinite(rest_length) || !std::isfinite(current_length) ||
            !std::isfinite(base_radius) || rest_length <= 1.0e-6f ||
            current_length <= 1.0e-6f || base_radius < 0.0f) {
            return result;
        }

        const float preservation = std::clamp(
            std::isfinite(volume_preservation) ? volume_preservation : 0.0f,
            0.0f, 1.0f);
        const float safe_max_bulge = std::max(
            1.0f, std::isfinite(max_bulge) ? max_bulge : 1.0f);

        result.length_scale = current_length / rest_length;
        const float ideal_radius_scale = std::sqrt(rest_length / current_length);
        const float blended_radius_scale =
            1.0f + (ideal_radius_scale - 1.0f) * preservation;
        result.radius_scale = std::clamp(blended_radius_scale, 1.0f / safe_max_bulge,
                                         safe_max_bulge);
        result.radius = base_radius * result.radius_scale;
        return result;
    }
};

} // namespace spoon::physics
