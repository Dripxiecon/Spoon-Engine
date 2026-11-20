#pragma once

#include "spoon/physics/math/quaternion.hpp"

namespace spoon::physics {

struct BonePose {
    Vec3 translation{};
    Quaternion rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Vec3 transform_point(const Vec3& local_point) const {
        const Vec3 scaled{local_point.x * scale.x,
                          local_point.y * scale.y,
                          local_point.z * scale.z};
        return rotation.rotate(scaled) + translation;
    }

    bool is_finite() const {
        return translation.is_finite() && scale.is_finite() &&
               std::isfinite(rotation.w) && std::isfinite(rotation.x) &&
               std::isfinite(rotation.y) && std::isfinite(rotation.z);
    }
};

} // namespace spoon::physics
