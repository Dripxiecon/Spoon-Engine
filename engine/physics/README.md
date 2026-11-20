# Spoon Physics

A modular C++17 physics subsystem for Spoon-Engine. The initial mobile target is Galaxy A05, with desktop scalability as a parallel goal.

## Implemented foundations

- Compact vector and quaternion math, plus bone transforms with translation, rotation and non-uniform scale.
- Four-influence weighted geometry deformation using bone-local bind positions.
- Muscle activation dynamics, active force-length response, passive stretch resistance and damping.
- Muscle shape response: contraction shortens the profile and can increase its radius to approximate volume preservation.
- Fixed-step clock with a bounded substep count to prevent runaway catch-up after frame stalls.
- Parent-child skeleton pose evaluation with cycle and invalid-index detection.
- Bone-local muscle attachments that produce equal/opposite forces and pivot torques.
- Rigid-ish bone body state with linear/angular velocity, force accumulation, local diagonal inverse inertia and quaternion orientation integration.
- XPBD-style ball-anchor constraints between world-space bone bodies, with compliance and angular correction.
- Bone solver that integrates muscle forces, body motion and joint constraints without allocating during each solve step.
- Eight deterministic test executables covering the physics foundations and solver.

## Bone bodies and joints

The simulation layer treats each dynamic bone as a world-space body. The skeleton hierarchy remains useful for authoring, rest poses and skinning, while explicit joint constraints connect simulated bodies. This avoids applying parent transforms twice. A BoneBody has mass, local diagonal inverse inertia, linear velocity and angular velocity. Static bodies use zero inverse mass and inverse inertia.

BoneJoint currently constrains two local anchor points to the same world position using an XPBD-style compliance term. It corrects both translation and rotation, allowing a joint to resist anchor drift without relying on very stiff penalty springs. It is currently a ball-anchor constraint: hinge axes, angular limits, motors and collision contacts are later additions. The body model uses diagonal local inertia, so it is intentionally a lightweight approximation rather than a general rigid-body engine.

BoneSolver applies gravity and accumulated forces, integrates bodies, then iterates joint constraints. Muscle attachment force pairs can be passed into the solver so muscle contraction can drive bone motion. Use the FixedStepClock to call step at a stable fixed interval; the solver does not own a frame clock.

## Bone-driven geometry

Each skinned vertex can carry up to four bone influences. Every influence stores a bind-local vertex position, and the current bone pose transforms that position. The weighted result moves, rotates, stretches or squashes the vertex with its bone. This is a first linear-blend-style deformation layer; inverse-bind matrix import, dual-quaternion skinning and runtime mesh upload are not implemented yet.

## Muscle behavior

MuscleUnit calculates active and passive tension between two attachment points. MuscleGeometry provides a cheap shape target for a muscle mesh, including configurable volume-preservation strength and a maximum bulge limit. The geometry helper does not itself simulate muscle fibers or solve a complete anatomical body. MuscleAttachment binds each muscle endpoint to a bone-local point and converts muscle tension into forces and torques consumed by BoneSolver.

## Build and test

Run CMake configure, build the project, then run CTest with output-on-failure. The CI workflow runs these checks on Linux and cross-compiles the library for Android arm64.

## Next architecture steps

1. Add hinge and ball-socket joint modes, angular limits and joint motors.
2. Add antagonistic muscle groups and joint stabilization.
3. Build a graph-based XPBD tissue layer for soft-tissue motion and damped ripple propagation.
4. Add shape-matching and volume-preserving soft-tissue clusters for selected high-detail areas.
5. Add collision detection and contact solving, then optional self-collision.
6. Connect simulated world poses to skinning and runtime mesh upload.
7. Profile representative rigs and benchmark on the Galaxy A05 before setting performance claims.

Performance targets must be verified on actual devices; no Galaxy A05 benchmark has been run yet.