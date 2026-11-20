#pragma once

#include "spoon/physics/math/vec3.hpp"

namespace spoon::physics {

struct MuscleSettings {
    float max_force{120.0f};
    float optimal_length{0.30f};
    float force_length_width{0.45f};
    float passive_slack_length{0.34f};
    float passive_stiffness{180.0f};
    float damping{8.0f};
    float activation_rate{8.0f};
    float deactivation_rate{12.0f};
};

struct MuscleForces {
    Vec3 origin{};
    Vec3 insertion{};
    float length{0.0f};
    float activation{0.0f};
    float tension{0.0f};
};

class MuscleUnit final {
public:
    explicit MuscleUnit(MuscleSettings settings = {});

    const MuscleSettings& settings() const { return settings_; }
    float activation() const { return activation_; }

    void set_settings(const MuscleSettings& settings);
    void set_activation(float activation);
    void step_activation(float delta_time);
    MuscleForces evaluate(const Vec3& origin_position,
                          const Vec3& insertion_position,
                          const Vec3& origin_velocity = {},
                          const Vec3& insertion_velocity = {}) const;

private:
    MuscleSettings settings_{};
    float activation_{0.0f};
    float target_activation_{0.0f};
};

} // namespace spoon::physics
