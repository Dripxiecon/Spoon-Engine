# Spoon Physics

A modular C++17 physics subsystem for Spoon-Engine. The initial mobile target is Galaxy A05, with desktop scalability as a parallel goal.

## Implemented foundations

- Allocation-free `Vec3` math and quaternion rotation.
- Bone pose transform with translation, rotation and non-uniform scale.
- Four-influence weighted geometry deformation using bone-local bind positions.
- Muscle activation dynamics, active force-length response, passive stretch resistance and damping.
- Muscle shape response: contraction shortens the profile and can increase its radius to approximate volume preservation.
- Fixed-step clock with a bounded substep count to prevent runaway catch-up after frame stalls.
- Separate deterministic test executables for the individual modules.

## Bone-driven geometry

Each skinned vertex can carry up to four bone influences. Every influence stores a bind-local vertex position, and the current bone pose transforms that position. The weighted result moves, rotates, stretches or squashes the vertex with its bone. This is a first linear-blend-style deformation layer; inverse-bind matrix import, dual-quaternion skinning, joint hierarchy evaluation and runtime mesh upload are not implemented yet.

## Muscle behavior

`MuscleUnit` calculates active and passive tension between two attachment points. `MuscleGeometry` provides a cheap shape target for a muscle mesh, including configurable volume-preservation strength and a maximum bulge limit. The geometry helper does not itself simulate muscle fibers or solve a complete anatomical body.

## Build and test

```sh
cmake -S . -B build -DSPOON_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Next architecture steps

1. Hierarchical bone graph and joint constraints.
2. Muscle attachment integration into the bone solver, including antagonistic muscle groups.
3. XPBD constraints for stable soft tissue and tendon behavior.
4. Tissue graph and ripple propagation with damping, material response and energy limits.
5. Shape-matching and volume-preserving soft-body clusters.
6. Collision detection, contact solving and optional self-collision.
7. Renderer integration, profiling and Android/desktop benchmarks.

Performance targets must be verified on actual devices; no Galaxy A05 benchmark has been run yet.
