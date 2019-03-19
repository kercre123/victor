project(cti_vision_clad)

include(generate_clad)

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_vision_clad.srcs.lst" SRCS)

set(CLAD_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../common/clad_src/"
)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/engine/coretech/vision")
set(PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

generate_clad_cpp(LIBRARY "cti_vision_clad"
  STATIC
  SRCS "${SRCS}"
  OUTPUT_DIR "${OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_CPP_GEN_SRCS
)

generate_clad_py(
  TARGET "cti_vision_clad_python"
  SRCS "${SRCS}"
  OUTPUT_DIR "${PY_OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_PY_GEN_SRCS
)

target_include_directories(cti_vision_clad
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/engine/coretech/common>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/engine/coretech/vision>
)

target_compile_definitions(cti_vision_clad
  PUBLIC
  USES_CLAD
)

target_link_libraries(cti_vision_clad
  clad
  cti_common_clad
)

#
# cti_vision_clad_cpplite - Need to build cti_vision_clad separately
# so that it can be used within EngineToRobot messages - some clad types
# included in the full coretech clad SRCS list violate message size requirements
# for cpplite
#

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_composite_image_clad.srcs.lst" COMPOSITE_SRCS)

set(CPPLITE_ROBOT_OUTPUT_DIR  "${CMAKE_SOURCE_DIR}/generated/clad/robot/coretech/vision")

set(CPPLITE_CLAD_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../common/clad_src/"
)

if (USE_CPPLITE_NAMESPACE)
    set(CLAD_CPPLITE_NS_FLAG "--namespace" "CppLite")
endif()

set(CLAD_CPPLITE_FLAGS
    --max-message-size 4096
    ${CLAD_CPPLITE_NS_FLAG})

generate_clad_cpplite(
  FLAGS "${CLAD_CPPLITE_FLAGS}"
  SRCS ${COMPOSITE_SRCS}
  OUTPUT_DIR "${CPPLITE_ROBOT_OUTPUT_DIR}"
  INCLUDES "${CPPLITE_CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_CPPLITE_GEN_SRCS
)

add_library(cti_vision_clad_cpplite STATIC
  ${CLAD_CPPLITE_GEN_SRCS}
)

target_include_directories(cti_vision_clad_cpplite
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot/coretech/common>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/generated/clad/robot/coretech/vision>
)

if (USE_CPPLITE_NAMESPACE)
  target_compile_definitions(cti_vision_clad_cpplite
    PUBLIC
    USES_CPPLITE=1
  )
endif()

target_link_libraries(cti_vision_clad_cpplite
  ${OPENCV_LIBS}
)

target_compile_definitions(cti_vision_clad_cpplite
    PUBLIC
    USES_CPPLITE
)

anki_build_target_license(cti_vision_clad_cpplite "ANKI")
