# Spoon Physics

A modular C++ physics subsystem for Spoon-Engine, designed to scale from Android-class hardware (initial target: Galaxy A05) to desktop.

## Current foundation

- Small, allocation-free vector math primitive.
- Configurable damped bone spring with impulse and acceleration input.
- Explicit active/sleep control for cheap inactive systems.
- CMake build and a small deterministic test executable.

## Build

```sh
cmake -S . -B build -DSPOON_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Architecture roadmap

1. Bone graph and fixed-step simulation scheduler.
2. Constraint solver (distance, bend, angular and attachment constraints).
3. Ripple propagation over bone graphs.
4. Hybrid soft-body clusters and volume-preserving constraints.
5. Collision detection, contact solving and optional self-collision.
6. Mesh deformation adapters and renderer integration.
7. Deterministic tests, profiling and Android/desktop benchmarks.

The current spring is a foundation primitive, not a complete soft-body or collision engine. Performance targets must be verified on actual devices.
