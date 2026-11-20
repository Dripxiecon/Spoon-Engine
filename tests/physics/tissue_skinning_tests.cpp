#include "spoon/physics/deformation/tissue_skinning.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace spoon::physics;

int main() {
    std::vector<SkinVertex> vertices(1);
    vertices[0].bind_position = {1.0f, 0.0f, 0.0f};
    vertices[0].influences[0] = {0, 1.0f, {1.0f, 0.0f, 0.0f}};

    std::vector<BonePose> bones(1);
    bones[0].translation = {10.0f, 0.0f, 0.0f};

    std::vector<TissueNode> tissue(2);
    tissue[0].position = {0.0f, 0.5f, 0.0f};
    tissue[1].position = {0.0f, 0.0f, 0.4f};

    const std::vector<TissueVertexBinding> bindings{
        {0, 0, {0.0f, 0.0f, 0.0f}, 0.5f},
        {0, 1, {0.0f, 0.0f, 0.0f}, 0.25f}
    };

    std::vector<Vec3> output(1);
    assert(TissueSkinningBridge::deform(vertices, bones, tissue, bindings, output));
    assert(std::fabs(output[0].x - 11.0f) < 1.0e-6f);
    assert(std::fabs(output[0].y - 0.25f) < 1.0e-6f);
    assert(std::fabs(output[0].z - 0.1f) < 1.0e-6f);

    // A later tissue step changes only the added tissue offset.
    tissue[0].position.y = 0.8f;
    assert(TissueSkinningBridge::deform(vertices, bones, tissue, bindings, output));
    assert(std::fabs(output[0].y - 0.4f) < 1.0e-6f);

    // Invalid bindings fail before overwriting the caller's output.
    std::vector<Vec3> unchanged{{7.0f, 8.0f, 9.0f}};
    const std::vector<TissueVertexBinding> invalid{{4, 0, {}, 1.0f}};
    assert(!TissueSkinningBridge::deform(
        vertices, bones, tissue, invalid, unchanged));
    assert(unchanged[0].x == 7.0f);
    assert(unchanged[0].y == 8.0f);
    assert(unchanged[0].z == 9.0f);

    // The output buffer must be correctly sized by the caller.
    std::vector<Vec3> wrong_size;
    assert(!TissueSkinningBridge::deform(
        vertices, bones, tissue, bindings, wrong_size));
    return 0;
}
