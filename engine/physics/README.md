# Spoon Physics

A modular C++17 physics subsystem for Spoon-Engine. The initial mobile target is Galaxy A05, with desktop scalability as a parallel goal.

## Implemented foundations

- Compact vector and quaternion math, plus bone transforms with translation, rotation and non-uniform scale.
- Four-influence weighted geometry deformation using bone-local bind positions.
- Muscle activation dynamics, active force-length response, passive stretch resistance and damping.
- Coordinated muscle groups with smoothed activation, weighted muscle recruitment, and agonist, antagonist and stabilizer roles.
- Muscle shape response: contraction shortens the profile and can increase its radius to approximate volume preservation.
- Fixed-step clock with a bounded substep count to prevent runaway catch-up after frame stalls.
- Parent-child skeleton pose evaluation with cycle and invalid-index detection.
- Bone-local muscle attachments that produce equal/opposite forces and pivot torques.
- Rigid-ish bone body state with linear/angular velocity, force accumulation, local diagonal inverse inertia and quaternion orientation integration.
- XPBD-style ball-anchor constraints between world-space bone bodies, with compliance and angular correction.
- Bone solver that integrates muscle forces, body motion and joint constraints without allocating during each solve step.
- Lightweight tissue graph with XPBD distance links, topology-generated surface bend-distance and signed dihedral-angle constraints, signed-volume tetrahedron constraints, compliance, damping, local impulses and radial ripple impulses.
- Bone-to-tissue bindings that make selected tissue nodes follow bone-local anchors and transmit anchor motion into neighboring tissue.
- Tissue-to-skinning bridge that blends simulated tissue-node displacement into already-skinned mesh vertices.
- Seventeen deterministic test executables covering the physics foundations, muscle-to-bone integration, bone solver and tissue deformation pipeline.

## Bone bodies and joints

The simulation layer treats each dynamic bone as a world-space body. The skeleton hierarchy remains useful for authoring, rest poses and skinning, while explicit joint constraints connect simulated bodies. This avoids applying parent transforms twice. A BoneBody has mass, local diagonal inverse inertia, linear velocity and angular velocity. Static bodies use zero inverse mass and inverse inertia.

BoneJoint currently constrains two local anchor points to the same world position using an XPBD-style compliance term. It corrects both translation and rotation, allowing a joint to resist anchor drift without relying on very stiff penalty springs. It is currently a ball-anchor constraint: hinge axes, angular limits, motors and collision contacts are later additions. The body model uses diagonal local inertia, so it is intentionally a lightweight approximation rather than a general rigid-body engine.

BoneSolver applies gravity and accumulated forces, integrates bodies, then iterates joint constraints. Muscle attachment force pairs can be passed into the solver so muscle contraction can drive bone motion. Use the FixedStepClock to call step at a stable fixed interval; the solver does not own a frame clock.

## Tissue ripple graph

`TissueGraph` is a small, pre-sized point-and-link network for soft tissue. Each node stores position, velocity, inverse mass and force; each link stores rest length, compliance and damping. XPBD distance constraints distribute a disturbance through neighboring nodes, while damping removes energy over time. A local or radial impulse can start a ripple. Nodes with zero inverse mass act as fixed attachments. `build_tissue_surface_bends` builds second-ring bend-distance constraints from pairs of triangles sharing an edge. Boundary and non-manifold edges are skipped. This preserves the distance between the two opposite vertices across a shared edge, resisting sharp folds at low cost. It is a topology-aware distance approximation; `build_tissue_surface_dihedrals` additionally creates four-node signed-angle hinges for consistently wound adjacent triangles. Dihedral hinges preserve the captured fold angle using analytic gradients and XPBD compliance, avoiding per-iteration finite differences. Degenerate triangles and inconsistent winding are skipped. Build topology once during mesh setup, not in the frame loop; tune compliance to avoid making tissue rigid. Optional `TissueTetrahedron` constraints preserve the signed volume of selected four-node regions using XPBD gradients. A unilateral minimum-volume barrier (default 5% of rest volume) resists collapse and inversion; it is a safeguard, not a guarantee under arbitrary forces or degenerate configurations, and this is not a complete volumetric soft-body solver. The graph does not create or simulate a triangle mesh or perform collisions; it is a lightweight motion layer intended to drive selected tissue points or later mesh deformation. Pre-size nodes and links before stepping to avoid runtime allocation. `TissueBindingSet` connects graph nodes to bone-local anchor points. Update the bindings after bone poses are evaluated and before stepping the graph. Bound nodes become kinematic anchors, and their frame-to-frame velocity is retained so neighboring links can receive motion and produce ripples. For surface-like transverse ripples, links should form a 2D triangulated or otherwise non-collinear network; a perfectly straight chain of distance-only links does not provide first-order bending resistance.

## Tissue-driven mesh deformation

`TissueSkinningBridge` adds the displacement of one or more tissue graph nodes to vertices after ordinary bone skinning. Each binding stores a vertex index, tissue-node index, rest node position and blend weight. This keeps bone deformation and tissue motion in separate stages, preventing the same bone transform from being applied twice. The caller owns and pre-sizes the output buffer; after its first setup, the bridge performs no per-frame allocation.

## Bone-driven geometry

Each skinned vertex can carry up to four bone influences. Every influence stores a bind-local vertex position, and the current bone pose transforms that position. The weighted result moves, rotates, stretches or squashes the vertex with its bone. This is a first linear-blend-style deformation layer; inverse-bind matrix import, dual-quaternion skinning and runtime mesh upload are not implemented yet.

## Muscle behavior

`MuscleGroupController` coordinates multiple `MuscleUnit` instances. Each group has its own smoothed activation, role (agonist, antagonist or stabilizer), and weighted member list. Separate agonist and antagonist groups can receive different targets; stabilizer groups can remain partially active while movement occurs. Overlapping group contributions are added and clamped to the muscle's 0–1 activation range. Call `prepare` when the muscle count changes, then `step` at the fixed simulation interval; the controller's frame step uses pre-sized scratch buffers. MuscleUnit calculates active and passive tension between two attachment points. MuscleGeometry provides a cheap shape target for a muscle mesh, including configurable volume-preservation strength and a maximum bulge limit. The geometry helper does not itself simulate muscle fibers or solve a complete anatomical body. MuscleAttachment binds each muscle endpoint to a bone-local point and converts muscle tension into forces and torques consumed by BoneSolver. `MuscleRig` owns the muscle units and actuator mapping, advances the group controller, derives attachment-point velocities from each body's linear and angular velocity, then stages and applies the force pairs. Call `rig.prepare(solver)` after sizing the bodies, muscles and actuators; on each fixed step call `rig.step(solver, dt)` before `solver.step(dt)`. Keep muscle/actuator/body counts unchanged after preparation, or prepare again outside the simulation loop.

## Build and test

Run CMake configure, build the project, then run CTest with output-on-failure. The CI workflow runs these checks on Linux and cross-compiles the library for Android arm64.

## Next architecture steps

1. Add hinge and ball-socket joint modes, angular limits and joint motors.
2. Add explicit antagonist coordination and joint-stabilization control.
3. Add optional shape-matching and volume-preserving clusters for selected high-detail areas.
4. Add collision detection and contact solving, then optional self-collision.
5. Build a small Android demo app and APK workflow to visualize bone, muscle and tissue behavior.
6. Profile representative rigs and benchmark on the Galaxy A05 before setting performance claims.

Performance targets must be verified on actual devices; no Galaxy A05 benchmark has been run yet.