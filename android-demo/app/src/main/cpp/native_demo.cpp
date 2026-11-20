#include <jni.h>

#include "spoon/physics/muscles/muscle_rig.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>

namespace {

using namespace spoon::physics;

struct DemoState {
    BoneSolver solver{2, 1};
    MuscleRig rig{1, 1};

    DemoState() {
        solver.gravity = {};
        solver.constraint_iterations = 12;

        solver.bodies[0] = BoneBody::make_static();
        solver.bodies[0].pose.translation = {-0.42f, 0.0f, 0.0f};

        solver.bodies[1].pose.translation = {0.42f, 0.0f, 0.0f};
        solver.bodies[1].inverse_inertia_local = {1.5f, 1.5f, 1.5f};

        BoneJoint& elbow = solver.joints[0];
        elbow.body_a = 0;
        elbow.body_b = 1;
        elbow.local_anchor_a = {0.42f, 0.0f, 0.0f};
        elbow.local_anchor_b = {-0.42f, 0.0f, 0.0f};
        elbow.compliance = 1.0e-7f;

        MuscleSettings settings{};
        settings.max_force = 58.0f;
        settings.optimal_length = 0.82f;
        settings.force_length_width = 0.48f;
        settings.passive_slack_length = 1.05f;
        settings.passive_stiffness = 22.0f;
        settings.damping = 3.5f;
        settings.activation_rate = 5.0f;
        settings.deactivation_rate = 7.0f;
        rig.muscles[0].set_settings(settings);

        rig.actuators[0].muscle_index = 0;
        rig.actuators[0].attachment.origin_bone = 0;
        rig.actuators[0].attachment.origin_local = {-0.18f, 0.16f, 0.0f};
        rig.actuators[0].attachment.insertion_bone = 1;
        rig.actuators[0].attachment.insertion_local = {-0.18f, 0.16f, 0.0f};

        MuscleGroup flexor{};
        flexor.role = MuscleGroupRole::agonist;
        flexor.members = {{0, 1.0f}};
        rig.groups.groups.push_back(flexor);
        rig.prepare(solver);
    }

    bool step(float activation, int frames) {
        if (frames < 1 || frames > 600 || !std::isfinite(activation)) return false;
        rig.groups.groups[0].set_target_activation(std::clamp(activation, 0.0f, 1.0f));
        constexpr float dt = 1.0f / 120.0f;
        for (int i = 0; i < frames; ++i) {
            if (!rig.step(solver, dt)) return false;
            solver.step(dt);
            BoneBody& forearm = solver.bodies[1];
            forearm.angular_velocity *= 0.997f;
            forearm.linear_velocity *= 0.995f;
        }
        return true;
    }

    Vec3 muscle_origin() const {
        return solver.bodies[0].pose.transform_point(
            rig.actuators[0].attachment.origin_local);
    }

    Vec3 muscle_insertion() const {
        return solver.bodies[1].pose.transform_point(
            rig.actuators[0].attachment.insertion_local);
    }

    MuscleForces muscle_forces() const {
        return rig.muscles[0].evaluate(muscle_origin(), muscle_insertion());
    }

    std::string status() const {
        const MuscleForces forces = muscle_forces();
        const Quaternion q = solver.bodies[1].pose.rotation.normalized();
        std::ostringstream out;
        out.setf(std::ios::fixed);
        out.precision(3);
        out << "SPOON 3D PHYSICS LAB\n\n"
            << "Muscle activation: " << rig.muscles[0].activation() * 100.0f << "%\n"
            << "Muscle tension: " << forces.tension << " N\n"
            << "Muscle length: " << forces.length << " m\n\n"
            << "Elbow joint: connected\n"
            << "Forearm rotation: " << q.w << ", " << q.x << ", "
            << q.y << ", " << q.z << "\n"
            << "Simulation: fixed 120 Hz\n"
            << "3D meshes: OpenGL ES 2.0";
        return out.str();
    }

    void scene_state(float* out) const {
        const BoneBody& upper = solver.bodies[0];
        const BoneBody& forearm = solver.bodies[1];
        const Quaternion qa = upper.pose.rotation.normalized();
        const Quaternion qb = forearm.pose.rotation.normalized();
        const Vec3 origin = muscle_origin();
        const Vec3 insertion = muscle_insertion();
        const MuscleForces forces = muscle_forces();

        out[0] = upper.pose.translation.x;
        out[1] = upper.pose.translation.y;
        out[2] = upper.pose.translation.z;
        out[3] = qa.w; out[4] = qa.x; out[5] = qa.y; out[6] = qa.z;
        out[7] = forearm.pose.translation.x;
        out[8] = forearm.pose.translation.y;
        out[9] = forearm.pose.translation.z;
        out[10] = qb.w; out[11] = qb.x; out[12] = qb.y; out[13] = qb.z;
        out[14] = origin.x; out[15] = origin.y; out[16] = origin.z;
        out[17] = insertion.x; out[18] = insertion.y; out[19] = insertion.z;
        out[20] = rig.muscles[0].activation();
        out[21] = forces.tension;
        out[22] = forces.length;
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
    jfloat values[23]{};
    g_demo->scene_state(values);
    jfloatArray result = env->NewFloatArray(23);
    if (result) env->SetFloatArrayRegion(result, 0, 23, values);
    return result;
}
