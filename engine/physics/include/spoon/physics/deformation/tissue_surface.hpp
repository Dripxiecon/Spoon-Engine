#pragma once

#include "spoon/physics/deformation/tissue_graph.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace spoon::physics {

struct TissueSurfaceTriangle final {
    std::size_t node_a{0};
    std::size_t node_b{0};
    std::size_t node_c{0};
};

namespace detail {

struct TissueSurfaceEdge final {
    std::size_t low{0};
    std::size_t high{0};

    bool operator==(const TissueSurfaceEdge& other) const {
        return low == other.low && high == other.high;
    }
};

struct TissueSurfaceEdgeHash final {
    std::size_t operator()(const TissueSurfaceEdge& edge) const {
        const std::size_t h1 = std::hash<std::size_t>{}(edge.low);
        const std::size_t h2 = std::hash<std::size_t>{}(edge.high);
        return h1 ^ (h2 + static_cast<std::size_t>(0x9e3779b9U) +
                     (h1 << 6U) + (h1 >> 2U));
    }
};

struct TissueSurfaceEdgeUse final {
    std::size_t first_opposite{0};
    std::size_t second_opposite{0};
    unsigned int use_count{0};
    std::size_t first_edge_a{0};
    std::size_t first_edge_b{0};
};

} // namespace detail

// Build one inexpensive second-ring bend constraint for each manifold edge
// shared by exactly two triangles. Call during mesh setup, not in the frame loop.
inline bool build_tissue_surface_bends(
    const std::vector<TissueNode>& nodes,
    const std::vector<TissueSurfaceTriangle>& triangles,
    float compliance,
    std::vector<TissueBendConstraint>& output) {
    output.clear();
    if (!std::isfinite(compliance) || compliance < 0.0f) return false;

    std::unordered_map<detail::TissueSurfaceEdge,
                       detail::TissueSurfaceEdgeUse,
                       detail::TissueSurfaceEdgeHash> edges;
    edges.reserve(triangles.size() * 3U);

    for (const TissueSurfaceTriangle& triangle : triangles) {
        const std::size_t ids[3] = {
            triangle.node_a, triangle.node_b, triangle.node_c
        };
        if (ids[0] >= nodes.size() || ids[1] >= nodes.size() ||
            ids[2] >= nodes.size() || ids[0] == ids[1] ||
            ids[1] == ids[2] || ids[2] == ids[0]) continue;

        for (unsigned int side = 0; side < 3U; ++side) {
            const std::size_t a = ids[side];
            const std::size_t b = ids[(side + 1U) % 3U];
            const std::size_t opposite = ids[(side + 2U) % 3U];
            const detail::TissueSurfaceEdge key{
                std::min(a, b), std::max(a, b)
            };
            detail::TissueSurfaceEdgeUse& use = edges[key];
            if (use.use_count == 0U) {
                use.first_opposite = opposite;
                use.first_edge_a = a;
                use.first_edge_b = b;
                use.use_count = 1U;
            } else if (use.use_count == 1U) {
                use.second_opposite = opposite;
                use.use_count = 2U;
            } else {
                ++use.use_count;
            }
        }
    }

    output.reserve(edges.size());
    for (const auto& entry : edges) {
        const detail::TissueSurfaceEdgeUse& use = entry.second;
        if (use.use_count != 2U ||
            use.first_opposite == use.second_opposite) continue;

        const Vec3 delta = nodes[use.second_opposite].position -
                           nodes[use.first_opposite].position;
        const float distance_squared = delta.length_squared();
        if (!std::isfinite(distance_squared) || distance_squared <= 1.0e-12f)
            continue;

        TissueBendConstraint bend;
        bend.node_a = use.first_opposite;
        bend.node_b = use.second_opposite;
        bend.rest_distance = std::sqrt(distance_squared);
        bend.compliance = compliance;
        output.push_back(bend);
    }
    return true;
}

// Creates true signed-angle hinges. Adjacent triangles must have opposite
// directions along their shared edge (consistent surface winding).
inline bool build_tissue_surface_dihedrals(
    const std::vector<TissueNode>& nodes,
    const std::vector<TissueSurfaceTriangle>& triangles,
    float compliance,
    std::vector<TissueDihedralConstraint>& output) {
    output.clear();
    if (!std::isfinite(compliance) || compliance < 0.0f) return false;
    std::unordered_map<detail::TissueSurfaceEdge,
                       detail::TissueSurfaceEdgeUse,
                       detail::TissueSurfaceEdgeHash> edges;
    edges.reserve(triangles.size()*3U);
    for (const auto& tri : triangles) {
        const std::size_t ids[3]={tri.node_a,tri.node_b,tri.node_c};
        if (ids[0]>=nodes.size() || ids[1]>=nodes.size() ||
            ids[2]>=nodes.size() || ids[0]==ids[1] ||
            ids[1]==ids[2] || ids[2]==ids[0]) continue;
        for (unsigned int side=0;side<3U;++side) {
            const std::size_t a=ids[side], b=ids[(side+1U)%3U];
            const std::size_t opposite=ids[(side+2U)%3U];
            auto& use=edges[{std::min(a,b),std::max(a,b)}];
            if (use.use_count==0U) {
                use.first_opposite=opposite;
                use.first_edge_a=a; use.first_edge_b=b;
                use.use_count=1U;
            } else if (use.use_count==1U) {
                use.second_opposite=opposite;
                use.use_count=2U;
            } else ++use.use_count;
        }
    }
    output.reserve(edges.size());
    for (const auto& entry:edges) {
        const auto& use=entry.second;
        if (use.use_count!=2U || use.first_opposite==use.second_opposite)
            continue;
        // Winding must run in opposite directions along the shared edge.
        const auto found=std::find_if(triangles.begin(),triangles.end(),
            [&](const TissueSurfaceTriangle& t) {
                const std::size_t ids[3]={t.node_a,t.node_b,t.node_c};
                for (int i=0;i<3;++i)
                    if (ids[i]==use.first_edge_a &&
                        ids[(i+1)%3]==use.first_edge_b) return true;
                return false;
            });
        if (found==triangles.end()) continue;
        bool opposite_winding=false;
        for (const auto& tri:triangles) {
            const std::size_t ids[3]={tri.node_a,tri.node_b,tri.node_c};
            for (int i=0;i<3;++i)
                if (ids[i]==use.first_edge_b &&
                    ids[(i+1)%3]==use.first_edge_a &&
                    (ids[(i+2)%3]==use.second_opposite)) opposite_winding=true;
        }
        if (!opposite_winding) continue;
        TissueDihedralConstraint hinge;
        hinge.edge_a=use.first_edge_a;
        hinge.edge_b=use.first_edge_b;
        hinge.opposite_a=use.first_opposite;
        hinge.opposite_b=use.second_opposite;
        hinge.compliance=compliance;
        const Vec3 edge=nodes[hinge.edge_b].position-nodes[hinge.edge_a].position;
        const Vec3 r1{edge.y*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).z-
                      edge.z*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).y,
                      edge.z*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).x-
                      edge.x*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).z,
                      edge.x*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).y-
                      edge.y*(nodes[hinge.opposite_a].position-nodes[hinge.edge_a].position).x};
        const Vec3 da=nodes[hinge.opposite_b].position-nodes[hinge.edge_b].position;
        const Vec3 rev=nodes[hinge.edge_a].position-nodes[hinge.edge_b].position;
        const Vec3 r2{da.y*rev.z-da.z*rev.y,da.z*rev.x-da.x*rev.z,da.x*rev.y-da.y*rev.x};
        const float l1=r1.length_squared(), l2=r2.length_squared();
        if (edge.length_squared()<=1.0e-12f || l1<=1.0e-12f || l2<=1.0e-12f)
            continue;
        const Vec3 n1=r1/std::sqrt(l1), n2=r2/std::sqrt(l2);
        const Vec3 edir=edge/std::sqrt(edge.length_squared());
        const float sn=edir.x*(n1.y*n2.z-n1.z*n2.y)+
                       edir.y*(n1.z*n2.x-n1.x*n2.z)+
                       edir.z*(n1.x*n2.y-n1.y*n2.x);
        const float cs=std::clamp(n1.x*n2.x+n1.y*n2.y+n1.z*n2.z,-1.0f,1.0f);
        hinge.rest_angle=std::atan2(sn,cs);
        output.push_back(hinge);
    }
    return true;
}

} // namespace spoon::physics
