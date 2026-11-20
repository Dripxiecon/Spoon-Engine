#include "spoon/physics/deformation/tissue_binding.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace spoon::physics;

int main() {
    TissueGraph graph(3, 2);
    graph.gravity = {};
    graph.constraint_iterations = 16;
    graph.nodes[0].position = {};
    graph.nodes[1].position = {1.0f, 0.0f, 0.0f};
    // A bent chain transfers transverse motion through its second link.
    // A perfectly straight distance-only chain has no first-order bending resistance.
    graph.nodes[2].position = {1.0f, 1.0f, 0.0f};
    graph.links[0] = {0, 1, 1.0f, 0.0f, 0.08f};
    graph.links[1] = {1, 2, 1.0f, 0.0f, 0.08f};

    TissueBindingSet bindings(1);
    bindings.bindings[0] = {0, 0, {}, {}, false};

    std::vector<BonePose> bones(1);
    assert(bindings.update_anchors(graph, bones, 1.0f / 120.0f));
    assert(graph.nodes[0].inverse_mass == 0.0f);
    assert(graph.nodes[0].position.length_squared() == 0.0f);

    bones[0].translation = {0.0f, 0.5f, 0.0f};
    assert(bindings.update_anchors(graph, bones, 1.0f / 120.0f));
    assert(std::fabs(graph.nodes[0].position.y - 0.5f) < 1.0e-6f);
    assert(graph.nodes[0].velocity.y > 0.0f);

    for (int i = 0; i < 8; ++i) graph.step(1.0f / 120.0f);
    assert(graph.nodes[1].position.y > 0.0f);
    assert(std::fabs(graph.nodes[2].position.y - 1.0f) > 1.0e-5f);
    assert(std::fabs(graph.nodes[0].position.y - 0.5f) < 1.0e-6f);

    // Rotated and translated bone-local anchors are transformed consistently.
    BonePose transformed{};
    transformed.translation = {2.0f, 1.0f, 0.0f};
    transformed.scale = {2.0f, 1.0f, 1.0f};
    TissueGraph second_graph(1, 0);
    TissueBindingSet second_bindings(1);
    second_bindings.bindings[0] = {0, 0, {1.0f, 0.0f, 0.0f}, {}, false};
    assert(second_bindings.update_anchors(
        second_graph, std::vector<BonePose>{transformed}, 1.0f / 60.0f));
    assert(std::fabs(second_graph.nodes[0].position.x - 4.0f) < 1.0e-6f);
    assert(std::fabs(second_graph.nodes[0].position.y - 1.0f) < 1.0e-6f);

    // Invalid bindings fail without partially changing the graph.
    TissueGraph invalid_graph(1, 0);
    TissueBindingSet invalid_bindings(1);
    invalid_bindings.bindings[0] = {2, 0, {}, {}, false};
    const Vec3 before = invalid_graph.nodes[0].position;
    assert(!invalid_bindings.update_anchors(
        invalid_graph, bones, 1.0f / 120.0f));
    assert(invalid_graph.nodes[0].position.x == before.x);
    assert(invalid_graph.nodes[0].position.y == before.y);
    return 0;
}
