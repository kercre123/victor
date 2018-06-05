project(cti_common_clad)

include(generate_clad)

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_common_clad.srcs.lst" SRCS)

set(CLAD_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/coretech/common")
set(PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

generate_clad_cpp(
  LIBRARY "cti_common_clad"
  SRCS "${SRCS}"
  RELATIVE_SRC_DIR ".."
  OUTPUT_DIR "${OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_CPP_GEN_SRCS
)

generate_clad_py(
  TARGET "cti_common_clad_python"
  SRCS "${SRCS}"
  RELATIVE_SRC_DIR ".."
  OUTPUT_DIR "${PY_OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_PY_GEN_SRCS
)

target_include_directories(cti_common_clad
  PUBLIC
  $<BUILD_INTERFACE:${OUTPUT_DIR}>
)

target_link_libraries(cti_common_clad
  clad
)

add_dependencies(cti_common_clad cti_common_clad_python)

#
# cti_common_clad_cpplite
#

set(CPPLITE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/clad/robot/common")
set(PYLITE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

generate_clad_cpplite(
  SRCS ${SRCS}
  OUTPUT_DIR "${CPPLITE_OUTPUT_DIR}"
  OUTPUT_SRCS CLAD_CPPLITE_GEN_SRCS
)

generate_clad_py(
  TARGET "cti_common_clad_python_lite"
  SRCS ${SRCS}
  OUTPUT_DIR "${PYLITE_OUTPUT_DIR}"
  OUTPUT_SRCS CLAD_PYLITE_GEN_SRCS
)

add_library(cti_common_clad_cpplite STATIC
  ${CLAD_CPPLITE_GEN_SRCS}
)

target_include_directories(cti_common_clad_cpplite
  PUBLIC
  $<BUILD_INTERFACE:${CPPLITE_OUTPUT_DIR}>
)

anki_build_target_license(cti_common_clad_cpplite "ANKI")
