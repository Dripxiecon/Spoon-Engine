#include "spoon/physics/deformation/tissue_graph.hpp"

#include <cassert>
#include <cmath>

using namespace spoon::physics;

int main() {
    TissueGraph graph(3, 2, 0, 1);
    graph.gravity = {};
    graph.constraint_iterations = 16;
    graph.nodes[0].position = {0.0f, 0.0f, 0.0f};
    graph.nodes[1].position = {1.0f, 0.0f, 0.0f};
    graph.nodes[2].position = {2.0f, 0.0f, 0.0f};
    graph.links[0] = {0, 1, 1.0f, 0.0f, 0.0f};
    graph.links[1] = {1, 2, 1.0f, 0.0f, 0.0f};
    graph.bends[0] = {0, 2, 2.0f, 0.0f};

    assert(graph.apply_impulse(1, {0.0f, 4.0f, 0.0f}));
    for (int i = 0; i < 6; ++i) graph.step(1.0f / 120.0f);

    const Vec3 span = graph.nodes[2].position - graph.nodes[0].position;
    assert(std::fabs(std::sqrt(span.length_squared()) - 2.0f) < 0.03f);
    assert(graph.nodes[1].position.y > 0.01f);

    TissueGraph soft(3, 2, 0, 1);
    soft.gravity = {};
    soft.nodes[0].position = {0.0f, 0.0f, 0.0f};
    soft.nodes[1].position = {1.0f, 0.0f, 0.0f};
    soft.nodes[2].position = {2.0f, 0.0f, 0.0f};
    soft.links[0] = {0, 1, 1.0f, 0.0f, 0.0f};
    soft.links[1] = {1, 2, 1.0f, 0.0f, 0.0f};
    soft.bends[0] = {0, 2, 2.0f, 1000.0f};
    soft.apply_impulse(1, {0.0f, 2.0f, 0.0f});
    soft.step(1.0f / 120.0f);
    assert(soft.nodes[1].position.y > 0.0f);

    TissueGraph invalid(2, 0, 0, 1);
    invalid.gravity = {};
    invalid.nodes[0].position = {0.0f, 0.0f, 0.0f};
    invalid.nodes[1].position = {1.0f, 0.0f, 0.0f};
    invalid.bends[0] = {0, 9, 1.0f, 0.0f};
    invalid.step(1.0f / 120.0f);
    assert(invalid.nodes[0].position.is_finite());
    assert(invalid.nodes[1].position.is_finite());

    TissueGraph repeat_a(3, 2, 0, 1);
    TissueGraph repeat_b(3, 2, 0, 1);
    for (TissueGraph* current : {&repeat_a, &repeat_b}) {
        current->gravity = {};
        for (int i = 0; i < 3; ++i)
            current->nodes[static_cast<std::size_t>(i)].position =
                {static_cast<float>(i), 0.0f, 0.0f};
        current->links[0] = {0, 1, 1.0f, 0.0f, 0.0f};
        current->links[1] = {1, 2, 1.0f, 0.0f, 0.0f};
        current->bends[0] = {0, 2, 2.0f, 0.001f};
        current->apply_impulse(1, {0.0f, 1.0f, 0.0f});
    }
    for (int i = 0; i < 5; ++i) {
        repeat_a.step(1.0f / 120.0f);
        repeat_b.step(1.0f / 120.0f);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        assert(std::fabs(repeat_a.nodes[i].position.y -
                         repeat_b.nodes[i].position.y) < 1.0e-7f);
    }
    return 0;
}
