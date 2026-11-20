#pragma once

#include <cmath>

namespace spoon::physics {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3() = default;
    constexpr Vec3(float x_value, float y_value, float z_value)
        : x(x_value), y(y_value), z(z_value) {}

    constexpr Vec3 operator+(const Vec3& rhs) const {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }
    constexpr Vec3 operator-(const Vec3& rhs) const {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }
    constexpr Vec3 operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }
    constexpr Vec3 operator/(float scalar) const {
        return scalar != 0.0f ? Vec3{x / scalar, y / scalar, z / scalar} : Vec3{};
    }
    Vec3& operator+=(const Vec3& rhs) {
        x += rhs.x; y += rhs.y; z += rhs.z; return *this;
    }
    Vec3& operator-=(const Vec3& rhs) {
        x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this;
    }

    constexpr float length_squared() const { return x*x + y*y + z*z; }
    float length() const { return std::sqrt(length_squared()); }
    bool is_finite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

inline constexpr Vec3 operator*(float scalar, const Vec3& value) {
    return value * scalar;
}

} // namespace spoon::physics
