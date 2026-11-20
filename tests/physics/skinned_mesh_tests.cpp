#include "spoon/physics/deformation/skinned_mesh.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace spoon::physics;

int main() {
    BonePose pose;
    pose.translation = {2.0f, 0.0f, 0.0f};
    pose.scale = {2.0f, 1.0f, 1.0f};

    SkinVertex vertex;
    vertex.bind_position = {1.0f, 0.0f, 0.0f};
    vertex.influences[0] = {0, 1.0f, {1.0f, 0.0f, 0.0f}};

    const Vec3 stretched = SkinnedMeshDeformer::deform_vertex(vertex, {pose});
    assert(std::abs(stretched.x - 4.0f) < 1.0e-5f);

    pose.scale = {0.5f, 1.0f, 1.0f};
    const Vec3 squashed = SkinnedMeshDeformer::deform_vertex(vertex, {pose});
    assert(std::abs(squashed.x - 2.5f) < 1.0e-5f);

    SkinVertex unbound;
    unbound.bind_position = {7.0f, 8.0f, 9.0f};
    const Vec3 unchanged = SkinnedMeshDeformer::deform_vertex(unbound, {pose});
    assert(std::abs(unchanged.x - 7.0f) < 1.0e-5f);

    std::vector<Vec3> output;
    SkinnedMeshDeformer::deform({vertex, unbound}, {pose}, output);
    assert(output.size() == 2);
    std::cout << "Spoon skinned mesh tests passed\n";
}
