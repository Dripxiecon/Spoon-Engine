#include "spoon/physics/deformation/tissue_surface.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using namespace spoon::physics;
int main() {
    std::vector<TissueNode> nodes(4);
    nodes[0].position={0,0,0}; nodes[1].position={1,0,0};
    nodes[2].position={0,1,0}; nodes[3].position={1,1,0};
    const std::vector<TissueSurfaceTriangle> tris{{0,1,2},{1,3,2}};
    std::vector<TissueDihedralConstraint> hinges;
    assert(build_tissue_surface_dihedrals(nodes,tris,0.0001f,hinges));
    assert(hinges.size()==1);
    assert(std::fabs(hinges[0].rest_angle)<1.0e-5f);
    // A hinge must reject inconsistent winding instead of producing a bad normal pair.
    const std::vector<TissueSurfaceTriangle> bad{{0,1,2},{1,2,3}};
    assert(build_tissue_surface_dihedrals(nodes,bad,0.0f,hinges));
    assert(hinges.empty());
    assert(!build_tissue_surface_dihedrals(nodes,tris,-1.0f,hinges));
    assert(hinges.empty());
    // The graph solver restores a folded hinge toward its captured rest angle.
    TissueGraph graph(4,0,0,0,1);
    graph.nodes=nodes;
    graph.dihedrals=std::vector<TissueDihedralConstraint>(1);
    graph.dihedrals[0]=TissueDihedralConstraint{0,1,2,3,0.0f,0.0f,0.0f,true};
    graph.nodes[3].position.z=0.35f;
    graph.nodes[0].inverse_mass=0; graph.nodes[1].inverse_mass=0;
    graph.nodes[2].inverse_mass=0;
    graph.constraint_iterations=12;
    graph.step(1.0f/120.0f);
    assert(std::fabs(graph.nodes[3].position.z-0.35f)>1.0e-4f);
    return 0;
}
