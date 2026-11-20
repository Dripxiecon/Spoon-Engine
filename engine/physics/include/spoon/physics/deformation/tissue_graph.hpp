#pragma once

#include "spoon/physics/math/vec3.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace spoon::physics {

struct TissueNode final {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 accumulated_force{};
    float inverse_mass{1.0f};
    bool enabled{true};

    bool is_dynamic() const {
        return enabled && std::isfinite(inverse_mass) && inverse_mass > 0.0f;
    }

    void apply_force(const Vec3& force) {
        if (is_dynamic() && force.is_finite()) accumulated_force += force;
    }

    void apply_impulse(const Vec3& impulse) {
        if (is_dynamic() && impulse.is_finite())
            velocity += impulse * inverse_mass;
    }
};

struct TissueTetrahedron final {
    std::size_t node_a{0}, node_b{0}, node_c{0}, node_d{0};
    float rest_volume{0.0f};
    float compliance{0.0f};
    float accumulated_lambda{0.0f};
    float minimum_volume_ratio{0.05f};
    float barrier_lambda{0.0f};
    bool enabled{true};
    void reset() { accumulated_lambda = 0.0f; barrier_lambda = 0.0f; }
};

struct TissueBendConstraint final {
    std::size_t node_a{0};
    std::size_t node_b{0};
    float rest_distance{0.0f};
    float compliance{0.001f};
    float accumulated_lambda{0.0f};
    bool enabled{true};
    void reset() { accumulated_lambda = 0.0f; }
};

// Four-node hinge joining two consistently wound surface triangles.
struct TissueDihedralConstraint final {
    std::size_t edge_a{0}, edge_b{0};
    std::size_t opposite_a{0}, opposite_b{0};
    float rest_angle{0.0f};
    float compliance{0.0005f};
    float accumulated_lambda{0.0f};
    bool enabled{true};
    void reset() { accumulated_lambda = 0.0f; }
};

struct TissueLink final {
    std::size_t node_a{0};
    std::size_t node_b{0};
    float rest_length{0.0f};
    float compliance{0.0f};
    float damping{0.08f};
    float accumulated_lambda{0.0f};
    bool enabled{true};

    void reset() { accumulated_lambda = 0.0f; }
};

class TissueGraph final {
public:
    explicit TissueGraph(std::size_t node_count = 0,
                         std::size_t link_count = 0,
                         std::size_t tetrahedron_count = 0,
                         std::size_t bend_count = 0,
                         std::size_t dihedral_count = 0)
        : nodes(node_count), links(link_count), tetrahedra(tetrahedron_count),
          bends(bend_count), dihedrals(dihedral_count),
          predicted_positions(node_count), previous_positions(node_count) {}

    std::vector<TissueNode> nodes;
    std::vector<TissueLink> links;
    std::vector<TissueTetrahedron> tetrahedra;
    std::vector<TissueBendConstraint> bends;
    std::vector<TissueDihedralConstraint> dihedrals;
    Vec3 gravity{};
    int constraint_iterations{6};

    bool apply_impulse(std::size_t node_index, const Vec3& impulse) {
        if (node_index >= nodes.size() || !impulse.is_finite()) return false;
        nodes[node_index].apply_impulse(impulse);
        return true;
    }

    std::size_t apply_radial_impulse(const Vec3& center, float radius,
                                     float strength) {
        if (!center.is_finite() || !std::isfinite(radius) || radius <= 0.0f ||
            !std::isfinite(strength)) return 0;
        std::size_t affected = 0;
        const float radius_squared = radius * radius;
        for (TissueNode& node : nodes) {
            if (!node.is_dynamic()) continue;
            const Vec3 offset = node.position - center;
            const float distance_squared = offset.length_squared();
            if (distance_squared > radius_squared || distance_squared <= 1.0e-12f)
                continue;
            const float distance = std::sqrt(distance_squared);
            const float falloff = 1.0f - distance / radius;
            node.apply_impulse((offset / distance) * (strength * falloff));
            ++affected;
        }
        return affected;
    }

    void step(float delta_time) {
        if (!std::isfinite(delta_time) || delta_time <= 0.0f ||
            !gravity.is_finite()) return;

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            TissueNode& node = nodes[i];
            previous_positions[i] = node.position;
            predicted_positions[i] = node.position;
            if (!node.is_dynamic()) {
                node.accumulated_force = {};
                continue;
            }
            const Vec3 acceleration = gravity +
                node.accumulated_force * node.inverse_mass;
            node.velocity += acceleration * delta_time;
            predicted_positions[i] += node.velocity * delta_time;
            node.accumulated_force = {};
        }

        for (TissueLink& link : links) link.reset();
        for (TissueTetrahedron& tetrahedron : tetrahedra) tetrahedron.reset();
        for (TissueBendConstraint& bend : bends) bend.reset();
        for (TissueDihedralConstraint& hinge : dihedrals) hinge.reset();
        const int iterations = std::max(1, constraint_iterations);
        for (int iteration = 0; iteration < iterations; ++iteration) {
            for (TissueLink& link : links) solve_link(link, delta_time);
            for (TissueBendConstraint& bend : bends) solve_bend(bend, delta_time);
            for (TissueDihedralConstraint& hinge : dihedrals)
                solve_dihedral(hinge, delta_time);
            for (TissueTetrahedron& tetrahedron : tetrahedra) {
                solve_tetrahedron_barrier(tetrahedron, delta_time);
                solve_tetrahedron(tetrahedron, delta_time);
            }
        }

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            TissueNode& node = nodes[i];
            if (node.is_dynamic()) {
                node.position = predicted_positions[i];
                node.velocity = (node.position - previous_positions[i]) /
                                delta_time;
            }
        }

        for (const TissueLink& link : links) apply_link_damping(link);
    }

private:
    std::vector<Vec3> predicted_positions;
    std::vector<Vec3> previous_positions;

    void solve_link(TissueLink& link, float delta_time) {
        if (!link.enabled || link.node_a >= nodes.size() ||
            link.node_b >= nodes.size() || link.node_a == link.node_b ||
            !std::isfinite(link.rest_length) || link.rest_length < 0.0f ||
            !std::isfinite(link.compliance) || link.compliance < 0.0f)
            return;

        TissueNode& a = nodes[link.node_a];
        TissueNode& b = nodes[link.node_b];
        const float weight_a = a.is_dynamic() ? a.inverse_mass : 0.0f;
        const float weight_b = b.is_dynamic() ? b.inverse_mass : 0.0f;
        const float weight_sum = weight_a + weight_b;
        if (weight_sum <= 0.0f) return;

        const Vec3 delta = predicted_positions[link.node_b] -
                           predicted_positions[link.node_a];
        const float distance_squared = delta.length_squared();
        if (!std::isfinite(distance_squared) || distance_squared <= 1.0e-12f)
            return;
        const float distance = std::sqrt(distance_squared);
        const Vec3 direction = delta / distance;
        const float constraint = distance - link.rest_length;
        const float alpha = link.compliance / (delta_time * delta_time);
        const float denominator = weight_sum + alpha;
        if (denominator <= 1.0e-12f || !std::isfinite(denominator)) return;

        const float delta_lambda =
            (-constraint - alpha * link.accumulated_lambda) / denominator;
        link.accumulated_lambda += delta_lambda;
        predicted_positions[link.node_a] -=
            direction * (weight_a * delta_lambda);
        predicted_positions[link.node_b] +=
            direction * (weight_b * delta_lambda);
    }




    void solve_bend(TissueBendConstraint& bend, float delta_time) {
        if (!bend.enabled || bend.node_a >= nodes.size() ||
            bend.node_b >= nodes.size() || bend.node_a == bend.node_b ||
            !std::isfinite(bend.rest_distance) || bend.rest_distance <= 0.0f ||
            !std::isfinite(bend.compliance) || bend.compliance < 0.0f)
            return;

        const float weight_a = nodes[bend.node_a].is_dynamic()
            ? nodes[bend.node_a].inverse_mass : 0.0f;
        const float weight_b = nodes[bend.node_b].is_dynamic()
            ? nodes[bend.node_b].inverse_mass : 0.0f;
        const float weight_sum = weight_a + weight_b;
        if (weight_sum <= 0.0f) return;

        const Vec3 delta = predicted_positions[bend.node_b] -
                           predicted_positions[bend.node_a];
        const float distance_squared = delta.length_squared();
        if (!std::isfinite(distance_squared) || distance_squared <= 1.0e-12f)
            return;
        const float distance = std::sqrt(distance_squared);
        const Vec3 direction = delta / distance;
        const float constraint = distance - bend.rest_distance;
        const float alpha = bend.compliance / (delta_time * delta_time);
        const float denominator = weight_sum + alpha;
        if (!std::isfinite(denominator) || denominator <= 1.0e-12f) return;

        const float delta_lambda =
            (-constraint - alpha * bend.accumulated_lambda) / denominator;
        if (!std::isfinite(delta_lambda)) return;
        bend.accumulated_lambda += delta_lambda;
        predicted_positions[bend.node_a] -= direction * (weight_a * delta_lambda);
        predicted_positions[bend.node_b] += direction * (weight_b * delta_lambda);
    }

    void solve_dihedral(TissueDihedralConstraint& hinge, float delta_time) {
        const std::size_t a=hinge.edge_a, b=hinge.edge_b;
        const std::size_t c=hinge.opposite_a, d=hinge.opposite_b;
        if (!hinge.enabled || a>=nodes.size() || b>=nodes.size() ||
            c>=nodes.size() || d>=nodes.size() || a==b || a==c || a==d ||
            b==c || b==d || c==d || !std::isfinite(hinge.rest_angle) ||
            !std::isfinite(hinge.compliance) || hinge.compliance<0.0f) return;
        const Vec3 pa=predicted_positions[a], pb=predicted_positions[b];
        const Vec3 pc=predicted_positions[c], pd=predicted_positions[d];
        const Vec3 edge=pb-pa;
        const float edge2=edge.length_squared();
        const Vec3 raw1=cross(edge,pc-pa);
        const Vec3 raw2=cross(pa-pb,pd-pb);
        const float n12=raw1.length_squared(), n22=raw2.length_squared();
        if (!std::isfinite(edge2) || edge2<=1.0e-12f ||
            !std::isfinite(n12) || !std::isfinite(n22) ||
            n12<=1.0e-12f || n22<=1.0e-12f) return;
        const float edge_length=std::sqrt(edge2);
        const Vec3 n1=raw1/std::sqrt(n12), n2=raw2/std::sqrt(n22);
        const Vec3 edge_dir=edge/edge_length;
        const float sine=dot(edge_dir,cross(n1,n2));
        const float cosine=std::clamp(dot(n1,n2),-1.0f,1.0f);
        const float angle=std::atan2(sine,cosine);
        const float pi=3.14159265358979323846f;
        const float constraint=std::remainder(angle-hinge.rest_angle,2.0f*pi);

        const Vec3 gc=raw1*(-edge_length/n12);
        const Vec3 gd=raw2*(-edge_length/n22);
        const Vec3 ga=gc*(dot(pc-pb,edge)/edge2) +
                      gd*(dot(pd-pb,edge)/edge2);
        const Vec3 gb=gc*(-dot(pc-pa,edge)/edge2) +
                      gd*(-dot(pd-pa,edge)/edge2);
        const Vec3 gradients[4]={ga,gb,gc,gd};
        const std::size_t ids[4]={a,b,c,d};
        float weights[4]{};
        float denominator=0.0f;
        for (int i=0;i<4;++i) {
            const TissueNode& node=nodes[ids[i]];
            weights[i]=node.is_dynamic()?node.inverse_mass:0.0f;
            denominator+=weights[i]*gradients[i].length_squared();
        }
        const float alpha=hinge.compliance/(delta_time*delta_time);
        denominator+=alpha;
        if (!std::isfinite(constraint) || !std::isfinite(denominator) ||
            denominator<=1.0e-12f) return;
        const float dl=(-constraint-alpha*hinge.accumulated_lambda)/denominator;
        if (!std::isfinite(dl)) return;
        hinge.accumulated_lambda+=dl;
        for (int i=0;i<4;++i)
            predicted_positions[ids[i]]+=gradients[i]*(weights[i]*dl);
    }

    static Vec3 cross(const Vec3& a, const Vec3& b) {
        return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    }
    static float dot(const Vec3& a, const Vec3& b) {
        return a.x*b.x+a.y*b.y+a.z*b.z;
    }

    void solve_tetrahedron_barrier(TissueTetrahedron& t, float delta_time) {
        if (!t.enabled || t.node_a >= nodes.size() || t.node_b >= nodes.size() ||
            t.node_c >= nodes.size() || t.node_d >= nodes.size() ||
            t.node_a == t.node_b || t.node_a == t.node_c ||
            t.node_a == t.node_d || t.node_b == t.node_c ||
            t.node_b == t.node_d || t.node_c == t.node_d ||
            !std::isfinite(t.rest_volume) || t.rest_volume <= 1.0e-10f ||
            !std::isfinite(t.minimum_volume_ratio) ||
            t.minimum_volume_ratio < 0.0f || t.minimum_volume_ratio >= 1.0f ||
            !std::isfinite(t.compliance) || t.compliance < 0.0f) return;

        const std::size_t ids[4] = {t.node_a, t.node_b, t.node_c, t.node_d};
        const Vec3 a = predicted_positions[ids[0]];
        const Vec3 ab = predicted_positions[ids[1]] - a;
        const Vec3 ac = predicted_positions[ids[2]] - a;
        const Vec3 ad = predicted_positions[ids[3]] - a;
        const Vec3 gb{
            (ac.y*ad.z-ac.z*ad.y)/6.0f,
            (ac.z*ad.x-ac.x*ad.z)/6.0f,
            (ac.x*ad.y-ac.y*ad.x)/6.0f};
        const Vec3 gc{
            (ad.y*ab.z-ad.z*ab.y)/6.0f,
            (ad.z*ab.x-ad.x*ab.z)/6.0f,
            (ad.x*ab.y-ad.y*ab.x)/6.0f};
        const Vec3 gd{
            (ab.y*ac.z-ab.z*ac.y)/6.0f,
            (ab.z*ac.x-ab.x*ac.z)/6.0f,
            (ab.x*ac.y-ab.y*ac.x)/6.0f};
        const Vec3 gradients[4] = {(gb+gc+gd)*-1.0f, gb, gc, gd};
        const float volume = (ab.x*(ac.y*ad.z-ac.z*ad.y) +
                              ab.y*(ac.z*ad.x-ac.x*ad.z) +
                              ab.z*(ac.x*ad.y-ac.y*ad.x))/6.0f;
        const float constraint = volume - t.rest_volume*t.minimum_volume_ratio;
        if (!std::isfinite(constraint) || constraint >= 0.0f) return;

        float weights[4]{};
        float denominator = 0.0f;
        for (int i=0; i<4; ++i) {
            const TissueNode& node = nodes[ids[i]];
            weights[i] = node.is_dynamic() ? node.inverse_mass : 0.0f;
            denominator += weights[i]*gradients[i].length_squared();
        }
        const float alpha = t.compliance/(delta_time*delta_time);
        denominator += alpha;
        if (!std::isfinite(denominator) || denominator <= 1.0e-12f) return;
        const float candidate = t.barrier_lambda +
            (-constraint-alpha*t.barrier_lambda)/denominator;
        if (!std::isfinite(candidate)) return;
        const float next_lambda = std::max(0.0f, candidate);
        const float delta_lambda = next_lambda-t.barrier_lambda;
        t.barrier_lambda = next_lambda;
        for (int i=0; i<4; ++i)
            predicted_positions[ids[i]] += gradients[i]*(weights[i]*delta_lambda);
    }

    void solve_tetrahedron(TissueTetrahedron& t, float delta_time) {
        if (!t.enabled || t.node_a >= nodes.size() || t.node_b >= nodes.size() ||
            t.node_c >= nodes.size() || t.node_d >= nodes.size() ||
            t.node_a == t.node_b || t.node_a == t.node_c ||
            t.node_a == t.node_d || t.node_b == t.node_c ||
            t.node_b == t.node_d || t.node_c == t.node_d ||
            !std::isfinite(t.rest_volume) || t.rest_volume <= 1.0e-10f ||
            !std::isfinite(t.compliance) || t.compliance < 0.0f) return;

        const std::size_t ids[4] = {t.node_a, t.node_b, t.node_c, t.node_d};
        const Vec3 a = predicted_positions[ids[0]];
        const Vec3 ab = predicted_positions[ids[1]] - a;
        const Vec3 ac = predicted_positions[ids[2]] - a;
        const Vec3 ad = predicted_positions[ids[3]] - a;
        const Vec3 gb{
            (ac.y*ad.z-ac.z*ad.y)/6.0f,
            (ac.z*ad.x-ac.x*ad.z)/6.0f,
            (ac.x*ad.y-ac.y*ad.x)/6.0f};
        const Vec3 gc{
            (ad.y*ab.z-ad.z*ab.y)/6.0f,
            (ad.z*ab.x-ad.x*ab.z)/6.0f,
            (ad.x*ab.y-ad.y*ab.x)/6.0f};
        const Vec3 gd{
            (ab.y*ac.z-ab.z*ac.y)/6.0f,
            (ab.z*ac.x-ab.x*ac.z)/6.0f,
            (ab.x*ac.y-ab.y*ac.x)/6.0f};
        const Vec3 gradients[4] = {(gb+gc+gd)*-1.0f, gb, gc, gd};
        const float volume = (ab.x*(ac.y*ad.z-ac.z*ad.y) +
                              ab.y*(ac.z*ad.x-ac.x*ad.z) +
                              ab.z*(ac.x*ad.y-ac.y*ad.x))/6.0f;
        const float constraint = volume - t.rest_volume;
        if (!std::isfinite(constraint)) return;

        float weights[4]{};
        float denominator = 0.0f;
        for (int i=0; i<4; ++i) {
            const TissueNode& node = nodes[ids[i]];
            weights[i] = node.is_dynamic() ? node.inverse_mass : 0.0f;
            denominator += weights[i] * gradients[i].length_squared();
        }
        const float alpha = t.compliance/(delta_time*delta_time);
        denominator += alpha;
        if (!std::isfinite(denominator) || denominator <= 1.0e-12f) return;
        const float dl = (-constraint-alpha*t.accumulated_lambda)/denominator;
        if (!std::isfinite(dl)) return;
        t.accumulated_lambda += dl;
        for (int i=0; i<4; ++i)
            predicted_positions[ids[i]] += gradients[i]*(weights[i]*dl);
    }

    void apply_link_damping(const TissueLink& link) {
        if (!link.enabled || link.node_a >= nodes.size() ||
            link.node_b >= nodes.size() || link.node_a == link.node_b ||
            !std::isfinite(link.damping)) return;
        const float damping = std::clamp(link.damping, 0.0f, 1.0f);
        if (damping <= 0.0f) return;

        TissueNode& a = nodes[link.node_a];
        TissueNode& b = nodes[link.node_b];
        const float weight_a = a.is_dynamic() ? a.inverse_mass : 0.0f;
        const float weight_b = b.is_dynamic() ? b.inverse_mass : 0.0f;
        const float weight_sum = weight_a + weight_b;
        if (weight_sum <= 0.0f) return;

        const Vec3 delta = b.position - a.position;
        const float distance_squared = delta.length_squared();
        if (distance_squared <= 1.0e-12f) return;
        const Vec3 direction = delta / std::sqrt(distance_squared);
        const float relative_speed =
            (b.velocity.x - a.velocity.x) * direction.x +
            (b.velocity.y - a.velocity.y) * direction.y +
            (b.velocity.z - a.velocity.z) * direction.z;
        const float impulse = damping * relative_speed / weight_sum;
        if (a.is_dynamic()) a.velocity += direction * (weight_a * impulse);
        if (b.is_dynamic()) b.velocity -= direction * (weight_b * impulse);
    }
};

} // namespace spoon::physics
