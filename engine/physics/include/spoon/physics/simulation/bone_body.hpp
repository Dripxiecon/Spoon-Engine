#pragma once

#include "spoon/physics/bones/bone_pose.hpp"

#include <algorithm>
#include <cmath>

namespace spoon::physics {

inline Quaternion multiply(const Quaternion& a, const Quaternion& b) {
    return {
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w
    };
}

struct BoneBody final {
    BonePose pose{};
    Vec3 linear_velocity{};
    Vec3 angular_velocity{};
    Vec3 accumulated_force{};
    Vec3 accumulated_torque{};
    float inverse_mass{1.0f};
    Vec3 inverse_inertia_local{1.0f, 1.0f, 1.0f};
    bool enabled{true};

    static BoneBody make_static(const BonePose& initial_pose = {}) {
        BoneBody body{};
        body.pose = initial_pose;
        body.inverse_mass = 0.0f;
        body.inverse_inertia_local = {};
        return body;
    }

    bool is_dynamic() const { return enabled && inverse_mass > 0.0f; }

    Vec3 inverse_inertia_world(const Vec3& world_vector) const {
        const Quaternion rotation = pose.rotation.normalized();
        const Vec3 local = rotation.conjugate().rotate(world_vector);
        const Vec3 scaled{local.x * inverse_inertia_local.x,
                          local.y * inverse_inertia_local.y,
                          local.z * inverse_inertia_local.z};
        return rotation.rotate(scaled);
    }

    void apply_force(const Vec3& force) {
        if (is_dynamic() && force.is_finite()) accumulated_force += force;
    }

    void apply_torque(const Vec3& torque) {
        if (is_dynamic() && torque.is_finite()) accumulated_torque += torque;
    }

    void apply_force_at_point(const Vec3& force, const Vec3& world_point) {
        if (!force.is_finite() || !world_point.is_finite()) return;
        apply_force(force);
        apply_torque(cross_product(world_point - pose.translation, force));
    }

    void integrate(float delta_time, const Vec3& gravity = {}) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f) {
            clear_accumulators();
            return;
        }
        if (is_dynamic()) {
            const Vec3 acceleration = gravity + accumulated_force * inverse_mass;
            linear_velocity += acceleration * delta_time;
            pose.translation += linear_velocity * delta_time;

            const Vec3 angular_acceleration = inverse_inertia_world(accumulated_torque);
            angular_velocity += angular_acceleration * delta_time;
            const Quaternion omega{0.0f, angular_velocity.x,
                                   angular_velocity.y, angular_velocity.z};
            const Quaternion derivative = multiply(omega, pose.rotation);
            const float half_delta = 0.5f * delta_time;
            pose.rotation = Quaternion{
                pose.rotation.w + derivative.w * half_delta,
                pose.rotation.x + derivative.x * half_delta,
                pose.rotation.y + derivative.y * half_delta,
                pose.rotation.z + derivative.z * half_delta
            }.normalized();
        }
        clear_accumulators();
    }

    void clear_accumulators() {
        accumulated_force = {};
        accumulated_torque = {};
    }

private:
    static Vec3 cross_product(const Vec3& a, const Vec3& b) {
        return {a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x};
    }
};

} // namespace spoon::physics
