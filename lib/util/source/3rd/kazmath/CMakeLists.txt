project(libkazmath LANGUAGES CXX)

add_library(kazmath STATIC
  src/aabb2.c
  src/aabb3.c
  src/GL/mat4stack.c
  src/GL/matrix.c
  src/mat3.c
  src/mat4.c
  src/plane.c
  src/quaternion.c
  src/ray2.c
  src/ray3.c
  src/utility.c
  src/vec2.c
  src/vec3.c
  src/vec4.c)

target_include_directories(kazmath PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

anki_build_target_license(kazmath "BSD-4,${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")

export(TARGETS kazmath FILE kazmath-config.cmake)
