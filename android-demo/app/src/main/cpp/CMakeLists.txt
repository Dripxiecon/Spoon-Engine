cmake_minimum_required(VERSION 3.22.1)
project(spoon_android_demo LANGUAGES CXX)

set(SPOON_BUILD_TESTS OFF CACHE BOOL "Disable host tests in Android app" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../.. ${CMAKE_CURRENT_BINARY_DIR}/spoon-engine)

add_library(spoon_demo SHARED native_demo.cpp)
target_compile_features(spoon_demo PRIVATE cxx_std_17)
target_link_libraries(spoon_demo PRIVATE spoon_physics)
