project(cti_common_clad)

include(generate_clad)

file(STRINGS "${ANKI_SRCLIST_DIR}/cti_vision_clad.srcs.lst" SRCS)

set(CLAD_INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/"
)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/coretech/vision")
set(PY_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/generated/cladPython")

generate_clad_cpp(LIBRARY "cti_vision_clad" 
  STATIC
  SRCS "${SRCS}"
  RELATIVE_SRC_DIR ".."
  OUTPUT_DIR "${OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_CPP_GEN_SRCS
)

generate_clad_py(
  TARGET "cti_vision_clad_python"
  SRCS "${SRCS}"
  RELATIVE_SRC_DIR ".."
  OUTPUT_DIR "${PY_OUTPUT_DIR}"
  INCLUDES "${CLAD_INCLUDES}"
  OUTPUT_SRCS CLAD_PY_GEN_SRCS
)

target_include_directories(cti_vision_clad
  PUBLIC
  $<BUILD_INTERFACE:${OUTPUT_DIR}>
)

target_link_libraries(cti_vision_clad
  clad
)
