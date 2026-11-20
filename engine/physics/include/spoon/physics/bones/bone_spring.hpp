#pragma once

#include "spoon/physics/math/vec3.hpp"

namespace spoon::physics {

struct BoneSpringSettings {
    float stiffness{90.0f};
    float damping{12.0f};
    float mass{1.0f};
    float max_speed{100.0f};
};

struct BoneSpringState {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 target{};
    Vec3 external_acceleration{};
    bool active{true};
};

class BoneSpring final {
public:
    explicit BoneSpring(BoneSpringSettings settings = {});

    const BoneSpringSettings& settings() const { return settings_; }
    const BoneSpringState& state() const { return state_; }

    void set_settings(const BoneSpringSettings& settings);
    void teleport(const Vec3& position);
    void set_target(const Vec3& target);
    void add_impulse(const Vec3& impulse);
    void add_acceleration(const Vec3& acceleration);
    void clear_acceleration();
    void set_active(bool active);
    void step(float delta_time);

private:
    BoneSpringSettings settings_{};
    BoneSpringState state_{};
};

} // namespace spoon::physics
