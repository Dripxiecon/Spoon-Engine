#pragma once

#include "spoon/physics/math/vec3.hpp"

#include <cmath>

namespace spoon::physics {

struct Quaternion {
    float w{1.0f};
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    static Quaternion identity() { return {}; }

    Quaternion normalized() const {
        const float magnitude_squared = w*w + x*x + y*y + z*z;
        if (!std::isfinite(magnitude_squared) || magnitude_squared <= 1.0e-12f)
            return {};
        const float inverse_magnitude = 1.0f / std::sqrt(magnitude_squared);
        return {w * inverse_magnitude, x * inverse_magnitude,
                y * inverse_magnitude, z * inverse_magnitude};
    }

    Quaternion conjugate() const { return {w, -x, -y, -z}; }

    Vec3 rotate(const Vec3& value) const {
        const Quaternion q = normalized();
        const Vec3 u{q.x, q.y, q.z};
        const Vec3 uv{u.y * value.z - u.z * value.y,
                      u.z * value.x - u.x * value.z,
                      u.x * value.y - u.y * value.x};
        const Vec3 uuv{u.y * uv.z - u.z * uv.y,
                       u.z * uv.x - u.x * uv.z,
                       u.x * uv.y - u.y * uv.x};
        return value + uv * (2.0f * q.w) + uuv * 2.0f;
    }
};

} // namespace spoon::physics
