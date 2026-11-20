#include "spoon/physics/deformation/tissue_surface.hpp"

#include <cassert>
#include <cmath>
#include <vector>

using namespace spoon::physics;

int main() {
    std::vector<TissueNode> nodes(4);
    nodes[0].position = {0.0f, 0.0f, 0.0f};
    nodes[1].position = {1.0f, 0.0f, 0.0f};
    nodes[2].position = {0.0f, 1.0f, 0.0f};
    nodes[3].position = {1.0f, 1.0f, 0.0f};

    const std::vector<TissueSurfaceTriangle> square{
        {0, 1, 2}, {1, 3, 2}
    };
    std::vector<TissueBendConstraint> bends;
    assert(build_tissue_surface_bends(nodes, square, 0.002f, bends));
    assert(bends.size() == 1);
    assert(std::fabs(bends[0].rest_distance - std::sqrt(2.0f)) < 1.0e-6f);
    assert(bends[0].compliance == 0.002f);

    // Boundary edges have one triangle and do not produce bend constraints.
    const std::vector<TissueSurfaceTriangle> single{{0, 1, 2}};
    assert(build_tissue_surface_bends(nodes, single, 0.0f, bends));
    assert(bends.empty());

    // Three triangles sharing one edge are non-manifold; that edge is skipped.
    const std::vector<TissueSurfaceTriangle> non_manifold{
        {0, 1, 2}, {1, 0, 3}, {0, 1, 3}
    };
    assert(build_tissue_surface_bends(nodes, non_manifold, 0.0f, bends));
    assert(bends.empty());

    // Invalid and degenerate triangles are ignored safely.
    const std::vector<TissueSurfaceTriangle> malformed{
        {0, 0, 1}, {0, 1, 99}, {0, 1, 2}
    };
    assert(build_tissue_surface_bends(nodes, malformed, 0.0f, bends));
    assert(bends.empty());

    assert(!build_tissue_surface_bends(nodes, square, -1.0f, bends));
    assert(bends.empty());
    return 0;
}
