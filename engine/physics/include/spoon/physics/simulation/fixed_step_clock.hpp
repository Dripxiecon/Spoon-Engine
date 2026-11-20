#pragma once

#include <algorithm>
#include <cmath>

namespace spoon::physics {

class FixedStepClock final {
public:
    explicit FixedStepClock(float fixed_delta = 1.0f / 120.0f,
                            int max_substeps = 8,
                            float max_frame_delta = 0.25f)
        : fixed_delta_(valid_positive(fixed_delta, 1.0f / 120.0f)),
          max_substeps_(std::max(1, max_substeps)),
          max_frame_delta_(valid_positive(max_frame_delta, 0.25f)) {}

    int consume(float frame_delta) {
        if (!std::isfinite(frame_delta) || frame_delta <= 0.0f) return 0;
        accumulator_ += std::min(frame_delta, max_frame_delta_);
        int steps = 0;
        while (accumulator_ >= fixed_delta_ && steps < max_substeps_) {
            accumulator_ -= fixed_delta_;
            ++steps;
        }
        // Drop excessive backlog rather than entering a permanent catch-up spiral.
        if (steps == max_substeps_ && accumulator_ >= fixed_delta_)
            accumulator_ = std::fmod(accumulator_, fixed_delta_);
        return steps;
    }

    float fixed_delta() const { return fixed_delta_; }
    float interpolation_alpha() const {
        return std::clamp(accumulator_ / fixed_delta_, 0.0f, 1.0f);
    }
    void reset() { accumulator_ = 0.0f; }

private:
    static float valid_positive(float value, float fallback) {
        return std::isfinite(value) && value > 0.0f ? value : fallback;
    }

    float fixed_delta_;
    int max_substeps_;
    float max_frame_delta_;
    float accumulator_{0.0f};
};

} // namespace spoon::physics
