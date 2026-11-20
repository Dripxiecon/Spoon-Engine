#include "spoon/physics/muscles/muscle_unit.hpp"

#include <algorithm>
#include <cmath>

namespace spoon::physics {
namespace {
float non_negative(float value, float fallback) {
    return std::isfinite(value) ? std::max(0.0f, value) : fallback;
}
}

MuscleUnit::MuscleUnit(MuscleSettings settings) {
    set_settings(settings);
}

void MuscleUnit::set_settings(const MuscleSettings& settings) {
    settings_.max_force = non_negative(settings.max_force, 120.0f);
    settings_.optimal_length = std::max(0.001f, non_negative(settings.optimal_length, 0.30f));
    settings_.force_length_width = std::max(0.001f, non_negative(settings.force_length_width, 0.45f));
    settings_.passive_slack_length = non_negative(settings.passive_slack_length, 0.34f);
    settings_.passive_stiffness = non_negative(settings.passive_stiffness, 180.0f);
    settings_.damping = non_negative(settings.damping, 8.0f);
    settings_.activation_rate = non_negative(settings.activation_rate, 8.0f);
    settings_.deactivation_rate = non_negative(settings.deactivation_rate, 12.0f);
}

void MuscleUnit::set_activation(float activation) {
    if (std::isfinite(activation))
        target_activation_ = std::clamp(activation, 0.0f, 1.0f);
}

void MuscleUnit::step_activation(float delta_time) {
    if (!std::isfinite(delta_time) || delta_time <= 0.0f) return;
    const float rate = target_activation_ > activation_
        ? settings_.activation_rate : settings_.deactivation_rate;
    const float blend = 1.0f - std::exp(-rate * delta_time);
    activation_ += (target_activation_ - activation_) * blend;
    activation_ = std::clamp(activation_, 0.0f, 1.0f);
}

MuscleForces MuscleUnit::evaluate(const Vec3& origin_position,
                                  const Vec3& insertion_position,
                                  const Vec3& origin_velocity,
                                  const Vec3& insertion_velocity) const {
    MuscleForces result{};
    result.activation = activation_;
    if (!origin_position.is_finite() || !insertion_position.is_finite() ||
        !origin_velocity.is_finite() || !insertion_velocity.is_finite()) return result;

    const Vec3 delta = insertion_position - origin_position;
    const float length_squared = delta.length_squared();
    if (length_squared <= 1.0e-12f) return result;

    result.length = std::sqrt(length_squared);
    const Vec3 direction = delta / result.length;
    const float normalized_offset =
        (result.length - settings_.optimal_length) / settings_.force_length_width;
    const float force_length = std::exp(-normalized_offset * normalized_offset);

    const float length_rate = (insertion_velocity - origin_velocity).x * direction.x
                            + (insertion_velocity - origin_velocity).y * direction.y
                            + (insertion_velocity - origin_velocity).z * direction.z;
    const float active_tension = settings_.max_force * activation_ * force_length;
    const float stretch = std::max(0.0f, result.length - settings_.passive_slack_length);
    const float passive_tension = settings_.passive_stiffness * stretch;
    const float damping_tension = settings_.damping * length_rate;
    result.tension = std::max(0.0f, active_tension + passive_tension + damping_tension);

    // A muscle can pull, but cannot push its attachment points apart.
    const Vec3 force = direction * result.tension;
    result.origin = force;
    result.insertion = force * -1.0f;
    return result;
}

} // namespace spoon::physics
