# Spoon Physics

A modular C++17 physics subsystem for Spoon-Engine. The initial mobile target is Galaxy A05, with desktop scalability as a parallel goal.

## Implemented foundations

- Allocation-free `Vec3` math primitive.
- Damped bone spring with impulses, external acceleration and inactive-state control.
- Muscle unit with activation dynamics, active force-length response, passive stretch resistance and damping.
- Equal-and-opposite muscle attachment forces, so the muscle pulls rather than pushes.
- Fixed-step clock with a bounded substep count to prevent runaway catch-up after frame stalls.
- Separate deterministic test executables for each foundation module.

## Build and test

```sh
cmake -S . -B build -DSPOON_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Architecture roadmap

1. Bone graph with joints and anatomical attachment points.
2. Muscle groups, tendons and joint stabilization.
3. XPBD constraint solver (distance, bend, angular and attachment constraints).
4. Tissue graph and ripple propagation with damping, material response and energy limits.
5. Hybrid soft-body clusters with shape and volume preservation.
6. Collision detection, contact solving and optional self-collision.
7. Mesh deformation adapters, profiling and Android/desktop benchmarks.

## Important model boundary

The muscle unit currently computes forces between two supplied attachment points. It does not yet own a skeleton, solve joint motion, or model full muscle fiber geometry. The fixed-step clock also leaves the actual simulation loop to its caller. These pieces are deliberately separate so the next modules can connect them without creating one oversized physics class.

Performance targets must be verified on actual devices; no Galaxy A05 benchmark has been run yet.
