#pragma once

#include "spoon/physics/muscles/muscle_unit.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

enum class MuscleGroupRole {
    agonist,
    antagonist,
    stabilizer,
    custom
};

struct MuscleGroupMember {
    std::size_t muscle_index{0};
    float weight{1.0f};
};

class MuscleGroup final {
public:
    MuscleGroupRole role{MuscleGroupRole::custom};
    std::vector<MuscleGroupMember> members;
    float activation_rate{10.0f};

    float activation() const { return activation_; }
    float target_activation() const { return target_activation_; }

    void set_target_activation(float value) {
        if (std::isfinite(value))
            target_activation_ = std::clamp(value, 0.0f, 1.0f);
    }

    void step(float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f) return;
        const float rate = std::isfinite(activation_rate)
            ? std::max(0.0f, activation_rate) : 0.0f;
        activation_ += (target_activation_ - activation_) *
                       (1.0f - std::exp(-rate * delta_time));
        activation_ = std::clamp(activation_, 0.0f, 1.0f);
    }

private:
    float activation_{0.0f};
    float target_activation_{0.0f};
};

class MuscleGroupController final {
public:
    explicit MuscleGroupController(std::size_t muscle_count = 0)
        : activation_sums_(muscle_count) {}

    std::vector<MuscleGroup> groups;

    // Call during rig setup, not during the simulation step.
    void prepare(std::size_t muscle_count) {
        activation_sums_.assign(muscle_count, 0.0f);
    }

    bool step(std::vector<MuscleUnit>& muscles, float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f ||
            activation_sums_.size() != muscles.size()) return false;

        std::fill(activation_sums_.begin(), activation_sums_.end(), 0.0f);
        for (MuscleGroup& group : groups) {
            group.step(delta_time);
            const float group_activation = group.activation();
            for (const MuscleGroupMember& member : group.members) {
                if (member.muscle_index >= muscles.size() ||
                    !std::isfinite(member.weight) || member.weight <= 0.0f)
                    continue;
                activation_sums_[member.muscle_index] +=
                    group_activation * std::min(member.weight, 1.0f);
            }
        }

        for (std::size_t i = 0; i < muscles.size(); ++i) {
            if (activation_sums_[i] > 0.0f)
                muscles[i].set_activation(
                    std::clamp(activation_sums_[i], 0.0f, 1.0f));
            muscles[i].step_activation(delta_time);
        }
        return true;
    }

private:
    std::vector<float> activation_sums_;
};

} // namespace spoon::physics
