#include "spoon/physics/deformation/tissue_graph.hpp"

#include <cassert>
#include <cmath>

using namespace spoon::physics;

int main() {
    TissueGraph graph(3, 2);
    graph.gravity = {};
    graph.constraint_iterations = 12;
    graph.nodes[0].position = {0.0f, 0.0f, 0.0f};
    graph.nodes[1].position = {1.0f, 0.0f, 0.0f};
    graph.nodes[2].position = {2.0f, 0.0f, 0.0f};
    graph.nodes[0].inverse_mass = 0.0f;
    graph.nodes[1].inverse_mass = 1.0f;
    graph.nodes[2].inverse_mass = 1.0f;

    graph.links[0] = {0, 1, 1.0f, 0.0f, 0.12f};
    graph.links[1] = {1, 2, 1.0f, 0.0f, 0.12f};

    assert(graph.apply_impulse(1, {0.0f, 2.0f, 0.0f}));
    for (int i = 0; i < 8; ++i) graph.step(1.0f / 120.0f);
    assert(graph.nodes[1].position.y > 0.0f);
    assert(graph.nodes[2].position.y > 0.0f);
    assert(graph.nodes[0].position.length_squared() == 0.0f);

    TissueGraph radial(5, 0);
    radial.gravity = {};
    radial.nodes[0].position = {0.0f, 0.0f, 0.0f};
    radial.nodes[1].position = {0.5f, 0.0f, 0.0f};
    radial.nodes[2].position = {-0.5f, 0.0f, 0.0f};
    radial.nodes[3].position = {0.0f, 0.5f, 0.0f};
    radial.nodes[4].position = {2.0f, 0.0f, 0.0f};
    radial.nodes[0].inverse_mass = 0.0f;
    assert(radial.apply_radial_impulse({}, 1.0f, 3.0f) == 3);
    assert(radial.nodes[1].velocity.x > 0.0f);
    assert(radial.nodes[2].velocity.x < 0.0f);
    assert(radial.nodes[3].velocity.y > 0.0f);
    assert(radial.nodes[4].velocity.length_squared() == 0.0f);

    TissueGraph fixed(1, 0);
    fixed.gravity = {0.0f, -10.0f, 0.0f};
    fixed.nodes[0].inverse_mass = 0.0f;
    fixed.step(1.0f / 60.0f);
    assert(fixed.nodes[0].position.length_squared() == 0.0f);

    TissueGraph repeat_a(2, 1);
    TissueGraph repeat_b(2, 1);
    repeat_a.gravity = repeat_b.gravity = {};
    for (TissueGraph* current : {&repeat_a, &repeat_b}) {
        current->nodes[0].position = {0.0f, 0.0f, 0.0f};
        current->nodes[1].position = {1.0f, 0.0f, 0.0f};
        current->links[0] = {0, 1, 1.0f, 0.0001f, 0.1f};
        current->apply_impulse(1, {0.0f, 1.0f, 0.0f});
    }
    for (int i = 0; i < 4; ++i) {
        repeat_a.step(1.0f / 120.0f);
        repeat_b.step(1.0f / 120.0f);
    }
    assert(std::fabs(repeat_a.nodes[1].position.y -
                     repeat_b.nodes[1].position.y) < 1.0e-7f);
    return 0;
}
