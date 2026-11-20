#include "spoon/physics/bones/bone_spring.hpp"

#include <algorithm>
#include <cmath>

namespace spoon::physics {
namespace {
float sane_non_negative(float value, float fallback) {
    return std::isfinite(value) ? std::max(0.0f, value) : fallback;
}
}

BoneSpring::BoneSpring(BoneSpringSettings settings) {
    set_settings(settings);
}

void BoneSpring::set_settings(const BoneSpringSettings& settings) {
    settings_.stiffness = sane_non_negative(settings.stiffness, 90.0f);
    settings_.damping = sane_non_negative(settings.damping, 12.0f);
    settings_.mass = std::isfinite(settings.mass) ? std::max(0.001f, settings.mass) : 1.0f;
    settings_.max_speed = std::isfinite(settings.max_speed)
        ? std::max(0.0f, settings.max_speed) : 100.0f;
}

void BoneSpring::teleport(const Vec3& position) {
    if (!position.is_finite()) return;
    state_.position = position;
    state_.velocity = {};
    state_.target = position;
}

void BoneSpring::set_target(const Vec3& target) {
    if (target.is_finite()) state_.target = target;
}

void BoneSpring::add_impulse(const Vec3& impulse) {
    if (impulse.is_finite()) state_.velocity += impulse / settings_.mass;
}

void BoneSpring::add_acceleration(const Vec3& acceleration) {
    if (acceleration.is_finite()) state_.external_acceleration += acceleration;
}

void BoneSpring::clear_acceleration() {
    state_.external_acceleration = {};
}

void BoneSpring::set_active(bool active) {
    state_.active = active;
    if (!active) {
        state_.velocity = {};
        state_.position = state_.target;
    }
}

void BoneSpring::step(float delta_time) {
    if (!state_.active || !std::isfinite(delta_time) || delta_time <= 0.0f) return;

    // Semi-implicit Euler: stable for small fixed steps and inexpensive on mobile.
    const Vec3 displacement = state_.target - state_.position;
    const Vec3 spring_force = displacement * settings_.stiffness;
    const Vec3 damping_force = state_.velocity * (-settings_.damping);
    const Vec3 acceleration =
        (spring_force + damping_force) / settings_.mass + state_.external_acceleration;

    state_.velocity += acceleration * delta_time;

    const float speed = state_.velocity.length();
    if (settings_.max_speed > 0.0f && speed > settings_.max_speed) {
        state_.velocity = state_.velocity * (settings_.max_speed / speed);
    }

    state_.position += state_.velocity * delta_time;
    if (!state_.position.is_finite() || !state_.velocity.is_finite()) {
        state_.position = state_.target;
        state_.velocity = {};
    }
}

} // namespace spoon::physics
