#include <jni.h>

#include "spoon/physics/muscles/muscle_rig.hpp"

#include <memory>
#include <sstream>
#include <string>

namespace {

using namespace spoon::physics;

struct DemoState {
    BoneSolver solver{2, 0};
    MuscleRig rig{1, 1};

    DemoState() {
        solver.gravity = {};
        solver.bodies[0].pose.translation = {0.0f, 0.0f, 0.0f};
        solver.bodies[1].pose.translation = {1.0f, 0.0f, 0.0f};

        MuscleSettings settings{};
        settings.max_force = 90.0f;
        settings.optimal_length = 0.70f;
        settings.force_length_width = 0.45f;
        settings.passive_slack_length = 1.25f;
        settings.passive_stiffness = 35.0f;
        settings.damping = 4.0f;
        rig.muscles[0].set_settings(settings);

        rig.actuators[0].muscle_index = 0;
        rig.actuators[0].attachment.origin_bone = 0;
        rig.actuators[0].attachment.origin_local = {0.0f, 0.18f, 0.0f};
        rig.actuators[0].attachment.insertion_bone = 1;
        rig.actuators[0].attachment.insertion_local = {0.0f, 0.18f, 0.0f};

        MuscleGroup flexor{};
        flexor.role = MuscleGroupRole::agonist;
        flexor.members = {{0, 1.0f}};
        rig.groups.groups.push_back(flexor);
        rig.prepare(solver);
    }

    bool step(float activation, int frames) {
        if (frames < 1 || frames > 600) return false;
        rig.groups.groups[0].set_target_activation(activation);
        constexpr float dt = 1.0f / 120.0f;
        for (int i = 0; i < frames; ++i) {
            if (!rig.step(solver, dt)) return false;
            solver.step(dt);
        }
        return true;
    }

    std::string status() const {
        const Vec3 delta = solver.bodies[1].pose.translation -
                           solver.bodies[0].pose.translation;
        std::ostringstream out;
        out.setf(std::ios::fixed);
        out.precision(3);
        out << "SPOON PHYSICS LAB\n\n"
            << "Muscle activation: " << rig.muscles[0].activation() * 100.0f << "%\n"
            << "Muscle tension: " << (rig.muscles[0].activation() * 90.0f) << " N (approx.)\n\n"
            << "Bone A: " << solver.bodies[0].pose.translation.x << ", "
            << solver.bodies[0].pose.translation.y << ", "
            << solver.bodies[0].pose.translation.z << "\n"
            << "Bone B: " << solver.bodies[1].pose.translation.x << ", "
            << solver.bodies[1].pose.translation.y << ", "
            << solver.bodies[1].pose.translation.z << "\n\n"
            << "Attachment distance: " << delta.length() << " m\n"
            << "Simulation: fixed 120 Hz\n"
            << "Bodies: 2 | Muscle actuators: 1";
        return out.str();
    }

    void scene_state(float* out) const {
        const Vec3 a = solver.bodies[0].pose.translation;
        const Vec3 b = solver.bodies[1].pose.translation;
        const Vec3 delta = b - a;
        out[0] = a.x; out[1] = a.y; out[2] = a.z;
        out[3] = b.x; out[4] = b.y; out[5] = b.z;
        out[6] = rig.muscles[0].activation();
        out[7] = rig.muscles[0].activation() * 90.0f;
        out[8] = delta.length();
    }
};

std::unique_ptr<DemoState> g_demo;

jstring to_java_string(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_spoonengine_demo_MainActivity_nativeReset(JNIEnv* env, jclass) {
    g_demo = std::make_unique<DemoState>();
    return to_java_string(env, g_demo->status());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_spoonengine_demo_MainActivity_nativeStep(
    JNIEnv* env, jclass, jfloat activation, jint frames) {
    if (!g_demo) g_demo = std::make_unique<DemoState>();
    if (!g_demo->step(activation, frames))
        return to_java_string(env, "Simulation rejected the step. Reset and try again.");
    return to_java_string(env, g_demo->status());
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_spoonengine_demo_MainActivity_nativeSceneState(JNIEnv* env, jclass) {
    if (!g_demo) g_demo = std::make_unique<DemoState>();
    jfloat values[9]{};
    g_demo->scene_state(values);
    jfloatArray result = env->NewFloatArray(9);
    if (result) env->SetFloatArrayRegion(result, 0, 9, values);
    return result;
}
