#include "spoon/physics/deformation/tissue_graph.hpp"
#include <cassert>
#include <cmath>
using namespace spoon::physics;
static float volume(const TissueGraph& g) {
    const Vec3 a=g.nodes[0].position;
    const Vec3 b=g.nodes[1].position-a, c=g.nodes[2].position-a, d=g.nodes[3].position-a;
    return (b.x*(c.y*d.z-c.z*d.y)+b.y*(c.z*d.x-c.x*d.z)+b.z*(c.x*d.y-c.y*d.x))/6.0f;
}
int main() {
    TissueGraph g(4,0,1);
    g.gravity={}; g.constraint_iterations=16;
    g.nodes[0].position={0,0,0}; g.nodes[1].position={1,0,0};
    g.nodes[2].position={0,1,0}; g.nodes[3].position={0,0,0.5f};
    g.nodes[0].inverse_mass=0; g.nodes[1].inverse_mass=0;
    g.nodes[2].inverse_mass=0; g.nodes[3].inverse_mass=1;
    g.tetrahedra[0]={0,1,2,3,1.0f/6.0f,0.0f};
    g.step(1.0f/120.0f);
    assert(std::fabs(volume(g)-1.0f/6.0f)<1.0e-4f);
    assert(g.nodes[3].position.z>0.5f);
    assert(g.nodes[0].position.length_squared()==0.0f);
    assert(g.nodes[1].position.x==1.0f && g.nodes[2].position.y==1.0f);
    TissueGraph invalid(4,0,1);
    invalid.gravity={}; invalid.tetrahedra[0]={0,1,2,3,0.0f,0.0f};
    invalid.step(1.0f/120.0f);
    assert(invalid.nodes[0].position.length_squared()==0.0f);
    return 0;
}