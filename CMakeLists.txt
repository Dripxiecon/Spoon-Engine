cmake_minimum_required(VERSION 3.16)
project(SpoonEnginePhysics VERSION 0.1.0 LANGUAGES CXX)

option(SPOON_BUILD_TESTS "Build Spoon physics tests" ON)

add_library(spoon_physics
    engine/physics/src/bone_spring.cpp
)
add_library(Spoon::Physics ALIAS spoon_physics)
target_include_directories(spoon_physics PUBLIC engine/physics/include)
target_compile_features(spoon_physics PUBLIC cxx_std_17)
set_target_properties(spoon_physics PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(spoon_physics PRIVATE /W4 /permissive-)
else()
    target_compile_options(spoon_physics PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SPOON_BUILD_TESTS)
    enable_testing()
    add_executable(spoon_physics_tests tests/physics/bone_spring_tests.cpp)
    target_link_libraries(spoon_physics_tests PRIVATE Spoon::Physics)
    add_test(NAME spoon_physics_tests COMMAND spoon_physics_tests)
endif()
